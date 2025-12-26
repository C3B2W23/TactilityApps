# Meshola Messenger - Roadmap

**Last Updated:** December 25, 2024

---

## Version Overview

| Version | Target | Status | Focus |
|---------|--------|--------|-------|
| 0.1.0 | Dec 2024 | ✅ Complete | Core scaffold, profiles, chat UI, persistence |
| 0.1.5 | Dec 2024 | 🚨 CRITICAL | **Architectural revision - Tactility Service model** |
| 0.2.0 | Jan 2025 | 📋 Planned | ContactsView, ChannelsView, Profile Editor |
| 0.3.0 | Feb 2025 | 📋 Planned | MeshCore integration, actual radio ops |
| 0.4.0 | Mar 2025 | 📋 Planned | Polish, testing, beta release |
| 1.0.0 | Q2 2025 | 📋 Planned | Stable release |

---

## Milestone Details

### v0.1.5 - Architectural Revision 🚨 CRITICAL

**Status:** In Progress (December 25, 2024)

**Why This Is Critical:**
The original architecture had MeshService as part of the app. This means the radio stops when the user switches to any other app (Settings, Launcher, Maps, etc.). This makes the messenger completely non-functional as a background service.

**Discovery:** Tactility OS has a **Service** system that runs independently of apps:
- Services persist across app switches
- Services run in background continuously  
- GpsService provides reference implementation
- Apps subscribe to Service events via PubSub

**Tasks:**
- [ ] Create MeshService as a Tactility Service (not app component)
- [ ] Implement PubSub event types (MessageEvent, ContactEvent, StatusEvent)
- [ ] Move ProfileManager, MessageStore, IProtocol into MeshService
- [ ] Implement background thread for radio loop
- [ ] Refactor MesholaApp to be thin UI layer
- [ ] App subscribes to MeshService PubSub on show
- [ ] App unsubscribes on hide
- [ ] Test message reception while in other apps

**Deliverables:**
- MeshService runs independently of app lifecycle
- Messages received while user is in other apps
- Foundation ready for Meshola Maps integration

---

### v0.1.0 - Foundation ✅

**Status:** Complete (December 25, 2024)

**Delivered:**
- [x] Project structure and build system
- [x] Protocol abstraction layer (IProtocol)
- [x] Protocol registry for runtime switching
- [x] MeshCoreProtocol stub
- [x] Profile system (create, switch, delete)
- [x] Per-profile identity (keypairs, node name)
- [x] MeshService singleton (⚠️ needs revision to Tactility Service)
- [x] ChatView with message bubbles
- [x] Message persistence (JSON Lines)
- [x] Bottom navigation UI
- [x] Settings view (placeholder)
- [x] Documentation suite
- [x] ContactsView implementation

---

### v0.2.0 - Views & Interaction 📋

**Status:** Blocked by v0.1.5

**Goals:**
- [ ] ChannelsView - channel list and management
- [ ] Profile Editor - full UI for creating/editing profiles
- [ ] Contact persistence
- [ ] Channel persistence
- [ ] Improved Settings view

**Tasks:**
- [ ] Sort by name / last seen / signal strength
- [ ] Search/filter contacts

#### ChannelsView
- [ ] Create ChannelsView class
- [ ] List configured channels
- [ ] Tap to open channel chat
- [ ] Add channel dialog
- [ ] Edit channel dialog
- [ ] Delete channel confirmation
- [ ] Public/private channel indication

#### Profile Editor
- [ ] Full-screen profile editor
- [ ] Name input field
- [ ] Protocol selection dropdown
- [ ] Radio configuration inputs:
  - [ ] Frequency (with presets)
  - [ ] Bandwidth dropdown
  - [ ] Spreading factor dropdown
  - [ ] Coding rate dropdown
  - [ ] TX power slider
- [ ] Node name input
- [ ] Key management:
  - [ ] View public key
  - [ ] Export key option
  - [ ] Import key option
  - [ ] Regenerate key option
- [ ] Protocol-specific settings (dynamic)
- [ ] Save / Cancel buttons
- [ ] Delete profile button (with confirmation)

#### Persistence
- [ ] Contact storage (contacts.json per profile)
- [ ] Channel storage (channels.json per profile)
- [ ] JSON read implementation (currently write-only)
- [ ] Migration for storage format changes

---

### v0.3.0 - Radio Integration 📋

**Status:** Planned

**Goals:**
- [ ] Actual MeshCore library integration
- [ ] RadioLib SX1262 driver
- [ ] Real message TX/RX
- [ ] Peer discovery
- [ ] Path routing

**Tasks:**

#### MeshCore Integration
- [ ] Add MeshCore as library dependency
- [ ] Implement MeshCoreProtocol fully:
  - [ ] Radio initialization (SX1262 pins for T-Deck)
  - [ ] Identity from profile keys
  - [ ] Advertisement creation/parsing
  - [ ] Message encoding/decoding
  - [ ] Contact management from mesh
  - [ ] Channel management from mesh
- [ ] Background thread for mesh loop
- [ ] Thread-safe LVGL updates

#### Radio Operations
- [ ] Frequency configuration
- [ ] TX power configuration
- [ ] Channel activity detection
- [ ] RSSI/SNR reporting
- [ ] Error handling (busy, timeout, etc.)

#### Features
- [ ] Real peer discovery
- [ ] Real message sending
- [ ] Delivery acknowledgments
- [ ] Path routing visualization
- [ ] Signal strength indicators

---

### v0.4.0 - Polish & Beta 📋

**Status:** Planned

**Goals:**
- [ ] UI polish and refinement
- [ ] Performance optimization
- [ ] Comprehensive testing
- [ ] Bug fixes
- [ ] Beta release to testers

**Tasks:**

#### UI Polish
- [ ] Consistent styling throughout
- [ ] Loading states and spinners
- [ ] Error messages and dialogs
- [ ] Confirmation dialogs
- [ ] Toast notifications
- [ ] Animations (subtle)
- [ ] Keyboard navigation improvements

#### Performance
- [ ] Memory optimization
- [ ] Message list virtualization (for large histories)
- [ ] Lazy loading
- [ ] Background task optimization
- [ ] Battery usage optimization

#### Testing
- [ ] Manual test plan
- [ ] Multi-device testing
- [ ] Range testing
- [ ] Stress testing (many messages)
- [ ] Profile switch testing
- [ ] Crash recovery testing

#### Beta Program
- [ ] Build release binaries
- [ ] Distribution method
- [ ] Feedback collection
- [ ] Bug tracking

---

### v1.0.0 - Stable Release 📋

**Status:** Planned (Q2 2025)

**Goals:**
- [ ] Stable, reliable operation
- [ ] Full feature set
- [ ] Documentation complete
- [ ] Community release

**Requirements:**
- All v0.x features complete
- No critical bugs
- Performance acceptable
- User documentation ready
- Developer documentation ready

---

## Future Versions (Post 1.0)

### v1.1.0 - Additional Protocols
- [ ] Custom fork protocol implementation
- [ ] Meshtastic protocol implementation
- [ ] Protocol comparison/testing tools

### v1.2.0 - Advanced Features
- [ ] Message search
- [ ] Message reactions/replies
- [ ] File/image transfer
- [ ] Voice messages (?)
- [ ] Location sharing with map

### v1.3.0 - Administration
- [ ] Remote node administration
- [ ] Network visualization
- [ ] Diagnostic tools
- [ ] OTA updates

### v2.0.0 - Multi-Platform
- [ ] Desktop companion app
- [ ] Web interface
- [ ] API for integrations

---

## Feature Wishlist

Items that may be added based on user feedback and priorities:

### High Priority
- [ ] Dark/light theme toggle
- [ ] Font size options
- [ ] Notification sounds
- [ ] Message timestamps in local time
- [ ] Auto-reconnect on radio error

### Medium Priority
- [ ] Message pinning
- [ ] Contact nicknames
- [ ] Block/ignore contacts
- [ ] Export chat history
- [ ] Backup/restore profiles

### Low Priority
- [ ] Custom node icons/avatars
- [ ] Typing indicators
- [ ] Read receipts
- [ ] Group direct messages
- [ ] Scheduled messages

### Research Required
- [ ] Mesh network visualization
- [ ] Range estimation
- [ ] Power consumption monitoring
- [ ] Protocol bridging (MeshCore ↔ Meshtastic)

---

## Dependencies & Blockers

### External Dependencies
| Dependency | Status | Notes |
|------------|--------|-------|
| Tactility SDK | Available | Using v0.6.0 |
| MeshCore Library | Available | Need to integrate |
| RadioLib | Available | SX1262 support confirmed |
| LVGL | Available | Via Tactility |

### Potential Blockers
- T-Deck hardware availability for testing
- MeshCore API changes
- Tactility SDK breaking changes
- ESP-IDF version compatibility

---

## Contributing to Roadmap

To suggest changes to the roadmap:
1. Open an issue describing the feature/change
2. Discuss priority and feasibility
3. Add to appropriate milestone

---

*This roadmap is updated as priorities and timelines evolve.*
