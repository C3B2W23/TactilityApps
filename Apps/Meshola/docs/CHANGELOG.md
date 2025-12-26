# Meshola - Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Planned
- ContactsView - peer selection UI
- ChannelsView - channel management UI
- Profile editor UI
- MeshCore protocol integration (RadioLib + actual mesh operations)
- Contact persistence
- Channel persistence

---

## [0.1.0] - 2024-12-25

### Added

#### Core Architecture
- **Protocol Abstraction Layer** (`IProtocol` interface)
  - Support for multiple mesh protocols
  - Runtime protocol switching via `ProtocolRegistry`
  - Feature detection (`hasFeature()`)
  - MeshCoreProtocol stub implementation

- **Profile System** (`ProfileManager`)
  - Multiple profiles with separate identities
  - Per-profile keypair generation
  - Per-profile radio configuration
  - Per-profile chat history
  - Profile switching without app restart
  - JSON persistence for profile settings

- **Message Persistence** (`MessageStore`)
  - JSON Lines format (append-only)
  - Immediate save on message receive/send
  - Separate files per contact/channel
  - Load last 50 messages on conversation open

- **Background Service** (`MeshService`)
  - Singleton pattern
  - Profile-aware initialization
  - Event forwarding to UI callbacks
  - Thread-safe method wrappers (placeholder mutexes)

#### User Interface
- **MesholaApp** - Main application with bottom navigation
  - Chat, Peers, Channels, Settings tabs
  - Profile switch handling
  - Message/contact/ack event routing

- **ChatView** - Full messaging interface
  - Welcome screen when no conversation selected
  - Conversation header with status info
  - Message bubbles (left=incoming, right=outgoing)
  - Timestamps and delivery status indicators
  - RSSI display for incoming messages
  - Input field with send button
  - Auto-scroll to newest messages
  - Load history on conversation open

- **Settings View** (placeholder)
  - Profile dropdown for switching
  - New/Edit profile buttons (UI only)
  - Radio configuration display
  - Node name and public key display
  - Radio status indicator

#### Identity
- Unique node naming: `Meshola-XXXX` from ESP32 MAC address
- Per-profile public/private keypair generation

#### Data Types
- `Message` - Full message structure with status, RSSI, SNR
- `Contact` - Peer information with path/online status
- `Channel` - Channel configuration
- `RadioConfig` - Frequency, BW, SF, CR, TX power
- `Profile` - Complete profile configuration
- `NodeStatus` - Telemetry structure

### Technical Details
- Namespace: `meshola`
- App ID: `com.meshola.app`
- Target: ESP32-S3 (LilyGo T-Deck)
- Framework: Tactility OS with LVGL
- SDK Version: 0.6.0

### Storage Paths
```
/data/meshola/
├── profiles.json
└── profiles/{id}/
    ├── config.json
    └── messages/*.jsonl
```

---

## Development History

### 2024-12-25 - Initial Development Session

**Session 1: Project Scaffolding**
- Created project structure
- Defined IProtocol interface
- Implemented ProtocolRegistry
- Created MeshCoreProtocol stub
- Built MeshService skeleton
- Created MesholaApp with navigation
- Added placeholder views

**Session 2: Rebranding**
- Renamed from "MeshCore App" to "Meshola"
- Updated namespace to `meshola`
- Changed app ID to `com.meshola.app`
- Updated all class names and references

**Session 3: Unique Node Naming**
- Added `generateUniqueNodeName()` using ESP32 MAC
- Format: `Meshola-XXXX` (last 2 bytes of MAC as hex)

**Session 4: Profile System**
- Created `Profile` struct with full configuration
- Implemented `ProfileManager` singleton
- Added profile creation, switching, deletion
- JSON serialization for profile settings
- Updated MeshService for profile-aware init
- Added profile dropdown in Settings

**Session 5: ChatView**
- Implemented full ChatView class
- Welcome screen with node info
- Message bubbles with metadata
- Input field and send button
- History loading placeholder

**Session 6: Message Persistence**
- Created `MessageStore` singleton
- JSON Lines format for append-only writes
- Immediate persistence on send/receive
- History loading in ChatView
- Profile-aware storage paths

**Session 7: Documentation**
- Created comprehensive documentation suite
- PROJECT_OVERVIEW.md
- ARCHITECTURE.md
- USER_GUIDE.md
- DEVELOPER_GUIDE.md
- API_REFERENCE.md
- CHANGELOG.md
- ROADMAP.md
- TODO.md

---

## Version History Summary

| Version | Date | Summary |
|---------|------|---------|
| 0.1.0 | 2024-12-25 | Initial scaffold with profile system, ChatView, message persistence |

---

*This changelog is updated with each significant change to the project.*
