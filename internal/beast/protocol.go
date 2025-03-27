package beast

import (
	"errors"
	"io"
)

// Message types
const (
	ModeAC    = '1' // Mode A/C message
	ModeShort = '2' // Mode S short message
	ModeLong  = '3' // Mode S long message
)

// Message lengths (excluding escape bytes)
const (
	ModeACLen    = 2
	ModeShortLen = 7
	ModeLongLen  = 14
)

// Beast protocol constants
const (
	EscapeChar = 0x1A
)

// Message represents a decoded Beast protocol message
type Message struct {
	Type        byte
	Timestamp   uint64
	SignalLevel byte
	Data        []byte
}

// Decoder reads and decodes Beast format messages from an io.Reader
type Decoder struct {
	r        io.Reader
	buffer   []byte
	msgBuf   []byte
	escaping bool
}

// NewDecoder creates a new Beast protocol decoder
func NewDecoder(r io.Reader) *Decoder {
	return &Decoder{
		r:      r,
		buffer: make([]byte, 0, 256),
		msgBuf: make([]byte, 0, 32),
	}
}

// ReadMessage reads and decodes the next Beast message
func (d *Decoder) ReadMessage() (*Message, error) {
	// Read more data if buffer is empty
	if len(d.buffer) == 0 {
		buf := make([]byte, 4096)
		n, err := d.r.Read(buf)
		if err != nil {
			return nil, err
		}
		d.buffer = append(d.buffer, buf[:n]...)
	}

	// Process buffer until we have a complete message
	for len(d.buffer) > 0 {
		b := d.buffer[0]
		d.buffer = d.buffer[1:]

		if d.escaping {
			// Previous byte was an escape character
			d.msgBuf = append(d.msgBuf, b)
			d.escaping = false
		} else if b == EscapeChar {
			// Current byte is an escape character
			if len(d.buffer) == 0 {
				// Need more data to see if this is an escape sequence
				d.buffer = append([]byte{EscapeChar}, d.buffer...)
				return nil, io.ErrUnexpectedEOF
			}

			// Look at next byte
			nextByte := d.buffer[0]
			if nextByte == '1' || nextByte == '2' || nextByte == '3' {
				// Start of a new message
				if len(d.msgBuf) > 0 {
					// Process the message we've collected so far
					msg, err := d.parseMessage()
					if err == nil {
						return msg, nil
					}
					// If we couldn't parse it, just start a new one
				}

				// Start new message buffer
				d.msgBuf = []byte{EscapeChar, nextByte}
				d.buffer = d.buffer[1:]
			} else {
				// Escaped data byte
				d.escaping = true
			}
		} else {
			// Regular data byte
			d.msgBuf = append(d.msgBuf, b)
		}

		// Check if we have enough data for a complete message
		if len(d.msgBuf) >= 2 {
			// First byte should be escape char, second is message type
			if d.msgBuf[0] != EscapeChar {
				// Invalid format, reset and try again
				d.msgBuf = d.msgBuf[:0]
				continue
			}

			var expectedLen int
			switch d.msgBuf[1] {
			case ModeAC:
				expectedLen = 2 + 6 + 1 + ModeACLen // 0x1A + type + timestamp + signal + MODEAC
			case ModeShort:
				expectedLen = 2 + 6 + 1 + ModeShortLen // 0x1A + type + timestamp + signal + Short Mode S
			case ModeLong:
				expectedLen = 2 + 6 + 1 + ModeLongLen // 0x1A + type + timestamp + signal + Long Mode S
			default:
				// Invalid message type, reset
				d.msgBuf = d.msgBuf[:0]
				continue
			}

			// Check if we have a full message
			if len(d.msgBuf) >= expectedLen {
				msg, err := d.parseMessage()
				d.msgBuf = d.msgBuf[:0]
				if err != nil {
					continue // Try to find next valid message
				}
				return msg, nil
			}
		}
	}

	// Need more data
	return nil, io.ErrUnexpectedEOF
}

// parseMessage extracts fields from the message buffer
func (d *Decoder) parseMessage() (*Message, error) {
	if len(d.msgBuf) < 9 { // At minimum: 0x1A + type + 6-byte timestamp + signal level
		return nil, errors.New("message too short")
	}

	// Get message type
	msgType := d.msgBuf[1]
	if msgType != ModeAC && msgType != ModeShort && msgType != ModeLong {
		return nil, errors.New("invalid message type")
	}

	// Get expected data length
	var dataLen int
	switch msgType {
	case ModeAC:
		dataLen = ModeACLen
	case ModeShort:
		dataLen = ModeShortLen
	case ModeLong:
		dataLen = ModeLongLen
	}

	// Process timestamp (6 bytes, big endian)
	timestamp := uint64(0)
	for i := 0; i < 6; i++ {
		timestamp = (timestamp << 8) | uint64(d.msgBuf[2+i])
	}

	// Signal level
	signalLevel := d.msgBuf[8]

	// Message data
	if len(d.msgBuf) < 9+dataLen {
		return nil, errors.New("message data incomplete")
	}
	data := make([]byte, dataLen)
	copy(data, d.msgBuf[9:9+dataLen])

	return &Message{
		Type:        msgType,
		Timestamp:   timestamp,
		SignalLevel: signalLevel,
		Data:        data,
	}, nil
}

// Encode creates a Beast format message
func Encode(msgType byte, data []byte, timestamp uint64, signalLevel byte) []byte {
	// Estimate buffer size (message + possible escape bytes)
	buf := make([]byte, 0, 2+6+1+len(data)*2)

	// Message tag
	buf = append(buf, EscapeChar, msgType)

	// Timestamp (big endian)
	for i := 5; i >= 0; i-- {
		b := byte((timestamp >> (8 * i)) & 0xFF)
		buf = append(buf, b)
		if b == EscapeChar {
			buf = append(buf, b) // Escape
		}
	}

	// Signal level
	buf = append(buf, signalLevel)
	if signalLevel == EscapeChar {
		buf = append(buf, signalLevel) // Escape
	}

	// Data
	for _, b := range data {
		buf = append(buf, b)
		if b == EscapeChar {
			buf = append(buf, b) // Escape
		}
	}

	return buf
}
