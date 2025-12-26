# Meshola Messenger - Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### CRITICAL: Architectural Revision Required
**Discovery Date:** December 25, 2024

**Problem Identified:** Original architecture had MeshService as part of the app. This means:
- Radio stops when user switches to another app
- No background message reception
- App is useless as a messenger

**Solution Found:** Tactility has a Service system separate from Apps:
- Services run continuously in background
- Services persist across app switches
- Apps subscribe to Service events via PubSub
- GpsService provides reference implementation

**Impact:**
- MeshService must be rewritten as a Tactility Service
- App becomes thin UI layer that subscribes to Service
- Enables background message reception
- Enables Meshola Maps to share the same MeshService

### Added
- **ContactsView** - Full peer list view with:
  - List of discovered peers with name, signal strength, status
  - Online/offline indicator dots
  - Last seen timestamps
  - Hop count display
  - Tap to open chat with peer
  - Broadcast advertisement button
  - Refresh button
  - Empty state when no peers discovered
- **Tactility Service Research:**
  - Analyzed Service.h, ServiceManifest.h, ServiceRegistration.h
  - Studied GpsService.h/cpp as reference implementation
  - Documented PubSub event pattern
  - Updated architecture documentation

### Changed
- Rebranded to "Meshola Messenger"
- App ID changed to `com.meshola.messenger`
- Storage path changed to `/data/meshola/messenger/` for future app compatibility

### Planned (Revised)
- **Rewrite MeshService as Tactility Service** (CRITICAL)
- Implement PubSub events (MessageEvent, ContactEvent, StatusEvent)
- Refactor MesholaApp to be thin UI layer
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
- App ID: `com.meshola.messenger`
- Target: ESP32-S3 (LilyGo T-Deck)
- Framework: Tactility OS with LVGL
- SDK Version: 0.6.0

### Storage Paths
```
/data/meshola/messenger/
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
- Changed app ID to `com.meshola.messenger`
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
