# Makefile for viz1090-go

.PHONY: all build clean run mock

GO=go
BINARY_DIR=bin
SERVER_BINARY=$(BINARY_DIR)/mockserver
APP_BINARY=$(BINARY_DIR)/viz1090

all: build

$(BINARY_DIR):
	mkdir -p $(BINARY_DIR)

build: $(BINARY_DIR) $(SERVER_BINARY) $(APP_BINARY)

$(SERVER_BINARY): cmd/mockserver/main.go
	$(GO) build -o $(SERVER_BINARY) cmd/mockserver/main.go

$(APP_BINARY): cmd/viz1090/main.go
	$(GO) build -o $(APP_BINARY) cmd/viz1090/main.go

clean:
	rm -rf $(BINARY_DIR)

run: $(APP_BINARY)
	$(APP_BINARY) $(ARGS)

mock: $(SERVER_BINARY)
	$(SERVER_BINARY)

install-deps:
	$(GO) get -u github.com/veandco/go-sdl2/sdl
	$(GO) get -u github.com/veandco/go-sdl2/ttf