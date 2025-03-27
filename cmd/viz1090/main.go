package main

import (
	"flag"
	"fmt"
	"os"
	"os/signal"
	"syscall"

	"github.com/OJPARKINSON/viz1090/internal/app"
	"github.com/OJPARKINSON/viz1090/internal/config"
)

func main() {
	// Parse command line flags
	cfg := config.DefaultConfig()

	// Define flags
	flag.StringVar(&cfg.ServerAddress, "server", cfg.ServerAddress, "Beast server address")
	flag.IntVar(&cfg.ServerPort, "port", cfg.ServerPort, "Beast server port")
	flag.Float64Var(&cfg.InitialLat, "lat", cfg.InitialLat, "Initial latitude")
	flag.Float64Var(&cfg.InitialLon, "lon", cfg.InitialLon, "Initial longitude")
	flag.BoolVar(&cfg.Metric, "metric", cfg.Metric, "Use metric units")
	flag.BoolVar(&cfg.Fullscreen, "fullscreen", cfg.Fullscreen, "Fullscreen mode")
	flag.IntVar(&cfg.ScreenWidth, "width", cfg.ScreenWidth, "Screen width")
	flag.IntVar(&cfg.ScreenHeight, "height", cfg.ScreenHeight, "Screen height")
	flag.IntVar(&cfg.UIScale, "uiscale", cfg.UIScale, "UI scaling factor")
	flag.Parse()

	// Create and initialize application
	application := app.New(cfg)
	if err := application.Initialize(); err != nil {
		fmt.Fprintf(os.Stderr, "Error initializing application: %v\n", err)
		os.Exit(1)
	}
	defer application.Cleanup()

	// Setup signal handling for clean shutdown
	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigCh
		application.Cleanup()
		os.Exit(0)
	}()

	// Run the application
	if err := application.Run(); err != nil {
		fmt.Fprintf(os.Stderr, "Error running application: %v\n", err)
		os.Exit(1)
	}
}
