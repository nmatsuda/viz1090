package config

// Config stores application configuration settings
type Config struct {
	// Network settings
	ServerAddress string
	ServerPort    int

	// Display settings
	ScreenWidth  int
	ScreenHeight int
	Fullscreen   bool
	UIScale      int
	Metric       bool

	// Initial map settings
	InitialLat  float64
	InitialLon  float64
	InitialZoom float64

	// Visualization options
	ShowTrails  bool
	TrailLength int
	LabelDetail int
}

// DefaultConfig returns a configuration with sensible defaults
func DefaultConfig() *Config {
	return &Config{
		ServerAddress: "localhost",
		ServerPort:    30005,
		ScreenWidth:   0, // Auto-detect
		ScreenHeight:  0, // Auto-detect
		Fullscreen:    false,
		UIScale:       1,
		Metric:        false,
		InitialLat:    37.6188,
		InitialLon:    -122.3756,
		InitialZoom:   50.0, // NM
		ShowTrails:    true,
		TrailLength:   50,
		LabelDetail:   2,
	}
}

// LoadFromFile loads configuration from a file
func LoadFromFile(filename string) (*Config, error) {
	// Implementation would read a YAML/JSON/TOML file
	// and parse it into a Config struct

	// For simplicity, just return default config for now
	return DefaultConfig(), nil
}

// SaveToFile saves current configuration to a file
func (c *Config) SaveToFile(filename string) error {
	// Implementation would serialize the Config struct
	// to YAML/JSON/TOML and write to a file
	return nil
}
