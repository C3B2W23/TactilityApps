# Meshola Messenger - API Reference

**Version:** 0.1.0  
**Last Updated:** December 25, 2024

---

## Table of Contents

1. [Data Types](#data-types)
2. [IProtocol Interface](#iprotocol-interface)
3. [MesholaMsgService](#meshservice)
4. [ProfileManager](#profilemanager)
5. [MessageStore](#messagestore)
6. [ChatView](#chatview)
7. [Constants](#constants)

---

## Data Types

All types defined in `protocol/IProtocol.h` within `namespace meshola`.

### RadioConfig

Radio configuration parameters.

```cpp
struct RadioConfig {
    float frequency;          // MHz (e.g., 906.875)
    float bandwidth;          // kHz (e.g., 62.5, 125, 250, 500)
    uint8_t spreadingFactor;  // 7-12
    uint8_t codingRate;       // 5-8 (represents 4/5 to 4/8)
    int8_t txPower;           // dBm
};
```

### Contact

Information about a discovered peer.

```cpp
struct Contact {
    uint8_t publicKey[PUBLIC_KEY_SIZE];  // 32 bytes
    char name[MAX_NODE_NAME_LEN];        // 32 chars
    uint32_t lastSeen;                   // Unix timestamp
    int16_t lastRssi;                    // dBm
    int8_t lastSnr;                      // dB * 4
    uint8_t pathLength;                  // Hop count
    bool hasPath;                        // Route available?
    bool isOnline;                       // Recently seen?
    
    // Optional location
    bool hasLocation;
    double latitude;
    double longitude;
};
```

### Channel

Channel/group configuration.

```cpp
struct Channel {
    uint8_t id[CHANNEL_ID_SIZE];         // 16 bytes
    char name[MAX_CHANNEL_NAME_LEN];     // 32 chars
    bool isPublic;
    uint8_t index;                       // Slot index
};
```

### Message

Message data structure.

```cpp
struct Message {
    uint8_t senderKey[PUBLIC_KEY_SIZE];  // Sender's public key (or recipient for outgoing DM)
    uint8_t channelId[CHANNEL_ID_SIZE];  // Channel ID (if channel message)
    char senderName[MAX_NODE_NAME_LEN];  // Sender display name
    char text[MAX_MESSAGE_LEN];          // Message content (256 chars)
    uint32_t timestamp;                  // Unix timestamp
    uint32_t ackId;                      // For tracking delivery
    MessageStatus status;                // Delivery status
    bool isChannel;                      // true = channel, false = DM
    bool isOutgoing;                     // true = sent, false = received
    int16_t rssi;                        // Signal strength
    int8_t snr;                          // Signal-to-noise ratio
};
```

### MessageStatus

```cpp
enum class MessageStatus {
    Pending,        // Queued for sending
    Sent,           // Transmitted, awaiting ACK
    Delivered,      // ACK received
    Failed,         // Send failed or timed out
    Received        // Incoming message
};
```

### ProtocolFeature

Feature flags for protocol capability detection.

```cpp
enum class ProtocolFeature {
    DirectMessages,      // Point-to-point encrypted messages
    Channels,            // Group channels
    SignedMessages,      // Cryptographically signed messages
    LocationSharing,     // GPS/location in advertisements
    PathRouting,         // Multi-hop path discovery
    Encryption,          // End-to-end encryption
    FileTransfer,        // Binary data transfer
    Telemetry,           // Sensor data
    RemoteAdmin,         // Remote node administration
};
```

### NodeStatus

Node telemetry/status.

```cpp
struct NodeStatus {
    uint16_t batteryMillivolts;
    uint8_t batteryPercent;
    uint32_t uptime;
    uint32_t freeHeap;
    int16_t lastRssi;
    int8_t lastSnr;
    bool radioRunning;
};
```

### Profile

Complete profile configuration.

```cpp
struct Profile {
    char id[PROFILE_ID_LEN];              // 16 char UUID
    char name[PROFILE_NAME_LEN];          // 32 char display name
    uint32_t createdAt;                   // Unix timestamp
    uint32_t lastUsedAt;                  // Unix timestamp
    
    char protocolId[32];                  // "meshcore", etc.
    RadioConfig radio;
    
    char nodeName[MAX_NODE_NAME_LEN];
    uint8_t publicKey[PUBLIC_KEY_SIZE];
    uint8_t privateKey[PUBLIC_KEY_SIZE];
    bool hasKeys;
    
    ProtocolSettingValue protocolSettings[MAX_PROTOCOL_SETTINGS];
    int protocolSettingCount;
    
    // Methods
    void initDefaults();
    const char* getProtocolSetting(const char* key) const;
    bool setProtocolSetting(const char* key, const char* value);
};
```

---

## IProtocol Interface

Abstract interface for mesh protocols. Defined in `protocol/IProtocol.h`.

### Lifecycle Methods

```cpp
// Initialize with radio configuration (does not start radio)
virtual bool init(const RadioConfig& config) = 0;

// Start radio operations
virtual bool start() = 0;

// Stop radio operations
virtual void stop() = 0;

// Check if running
virtual bool isRunning() const = 0;

// Main loop - call regularly from service thread
virtual void loop() = 0;
```

### Protocol Information

```cpp
// Get protocol metadata
virtual ProtocolInfo getInfo() const = 0;

// Check if protocol supports a feature
virtual bool hasFeature(ProtocolFeature feature) const = 0;
```

### Identity

```cpp
// Get/set node display name
virtual const char* getNodeName() const = 0;
virtual bool setNodeName(const char* name) = 0;

// Get public key
virtual void getPublicKey(uint8_t out[PUBLIC_KEY_SIZE]) const = 0;

// Broadcast presence advertisement
virtual bool sendAdvertisement() = 0;
```

### Messaging

```cpp
// Send direct message, returns ack ID for tracking
virtual uint32_t sendMessage(const Contact& to, const char* text) = 0;

// Send channel message
virtual bool sendChannelMessage(const Channel& channel, const char* text) = 0;
```

### Contacts

```cpp
virtual int getContactCount() const = 0;
virtual bool getContact(int index, Contact& out) const = 0;
virtual bool findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const = 0;
virtual bool addContact(const Contact& contact) = 0;
virtual bool removeContact(const uint8_t publicKey[PUBLIC_KEY_SIZE]) = 0;
virtual void resetPath(const uint8_t publicKey[PUBLIC_KEY_SIZE]) = 0;
```

### Channels

```cpp
virtual int getChannelCount() const = 0;
virtual bool getChannel(int index, Channel& out) const = 0;
virtual bool setChannel(int index, const Channel& channel) = 0;
```

### Radio Configuration

```cpp
virtual RadioConfig getRadioConfig() const = 0;
virtual bool setRadioConfig(const RadioConfig& config) = 0;
virtual NodeStatus getStatus() const = 0;
```

### Callbacks

```cpp
virtual void setMessageCallback(MessageCallback callback) = 0;
virtual void setContactCallback(ContactCallback callback) = 0;
virtual void setStatusCallback(StatusCallback callback) = 0;
virtual void setAckCallback(AckCallback callback) = 0;
virtual void setErrorCallback(ErrorCallback callback) = 0;
```

**Callback Types:**

```cpp
using MessageCallback = std::function<void(const Message& msg)>;
using ContactCallback = std::function<void(const Contact& contact, bool isNew)>;
using StatusCallback = std::function<void(const NodeStatus& status)>;
using AckCallback = std::function<void(uint32_t ackId, bool success)>;
using ErrorCallback = std::function<void(int errorCode, const char* message)>;
```

### Persistence

```cpp
virtual bool saveState() = 0;
virtual bool loadState() = 0;
```

---

## MesholaMsgService

Singleton service managing protocol lifecycle. Defined in `mesh/MesholaMsgService.h`.

### Getting Instance

```cpp
MesholaMsgService& service = MesholaMsgService::getInstance();
```

### Initialization

```cpp
// Initialize with active profile from ProfileManager
bool init();

// Initialize with specific profile
bool initWithProfile(const Profile& profile);

// Reinitialize after profile switch
bool reinitWithProfile(const Profile& profile);
```

### Lifecycle

```cpp
bool start();
void stop();
bool isRunning() const;
```

### Protocol Access

```cpp
// Get underlying protocol (use with caution - prefer service methods)
IProtocol* getProtocol();

// Get current protocol ID
const char* getCurrentProtocolId() const;
```

### Messaging (Thread-Safe Wrappers)

```cpp
bool sendMessage(const Contact& to, const char* text, uint32_t& outAckId);
bool sendChannelMessage(const Channel& channel, const char* text);
bool sendAdvertisement();
```

### Data Access (Thread-Safe)

```cpp
int getContactCount();
bool getContact(int index, Contact& out);
bool findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out);

int getChannelCount();
bool getChannel(int index, Channel& out);

NodeStatus getStatus();
RadioConfig getRadioConfig();
bool setRadioConfig(const RadioConfig& config);

const char* getNodeName();
bool setNodeName(const char* name);
```

### Callbacks

```cpp
void setMessageCallback(MessageCallback callback);
void setContactCallback(ContactCallback callback);
void setStatusCallback(StatusCallback callback);
void setAckCallback(AckCallback callback);
```

---

## ProfileManager

Singleton managing profiles. Defined in `profile/Profile.h`.

### Getting Instance

```cpp
ProfileManager& mgr = ProfileManager::getInstance();
```

### Initialization

```cpp
// Load profiles from storage, create default if none exist
bool init();
```

### Active Profile

```cpp
// Get active profile (read-only)
const Profile* getActiveProfile() const;

// Get active profile for modification (call saveActiveProfile after)
Profile* getActiveProfileMutable();

// Save active profile to storage
bool saveActiveProfile();
```

### Profile Management

```cpp
// Get profile count
int getProfileCount() const;

// Get profile by index
const Profile* getProfile(int index) const;

// Find by ID or name
const Profile* findProfileById(const char* id) const;
const Profile* findProfileByName(const char* name) const;

// Create new profile with defaults
Profile* createProfile(const char* name);

// Delete profile (cannot delete if only one)
bool deleteProfile(const char* id);

// Switch active profile
bool switchToProfile(const char* id);

// Save profile list
bool saveProfileList();
```

### Utilities

```cpp
// Generate unique node name from hardware ID
void generateNodeName(char* dest, size_t maxLen);

// Generate keypair for profile
bool generateKeys(Profile& profile);

// Get storage path for profile data
void getProfileDataPath(const char* profileId, char* dest, size_t maxLen);
```

### Callback

```cpp
using ProfileSwitchCallback = std::function<void(const Profile& newProfile)>;
void setProfileSwitchCallback(ProfileSwitchCallback callback);
```

---

## MessageStore

Singleton for message persistence. Defined in `storage/MessageStore.h`.

### Getting Instance

```cpp
MessageStore& store = MessageStore::getInstance();
```

### Profile Selection

```cpp
// Set active profile (call when profile switches)
void setActiveProfile(const char* profileId);
```

### Writing Messages

```cpp
// Append message immediately (returns true on success)
bool appendMessage(const Message& msg);
```

### Reading Messages

```cpp
// Load DM history with contact
bool loadContactMessages(
    const uint8_t publicKey[PUBLIC_KEY_SIZE],
    int maxMessages,        // 0 = all
    std::vector<Message>& outMessages
);

// Load channel message history
bool loadChannelMessages(
    const uint8_t channelId[CHANNEL_ID_SIZE],
    int maxMessages,
    std::vector<Message>& outMessages
);
```

### Message Counts

```cpp
int getContactMessageCount(const uint8_t publicKey[PUBLIC_KEY_SIZE]);
int getChannelMessageCount(const uint8_t channelId[CHANNEL_ID_SIZE]);
```

### Deletion

```cpp
bool deleteContactMessages(const uint8_t publicKey[PUBLIC_KEY_SIZE]);
bool deleteChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE]);
bool deleteAllMessages();
```

---

## ChatView

Messaging UI component. Defined in `views/ChatView.h`.

### Lifecycle

```cpp
ChatView();
~ChatView();

void create(lv_obj_t* parent);
void destroy();
```

### Conversation Selection

```cpp
// Set active contact (loads message history)
void setActiveContact(const Contact* contact);

// Set active channel (loads message history)
void setActiveChannel(const Channel* channel);

// Clear conversation (show welcome screen)
void clearActiveConversation();

// Check if conversation is active
bool hasActiveConversation() const;

// Get current conversation target
const Contact* getActiveContact() const;
const Channel* getActiveChannel() const;
```

### Messages

```cpp
// Add message to display
void addMessage(const Message& msg);

// Update message status (for ack tracking)
void updateMessageStatus(uint32_t ackId, MessageStatus status);

// Refresh display from message list
void refresh();
```

### Callback

```cpp
using SendMessageCallback = void(*)(const char* text, void* userData);
void setSendCallback(SendMessageCallback callback, void* userData);
```

---

## Constants

Defined in `protocol/IProtocol.h`:

```cpp
constexpr size_t MAX_NODE_NAME_LEN = 32;
constexpr size_t MAX_MESSAGE_LEN = 256;
constexpr size_t MAX_CHANNEL_NAME_LEN = 32;
constexpr size_t PUBLIC_KEY_SIZE = 32;
constexpr size_t CHANNEL_ID_SIZE = 16;
```

Defined in `profile/Profile.h`:

```cpp
constexpr size_t PROFILE_ID_LEN = 16;
constexpr size_t PROFILE_NAME_LEN = 32;
constexpr size_t MAX_PROFILES = 16;
constexpr size_t MAX_PROTOCOL_SETTINGS = 32;
```

UI Colors (in `MesholaApp.cpp`):

```cpp
static const uint32_t COLOR_BG_DARK = 0x1a1a1a;
static const uint32_t COLOR_BG_CARD = 0x2d2d2d;
static const uint32_t COLOR_ACCENT = 0x0066cc;
static const uint32_t COLOR_ACCENT_DIM = 0x333333;
static const uint32_t COLOR_TEXT = 0xffffff;
static const uint32_t COLOR_TEXT_DIM = 0x888888;
static const uint32_t COLOR_SUCCESS = 0x00aa55;
static const uint32_t COLOR_WARNING = 0xffaa00;
static const uint32_t COLOR_ERROR = 0xcc3333;
static const uint32_t COLOR_MSG_OUTGOING = 0x0055aa;
static const uint32_t COLOR_MSG_INCOMING = 0x3d3d3d;
```

---

*This reference is automatically updated as the API evolves.*
