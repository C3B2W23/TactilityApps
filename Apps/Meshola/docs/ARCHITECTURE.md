# Meshola Messenger - Architecture Document

**Version:** 0.2.0  
**Last Updated:** December 25, 2024

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Tactility Service vs App Model](#tactility-service-vs-app-model)
3. [Component Architecture](#component-architecture)
4. [MeshService (Tactility Service)](#meshservice-tactility-service)
5. [Meshola Messenger (Tactility App)](#meshola-messenger-tactility-app)
6. [Data Flow](#data-flow)
7. [Storage Architecture](#storage-architecture)
8. [Protocol Abstraction](#protocol-abstraction)
9. [PubSub Event System](#pubsub-event-system)
10. [Threading Model](#threading-model)
11. [UI Architecture](#ui-architecture)
12. [Future: Meshola Maps Integration](#future-meshola-maps-integration)

---

## System Overview

**Critical Discovery (December 25, 2024):** Tactility OS has a **Service** system that runs independently of apps. Services persist across app switches, enabling background operations like radio RX/TX.

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           TACTILITY OS                                   │
├─────────────────────────────────────────────────────────────────────────┤
│  SERVICES (Always Running - Persist Across App Switches)                 │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌─────────────────────┐   │
│  │ WifiService│ │ GpsService │ │ GuiService │ │    MeshService      │   │
│  │ (built-in) │ │ (built-in) │ │ (built-in) │ │    (NEW - Ours)     │   │
│  │            │ │            │ │            │ │                     │   │
│  │            │ │ Coordinates│ │            │ │ • Radio RX/TX       │   │
│  │            │ │ PubSub     │ │            │ │ • Message Queue     │   │
│  │            │ │            │ │            │ │ • Contact Discovery │   │
│  │            │ │            │ │            │ │ • PubSub Events     │   │
│  └────────────┘ └────────────┘ └────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────────────────┤
│  APPS (One Foreground at a Time)                                         │
│  ┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐    │
│  │ Meshola Messenger │  │   Meshola Maps    │  │   Other Apps      │    │
│  │                   │  │                   │  │   (Launcher,      │    │
│  │ • Chat UI         │  │ • Map Display     │  │    Settings...)   │    │
│  │ • Contacts List   │  │ • Location Pins   │  │                   │    │
│  │ • Channels        │  │ • Track Routes    │  │                   │    │
│  │ • Settings        │  │                   │  │                   │    │
│  │                   │  │                   │  │                   │    │
│  │ Uses: MeshService │  │ Uses: MeshService │  │                   │    │
│  │       GpsService  │  │       GpsService  │  │                   │    │
│  └───────────────────┘  └───────────────────┘  └───────────────────┘    │
├─────────────────────────────────────────────────────────────────────────┤
│  HARDWARE LAYER                                                          │
│  ┌──────────────┐  ┌─────────────┐  ┌────────────┐  ┌──────────────┐    │
│  │   SX1262     │  │    GPS      │  │  Display   │  │   Keyboard   │    │
│  │   (LoRa)     │  │   Module    │  │  ST7789    │  │   + Trackball│    │
│  └──────────────┘  └─────────────┘  └────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────────────────┘
```

**Key Insight:** Messages are received even when user is in Maps, Settings, or Launcher because MeshService runs continuously as a Tactility Service.

---

## Tactility Service vs App Model

### Services (Background - Always Running)

```cpp
// Tactility Service base class
class Service {
    virtual bool onStart(ServiceContext& serviceContext) { return true; }
    virtual void onStop(ServiceContext& serviceContext) {}
};

// Service manifest for registration
struct ServiceManifest {
    std::string id;                    // "Mesh", "Gps", etc.
    CreateService createService;       // Factory function
};

// Registration (auto-start by default)
void addService(const ServiceManifest& manifest, bool autoStart = true);

// Finding services from anywhere
std::shared_ptr<Service> findServiceById(const std::string& id);
```

**Services:**
- Start when Tactility boots (if autoStart = true)
- Run continuously in background
- Persist across app switches
- Provide APIs for apps to call
- Use PubSub to broadcast events to subscribers

### Apps (Foreground - One at a Time)

```cpp
// Tactility App callbacks
struct AppManifest {
    void (*onShow)(AppHandle handle, lv_obj_t* parent);
    void (*onHide)(AppHandle handle);  // Called when going to background
    void (*onResult)(AppHandle handle, Result result);
};
```

**Apps:**
- One foreground app at a time
- `onShow` called when becoming visible
- `onHide` called when another app launches (goes to background)
- Can launch other apps and receive results
- Subscribe to Service PubSub for real-time updates

---

## Component Architecture

### Components Overview

| Component | Type | Purpose | Persists Across App Switch? |
|-----------|------|---------|----------------------------|
| **MeshService** | Tactility Service | Radio, messaging, contacts | ✅ YES |
| **ProfileManager** | Within MeshService | Identity/config management | ✅ YES |
| **MessageStore** | Within MeshService | Message persistence | ✅ YES |
| **IProtocol** | Within MeshService | Protocol abstraction | ✅ YES |
| **Meshola Messenger** | Tactility App | Chat UI | ❌ No (recreated) |
| **Meshola Maps** | Tactility App | Map UI | ❌ No (recreated) |

### Dependency Graph

```
┌─────────────────────────────────────────────────────────────────┐
│                    MeshService (Tactility Service)               │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Public API                            │    │
│  │  • sendMessage()      • getContacts()                   │    │
│  │  • sendChannelMessage() • getChannels()                 │    │
│  │  • getMessagePubSub() • getContactPubSub()              │    │
│  │  • getActiveProfile() • switchProfile()                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│  ┌───────────────┬───────────┴───────────┬─────────────────┐    │
│  │               │                       │                 │    │
│  ▼               ▼                       ▼                 │    │
│  ┌─────────┐  ┌─────────────┐  ┌──────────────┐           │    │
│  │ Profile │  │ MessageStore│  │  IProtocol   │           │    │
│  │ Manager │  │             │  │  Interface   │           │    │
│  │         │  │ (JSON Lines)│  │              │           │    │
│  └─────────┘  └─────────────┘  └──────┬───────┘           │    │
│                                       │                    │    │
│                          ┌────────────┼────────────┐       │    │
│                          ▼            ▼            ▼       │    │
│                    ┌──────────┐ ┌──────────┐ ┌──────────┐ │    │
│                    │ MeshCore │ │CustomFork│ │Meshtastic│ │    │
│                    │ Protocol │ │ Protocol │ │ Protocol │ │    │
│                    └──────────┘ └──────────┘ └──────────┘ │    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ findMeshService()
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         APPS                                     │
│  ┌─────────────────────┐    ┌─────────────────────┐             │
│  │  Meshola Messenger  │    │    Meshola Maps     │             │
│  │  ─────────────────  │    │  ─────────────────  │             │
│  │  • Subscribe to     │    │  • Subscribe to     │             │
│  │    message PubSub   │    │    contact PubSub   │             │
│  │  • Call sendMessage │    │  • Display contact  │             │
│  │  • Display chat UI  │    │    locations on map │             │
│  └─────────────────────┘    └─────────────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

---

## MeshService (Tactility Service)

### Header Definition

```cpp
// service/mesh/MeshService.h
#pragma once

#include "Tactility/Mutex.h"
#include "Tactility/PubSub.h"
#include "Tactility/service/Service.h"
#include "Tactility/service/ServiceContext.h"

#include "Protocol/IProtocol.h"
#include "Profile/ProfileManager.h"
#include "Storage/MessageStore.h"

namespace meshola::service {

// Event types for PubSub
struct MessageEvent {
    Message message;
    bool isNew;  // true = just received, false = loaded from storage
};

struct ContactEvent {
    Contact contact;
    bool isNew;  // true = newly discovered
};

struct StatusEvent {
    bool radioRunning;
    int contactCount;
    int pendingMessageCount;
};

class MeshService final : public tt::service::Service {

    tt::Mutex mutex = tt::Mutex(tt::Mutex::Type::Recursive);
    
    // Core components
    std::unique_ptr<ProfileManager> profileManager;
    std::unique_ptr<MessageStore> messageStore;
    std::unique_ptr<IProtocol> protocol;
    
    // PubSub for event broadcasting
    std::shared_ptr<tt::PubSub<MessageEvent>> messagePubSub;
    std::shared_ptr<tt::PubSub<ContactEvent>> contactPubSub;
    std::shared_ptr<tt::PubSub<StatusEvent>> statusPubSub;
    
    // Background thread
    std::unique_ptr<tt::Thread> meshThread;
    bool running = false;
    
    void meshLoop();
    void onMessageReceived(const Message& msg);
    void onContactDiscovered(const Contact& contact, bool isNew);

public:

    bool onStart(tt::service::ServiceContext& serviceContext) override;
    void onStop(tt::service::ServiceContext& serviceContext) override;
    
    // Messaging API
    bool sendMessage(const uint8_t recipientKey[32], const char* text, uint32_t& outAckId);
    bool sendChannelMessage(const uint8_t channelId[16], const char* text);
    
    // Contact API
    int getContactCount() const;
    bool getContact(int index, Contact& out) const;
    bool findContact(const uint8_t publicKey[32], Contact& out) const;
    
    // Channel API
    int getChannelCount() const;
    bool getChannel(int index, Channel& out) const;
    
    // Profile API
    const Profile* getActiveProfile() const;
    bool switchProfile(const char* profileId);
    ProfileManager* getProfileManager();
    
    // Message history API
    bool getMessages(const uint8_t contactKey[32], int maxCount, std::vector<Message>& out);
    bool getChannelMessages(const uint8_t channelId[16], int maxCount, std::vector<Message>& out);
    
    // Radio control
    bool startRadio();
    void stopRadio();
    bool isRadioRunning() const;
    
    // PubSub accessors (for apps to subscribe)
    std::shared_ptr<tt::PubSub<MessageEvent>> getMessagePubSub() const { return messagePubSub; }
    std::shared_ptr<tt::PubSub<ContactEvent>> getContactPubSub() const { return contactPubSub; }
    std::shared_ptr<tt::PubSub<StatusEvent>> getStatusPubSub() const { return statusPubSub; }
};

// Global accessor
std::shared_ptr<MeshService> findMeshService();

// Service manifest
extern const tt::service::ServiceManifest manifest;

} // namespace meshola::service
```

### Service Registration

```cpp
// service/mesh/MeshService.cpp

namespace meshola::service {

extern const tt::service::ServiceManifest manifest = {
    .id = "Mesh",
    .createService = tt::service::create<MeshService>
};

std::shared_ptr<MeshService> findMeshService() {
    auto service = tt::service::findServiceById(manifest.id);
    return std::static_pointer_cast<MeshService>(service);
}

} // namespace meshola::service
```

### Lifecycle

```cpp
bool MeshService::onStart(ServiceContext& serviceContext) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    
    // Initialize PubSub channels
    messagePubSub = std::make_shared<tt::PubSub<MessageEvent>>();
    contactPubSub = std::make_shared<tt::PubSub<ContactEvent>>();
    statusPubSub = std::make_shared<tt::PubSub<StatusEvent>>();
    
    // Initialize profile manager
    profileManager = std::make_unique<ProfileManager>();
    profileManager->init();
    
    // Initialize message store with active profile
    messageStore = std::make_unique<MessageStore>();
    messageStore->setActiveProfile(profileManager->getActiveProfile()->id);
    
    // Initialize protocol based on active profile
    const Profile* profile = profileManager->getActiveProfile();
    protocol = ProtocolRegistry::createProtocol(profile->protocolId);
    if (protocol) {
        protocol->init(profile->radio);
        protocol->setMessageCallback([this](const Message& msg) {
            onMessageReceived(msg);
        });
        protocol->setContactCallback([this](const Contact& contact, bool isNew) {
            onContactDiscovered(contact, isNew);
        });
    }
    
    TT_LOG_I("MeshService", "Started");
    return true;
}

void MeshService::onStop(ServiceContext& serviceContext) {
    stopRadio();
    TT_LOG_I("MeshService", "Stopped");
}
```

---

## Meshola Messenger (Tactility App)

### App Structure (Simplified)

The app is now much simpler - it's just UI that talks to MeshService:

```cpp
// MesholaApp.cpp

#include "MesholaApp.h"
#include "service/mesh/MeshService.h"

void MesholaApp::onShow(AppHandle handle, lv_obj_t* parent) {
    _handle = handle;
    
    // Get the mesh service
    _meshService = meshola::service::findMeshService();
    
    // Subscribe to message events
    _messageSubscription = _meshService->getMessagePubSub()->subscribe(
        [this](const MessageEvent& event) {
            tt_lvgl_lock();
            if (event.isNew) {
                _chatView.addMessage(event.message);
            }
            tt_lvgl_unlock();
        }
    );
    
    // Subscribe to contact events
    _contactSubscription = _meshService->getContactPubSub()->subscribe(
        [this](const ContactEvent& event) {
            tt_lvgl_lock();
            refreshContactList();
            tt_lvgl_unlock();
        }
    );
    
    // Build UI
    createUI(parent);
    
    // Load existing data from service
    refreshContactList();
    refreshChannelList();
}

void MesholaApp::onHide(AppHandle handle) {
    // Unsubscribe from PubSub
    _meshService->getMessagePubSub()->unsubscribe(_messageSubscription);
    _meshService->getContactPubSub()->unsubscribe(_contactSubscription);
    
    // UI cleanup handled by LVGL parent destruction
}

void MesholaApp::onSendMessage(const char* text) {
    if (_activeContact) {
        uint32_t ackId;
        _meshService->sendMessage(_activeContact->publicKey, text, ackId);
    }
}
```

---

## Data Flow

### Message Reception (Background)

```
Radio RX ──► IProtocol.loop() ──► MeshService.onMessageReceived()
                                         │
                                         ├──► MessageStore.appendMessage()
                                         │
                                         └──► messagePubSub.publish(event)
                                                      │
                                    ┌─────────────────┼─────────────────┐
                                    ▼                 ▼                 ▼
                            [Messenger App]   [Maps App]        [Not Running]
                            (if subscribed)   (if subscribed)   (stored for later)
```

### Message Sending (From App)

```
[User taps Send] ──► MesholaApp.onSendMessage()
                              │
                              ▼
                     MeshService.sendMessage()
                              │
                              ├──► IProtocol.sendMessage() ──► Radio TX
                              │
                              ├──► MessageStore.appendMessage()
                              │
                              └──► messagePubSub.publish(event)
```

### App Switch Scenario

```
User in Messenger ──► User opens Maps ──► Message arrives
        │                    │                   │
        ▼                    ▼                   ▼
  Messenger.onHide()   Maps.onShow()      MeshService receives
  (unsubscribe)        (subscribe)        (stores message)
                                          (publishes to Maps)
                                                 │
User returns to Messenger                        ▼
        │                               Maps shows notification
        ▼                               (optional)
  Messenger.onShow()
  (subscribe)
  (load messages from MeshService)
```

---

## Storage Architecture

### Directory Structure

```
/data/meshola/
├── service/                         # MeshService data
│   ├── profiles.json                # Profile list + active ID
│   └── profiles/
│       ├── {profileId}/
│       │   ├── config.json          # Profile settings
│       │   ├── contacts.json        # Contact list
│       │   ├── channels.json        # Channel list
│       │   └── messages/
│       │       ├── dm_{keyHex}.jsonl
│       │       └── ch_{idHex}.jsonl
│       └── ...
│
└── messenger/                       # App-specific settings (if any)
    └── ui_preferences.json          # UI state, last viewed contact, etc.
```

### Storage Ownership

| Data | Owner | Reason |
|------|-------|--------|
| Profiles | MeshService | Shared across apps |
| Contacts | MeshService | Shared across apps |
| Channels | MeshService | Shared across apps |
| Messages | MeshService | Must persist during app switches |
| UI preferences | Each App | App-specific |

---

## Protocol Abstraction

Same as before, but now lives within MeshService:

```cpp
class IProtocol {
    virtual bool init(const RadioConfig& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void loop() = 0;  // Called by MeshService background thread
    
    virtual uint32_t sendMessage(const Contact& to, const char* text) = 0;
    virtual void setMessageCallback(MessageCallback cb) = 0;
    virtual void setContactCallback(ContactCallback cb) = 0;
};
```

---

## PubSub Event System

Tactility provides a PubSub system for event broadcasting. MeshService uses this pattern (learned from GpsService):

```cpp
// Creating PubSub
std::shared_ptr<PubSub<MessageEvent>> messagePubSub = std::make_shared<PubSub<MessageEvent>>();

// Publishing events (from MeshService)
MessageEvent event = { .message = msg, .isNew = true };
messagePubSub->publish(event);

// Subscribing (from Apps)
auto subscriptionId = meshService->getMessagePubSub()->subscribe(
    [](const MessageEvent& event) {
        // Handle new message
    }
);

// Unsubscribing (on app hide)
meshService->getMessagePubSub()->unsubscribe(subscriptionId);
```

---

## Threading Model

### Service Thread

```
┌─────────────────────────────────────────────────────────────┐
│                    MeshService Thread                        │
│                    (Background - Always Running)             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│    while (running) {                                         │
│        protocol->loop();     // Radio RX/TX                  │
│        processIncoming();    // Handle received messages     │
│        checkTimeouts();      // ACK timeouts, etc.          │
│        vTaskDelay(10ms);                                    │
│    }                                                         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### LVGL Thread (Main)

```
┌─────────────────────────────────────────────────────────────┐
│                    LVGL Thread (Main)                        │
│                    (UI Rendering)                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│    Apps run here                                             │
│    PubSub callbacks run here (after tt_lvgl_lock)           │
│    All UI updates happen here                                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Thread Safety

```cpp
// MeshService methods use internal mutex
bool MeshService::sendMessage(...) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    // ... thread-safe operations
}

// PubSub callbacks in apps must lock LVGL
_meshService->getMessagePubSub()->subscribe([this](const MessageEvent& event) {
    tt_lvgl_lock();
    _chatView.addMessage(event.message);
    tt_lvgl_unlock();
});
```

---

## UI Architecture

### View Hierarchy (App Only)

```
lv_obj_t* parent (from Tactility)
    │
    ├── Toolbar (tt_lvgl_toolbar)
    │
    ├── Content Container (flex grow)
    │       │
    │       └── [Active View]
    │           - ChatView
    │           - ContactsView
    │           - ChannelsView
    │           - SettingsView
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
| COLOR_TEXT | 0xffffff | Primary text |
| COLOR_TEXT_DIM | 0x888888 | Secondary text |
| COLOR_SUCCESS | 0x00aa55 | Online, delivered |
| COLOR_MSG_OUTGOING | 0x0055aa | Sent message bubbles |
| COLOR_MSG_INCOMING | 0x3d3d3d | Received message bubbles |

---

## Future: Meshola Maps Integration

Both apps share MeshService:

```cpp
// In Meshola Maps
void MesholaMapsApp::onShow(...) {
    _meshService = meshola::service::findMeshService();
    
    // Subscribe to contact updates (for location pins)
    _contactSub = _meshService->getContactPubSub()->subscribe(
        [this](const ContactEvent& event) {
            if (event.contact.hasLocation) {
                updateMapPin(event.contact);
            }
        }
    );
    
    // Also use GpsService for own location
    _gpsService = tt::service::gps::findGpsService();
    // ...
}
```

---

## Key Lessons Learned

1. **Tactility Services persist across app switches** - Critical for background radio operation
2. **Tactility uses PubSub for event broadcasting** - Apps subscribe to service events
3. **GpsService is the reference implementation** - Our MeshService follows the same pattern
4. **One app foreground at a time** - Apps have onShow/onHide lifecycle
5. **Services can be found from anywhere** - `findServiceById()` pattern
6. **Thread safety via Mutex** - Both services and LVGL require locking

---

*This document reflects the architectural revision discovered on December 25, 2024.*
