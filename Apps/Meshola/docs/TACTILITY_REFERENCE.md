# Tactility OS Reference

**Last Updated:** December 25, 2024

This document captures what we've learned about Tactility OS architecture, specifically for developing Meshola applications.

---

## Overview

Tactility is an operating system for ESP32 devices that provides:
- App framework with manifest-based loading
- Service system for background operations
- LVGL-based UI
- FreeRTOS foundation

---

## Service vs App Model

### Key Discovery

Tactility has TWO distinct component types:

| Aspect | Service | App |
|--------|---------|-----|
| Lifecycle | Runs continuously | One foreground at a time |
| Persistence | Persists across app switches | Destroyed on hide |
| UI | No UI (headless) | Full LVGL UI |
| Purpose | Background operations | User interaction |
| Example | GpsService, WifiService | Calculator, Files, Chat |

### Why This Matters

For a messenger app, the radio must run in background to receive messages. If radio is part of the app, it stops when user switches to Settings or Launcher. **MeshService must be a Tactility Service.**

---

## Service Implementation

### Base Class

```cpp
// Tactility/Include/Tactility/service/Service.h
namespace tt::service {

class Service {
public:
    Service() = default;
    virtual ~Service() = default;

    virtual bool onStart(ServiceContext& serviceContext) { return true; }
    virtual void onStop(ServiceContext& serviceContext) {}
};

template<typename T>
std::shared_ptr<Service> create() { return std::shared_ptr<T>(new T); }

}
```

### Service Manifest

```cpp
// Tactility/Include/Tactility/service/ServiceManifest.h
struct ServiceManifest {
    std::string id;                    // Unique identifier (e.g., "Gps", "Mesh")
    CreateService createService;       // Factory function
};
```

### Registration & Discovery

```cpp
// Tactility/Include/Tactility/service/ServiceRegistration.h

// Register a service (autoStart = true by default)
void addService(const ServiceManifest& manifest, bool autoStart = true);

// Start/stop manually
bool startService(const std::string& id);
bool stopService(const std::string& id);

// Get service state
State getState(const std::string& id);

// Find a running service
std::shared_ptr<Service> findServiceById(const std::string& id);

// Templated version for type safety
template <typename T>
std::shared_ptr<T> findServiceById(const std::string& id);
```

---

## GpsService Reference Implementation

GpsService is the best reference for how to build a Tactility Service.

### Header Pattern

```cpp
// Key elements from GpsService.h
class GpsService final : public Service {

    // Thread safety
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    Mutex stateMutex;
    
    // Internal state
    std::vector<GpsDeviceRecord> deviceRecords;
    State state = State::Off;
    
    // PubSub for event broadcasting
    std::shared_ptr<PubSub<State>> statePubSub = std::make_shared<PubSub<State>>();
    
    // Service paths (for storage)
    std::unique_ptr<ServicePaths> paths;

public:
    // Lifecycle
    bool onStart(ServiceContext& serviceContext) override;
    void onStop(ServiceContext& serviceContext) override;
    
    // Public API
    bool startReceiving();
    void stopReceiving();
    State getState() const;
    bool hasCoordinates() const;
    bool getCoordinates(minmea_sentence_rmc& rmc) const;
    
    // PubSub accessor
    std::shared_ptr<PubSub<State>> getStatePubsub() const { return statePubSub; }
};

// Global finder function
std::shared_ptr<GpsService> findGpsService();
```

### Implementation Pattern

```cpp
// Key elements from GpsService.cpp

// Service manifest definition
extern const ServiceManifest manifest = {
    .id = "Gps",
    .createService = create<GpsService>
};

// Global finder
std::shared_ptr<GpsService> findGpsService() {
    auto service = findServiceById(manifest.id);
    return std::static_pointer_cast<GpsService>(service);
}

// Lifecycle
bool GpsService::onStart(ServiceContext& serviceContext) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    paths = serviceContext.getPaths();
    return true;
}

void GpsService::onStop(ServiceContext& serviceContext) {
    if (getState() == State::On) {
        stopReceiving();
    }
}

// State management with PubSub
void GpsService::setState(State newState) {
    auto lock = stateMutex.asScopedLock();
    lock.lock();
    state = newState;
    lock.unlock();
    statePubSub->publish(state);  // Notify all subscribers
}
```

---

## PubSub System

Tactility provides a PubSub system for event broadcasting.

### Creating PubSub

```cpp
std::shared_ptr<PubSub<MyEventType>> myPubSub = std::make_shared<PubSub<MyEventType>>();
```

### Publishing Events

```cpp
MyEventType event = { /* ... */ };
myPubSub->publish(event);
```

### Subscribing (from Apps)

```cpp
auto subscriptionId = service->getPubSub()->subscribe(
    [this](const MyEventType& event) {
        // Handle event
        // IMPORTANT: Lock LVGL if updating UI
        tt_lvgl_lock();
        updateUI(event);
        tt_lvgl_unlock();
    }
);
```

### Unsubscribing

```cpp
service->getPubSub()->unsubscribe(subscriptionId);
```

---

## App Lifecycle

### App Callbacks

```cpp
// From tt_app.h
struct AppManifest {
    void (*onShow)(AppHandle handle, lv_obj_t* parent);
    void (*onHide)(AppHandle handle);  // "going to background"
    void (*onResult)(AppHandle handle, Result result);
};
```

### Lifecycle Flow

```
App Launch:
  onShow() called → Build UI → Subscribe to services

App Background (another app launched):
  onHide() called → Unsubscribe from services → UI destroyed

App Resume:
  onShow() called again → Rebuild UI → Resubscribe to services
```

### Important Notes

1. `onHide` is called when app goes to background, NOT when destroyed
2. UI is destroyed when app goes to background
3. Apps must rebuild UI in `onShow` each time
4. Services continue running - apps must fetch latest state on `onShow`

---

## Thread Safety

### Service Threading

Services typically run on their own thread or use callbacks from hardware interrupts.

```cpp
// Use Mutex for thread safety
Mutex mutex = Mutex(Mutex::Type::Recursive);

void MyService::someMethod() {
    auto lock = mutex.asScopedLock();
    lock.lock();
    // Thread-safe operations
}
```

### LVGL Thread Safety

**CRITICAL:** All LVGL operations must happen on the main thread or with lock held.

```cpp
// When updating UI from a callback
tt_lvgl_lock();
lv_label_set_text(myLabel, "New text");
tt_lvgl_unlock();
```

---

## File Paths

### Services

Services get their own storage paths via `ServiceContext`:

```cpp
bool MyService::onStart(ServiceContext& serviceContext) {
    paths = serviceContext.getPaths();
    // paths->getDataPath() returns service-specific data directory
}
```

### Apps

Apps have separate storage from services.

---

## Built-in Services

| Service | ID | Purpose |
|---------|-----|---------|
| WifiService | "Wifi" | WiFi connectivity |
| GpsService | "Gps" | GPS coordinates |
| GuiService | "Gui" | Display management |
| EspNowService | "EspNow" | ESP-NOW communication |

---

## Best Practices

1. **Services for background operations** - Radio, GPS, network
2. **Apps for UI** - Keep apps thin, delegate to services
3. **PubSub for events** - Don't poll, subscribe
4. **Lock LVGL** - Always lock when updating UI from callbacks
5. **Unsubscribe on hide** - Prevent callbacks to destroyed UI
6. **Fetch state on show** - Services may have changed while app was hidden

---

## References

- Tactility GitHub: https://github.com/ByteWelder/Tactility
- Tactility Website: https://tactility.one
- GpsService: `Tactility/Source/service/gps/GpsService.cpp`
- Service headers: `Tactility/Include/Tactility/service/`

---

*This document is updated as we learn more about Tactility.*
