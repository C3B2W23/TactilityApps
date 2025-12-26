# Meshola Messenger - Project Overview
**Version:** 0.1.5  
**Last Updated:** December 25, 2024  
**Status:** Active Development - Architectural Revision In Progress
---

## Critical Update (December 25, 2024)

**Discovery:** Tactility OS has a Service system separate from Apps. Services run in the background and persist across app switches. This is critical for a messenger app - without it, radio stops when user switches apps.

**Impact:** MeshService must be rewritten as a Tactility Service (not part of the app). This enables:
- Background message reception
- Shared service between Meshola Messenger and Meshola Maps
- Proper mesh network participation

---
## What is Meshola Messenger?
Meshola Messenger is a **protocol-agnostic mesh networking chat application** for the LilyGo T-Deck running Tactility OS. It provides a unified interface for communicating over various mesh protocols including MeshCore, custom forks, and potentially Meshtastic in the future.
The name "Meshola Messenger" combines "Mesh" (mesh networking) with a unique, brandable suffix. The domain meshola.com is owned by the project maintainer.
---
## Vision
Create a professional-grade mesh messaging application that:
1. **Works across protocols** - Switch between MeshCore, Custom fork, Meshtastic without changing apps
2. **Maintains separate identities** - Each profile has its own keys, contacts, and chat history
3. **Persists everything** - Never lose messages, even on crash or power loss
4. **Runs in background** - Receive messages even when in other apps (via Tactility Service)
5. **Runs on T-Deck hardware** - Optimized for ESP32-S3 with physical keyboard and 320x240 display
6. **Enables community building** - Support local mesh communities with easy setup and reliable messaging
---
## Target Hardware
**Primary Platform:** LilyGo T-Deck  
- ESP32-S3 microcontroller
- 320x240 TFT display (ST7789)
- Physical QWERTY keyboard
- Onboard SX1262 LoRa radio (868/915 MHz)
- GPS module
- Trackball navigation
- Battery powered with charging
**Operating System:** Tactility OS  
- ESP-IDF based
- LVGL for graphics
- **Service system** for background operations (critical discovery!)
- App framework with manifest-based loading
---
## Key Features
### Current (v0.1.0)
- [x] Protocol abstraction layer (IProtocol interface)
- [x] Profile system (multiple identities/networks)
- [x] ChatView with message bubbles
- [x] Message persistence (append-on-receive)
- [x] Unique node naming (Meshola-XXXX from MAC)
- [x] Per-profile keypair generation
- [x] Bottom navigation (Chat, Peers, Channels, Settings)
- [x] Settings view with profile switching
- [x] ContactsView with peer list
### In Progress (v0.1.5) - CRITICAL
- [ ] **Rewrite MeshService as Tactility Service**
- [ ] Implement PubSub event system
- [ ] Refactor app to subscribe to service
- [ ] Enable background message reception
### Planned (v0.2.0)
- [ ] ChannelsView - channel management
- [ ] Profile editor UI
- [ ] Actual MeshCore protocol integration
- [ ] Radio initialization
### Future
- [ ] Meshola Maps integration (shares MeshService)
- [ ] Meshtastic protocol support
- [ ] Custom fork protocol
- [ ] Message search
- [ ] File/image transfer
- [ ] Location sharing
- [ ] Remote node administration
---
## Project Links
- **Domain:** meshola.com (owned)
- **Repository:** github.com/YourUsername/TactilityApps (Apps/Meshola)
- **MeshCore:** github.com/meshcore-dev/MeshCore
- **Tactility:** github.com/ByteWelder/Tactility
---
## Team
- **Lead Developer:** [Your Name]
- **AI Assistant:** Claude (Anthropic) - Architecture, code generation, documentation
---
## Related Projects
- **T900 Pager Conversion** - Vintage Motorola pager to MeshCore node
---
## Document Index
| Document | Purpose |
|----------|---------|
| [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) | This file - high-level project summary |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Technical architecture and design |
| [USER_GUIDE.md](USER_GUIDE.md) | End-user documentation |
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | Developer setup and contribution guide |
| [API_REFERENCE.md](API_REFERENCE.md) | Code API documentation |
| [CHANGELOG.md](CHANGELOG.md) | Version history and changes |
| [ROADMAP.md](ROADMAP.md) | Feature roadmap and milestones |
| [TODO.md](TODO.md) | Current tasks and status |
---
*This document is automatically updated as the project evolves.*
