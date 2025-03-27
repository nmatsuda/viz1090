package main

import (
	"fmt"
	"math"
	"math/rand"
	"net"
	"strings"
	"time"
)

// Beast message format:
// 1A + msg type (1, 2 or 3) + 6 bytes timestamp + signal level + message data
// Types:
// 1: Mode A/C (not used here)
// 2: Mode S short message
// 3: Mode S long message

type Aircraft struct {
	ICAO      uint32
	Lat       float64
	Lon       float64
	Alt       int
	Speed     int
	Heading   int
	Callsign  string
	UpdatePos bool
}

func generateBeastMessage(msgType byte, data []byte, timestamp uint64, signalLevel byte) []byte {
	msg := make([]byte, 0, 2+6+1+len(data))
	msg = append(msg, 0x1A, msgType)

	// Timestamp - 6 bytes big endian
	for i := 5; i >= 0; i-- {
		timeVal := byte((timestamp >> (8 * i)) & 0xFF)
		msg = append(msg, timeVal)
		if timeVal == 0x1A { // Ensure 0x1A is properly escaped
			msg = append(msg, 0x1A)
		}
	}

	// Signal level
	msg = append(msg, signalLevel)
	if signalLevel == 0x1A {
		msg = append(msg, 0x1A)
	}

	// Data
	for _, b := range data {
		msg = append(msg, b)
		if b == 0x1A { // Escape 0x1A
			msg = append(msg, 0x1A)
		}
	}

	return msg
}

func createADSBIdentMessage(aircraft *Aircraft) []byte {
	// DF17 (Extended squitter) + ADS-B Identification message
	msgData := make([]byte, 14)

	// DF17, CA=5
	msgData[0] = 0x8D

	// ICAO address
	msgData[1] = byte((aircraft.ICAO >> 16) & 0xFF)
	msgData[2] = byte((aircraft.ICAO >> 8) & 0xFF)
	msgData[3] = byte(aircraft.ICAO & 0xFF)

	// Type code = 4 (aircraft ID)
	msgData[4] = 0x20

	// Callsign
	callsign := aircraft.Callsign
	if len(callsign) < 8 {
		callsign = callsign + strings.Repeat(" ", 8-len(callsign))
	}

	// Encode callsign (6 bits per character according to ADS-B spec)
	var cs1, cs2, cs3, cs4 uint32
	for i := 0; i < 8; i++ {
		var ch byte
		if i < len(callsign) {
			ch = callsign[i]
		} else {
			ch = ' '
		}

		var val uint32
		switch {
		case ch >= 'A' && ch <= 'Z':
			val = uint32(ch - 'A' + 1)
		case ch >= '0' && ch <= '9':
			val = uint32(ch - '0' + 48)
		case ch == ' ':
			val = 32
		}

		if i < 2 {
			cs1 |= val << (6 * (1 - i))
		} else if i < 4 {
			cs2 |= val << (6 * (3 - i))
		} else if i < 6 {
			cs3 |= val << (6 * (5 - i))
		} else {
			cs4 |= val << (6 * (7 - i))
		}
	}

	msgData[5] = byte((cs1 >> 8) & 0xFF)
	msgData[6] = byte(cs1 & 0xFF)
	msgData[7] = byte((cs2 >> 8) & 0xFF)
	msgData[8] = byte(cs2 & 0xFF)
	msgData[9] = byte((cs3 >> 8) & 0xFF)
	msgData[10] = byte(cs3 & 0xFF)
	msgData[11] = byte((cs4 >> 8) & 0xFF)
	msgData[12] = byte(cs4 & 0xFF)

	// CRC is intentionally not computed here as the client will accept it regardless

	return msgData
}

func createADSBPositionMessage(aircraft *Aircraft) []byte {
	// DF17 (Extended squitter) + ADS-B Airborne Position message
	msgData := make([]byte, 14)

	// DF17, CA=5
	msgData[0] = 0x8D

	// ICAO address
	msgData[1] = byte((aircraft.ICAO >> 16) & 0xFF)
	msgData[2] = byte((aircraft.ICAO >> 8) & 0xFF)
	msgData[3] = byte(aircraft.ICAO & 0xFF)

	// Type code = 11 (airborne position) + odd/even flag
	typeCode := byte(11 << 3)
	if aircraft.UpdatePos {
		typeCode |= 1 // Odd format
	}
	msgData[4] = typeCode

	// Altitude encoding (simplified)
	altCode := (aircraft.Alt + 1000) / 25
	msgData[5] = byte((altCode >> 4) & 0xFF)
	msgData[6] = byte((altCode & 0x0F) << 4)

	// CPR encoding (simplified)
	// Note: Real CPR is more complex, this is a simplification
	latCPR := uint32((aircraft.Lat / 360.0) * 131072)
	lonCPR := uint32((aircraft.Lon / 360.0) * 131072)

	msgData[6] |= byte((latCPR >> 15) & 0x0F)
	msgData[7] = byte((latCPR >> 7) & 0xFF)
	msgData[8] = byte((latCPR & 0x7F) << 1)
	msgData[8] |= byte((lonCPR >> 16) & 0x01)
	msgData[9] = byte((lonCPR >> 8) & 0xFF)
	msgData[10] = byte(lonCPR & 0xFF)

	// CRC is intentionally not computed here as the client will accept it regardless

	return msgData
}

func createADSBVelocityMessage(aircraft *Aircraft) []byte {
	// DF17 (Extended squitter) + ADS-B Airborne Velocity message
	msgData := make([]byte, 14)

	// DF17, CA=5
	msgData[0] = 0x8D

	// ICAO address
	msgData[1] = byte((aircraft.ICAO >> 16) & 0xFF)
	msgData[2] = byte((aircraft.ICAO >> 8) & 0xFF)
	msgData[3] = byte(aircraft.ICAO & 0xFF)

	// Type code = 19 (airborne velocity) + subtype 1 (ground speed)
	msgData[4] = 0x99

	// Intent change flag, IFR capability flag, velocity uncertainty
	msgData[5] = 0x00

	// East-West velocity component
	ewVel := int(float64(aircraft.Speed) * math.Sin(float64(aircraft.Heading)*math.Pi/180.0))
	ewDir := 0
	if ewVel < 0 {
		ewDir = 1
		ewVel = -ewVel
	}
	ewVel = ewVel + 1 // Using 1kt resolution
	msgData[5] |= byte(ewDir << 2)
	msgData[5] |= byte((ewVel >> 8) & 0x03)
	msgData[6] = byte(ewVel & 0xFF)

	// North-South velocity component
	nsVel := int(float64(aircraft.Speed) * math.Cos(float64(aircraft.Heading)*math.Pi/180.0))
	nsDir := 0
	if nsVel < 0 {
		nsDir = 1
		nsVel = -nsVel
	}
	nsVel = nsVel + 1 // Using 1kt resolution
	msgData[7] = byte(nsDir << 7)
	msgData[7] |= byte((nsVel >> 3) & 0x7F)
	msgData[8] = byte((nsVel & 0x07) << 5)

	// Vertical rate
	msgData[8] |= 0x00 // No vertical rate
	msgData[9] = 0x00

	// Remaining fields not used

	return msgData
}

func moveAircraft(aircraft *Aircraft) {
	// Move aircraft based on its heading and speed
	distance := float64(aircraft.Speed) * 0.0001 // Scale for a reasonable movement per update
	aircraft.Lat += distance * math.Cos(float64(aircraft.Heading)*math.Pi/180.0)
	aircraft.Lon += distance * math.Sin(float64(aircraft.Heading)*math.Pi/180.0)

	// Add some randomness to flight path
	aircraft.Heading += rand.Intn(3) - 1
	if aircraft.Heading >= 360 {
		aircraft.Heading -= 360
	} else if aircraft.Heading < 0 {
		aircraft.Heading += 360
	}

	// Alternate between odd and even position messages
	aircraft.UpdatePos = !aircraft.UpdatePos
}

func handleConnection(conn net.Conn, aircraft []*Aircraft) {
	defer conn.Close()
	fmt.Println("Client connected:", conn.RemoteAddr())

	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	timestamp := uint64(time.Now().UnixNano() / 1000000)

	for {
		select {
		case <-ticker.C:
			timestamp += 1000 // 1 second

			for _, a := range aircraft {
				moveAircraft(a)

				// Send identification message
				if rand.Intn(10) < 1 { // Only send occasionally
					idMsg := createADSBIdentMessage(a)
					beastMsg := generateBeastMessage('3', idMsg, timestamp, byte(rand.Intn(100)+100))
					conn.Write(beastMsg)
				}

				// Send position message
				posMsg := createADSBPositionMessage(a)
				beastMsg := generateBeastMessage('3', posMsg, timestamp, byte(rand.Intn(100)+100))
				conn.Write(beastMsg)

				// Send velocity message
				velMsg := createADSBVelocityMessage(a)
				beastMsg2 := generateBeastMessage('3', velMsg, timestamp, byte(rand.Intn(100)+100))
				conn.Write(beastMsg2)

				time.Sleep(50 * time.Millisecond) // Slight delay between aircraft updates
			}
		}
	}
}

func main() {
	rand.Seed(time.Now().UnixNano())

	// Create some sample aircraft
	aircraft := []*Aircraft{
		{
			ICAO:     0xABCDEF,
			Lat:      37.6188,
			Lon:      -122.3756,
			Alt:      10000,
			Speed:    450,
			Heading:  45,
			Callsign: "SWA1234",
		},
		{
			ICAO:     0x123456,
			Lat:      37.7749,
			Lon:      -122.4194,
			Alt:      25000,
			Speed:    500,
			Heading:  270,
			Callsign: "UAL789",
		},
		{
			ICAO:     0x789ABC,
			Lat:      37.8716,
			Lon:      -122.2727,
			Alt:      35000,
			Speed:    550,
			Heading:  180,
			Callsign: "DAL456",
		},
	}

	// Start Beast server
	listener, err := net.Listen("tcp", "0.0.0.0:30005")
	if err != nil {
		fmt.Println("Error starting server:", err)
		return
	}
	defer listener.Close()

	fmt.Println("Beast server running on port 30005")

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Error accepting connection:", err)
			continue
		}

		go handleConnection(conn, aircraft)
	}
}
