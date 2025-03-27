package app

import (
	"fmt"
	"math"
	"net"
	"sync"
	"time"

	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"

	"github.com/OJPARKINSON/viz1090/internal/adsb"
	"github.com/OJPARKINSON/viz1090/internal/beast"
	"github.com/OJPARKINSON/viz1090/internal/config"
	"github.com/OJPARKINSON/viz1090/internal/viz"
)

// App represents the main application
type App struct {
	config       *config.Config
	aircraft     map[uint32]*adsb.Aircraft
	selectedICAO uint32
	centerLat    float64
	centerLon    float64
	maxDistance  float64

	window      *sdl.Window
	renderer    *sdl.Renderer
	vizRenderer *viz.Renderer
	regularFont *ttf.Font
	boldFont    *ttf.Font

	beastConn     net.Conn
	isConnected   bool
	lastFrameTime time.Time

	mutex   sync.RWMutex
	running bool
}

// New creates a new application instance
func New(config *config.Config) *App {
	return &App{
		config:      config,
		aircraft:    make(map[uint32]*adsb.Aircraft),
		centerLat:   config.InitialLat,
		centerLon:   config.InitialLon,
		maxDistance: config.InitialZoom,
		running:     false,
	}
}

// Initialize sets up SDL, fonts, and network connections
func (a *App) Initialize() error {
	// Initialize SDL
	if err := sdl.Init(sdl.INIT_VIDEO); err != nil {
		return fmt.Errorf("failed to initialize SDL: %v", err)
	}
	// Initialize TTF
	if err := ttf.Init(); err != nil {
		sdl.Quit()
		return fmt.Errorf("failed to initialize TTF: %v", err)
	}

	// Create window and renderer

	width, height := a.config.ScreenWidth, a.config.ScreenHeight
	if width == 0 || height == 0 {
		displayCount, err := sdl.GetNumVideoDisplays()
		if err != nil {
			return fmt.Errorf("failed to get display count: %v", err)
		}

		for i := 0; i < displayCount; i++ {
			bounds, err := sdl.GetDisplayBounds(i)
			if err != nil {
				continue
			}
			width = int(bounds.W)
			height = int(bounds.H)
			break
		}
	}

	var windowFlags uint32 = sdl.WINDOW_SHOWN
	if a.config.Fullscreen {
		windowFlags |= sdl.WINDOW_FULLSCREEN_DESKTOP
	}

	window, err := sdl.CreateWindow("viz1090 Go",
		0x1FFF0000,
		0x1FFF0000,
		int32(width), int32(height), windowFlags)
	if err != nil {
		ttf.Quit()
		sdl.Quit()
		return fmt.Errorf("failed to create window: %v", err)
	}
	a.window = window

	renderer, err := sdl.CreateRenderer(window, -1, uint32(2))
	if err != nil {
		window.Destroy()
		ttf.Quit()
		sdl.Quit()
		return fmt.Errorf("failed to create renderer: %v", err)
	}
	a.renderer = renderer

	// Load fonts
	regularFont, err := ttf.OpenFont("font/TerminusTTF-4.46.0.ttf", 12*a.config.UIScale)
	if err != nil {
		renderer.Destroy()
		window.Destroy()
		ttf.Quit()
		sdl.Quit()
		return fmt.Errorf("failed to load regular font: %v", err)
	}
	a.regularFont = regularFont

	boldFont, err := ttf.OpenFont("font/TerminusTTF-Bold-4.46.0.ttf", 12*a.config.UIScale)
	if err != nil {
		regularFont.Close()
		renderer.Destroy()
		window.Destroy()
		ttf.Quit()
		sdl.Quit()
		return fmt.Errorf("failed to load bold font: %v", err)
	}
	a.boldFont = boldFont

	// Create visualization renderer
	w, h := window.GetSize()
	a.vizRenderer = viz.NewRenderer(renderer, regularFont, boldFont, int(w), int(h),
		a.config.UIScale, a.config.Metric)

	// Connect to Beast server
	a.connectToBeast()

	return nil
}

// connectToBeast attempts to connect to the Beast data server
func (a *App) connectToBeast() {
	conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", a.config.ServerAddress, a.config.ServerPort))
	if err != nil {
		fmt.Printf("Failed to connect to Beast server: %v\n", err)
		a.isConnected = false
		return
	}

	a.beastConn = conn
	a.isConnected = true

	// Start receiver goroutine
	go a.receiveBeastData()
}

// receiveBeastData receives and processes Beast protocol data
func (a *App) receiveBeastData() {
	decoder := beast.NewDecoder(a.beastConn)

	for a.running {
		// Try to read a message
		msg, err := decoder.ReadMessage()
		if err != nil {
			if a.running {
				fmt.Printf("Beast protocol error: %v\n", err)
				// Try to reconnect after a short delay
				time.Sleep(5 * time.Second)
				a.connectToBeast()
				return
			}
			fmt.Println(err)
			break
		} else {
			fmt.Printf("Received Beast message: Type=%d, Length=%d\n", msg.Type, len(msg.Data))

			// Print the ICAO address if it's a Mode-S message
			if len(msg.Data) >= 4 {
				icao := uint32(msg.Data[1])<<16 | uint32(msg.Data[2])<<8 | uint32(msg.Data[3])
				fmt.Printf("  ICAO: %06X\n", icao)
			}
		}
		fmt.Printf("Received message: Type=%d, Length=%d\n", msg.Type, len(msg.Data))

		// Process the message if it's a Mode S long message
		if msg.Type == beast.ModeLong {
			a.processModeS(msg.Data, msg.Timestamp)
		}
	}
}

// processModeS decodes and handles a Mode S message
func (a *App) processModeS(data []byte, timestamp uint64) {
	// Extract downlink format
	// df := data[0] >> 3

	// Only process DF17 and DF18 (ADS-B messages)
	// if df != 17 && df != 18 {
	// 	return
	// }

	// Extract ICAO address from bytes 1-3
	icao := uint32(data[1])<<16 | uint32(data[2])<<8 | uint32(data[3])

	// Lock for map access
	a.mutex.Lock()

	// Get or create aircraft entry
	aircraft, exists := a.aircraft[icao]
	if !exists {
		aircraft = &adsb.Aircraft{
			ICAO: icao,
			Seen: time.Now(),
		}
		a.aircraft[icao] = aircraft
	}

	// Update last seen time
	aircraft.Seen = time.Now()

	// Extract message type
	msgType := data[4] >> 3

	// Process message based on type
	switch msgType {
	case 1, 2, 3, 4: // Aircraft identification
		aircraft.Flight = adsb.DecodeIdentification(data)

	case 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22: // Airborne position
		// Extract altitude
		aircraft.Altitude = adsb.DecodeAltitude(data)

		// Extract and store CPR position
		cpr := adsb.DecodeCPRPosition(data)

		// Try to calculate actual position if we have both odd and even frames
		// This is a simplified approach - real implementation would use proper CPR decoding
		if cpr.OddFlag {
			// Process odd frame
			aircraft.Lat = 37.0 + float64(timestamp%10)/100
			aircraft.Lon = -122.0 + float64(timestamp%10)/100
		} else {
			// Process even frame
			aircraft.Lat = 37.0 + float64(timestamp%10)/100
			aircraft.Lon = -122.0 + float64(timestamp%10)/100
		}

		// Record position time
		aircraft.SeenLatLon = time.Now()

		// Add to trail if position is valid
		if aircraft.Lat != 0 && aircraft.Lon != 0 {
			// Add position to trail
			if len(aircraft.Trail) >= a.config.TrailLength {
				// Remove oldest point
				aircraft.Trail = aircraft.Trail[1:]
			}

			aircraft.Trail = append(aircraft.Trail, adsb.Position{
				Lat:       aircraft.Lat,
				Lon:       aircraft.Lon,
				Altitude:  aircraft.Altitude,
				Timestamp: time.Now(),
			})
		}

	case 19: // Airborne velocity
		speed, heading, vertRate, ok := adsb.DecodeVelocity(data)
		if ok {
			aircraft.Speed = speed
			aircraft.Heading = heading
			aircraft.VertRate = vertRate
		}
	}

	a.mutex.Unlock()
}

// cleanupStaleAircraft removes aircraft that haven't been seen recently
func (a *App) cleanupStaleAircraft() {
	now := time.Now()
	var toRemove []uint32

	a.mutex.Lock()
	for icao, aircraft := range a.aircraft {
		if now.Sub(aircraft.Seen) > 60*time.Second {
			toRemove = append(toRemove, icao)
		}
	}

	for _, icao := range toRemove {
		delete(a.aircraft, icao)
	}
	a.mutex.Unlock()
}

// handleInput processes user input events
func (a *App) handleInput() bool {
	for event := sdl.PollEvent(); event != nil; event = sdl.PollEvent() {
		switch e := event.(type) {
		case *sdl.QuitEvent:
			return false

		case *sdl.KeyboardEvent:
			if e.Type == sdl.KEYDOWN {
				switch e.Keysym.Sym {
				case sdl.K_ESCAPE:
					return false
				case sdl.K_PLUS, sdl.K_EQUALS:
					a.maxDistance *= 0.8
				case sdl.K_MINUS:
					a.maxDistance *= 1.25
				}
			}

		case *sdl.MouseButtonEvent:
			if e.Type == sdl.MOUSEBUTTONDOWN {
				a.mutex.RLock()
				// Check if clicked on an aircraft
				mouseX, mouseY := int(e.X), int(e.Y)
				closestDist := 900 // 30^2
				closestICAO := uint32(0)

				for _, aircraft := range a.aircraft {
					if aircraft.X == 0 && aircraft.Y == 0 {
						continue
					}

					dx := aircraft.X - mouseX
					dy := aircraft.Y - mouseY
					distSq := dx*dx + dy*dy

					if distSq < closestDist {
						closestDist = distSq
						closestICAO = aircraft.ICAO
					}
				}

				if closestICAO != 0 {
					a.selectedICAO = closestICAO
				}
				a.mutex.RUnlock()
			}

		case *sdl.MouseWheelEvent:
			if e.Y > 0 {
				a.maxDistance *= 0.8
			} else if e.Y < 0 {
				a.maxDistance *= 1.25
			}

		case *sdl.MouseMotionEvent:
			if e.State&sdl.ButtonLMask() != 0 {
				// Dragging - move map center
				dx := float64(e.XRel) * (a.maxDistance / float64(a.vizRenderer.GetHeight()/2))
				dy := float64(e.YRel) * (a.maxDistance / float64(a.vizRenderer.GetHeight()/2))

				// Convert screen deltas to lat/lon deltas
				lonDelta := dx / (111195 * math.Cos(a.centerLat*math.Pi/180))
				latDelta := -dy / 111195

				a.centerLon -= lonDelta
				a.centerLat -= latDelta
			}
		}
	}

	return true
}

// Run starts the main application loop
func (a *App) Run() error {
	a.mutex.Lock()
	testAircraft := &adsb.Aircraft{
		ICAO:       0xABCDEF,
		Flight:     "TEST01",
		Altitude:   10000,
		Speed:      450,
		Heading:    45,
		Lat:        122.4195,
		Lon:        37.7747,
		Seen:       time.Now(),
		SeenLatLon: time.Now(),
	}
	a.aircraft[0xABCDEF] = testAircraft
	a.mutex.Unlock()
	fmt.Println("aircraft ", a.aircraft)
	a.running = true

	// Setup cleanup timer
	cleanupTicker := time.NewTicker(10 * time.Second)
	defer cleanupTicker.Stop()

	a.lastFrameTime = time.Now()

	// Main loop
	for a.running {
		// Handle input
		a.running = a.handleInput()

		// Check for cleanup
		select {
		case <-cleanupTicker.C:
			a.cleanupStaleAircraft()
		default:
			// Continue without blocking
		}

		// Render frame
		a.mutex.RLock()
		aircraftCopy := make(map[uint32]*adsb.Aircraft, len(a.aircraft))
		for k, v := range a.aircraft {
			aircraftCopy[k] = v
		}
		a.mutex.RUnlock()

		a.vizRenderer.RenderFrame(aircraftCopy, a.centerLat, a.centerLon, a.maxDistance, a.selectedICAO)

		// Cap frame rate
		elapsed := time.Since(a.lastFrameTime)
		if elapsed < 33*time.Millisecond {
			time.Sleep(33*time.Millisecond - elapsed)
		}
		a.lastFrameTime = time.Now()
	}

	return nil
}

// Cleanup releases all resources
func (a *App) Cleanup() {
	a.running = false

	if a.beastConn != nil {
		a.beastConn.Close()
	}

	if a.boldFont != nil {
		a.boldFont.Close()
	}

	if a.regularFont != nil {
		a.regularFont.Close()
	}

	if a.renderer != nil {
		a.renderer.Destroy()
	}

	if a.window != nil {
		a.window.Destroy()
	}

	ttf.Quit()
	sdl.Quit()
}
