#!/bin/bash
# setup-macos.sh

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
fi

# Install required libraries
echo "Installing SDL2 libraries..."
brew install sdl2 sdl2_ttf

# Install Go if not already installed
if ! command -v go &> /dev/null; then
    echo "Go not found. Installing..."
    brew install go
fi

# Create directories
mkdir -p bin
mkdir -p font

# Download required fonts if they don't exist
if [ ! -f "font/TerminusTTF-4.46.0.ttf" ] || [ ! -f "font/TerminusTTF-Bold-4.46.0.ttf" ]; then
    echo "Downloading fonts..."
    curl -L -o terminus-ttf.zip "https://files.ax86.net/terminus-ttf/files/terminus-ttf-4.46.0.zip"
    unzip -j terminus-ttf.zip "terminus-ttf-4.46.0/TerminusTTF-*.ttf" -d font/
    rm terminus-ttf.zip
fi

# Initialize Go modules if not already done
if [ ! -f "go.mod" ]; then
    echo "Initializing Go modules..."
    go mod init viz1090-go
    go get -u github.com/veandco/go-sdl2/sdl
    go get -u github.com/veandco/go-sdl2/ttf
fi

echo "Setup complete! You can now build the project with 'make build'."