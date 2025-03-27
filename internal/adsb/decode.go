package adsb

import (
	"math"
	"time"
)

// Aircraft represents a tracked aircraft with all its data
type Aircraft struct {
	ICAO       uint32
	Flight     string
	Altitude   int
	Speed      int
	Heading    int
	VertRate   int
	Lat        float64
	Lon        float64
	Seen       time.Time
	SeenLatLon time.Time
	X          int // Screen position
	Y          int // Screen position
	Trail      []Position
}

// Position represents a historical position with timestamp
type Position struct {
	Lat       float64
	Lon       float64
	Altitude  int
	Timestamp time.Time
}

// MessageType constants
const (
	DF17 = 17 // ADS-B message
	DF18 = 18 // ADS-B message via TIS-B
)

// TypeCode constants
const (
	TC_IDENT        = 4  // Aircraft identification
	TC_AIRBORNE_POS = 11 // Airborne position
	TC_AIRBORNE_VEL = 19 // Airborne velocity
)

// CPR represents Compact Position Reporting data
type CPRPosition struct {
	Lat     int  // CPR Latitude
	Lon     int  // CPR Longitude
	OddFlag bool // CPR Odd/Even flag
}

// DecodeIdentification extracts aircraft callsign from DF17/DF18 identification message
func DecodeIdentification(msg []byte) string {
	flight := ""
	charset := "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????"

	for i := 0; i < 8; i++ {
		var idx uint8
		if i < 4 {
			shift := uint((3 - i) * 6)
			idx = uint8((uint32(msg[5])<<8 | uint32(msg[6])) >> shift & 0x3F)
		} else {
			shift := uint((7 - i) * 6)
			idx = uint8((uint32(msg[7])<<8 | uint32(msg[8])) >> shift & 0x3F)
		}

		if idx < uint8(len(charset)) {
			flight += string(charset[idx])
		}
	}

	// Trim spaces
	return trimSpace(flight)
}

// DecodeAltitude extracts altitude from DF17/DF18 airborne position message
func DecodeAltitude(msg []byte) int {
	// Extract altitude field (AC12)
	ac12Field := ((uint16(msg[5]) << 4) | (uint16(msg[6]) >> 4)) & 0x0FFF

	if ac12Field == 0 {
		return 0
	}

	// Check Q bit for encoding type
	qBit := ac12Field & 0x10
	if qBit != 0 {
		// Q=1: 25ft encoding
		n := ((ac12Field & 0x0FE0) >> 1) | (ac12Field & 0x000F)
		return int(n*25) - 1000
	} else {
		// Q=0: Gillham encoding (simplified)
		return gillhamToAltitude(ac12Field)
	}
}

// DecodeCPRPosition extracts lat/lon in CPR format
func DecodeCPRPosition(msg []byte) CPRPosition {
	// Extract CPR latitude and longitude
	rawLat := ((uint32(msg[6]) & 0x03) << 15) | (uint32(msg[7]) << 7) | (uint32(msg[8]) >> 1)
	rawLon := ((uint32(msg[8]) & 0x01) << 16) | (uint32(msg[9]) << 8) | uint32(msg[10])

	// Determine if this is an odd or even frame
	oddFlag := (msg[6] & 0x04) != 0

	return CPRPosition{
		Lat:     int(rawLat),
		Lon:     int(rawLon),
		OddFlag: oddFlag,
	}
}

// DecodePosition uses a pair of CPR positions to decode actual latitude/longitude
func DecodePosition(evenPos, oddPos CPRPosition) (float64, float64, bool) {
	// CPR constants
	const airDlat0 = 360.0 / 60.0
	const airDlat1 = 360.0 / 59.0

	// CPR latitude index
	j := ((59 * float64(evenPos.Lat)) - (60 * float64(oddPos.Lat))) / 131072.0
	j = math.Floor(j + 0.5)

	// CPR latitude
	rlat0 := airDlat0 * (float64(cprMod(int(j), 60)) + (float64(evenPos.Lat) / 131072.0))
	rlat1 := airDlat1 * (float64(cprMod(int(j), 59)) + (float64(oddPos.Lat) / 131072.0))

	if rlat0 >= 270 {
		rlat0 -= 360
	}
	if rlat1 >= 270 {
		rlat1 -= 360
	}

	// Check latitude validity
	if rlat0 < -90 || rlat0 > 90 || rlat1 < -90 || rlat1 > 90 {
		return 0, 0, false
	}

	// Check that both are in the same latitude zone
	if cprNL(rlat0) != cprNL(rlat1) {
		return 0, 0, false
	}

	// Compute longitude
	var lat, lon float64
	if evenPos.OddFlag {
		// Use odd position
		lat = rlat1

		// Calculate longitude
		ni := cprN(rlat1, true)
		m := math.Floor(((float64(evenPos.Lon)*float64(cprNL(rlat1)-1))-
			(float64(oddPos.Lon)*float64(cprNL(rlat1))))/131072.0 + 0.5)

		lon = cprDlon(rlat1, true) * (float64(cprMod(int(m), ni)) + float64(oddPos.Lon)/131072.0)
	} else {
		// Use even position
		lat = rlat0

		// Calculate longitude
		ni := cprN(rlat0, false)
		m := math.Floor(((float64(evenPos.Lon)*float64(cprNL(rlat0)-1))-
			(float64(oddPos.Lon)*float64(cprNL(rlat0))))/131072.0 + 0.5)

		lon = cprDlon(rlat0, false) * (float64(cprMod(int(m), ni)) + float64(evenPos.Lon)/131072.0)
	}

	// Normalize longitude
	if lon > 180 {
		lon -= 360
	}

	return lat, lon, true
}

// DecodeVelocity extracts speed, heading, and vertical rate from velocity message
func DecodeVelocity(msg []byte) (int, int, int, bool) {
	subType := msg[4] & 0x07

	if subType != 1 && subType != 2 {
		return 0, 0, 0, false
	}

	// Superfast flag (subtype 2 uses 4x multiplier)
	superfast := subType == 2

	// Extract East-West and North-South velocity components
	ewDir := (msg[5] & 0x04) >> 2
	ewVel := int(((uint16(msg[5]) & 0x03) << 8) | uint16(msg[6]))
	nsDir := (msg[7] & 0x80) >> 7
	nsVel := int(((uint16(msg[7]) & 0x7F) << 3) | ((uint16(msg[8]) & 0xE0) >> 5))

	// Apply direction
	if ewDir != 0 {
		ewVel = -ewVel
	}
	if nsDir != 0 {
		nsVel = -nsVel
	}

	// Apply supersonic multiplier if needed
	if superfast {
		ewVel *= 4
		nsVel *= 4
	}

	// Calculate speed and heading
	speed := int(math.Sqrt(float64(ewVel*ewVel + nsVel*nsVel)))
	heading := 0

	if speed > 0 {
		heading = int(math.Atan2(float64(ewVel), float64(nsVel)) * 180 / math.Pi)
		if heading < 0 {
			heading += 360
		}
	}

	// Extract vertical rate
	vertRateSign := (msg[8] & 0x08) >> 3
	vertRateData := ((uint16(msg[8]) & 0x07) << 6) | ((uint16(msg[9]) & 0xFC) >> 2)

	vertRate := 0
	if vertRateData != 0 {
		vertRate = int(vertRateData * 64)
		if vertRateSign != 0 {
			vertRate = -vertRate
		}
	}

	return speed, heading, vertRate, true
}

// Helper functions for CPR decoding
func cprMod(a, b int) int {
	res := a % b
	if res < 0 {
		res += b
	}
	return res
}

func cprNL(lat float64) int {
	// NL function from 1090-WP-9-14
	if lat < 0 {
		lat = -lat
	}

	if lat < 10.47047130 {
		return 59
	}
	if lat < 14.82817437 {
		return 58
	}
	if lat < 18.18626357 {
		return 57
	}
	// ... (rest of the lookup table)
	if lat < 87.00000000 {
		return 2
	}
	return 1
}

func cprN(lat float64, odd bool) int {
	nl := cprNL(lat)
	if odd {
		return nl - 1
	}
	return nl
}

func cprDlon(lat float64, odd bool) float64 {
	return 360.0 / float64(cprN(lat, odd))
}

func gillhamToAltitude(code uint16) int {
	// Simplified Gillham code to altitude conversion
	// Real implementation is more complex
	return int(code) * 100
}

func trimSpace(s string) string {
	// Trim trailing spaces
	i := len(s)
	for i > 0 && s[i-1] == ' ' {
		i--
	}
	return s[:i]
}
