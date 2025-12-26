# Meshola Messenger - Developer Guide

**Version:** 0.1.0  
**Last Updated:** December 25, 2024

---

## Table of Contents

1. [Development Setup](#development-setup)
2. [Project Structure](#project-structure)
3. [Building](#building)
4. [Code Style](#code-style)
5. [Adding a New Protocol](#adding-a-new-protocol)
6. [Adding a New View](#adding-a-new-view)
7. [Testing](#testing)
8. [Contributing](#contributing)

---

## Development Setup

### Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| ESP-IDF | 5.x | ESP32 development framework |
| Tactility SDK | 0.6.0+ | Application framework |
| Git | 2.x | Version control |
| Python | 3.8+ | Build scripts |

### Clone Repository

```bash
# Clone TactilityApps (contains Meshola)
git clone git@github.com:YourUsername/TactilityApps.git
cd TactilityApps

# Meshola is in Apps/Meshola
cd Apps/Meshola
```

### Environment Setup

```bash
# Set ESP-IDF path
export IDF_PATH=/path/to/esp-idf
source $IDF_PATH/export.sh

# Set Tactility SDK path
export TACTILITY_SDK_PATH=/path/to/TactilitySDK
```

### IDE Setup (Cursor/VS Code)

Recommended extensions:
- C/C++ (Microsoft)
- PlatformIO (optional)
- ESP-IDF (Espressif)

---

## Project Structure

```
Meshola/
├── manifest.properties          # Tactility app manifest
├── CMakeLists.txt              # Top-level CMake
├── README.md                   # Quick start guide
│
└── main/
    ├── CMakeLists.txt          # Component CMake
    └── Source/
        ├── main.cpp            # Entry point
        ├── MesholaApp.h        # Main app class
        ├── MesholaApp.cpp
        │
        ├── protocol/           # Protocol abstraction
        │   ├── IProtocol.h     # Interface + data types
        │   ├── ProtocolRegistry.cpp
        │   ├── MeshCoreProtocol.h
        │   └── MeshCoreProtocol.cpp
        │
        ├── profile/            # Identity management
        │   ├── Profile.h       # Profile struct + ProfileManager
        │   └── ProfileManager.cpp
        │
        ├── storage/            # Persistence
        │   ├── MessageStore.h
        │   └── MessageStore.cpp
        │
        ├── mesh/               # Background service
        │   ├── MeshService.h
        │   └── MeshService.cpp
        │
        └── views/              # UI components
            ├── ChatView.h
            └── ChatView.cpp
```

### Key Files

| File | Purpose |
|------|---------|
| `IProtocol.h` | Protocol interface + all data types (Message, Contact, Channel, etc.) |
| `Profile.h` | Profile struct + ProfileManager class |
| `MeshService.h` | Singleton service managing protocol lifecycle |
| `MessageStore.h` | Message persistence with JSON Lines |
| `MesholaApp.h` | Main app with navigation and event routing |
| `ChatView.h` | Messaging UI implementation |

---

## Building

### Using ESP-IDF

```bash
cd Apps/Meshola

# Configure (first time)
idf.py set-target esp32s3

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor
```

### Using Tactility Tool

```bash
cd TactilityApps
python tactility.py build Apps/Meshola
python tactility.py flash Apps/Meshola
```

### Build Output

```
build/
├── Meshola.bin          # Application binary
└── Meshola.elf          # Debug symbols
```

---

## Code Style

### Naming Conventions

| Element | Style | Example |
|---------|-------|---------|
| Classes | PascalCase | `MeshService`, `ChatView` |
| Methods | camelCase | `sendMessage()`, `getNodeName()` |
| Member variables | _camelCase | `_protocol`, `_messageCallback` |
| Constants | UPPER_SNAKE | `MAX_MESSAGE_LEN`, `COLOR_ACCENT` |
| Namespaces | lowercase | `meshola` |
| Files | PascalCase | `MeshService.cpp`, `IProtocol.h` |

### Code Organization

```cpp
// File: Example.cpp

#include "Example.h"          // Own header first
#include "other/Internal.h"   // Project headers
#include <system_header.h>    // System headers
#include <cstring>            // C++ standard

namespace meshola {

// Constants at top
static const uint32_t SOME_CONSTANT = 42;

// Static/singleton instance
static Example* s_instance = nullptr;

// Implementation
Example::Example() : _member(0) {
}

} // namespace meshola
```

### LVGL UI Code

```cpp
// Create objects with clear hierarchy
auto* container = lv_obj_create(parent);
lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);

// Style consistently
lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
lv_obj_set_style_border_width(container, 0, LV_STATE_DEFAULT);
lv_obj_set_style_pad_all(container, 8, LV_STATE_DEFAULT);

// Event callbacks as static methods or lambdas
lv_obj_add_event_cb(button, onButtonClicked, LV_EVENT_CLICKED, this);
```

---

## Adding a New Protocol

### 1. Create Header

```cpp
// protocol/MyProtocol.h
#pragma once
#include "IProtocol.h"

namespace meshola {

class MyProtocol : public IProtocol {
public:
    MyProtocol();
    ~MyProtocol() override;

    // Implement all IProtocol methods...
    bool init(const RadioConfig& config) override;
    bool start() override;
    void stop() override;
    // ... etc

    // Factory
    static IProtocol* create();
    static void registerSelf();

private:
    // Protocol-specific members
};

} // namespace meshola
```

### 2. Create Implementation

```cpp
// protocol/MyProtocol.cpp
#include "MyProtocol.h"

namespace meshola {

static const ProtocolEntry myProtocolEntry = {
    .id = "myprotocol",
    .name = "My Protocol",
    .create = MyProtocol::create
};

void MyProtocol::registerSelf() {
    ProtocolRegistry::registerProtocol(myProtocolEntry);
}

IProtocol* MyProtocol::create() {
    return new MyProtocol();
}

// Implement all methods...

} // namespace meshola
```

### 3. Register at Startup

```cpp
// In MeshService constructor
MeshService::MeshService() {
    MeshCoreProtocol::registerSelf();
    MyProtocol::registerSelf();  // Add this
}
```

### 4. Test

Create a profile using the new protocol and verify:
- Protocol appears in Settings dropdown
- Messages can be sent/received
- Contacts are discovered
- State persists correctly

---

## Adding a New View

### 1. Create Header

```cpp
// views/MyView.h
#pragma once
#include <lvgl.h>

namespace meshola {

class MyView {
public:
    MyView();
    ~MyView();
    
    void create(lv_obj_t* parent);
    void destroy();
    void refresh();

private:
    lv_obj_t* _container;
    // UI elements...
    
    static void onSomeEvent(lv_event_t* event);
};

} // namespace meshola
```

### 2. Create Implementation

```cpp
// views/MyView.cpp
#include "MyView.h"

namespace meshola {

MyView::MyView() : _container(nullptr) {
}

MyView::~MyView() {
    destroy();
}

void MyView::create(lv_obj_t* parent) {
    _container = lv_obj_create(parent);
    // Build UI...
}

void MyView::destroy() {
    _container = nullptr;  // LVGL cleans up children
}

} // namespace meshola
```

### 3. Integrate with MesholaApp

```cpp
// In MesholaApp.h
#include "views/MyView.h"

class MesholaApp {
    // ...
    MyView _myView;
};

// In MesholaApp.cpp showView()
case ViewType::MyView:
    _myView.create(_contentContainer);
    break;
```

---

## Testing

### Unit Testing

Currently no formal unit test framework. Testing is done via:
- Manual testing on hardware
- Serial debug output
- Log analysis

### Testing Checklist

- [ ] App launches without crash
- [ ] Navigation between tabs works
- [ ] Profile creation works
- [ ] Profile switching works
- [ ] Messages persist across restart
- [ ] UI updates on incoming messages
- [ ] Memory usage stable over time

### Debug Output

```cpp
#include <esp_log.h>

static const char* TAG = "Meshola";

ESP_LOGI(TAG, "Message sent: %s", text);
ESP_LOGW(TAG, "No contacts found");
ESP_LOGE(TAG, "Failed to save profile");
```

---

## Contributing

### Git Workflow

```bash
# Create feature branch
git checkout -b feature/my-feature

# Make changes
# ...

# Commit with descriptive message
git add .
git commit -m "Add feature X with Y functionality"

# Push
git push origin feature/my-feature

# Create PR (if contributing upstream)
```

### Commit Message Format

```
<type>: <short description>

<optional longer description>

<optional references>
```

Types:
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation
- `refactor:` Code restructuring
- `style:` Formatting
- `test:` Testing
- `chore:` Maintenance

Example:
```
feat: Add message persistence with JSON Lines

- Messages saved immediately on receive/send
- Load last 50 messages when opening conversation
- Separate files per contact/channel

Closes #123
```

### Pull Request Checklist

- [ ] Code compiles without warnings
- [ ] Tested on hardware
- [ ] Documentation updated
- [ ] No memory leaks
- [ ] Follows code style

---

## Useful Resources

### Tactility
- [Tactility GitHub](https://github.com/ByteWelder/Tactility)
- [Tactility SDK Docs](https://github.com/ByteWelder/Tactility/wiki)

### LVGL
- [LVGL Docs](https://docs.lvgl.io/8/)
- [LVGL Examples](https://docs.lvgl.io/8/examples.html)

### MeshCore
- [MeshCore GitHub](https://github.com/meshcore-dev/MeshCore)

### ESP-IDF
- [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

---

*This guide is updated as development practices evolve.*
