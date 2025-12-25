# Meshola

**Multi-protocol mesh messaging for Tactility OS**

Meshola is a protocol-agnostic mesh networking client for the LilyGo T-Deck running Tactility. It supports multiple mesh protocols including MeshCore, with a clean abstraction layer allowing easy addition of new protocols.

## Features

- 📡 **Multi-Protocol Support** - MeshCore, custom forks, and future protocols
- 💬 **Direct Messages** - End-to-end encrypted peer-to-peer messaging
- 📢 **Channels** - Group messaging with shared encryption
- 🔍 **Auto-Discovery** - Automatic peer discovery via advertisements
- ⚡ **Runtime Switching** - Change protocols without recompiling
- 🎛️ **Feature Detection** - UI adapts based on protocol capabilities

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Meshola App UI                           │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │ ChatView │ │  Peers   │ │ Channels │ │     Settings     │   │
│  │          │ │   View   │ │   View   │ │                  │   │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────────┬─────────┘   │
│       │            │            │                │              │
│       └────────────┴────────────┴────────────────┘              │
│                            │                                    │
│                    ┌───────▼───────┐                           │
│                    │  MeshService  │  (Background Thread)       │
│                    └───────┬───────┘                           │
└────────────────────────────┼────────────────────────────────────┘
                             │
              ┌──────────────▼──────────────┐
              │     IProtocol Interface      │
              │  (Protocol Abstraction Layer)│
              └──────────────┬──────────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
   ┌─────▼─────┐      ┌─────▼─────┐      ┌─────▼─────┐
   │ MeshCore  │      │CustomFork │      │ Meshtastic│
   │ (Standard)│      │  (Fork)   │      │ (Future)  │
   └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
         │                   │                   │
         └───────────────────┴───────────────────┘
                             │
                    ┌────────▼────────┐
                    │    RadioLib     │
                    │  (SX1262 Driver)│
                    └─────────────────┘
```

## Protocol Abstraction

The `IProtocol` interface decouples the UI from protocol specifics:

```cpp
class IProtocol {
    // Lifecycle
    virtual bool init(const RadioConfig& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    
    // Messaging
    virtual uint32_t sendMessage(const Contact& to, const char* text) = 0;
    virtual bool sendChannelMessage(const Channel& ch, const char* text) = 0;
    
    // Feature detection
    virtual bool hasFeature(ProtocolFeature feature) const = 0;
    
    // ... and more
};
```

### Adding a New Protocol

1. Create a class implementing `IProtocol`
2. Register it with `ProtocolRegistry::registerProtocol()`
3. It appears in Settings for selection

```cpp
class CustomForkProtocol : public IProtocol {
    // Implement interface methods...
};

// Register at startup
ProtocolRegistry::registerProtocol({
    .id = "customfork",
    .name = "CustomFork Mesh",
    .create = CustomForkProtocol::create
});
```

## Project Structure

```
Meshola/
├── manifest.properties     # Tactility app manifest
├── CMakeLists.txt         # Build configuration
├── README.md              # This file
├── main/
│   ├── CMakeLists.txt
│   └── Source/
│       ├── main.cpp               # Entry point
│       ├── MesholaApp.h/cpp       # Main app class
│       ├── protocol/
│       │   ├── IProtocol.h        # Protocol interface
│       │   ├── ProtocolRegistry.cpp
│       │   └── MeshCoreProtocol.h/cpp
│       ├── mesh/
│       │   └── MeshService.h/cpp  # Background service
│       └── views/                 # (TODO) Full view classes
└── lib/                           # (TODO) Protocol libraries
```

## Building

### Prerequisites

- ESP-IDF v5.x
- Tactility SDK
- TactilityTool

### Build Steps

```bash
# Set environment
export IDF_PATH=/path/to/esp-idf
export TACTILITY_SDK_PATH=/path/to/TactilitySDK

# Build
idf.py build

# Or use TactilityTool
python tactility.py build
```

## Development Status

### Completed ✅
- [x] Project structure
- [x] Protocol abstraction layer (IProtocol)
- [x] Protocol registry (runtime switching)
- [x] MeshCore protocol stub
- [x] MeshService (background thread)
- [x] Main app with navigation
- [x] Placeholder views (Chat, Peers, Channels, Settings)
- [x] Rebranded as Meshola

### In Progress 🚧
- [ ] Full ChatView implementation
- [ ] Full ContactsView implementation
- [ ] Full ChannelsView implementation
- [ ] Full SettingsView implementation

### Planned 📋
- [ ] Integrate actual MeshCore library
- [ ] Radio initialization for T-Deck SX1262
- [ ] Message persistence
- [ ] Contact persistence
- [ ] Channel configuration UI
- [ ] Protocol switching in Settings
- [ ] Hardware testing

## Contributing

Contributions welcome! This is an open-source project.

## License

[TBD]

## Links

- **Website**: [meshola.com](https://meshola.com)
- **MeshCore**: [github.com/meshcore-dev/MeshCore](https://github.com/meshcore-dev/MeshCore)
- **Tactility**: [github.com/ByteWelder/Tactility](https://github.com/ByteWelder/Tactility)
