# Meshola - Architecture Document

**Version:** 0.1.0  
**Last Updated:** December 25, 2024

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Layer Architecture](#layer-architecture)
3. [Component Details](#component-details)
4. [Data Flow](#data-flow)
5. [Storage Architecture](#storage-architecture)
6. [Protocol Abstraction](#protocol-abstraction)
7. [Threading Model](#threading-model)
8. [UI Architecture](#ui-architecture)

---

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                         MESHOLA APPLICATION                         │
├─────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                      UI LAYER (LVGL)                        │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐   │   │
│  │  │ ChatView │ │ Contacts │ │ Channels │ │   Settings   │   │   │
│  │  │          │ │   View   │ │   View   │ │     View     │   │   │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └──────┬───────┘   │   │
│  └───────┼────────────┼────────────┼──────────────┼───────────┘   │
│          │            │            │              │                │
│  ┌───────┴────────────┴────────────┴──────────────┴───────────┐   │
│  │                      MesholaApp                             │   │
│  │              (Main App Class / Navigation)                  │   │
│  └─────────────────────────┬───────────────────────────────────┘   │
│                            │                                        │
├────────────────────────────┼────────────────────────────────────────┤
│  ┌─────────────────────────┴───────────────────────────────────┐   │
│  │                     SERVICE LAYER                            │   │
│  │  ┌────────────────┐  ┌─────────────────┐  ┌──────────────┐  │   │
│  │  │  MeshService   │  │ ProfileManager  │  │ MessageStore │  │   │
│  │  │ (Background)   │  │  (Identity)     │  │ (Persistence)│  │   │
│  │  └───────┬────────┘  └────────┬────────┘  └──────┬───────┘  │   │
│  └──────────┼───────────────────┼───────────────────┼──────────┘   │
│             │                   │                   │               │
├─────────────┼───────────────────┼───────────────────┼───────────────┤
│  ┌──────────┴───────────────────┴───────────────────┴──────────┐   │
│  │                   PROTOCOL LAYER                             │   │
│  │  ┌──────────────────────────────────────────────────────┐   │   │
│  │  │              IProtocol Interface                      │   │   │
│  │  └──────────────────────┬───────────────────────────────┘   │   │
│  │                         │                                    │   │
│  │  ┌──────────┐  ┌───────┴────────┐  ┌──────────────────┐    │   │
│  │  │ MeshCore │  │   CustomFork   │  │    Meshtastic    │    │   │
│  │  │ Protocol │  │    Protocol    │  │    Protocol      │    │   │
│  │  │  (impl)  │  │    (future)    │  │    (future)      │    │   │
│  │  └────┬─────┘  └────────────────┘  └──────────────────┘    │   │
│  └───────┼──────────────────────────────────────────────────────┘   │
│          │                                                          │
├──────────┼──────────────────────────────────────────────────────────┤
│  ┌───────┴──────────────────────────────────────────────────────┐   │
│  │                    HARDWARE LAYER                             │   │
│  │  ┌──────────────┐  ┌─────────────┐  ┌────────────────────┐   │   │
│  │  │   RadioLib   │  │  ESP32 HAL  │  │  Tactility SDK     │   │   │
│  │  │   (SX1262)   │  │  (SPI/GPIO) │  │  (Display/Input)   │   │   │
│  │  └──────────────┘  └─────────────┘  └────────────────────┘   │   │
│  └──────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Layer Architecture

### UI Layer
- **Framework:** LVGL 8.x
- **Responsibility:** User interface rendering and input handling
- **Components:** Views (ChatView, ContactsView, ChannelsView, SettingsView)
- **Threading:** Runs on main LVGL thread, requires `tt_lvgl_lock()` for updates from other threads

### Application Layer
- **Component:** MesholaApp
- **Responsibility:** App lifecycle, navigation, event routing
- **Pattern:** Single main app class with view composition

### Service Layer
- **Components:** MeshService, ProfileManager, MessageStore
- **Responsibility:** Business logic, persistence, background operations
- **Pattern:** Singletons with getInstance()

### Protocol Layer
- **Interface:** IProtocol (abstract)
- **Implementations:** MeshCoreProtocol, (future: CustomForkProtocol, MeshtasticProtocol)
- **Pattern:** Strategy pattern with runtime switching via ProtocolRegistry

### Hardware Layer
- **Components:** RadioLib (SX1262), ESP-IDF drivers, Tactility SDK
- **Responsibility:** Hardware abstraction

---

## Component Details

### MesholaApp (`MesholaApp.h/cpp`)

Main application class implementing Tactility's `App` interface.

```cpp
class MesholaApp : public App {
    void onShow(AppHandle handle, lv_obj_t* parent);
    void onHide(AppHandle handle);
    
    // Navigation
    void showView(ViewType view);
    
    // Event handlers
    void onMessageReceived(const Message& msg);
    void onContactUpdated(const Contact& contact, bool isNew);
    void onAckReceived(uint32_t ackId, bool success);
    void onProfileSwitch(const Profile& newProfile);
};
```

**Responsibilities:**
- Create/destroy UI on show/hide
- Route between views via bottom navigation
- Wire up MeshService callbacks
- Handle profile switches

---

### MeshService (`mesh/MeshService.h/cpp`)

Background service managing protocol operations.

```cpp
class MeshService {
    static MeshService& getInstance();
    
    bool init();
    bool initWithProfile(const Profile& profile);
    bool reinitWithProfile(const Profile& profile);
    bool start();
    void stop();
    
    // Messaging
    bool sendMessage(const Contact& to, const char* text, uint32_t& outAckId);
    bool sendChannelMessage(const Channel& channel, const char* text);
    
    // Callbacks
    void setMessageCallback(MessageCallback callback);
    void setContactCallback(ContactCallback callback);
};
```

**Responsibilities:**
- Initialize protocol from active profile
- Run protocol loop in background thread (TODO)
- Forward events to UI callbacks
- Persist messages on receive (via MessageStore)

---

### ProfileManager (`profile/Profile.h`, `ProfileManager.cpp`)

Manages user profiles (identities/configurations).

```cpp
struct Profile {
    char id[16];              // UUID
    char name[32];            // Display name
    char protocolId[32];      // "meshcore", "customfork", etc.
    RadioConfig radio;        // Frequency, BW, SF, CR, TX power
    char nodeName[32];        // "Meshola-A3F7"
    uint8_t publicKey[32];
    uint8_t privateKey[32];
};

class ProfileManager {
    static ProfileManager& getInstance();
    
    const Profile* getActiveProfile();
    Profile* createProfile(const char* name);
    bool switchToProfile(const char* id);
    bool deleteProfile(const char* id);
};
```

**Storage:**
```
/data/meshola/
├── profiles.json           # Profile list + active ID
└── profiles/
    └── {profileId}/
        └── config.json     # Profile settings
```

---

### MessageStore (`storage/MessageStore.h/cpp`)

Persistent message storage using JSON Lines format.

```cpp
class MessageStore {
    static MessageStore& getInstance();
    
    void setActiveProfile(const char* profileId);
    bool appendMessage(const Message& msg);
    bool loadContactMessages(const uint8_t publicKey[32], int max, vector<Message>& out);
    bool loadChannelMessages(const uint8_t channelId[16], int max, vector<Message>& out);
};
```

**Storage:**
```
/data/meshola/profiles/{profileId}/messages/
├── dm_{contactKeyHex}.jsonl    # DM history per contact
└── ch_{channelIdHex}.jsonl     # Channel history per channel
```

**Format (JSON Lines):**
```json
{"ts":1703520000,"sk":"abc...","txt":"Hello","st":2,"isOut":true,"rssi":-85}
```

**Write Strategy:** Append immediately on receive/send (no message loss)

---

### IProtocol (`protocol/IProtocol.h`)

Abstract interface for mesh protocols.

```cpp
class IProtocol {
    // Lifecycle
    virtual bool init(const RadioConfig& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void loop() = 0;
    
    // Identity
    virtual const char* getNodeName() const = 0;
    virtual bool setNodeName(const char* name) = 0;
    virtual void getPublicKey(uint8_t out[32]) const = 0;
    
    // Messaging
    virtual uint32_t sendMessage(const Contact& to, const char* text) = 0;
    virtual bool sendChannelMessage(const Channel& ch, const char* text) = 0;
    
    // Feature detection
    virtual bool hasFeature(ProtocolFeature feature) const = 0;
    
    // Callbacks
    virtual void setMessageCallback(MessageCallback cb) = 0;
    virtual void setContactCallback(ContactCallback cb) = 0;
};
```

**Implementations:**
- `MeshCoreProtocol` - Standard MeshCore (stub, needs RadioLib integration)
- `CustomForkProtocol` - Future custom fork
- `MeshtasticProtocol` - Future Meshtastic support

---

### ChatView (`views/ChatView.h/cpp`)

Main messaging interface.

```cpp
class ChatView {
    void create(lv_obj_t* parent);
    void destroy();
    
    void setActiveContact(const Contact* contact);
    void setActiveChannel(const Channel* channel);
    void clearActiveConversation();
    
    void addMessage(const Message& msg);
    void updateMessageStatus(uint32_t ackId, MessageStatus status);
};
```

**UI Elements:**
- Welcome screen (when no conversation selected)
- Header bar (contact/channel name, status, signal info)
- Scrollable message list
- Message bubbles (left=incoming, right=outgoing)
- Input row (textarea + send button)

---

## Data Flow

### Sending a Message

```
User types message
        │
        ▼
[ChatView] onSendClicked()
        │
        ▼
[MesholaApp] onSendMessage()
        │
        ├──► [MeshService] sendMessage()
        │           │
        │           ▼
        │    [IProtocol] sendMessage()
        │           │
        │           ▼
        │    [Radio TX] ─────► RF
        │
        ├──► [MessageStore] appendMessage()  // Persist immediately
        │
        └──► [ChatView] addMessage()  // Show in UI
```

### Receiving a Message

```
RF ─────► [Radio RX]
              │
              ▼
       [IProtocol] (internal callback)
              │
              ▼
       [MeshService] messageCallback
              │
              ├──► [MessageStore] appendMessage()  // Persist immediately
              │
              └──► [MesholaApp] onMessageReceived()
                          │
                          ▼
                   [ChatView] addMessage()  // Show in UI (if active)
```

### Profile Switch

```
User selects new profile in Settings
              │
              ▼
       [ProfileManager] switchToProfile()
              │
              ├──► Save current profile
              │
              ├──► Load new profile
              │
              └──► Trigger callback
                          │
                          ▼
              [MesholaApp] onProfileSwitch()
                          │
                          ├──► [MeshService] reinitWithProfile()
                          │           │
                          │           ├──► Stop current protocol
                          │           ├──► Create new protocol instance
                          │           ├──► [MessageStore] setActiveProfile()
                          │           └──► Start new protocol
                          │
                          └──► [ChatView] clearActiveConversation()
```

---

## Storage Architecture

### Directory Structure

```
/data/meshola/
├── profiles.json                    # Profile list and active profile ID
└── profiles/
    ├── a1b2c3d4/                    # Profile "Home"
    │   ├── config.json              # Profile settings
    │   ├── contacts.json            # Contact list (TODO)
    │   ├── channels.json            # Channel list (TODO)
    │   └── messages/
    │       ├── dm_abc123...jsonl    # DMs with contact abc123...
    │       ├── dm_def456...jsonl    # DMs with contact def456...
    │       └── ch_112233...jsonl    # Messages in channel 112233...
    │
    └── e5f6g7h8/                    # Profile "CustomFork"
        ├── config.json
        └── messages/
            └── ...
```

### File Formats

**profiles.json:**
```json
{
  "activeProfileId": "a1b2c3d4",
  "profiles": [
    {"id": "a1b2c3d4", "name": "Home"},
    {"id": "e5f6g7h8", "name": "CustomFork"}
  ]
}
```

**config.json (per profile):**
```json
{
  "id": "a1b2c3d4",
  "name": "Home",
  "protocolId": "meshcore",
  "frequency": 906.875,
  "bandwidth": 250.0,
  "spreadingFactor": 11,
  "codingRate": 5,
  "txPower": 22,
  "nodeName": "Meshola-A3F7",
  "hasKeys": true,
  "publicKey": "abc123...",
  "privateKey": "def456..."
}
```

**messages/*.jsonl (JSON Lines):**
```
{"ts":1703520000,"sk":"abc...","txt":"Hello!","st":2,"isOut":false,"rssi":-85}
{"ts":1703520060,"sk":"abc...","txt":"Hi there","st":2,"isOut":true,"rssi":0}
```

---

## Protocol Abstraction

### Why Protocol Agnostic?

1. **Local flexibility** - CustomFork community may fork MeshCore for custom features
2. **Future compatibility** - Meshtastic integration possible without app rewrite
3. **Travel profiles** - Different protocols/settings for different regions
4. **Testing** - Mock protocol for UI development

### Registration Pattern

```cpp
// In MeshCoreProtocol.cpp
static const ProtocolEntry meshCoreEntry = {
    .id = "meshcore",
    .name = "MeshCore (Standard)",
    .create = MeshCoreProtocol::create
};

void MeshCoreProtocol::registerSelf() {
    ProtocolRegistry::registerProtocol(meshCoreEntry);
}

// At startup (MeshService constructor)
MeshCoreProtocol::registerSelf();
// CustomForkProtocol::registerSelf();  // Future
// MeshtasticProtocol::registerSelf();  // Future

// Creating instance
IProtocol* protocol = ProtocolRegistry::createProtocol("meshcore");
```

### Feature Detection

```cpp
if (protocol->hasFeature(ProtocolFeature::LocationSharing)) {
    // Show location UI
}

if (protocol->hasFeature(ProtocolFeature::Channels)) {
    // Show channels tab
}
```

---

## Threading Model

### Current (Single-Threaded)
- All operations on main LVGL thread
- Protocol loop called from app (TODO)

### Target (Multi-Threaded)
```
┌─────────────────┐     ┌─────────────────┐
│   LVGL Thread   │     │  Mesh Thread    │
│   (Main/UI)     │     │  (Background)   │
├─────────────────┤     ├─────────────────┤
│ MesholaApp      │     │ MeshService     │
│ Views           │◄────│ Protocol loop   │
│ Input handling  │     │ Radio RX/TX     │
│ Display updates │     │ Message persist │
└─────────────────┘     └─────────────────┘
        │                       │
        └───────┬───────────────┘
                │
        ┌───────▼───────┐
        │  Mutex Lock   │
        │ (tt_lvgl_lock)│
        └───────────────┘
```

**Thread Safety Rules:**
1. UI updates from mesh thread require `tt_lvgl_lock()`
2. MeshService methods use internal mutex
3. MessageStore file operations are atomic (single append)

---

## UI Architecture

### View Hierarchy

```
lv_obj_t* parent (from Tactility)
    │
    ├── Toolbar (tt_lvgl_toolbar)
    │
    ├── Content Container (flex grow)
    │       │
    │       └── [Active View]
    │           - ChatView
    │           - ContactsView (TODO)
    │           - ChannelsView (TODO)
    │           - SettingsView (placeholder)
    │
    └── Navigation Bar
            │
            ├── Chat Button
            ├── Peers Button
            ├── Channels Button
            └── Settings Button
```

### Color Scheme

| Constant | Hex | Usage |
|----------|-----|-------|
| COLOR_BG_DARK | 0x1a1a1a | Main background |
| COLOR_BG_CARD | 0x2d2d2d | Cards, input areas |
| COLOR_ACCENT | 0x0066cc | Buttons, active states |
| COLOR_ACCENT_DIM | 0x333333 | Inactive buttons |
| COLOR_TEXT | 0xffffff | Primary text |
| COLOR_TEXT_DIM | 0x888888 | Secondary text |
| COLOR_SUCCESS | 0x00aa55 | Online, delivered |
| COLOR_WARNING | 0xffaa00 | Warnings |
| COLOR_ERROR | 0xcc3333 | Failed, errors |
| COLOR_MSG_OUTGOING | 0x0055aa | Sent message bubbles |
| COLOR_MSG_INCOMING | 0x3d3d3d | Received message bubbles |

---

*This document is automatically updated as the architecture evolves.*
