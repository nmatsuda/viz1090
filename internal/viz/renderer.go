package viz

import (
	"fmt"
	"math"

	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"

	"github.com/OJPARKINSON/viz1090/internal/adsb"
)

// Colors for rendering
var (
	ColorBackground = sdl.Color{R: 0, G: 0, B: 0, A: 255}
	ColorPlane      = sdl.Color{R: 253, G: 250, B: 31, A: 255}
	ColorSelected   = sdl.Color{R: 249, G: 38, B: 114, A: 255}
	ColorTrail      = sdl.Color{R: 90, G: 133, B: 50, A: 255}
	ColorLabel      = sdl.Color{R: 255, G: 255, B: 255, A: 255}
	ColorSubLabel   = sdl.Color{R: 127, G: 127, B: 127, A: 255}
	ColorScaleBar   = sdl.Color{R: 196, G: 196, B: 196, A: 255}
)

// Renderer handles drawing the radar display
type Renderer struct {
	renderer    *sdl.Renderer
	regularFont *ttf.Font
	boldFont    *ttf.Font
	width       int
	height      int
	uiScale     int
	metric      bool
	centerLat   float64 // Current center latitude
	centerLon   float64 // Current center longitude
	maxDistance float64 // Current zoom level
}

// NewRenderer creates a new visualization renderer
func NewRenderer(renderer *sdl.Renderer, regularFont, boldFont *ttf.Font, width, height int,
	uiScale int, metric bool) *Renderer {
	return &Renderer{
		renderer:    renderer,
		regularFont: regularFont,
		boldFont:    boldFont,
		width:       width,
		height:      height,
		uiScale:     uiScale,
		metric:      metric,
		centerLat:   0,
		centerLon:   0,
		maxDistance: 50.0,
	}
}

func (r *Renderer) SetMapView(centerLat, centerLon, maxDistance float64) {
	r.centerLat = centerLat
	r.centerLon = centerLon
	r.maxDistance = maxDistance
}

func (r *Renderer) GetHeight() int {
	return r.height
}

// RenderFrame draws a complete frame with all aircraft
func (r *Renderer) RenderFrame(aircraft map[uint32]*adsb.Aircraft, centerLat, centerLon, maxDistance float64, selectedICAO uint32) {
	// Update internal values
	r.SetMapView(centerLat, centerLon, maxDistance)

	// Clear screen
	r.renderer.SetDrawColor(ColorBackground.R, ColorBackground.G, ColorBackground.B, ColorBackground.A)
	r.renderer.Clear()

	// Draw map background elements (if any)

	// Draw scale bars
	r.drawScaleBars(r.maxDistance, r.metric)

	// Draw all aircraft
	for _, a := range aircraft {
		// Skip aircraft without positions
		if a.Lat == 0 && a.Lon == 0 {
			continue
		}

		// Calculate screen position
		x, y := r.latLonToScreen(a.Lat, a.Lon, r.centerLat, r.centerLon, r.maxDistance)

		// Update aircraft screen position
		a.X = x
		a.Y = y

		// Skip if outside screen
		if x < -50 || y < -50 || x > r.width+50 || y > r.height+50 {
			continue
		}

		// Choose color based on selection state
		color := ColorPlane
		if a.ICAO == selectedICAO {
			color = ColorSelected
		}

		// Draw trails if available
		r.drawTrails(a)

		// Draw aircraft symbol
		r.drawAircraft(x, y, a.Heading, color, a.ICAO == selectedICAO)

		// Draw label
		r.drawLabel(a)
	}

	// Draw status information
	r.drawStatus(len(aircraft), r.centerLat, r.centerLon)

	// Present the renderer
	r.renderer.Present()
}

// latLonToScreen converts geographical coordinates to screen coordinates
func (r *Renderer) latLonToScreen(lat, lon, centerLat, centerLon, maxDistance float64) (int, int) {
	// Convert lat/lon to nautical miles
	dx := (lon - centerLon) * math.Cos((lat+centerLat)/2*math.Pi/180) * 60
	dy := (lat - centerLat) * 60

	// Scale to screen coordinates
	scale := float64(r.height) / (maxDistance * 2)

	x := r.width/2 + int(dx*scale)
	y := r.height/2 - int(dy*scale)

	return x, y
}

// drawAircraft draws an aircraft symbol at the specified position
func (r *Renderer) drawAircraft(x, y, heading int, color sdl.Color, selected bool) {
	// Convert heading to radians
	headingRad := float64(heading) * math.Pi / 180.0

	// Scale factors for plane size
	bodyLen := float64(8 * r.uiScale)
	wingLen := float64(6 * r.uiScale)
	tailLen := float64(3 * r.uiScale)

	// Calculate direction vectors
	dirX := math.Sin(headingRad)
	dirY := -math.Cos(headingRad)

	// Calculate perpendicular vectors (for wings)
	perpX := -dirY
	perpY := dirX

	// Compute points
	noseX := float64(x) + dirX*bodyLen
	noseY := float64(y) + dirY*bodyLen
	tailX := float64(x) - dirX*bodyLen*0.75
	tailY := float64(y) - dirY*bodyLen*0.75
	leftWingX := float64(x) + perpX*wingLen
	leftWingY := float64(y) + perpY*wingLen
	rightWingX := float64(x) - perpX*wingLen
	rightWingY := float64(y) - perpY*wingLen
	leftTailX := tailX + perpX*tailLen
	leftTailY := tailY + perpY*tailLen
	rightTailX := tailX - perpX*tailLen
	rightTailY := tailY - perpY*tailLen

	// Draw aircraft
	r.renderer.SetDrawColor(color.R, color.G, color.B, color.A)

	// Body
	r.renderer.DrawLine(int32(x), int32(y), int32(noseX), int32(noseY))
	r.renderer.DrawLine(int32(x), int32(y), int32(tailX), int32(tailY))

	// Wings
	r.renderer.DrawLine(int32(x), int32(y), int32(leftWingX), int32(leftWingY))
	r.renderer.DrawLine(int32(x), int32(y), int32(rightWingX), int32(rightWingY))

	// Tail
	r.renderer.DrawLine(int32(tailX), int32(tailY), int32(leftTailX), int32(leftTailY))
	r.renderer.DrawLine(int32(tailX), int32(tailY), int32(rightTailX), int32(rightTailY))

	// Draw selection box if selected
	if selected {
		boxSize := int32(20 * r.uiScale)
		r.renderer.DrawRect(&sdl.Rect{
			X: int32(x) - boxSize/2,
			Y: int32(y) - boxSize/2,
			W: boxSize,
			H: boxSize,
		})
	}
}

// drawTrails draws the trail of an aircraft's past positions
func (r *Renderer) drawTrails(a *adsb.Aircraft) {
	if len(a.Trail) < 2 {
		return
	}

	r.renderer.SetDrawColor(ColorTrail.R, ColorTrail.G, ColorTrail.B, 128)

	// Draw connecting lines between trail points
	for i := 0; i < len(a.Trail)-1; i++ {
		// Calculate opacity based on age
		age := 1.0 - float64(i)/float64(len(a.Trail))
		alpha := uint8(128 * age)

		// Convert trail positions to screen coordinates
		x1, y1 := r.latLonToScreen(a.Trail[i].Lat, a.Trail[i].Lon, r.centerLat, r.centerLon, r.maxDistance)
		x2, y2 := r.latLonToScreen(a.Trail[i+1].Lat, a.Trail[i+1].Lon, r.centerLat, r.centerLon, r.maxDistance)

		// Draw trail segment
		r.renderer.SetDrawColor(ColorTrail.R, ColorTrail.G, ColorTrail.B, alpha)
		r.renderer.DrawLine(int32(x1), int32(y1), int32(x2), int32(y2))
	}
}

// drawLabel draws aircraft information label
func (r *Renderer) drawLabel(a *adsb.Aircraft) {
	// Position label to the right of the aircraft
	x := a.X + 15
	y := a.Y - 15

	// Draw callsign
	_, h := r.drawText(a.Flight, x, y, r.boldFont, ColorLabel)

	// Draw altitude below callsign
	altText := ""
	if r.metric {
		altText = fmt.Sprintf(" %dm", int(float64(a.Altitude)/3.2828))
	} else {
		altText = fmt.Sprintf(" %d'", a.Altitude)
	}
	_, altH := r.drawText(altText, x, y+h, r.regularFont, ColorSubLabel)

	// Draw speed below altitude
	speedText := ""
	if r.metric {
		speedText = fmt.Sprintf(" %dkm/h", int(float64(a.Speed)*1.852))
	} else {
		speedText = fmt.Sprintf(" %dmph", a.Speed)
	}
	r.drawText(speedText, x, y+h+altH, r.regularFont, ColorSubLabel)
}

// drawText renders text and returns its dimensions
func (r *Renderer) drawText(text string, x, y int, font *ttf.Font, color sdl.Color) (int, int) {
	if len(text) == 0 {
		return 0, 0
	}

	surface, err := font.RenderUTF8Solid(text, color)
	if err != nil {
		return 0, 0
	}
	defer surface.Free()

	texture, err := r.renderer.CreateTextureFromSurface(surface)
	if err != nil {
		return 0, 0
	}
	defer texture.Destroy()

	rect := &sdl.Rect{
		X: int32(x),
		Y: int32(y),
		W: surface.W,
		H: surface.H,
	}
	r.renderer.Copy(texture, nil, rect)

	return int(surface.W), int(surface.H)
}

// drawScaleBars draws distance scale indicators
func (r *Renderer) drawScaleBars(maxDistance float64, metric bool) {
	// Find appropriate scale
	scalePower := 0
	for {
		dist := math.Pow10(scalePower)
		screenDist := float64(r.height/2) * (dist / maxDistance)
		if screenDist > 100 {
			break
		}
		scalePower++
	}

	// Convert distance to screen coordinates
	scaleBarDist := int(float64(r.height/2) * (math.Pow10(scalePower) / maxDistance))

	// Draw horizontal scale bar
	r.renderer.SetDrawColor(ColorScaleBar.R, ColorScaleBar.G, ColorScaleBar.B, ColorScaleBar.A)
	r.renderer.DrawLine(10, 10, 10+int32(scaleBarDist), 10)
	r.renderer.DrawLine(10, 10, 10, 20)
	r.renderer.DrawLine(10+int32(scaleBarDist), 10, 10+int32(scaleBarDist), 15)

	// Draw scale label
	scaleLabel := ""
	if metric {
		scaleLabel = fmt.Sprintf("%dkm", int(math.Pow10(scalePower)))
	} else {
		scaleLabel = fmt.Sprintf("%dnm", int(math.Pow10(scalePower)))
	}
	r.drawText(scaleLabel, 15+scaleBarDist, 15, r.regularFont, ColorScaleBar)
}

// drawStatus draws status information at the bottom of the screen
func (r *Renderer) drawStatus(aircraftCount int, centerLat, centerLon float64) {
	// Count visible aircraft
	visibleCount := 0

	// Format location text
	locText := fmt.Sprintf("loc %.4fN %.4f%c", centerLat,
		math.Abs(centerLon), map[bool]byte{true: 'E', false: 'W'}[centerLon >= 0])

	// Format aircraft count text
	dispText := fmt.Sprintf("disp %d/%d", visibleCount, aircraftCount)

	// Draw status boxes
	padding := 5
	x := padding
	y := r.height - 30

	// Location box
	w, _ := r.drawText(locText, x, y, r.boldFont, ColorScaleBar)
	x += w + padding

	// Display counts box
	r.drawText(dispText, x, y, r.boldFont, ColorScaleBar)
}
