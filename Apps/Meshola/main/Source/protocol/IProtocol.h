#pragma once

/**
 * Meshola Messenger Protocol Abstraction Layer
 * 
 * This interface decouples the Meshola Messenger UI from specific mesh protocol
 * implementations, allowing support for MeshCore, Meshtastic, custom
 * forks, and future protocols.
 */

#include <cstdint>
#include <cstddef>
#include <functional>

namespace meshola {

// ============================================================================
// Common Data Structures (Protocol-Agnostic)
// ============================================================================

constexpr size_t MAX_NODE_NAME_LEN = 32;
constexpr size_t MAX_MESSAGE_LEN = 256;
constexpr size_t MAX_CHANNEL_NAME_LEN = 32;
constexpr size_t PUBLIC_KEY_SIZE = 32;
constexpr size_t CHANNEL_ID_SIZE = 16;

/**
 * Message delivery status
 */
enum class MessageStatus {
    Pending,        // Queued for sending
    Sent,           // Transmitted, awaiting ACK
    Delivered,      // ACK received
    Failed,         // Send failed or timed out
    Received        // Incoming message
};

/**
 * Protocol capabilities - UI can query these to show/hide features
 */
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

/**
 * Radio configuration
 */
struct RadioConfig {
    float frequency;          // MHz (e.g., 908.205)
    float bandwidth;          // kHz (e.g., 62.5, 125, 250, 500)
    uint8_t spreadingFactor;  // 7-12
    uint8_t codingRate;       // 5-8 (4/5 to 4/8)
    int8_t txPower;           // dBm
};

/**
 * Protocol information
 */
struct ProtocolInfo {
    const char* id;             // e.g., "meshcore", "customfork", "meshtastic"
    const char* name;           // e.g., "MeshCore", "CustomFork Mesh"
    const char* version;        // e.g., "1.0.0"
    const char* description;    // Human-readable description
    uint32_t capabilities;      // Bitmask of ProtocolFeature
};

/**
 * Contact/peer information
 */
struct Contact {
    uint8_t publicKey[PUBLIC_KEY_SIZE];
    char name[MAX_NODE_NAME_LEN];
    uint32_t lastSeen;          // Unix timestamp
    int16_t lastRssi;           // dBm
    int8_t lastSnr;             // dB * 4
    uint8_t pathLength;         // Hops to reach
    bool hasPath;               // Do we have a route?
    bool isOnline;              // Recently seen?
    
    // Optional location (if protocol supports it)
    bool hasLocation;
    double latitude;
    double longitude;
};

/**
 * Channel/group information
 */
struct Channel {
    uint8_t id[CHANNEL_ID_SIZE];
    char name[MAX_CHANNEL_NAME_LEN];
    bool isPublic;
    uint8_t index;              // Channel slot index
};

/**
 * Message structure
 */
struct Message {
    uint8_t senderKey[PUBLIC_KEY_SIZE];  // For DM
    uint8_t channelId[CHANNEL_ID_SIZE];  // For channel msg (if applicable)
    char senderName[MAX_NODE_NAME_LEN];
    char text[MAX_MESSAGE_LEN];
    uint32_t timestamp;
    uint32_t ackId;             // For tracking delivery
    MessageStatus status;
    bool isChannel;             // true = channel msg, false = DM
    bool isOutgoing;            // true = we sent it
    int16_t rssi;
    int8_t snr;
};

/**
 * Node status/telemetry
 */
struct NodeStatus {
    uint16_t batteryMillivolts;
    uint8_t batteryPercent;
    uint32_t uptime;
    uint32_t freeHeap;
    int16_t lastRssi;
    int8_t lastSnr;
    bool radioRunning;
};

// ============================================================================
// Callback Types
// ============================================================================

using MessageCallback = std::function<void(const Message& msg)>;
using ContactCallback = std::function<void(const Contact& contact, bool isNew)>;
using StatusCallback = std::function<void(const NodeStatus& status)>;
using AckCallback = std::function<void(uint32_t ackId, bool success)>;
using ErrorCallback = std::function<void(int errorCode, const char* message)>;

// ============================================================================
// Protocol Interface
// ============================================================================

/**
 * Abstract interface for mesh protocols.
 * 
 * All protocol implementations (MeshCore, CustomFork fork, Meshtastic, etc.)
 * must implement this interface. The Meshola Messenger UI only interacts through this
 * abstraction, making it easy to swap protocols at runtime.
 */
class IProtocol {
public:
    virtual ~IProtocol() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    /**
     * Initialize the protocol with radio configuration.
     * Does not start the radio - call start() for that.
     */
    virtual bool init(const RadioConfig& config) = 0;
    
    /**
     * Start the protocol (begin radio operations).
     */
    virtual bool start() = 0;
    
    /**
     * Stop the protocol (halt radio operations).
     */
    virtual void stop() = 0;
    
    /**
     * Check if protocol is currently running.
     */
    virtual bool isRunning() const = 0;
    
    /**
     * Main loop - call this regularly from the mesh service thread.
     */
    virtual void loop() = 0;

    // ========================================================================
    // Protocol Information
    // ========================================================================
    
    /**
     * Get protocol information (name, version, capabilities).
     */
    virtual ProtocolInfo getInfo() const = 0;
    
    /**
     * Check if protocol supports a specific feature.
     */
    virtual bool hasFeature(ProtocolFeature feature) const = 0;

    // ========================================================================
    // Identity
    // ========================================================================
    
    /**
     * Get this node's display name.
     */
    virtual const char* getNodeName() const = 0;
    
    /**
     * Set this node's display name.
     */
    virtual bool setNodeName(const char* name) = 0;
    
    /**
     * Get this node's public key.
     */
    virtual void getPublicKey(uint8_t out[PUBLIC_KEY_SIZE]) const = 0;
    
    /**
     * Send an advertisement (beacon) to announce presence.
     */
    virtual bool sendAdvertisement() = 0;

    // ========================================================================
    // Messaging
    // ========================================================================
    
    /**
     * Send a direct message to a contact.
     * Returns an ack ID that can be tracked via AckCallback.
     */
    virtual uint32_t sendMessage(const Contact& to, const char* text) = 0;
    
    /**
     * Send a message to a channel.
     */
    virtual bool sendChannelMessage(const Channel& channel, const char* text) = 0;

    // ========================================================================
    // Contacts
    // ========================================================================
    
    /**
     * Get the number of known contacts.
     */
    virtual int getContactCount() const = 0;
    
    /**
     * Get contact by index.
     */
    virtual bool getContact(int index, Contact& out) const = 0;
    
    /**
     * Find contact by public key.
     */
    virtual bool findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const = 0;
    
    /**
     * Add or update a contact.
     */
    virtual bool addContact(const Contact& contact) = 0;
    
    /**
     * Remove a contact.
     */
    virtual bool removeContact(const uint8_t publicKey[PUBLIC_KEY_SIZE]) = 0;
    
    /**
     * Reset path to a contact (force re-discovery).
     */
    virtual void resetPath(const uint8_t publicKey[PUBLIC_KEY_SIZE]) = 0;

    // ========================================================================
    // Channels
    // ========================================================================
    
    /**
     * Get the number of configured channels.
     */
    virtual int getChannelCount() const = 0;
    
    /**
     * Get channel by index.
     */
    virtual bool getChannel(int index, Channel& out) const = 0;
    
    /**
     * Add or update a channel.
     */
    virtual bool setChannel(int index, const Channel& channel) = 0;

    // ========================================================================
    // Radio Configuration
    // ========================================================================
    
    /**
     * Get current radio configuration.
     */
    virtual RadioConfig getRadioConfig() const = 0;
    
    /**
     * Update radio configuration (may require restart).
     */
    virtual bool setRadioConfig(const RadioConfig& config) = 0;
    
    /**
     * Get current node status/telemetry.
     */
    virtual NodeStatus getStatus() const = 0;

    // ========================================================================
    // Event Callbacks
    // ========================================================================
    
    /**
     * Set callback for incoming messages.
     */
    virtual void setMessageCallback(MessageCallback callback) = 0;
    
    /**
     * Set callback for contact discovery/updates.
     */
    virtual void setContactCallback(ContactCallback callback) = 0;
    
    /**
     * Set callback for status updates.
     */
    virtual void setStatusCallback(StatusCallback callback) = 0;
    
    /**
     * Set callback for message acknowledgments.
     */
    virtual void setAckCallback(AckCallback callback) = 0;
    
    /**
     * Set callback for errors.
     */
    virtual void setErrorCallback(ErrorCallback callback) = 0;

    // ========================================================================
    // Persistence
    // ========================================================================
    
    /**
     * Save current state (contacts, channels, settings) to storage.
     */
    virtual bool saveState() = 0;
    
    /**
     * Load state from storage.
     */
    virtual bool loadState() = 0;
};

// ============================================================================
// Protocol Factory
// ============================================================================

/**
 * Protocol registration entry
 */
struct ProtocolEntry {
    const char* id;             // Unique identifier (e.g., "meshcore", "customfork")
    const char* name;           // Display name
    IProtocol* (*create)();     // Factory function
};

/**
 * Protocol registry - manages available protocol implementations
 */
class ProtocolRegistry {
public:
    static constexpr int MAX_PROTOCOLS = 8;
    
    /**
     * Register a protocol implementation.
     */
    static bool registerProtocol(const ProtocolEntry& entry);
    
    /**
     * Get number of registered protocols.
     */
    static int getProtocolCount();
    
    /**
     * Get protocol entry by index.
     */
    static const ProtocolEntry* getProtocol(int index);
    
    /**
     * Find protocol by ID.
     */
    static const ProtocolEntry* findProtocol(const char* id);
    
    /**
     * Create protocol instance by ID.
     */
    static IProtocol* createProtocol(const char* id);

private:
    static ProtocolEntry protocols[MAX_PROTOCOLS];
    static int protocolCount;
};

} // namespace meshola
