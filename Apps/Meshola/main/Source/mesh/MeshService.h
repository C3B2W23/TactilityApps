#pragma once

#include "protocol/IProtocol.h"
#include "profile/Profile.h"
#include "storage/MessageStore.h"
#include <memory>

namespace meshola {

/**
 * MeshService manages the protocol in a background thread.
 * 
 * This service:
 * - Runs the protocol loop in a dedicated thread
 * - Provides thread-safe access to protocol methods
 * - Manages protocol lifecycle
 * - Forwards events to registered callbacks
 */
class MeshService {
public:
    /**
     * Get the singleton instance.
     */
    static MeshService& getInstance();
    
    /**
     * Initialize the service with the active profile.
     * Uses ProfileManager to get current profile settings.
     */
    bool init();
    
    /**
     * Initialize the service with a specific profile.
     */
    bool initWithProfile(const Profile& profile);
    
    /**
     * Reinitialize with new profile (called on profile switch).
     */
    bool reinitWithProfile(const Profile& profile);
    
    /**
     * Start the mesh service (begins background thread).
     */
    bool start();
    
    /**
     * Stop the mesh service.
     */
    void stop();
    
    /**
     * Check if service is running.
     */
    bool isRunning() const;
    
    /**
     * Get the current protocol (thread-safe access).
     * Note: Lock before extended use if accessing from UI thread.
     */
    IProtocol* getProtocol();
    
    /**
     * Switch to a different protocol.
     * Service must be stopped first.
     */
    bool switchProtocol(const char* protocolId);
    
    /**
     * Get current protocol ID.
     */
    const char* getCurrentProtocolId() const;

    // ========================================================================
    // Convenience methods - delegate to protocol with proper locking
    // ========================================================================
    
    // Messaging
    bool sendMessage(const Contact& to, const char* text, uint32_t& outAckId);
    bool sendChannelMessage(const Channel& channel, const char* text);
    bool sendAdvertisement();
    
    // Contacts
    int getContactCount();
    bool getContact(int index, Contact& out);
    bool findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out);
    
    // Channels
    int getChannelCount();
    bool getChannel(int index, Channel& out);
    
    // Status
    NodeStatus getStatus();
    RadioConfig getRadioConfig();
    bool setRadioConfig(const RadioConfig& config);
    const char* getNodeName();
    bool setNodeName(const char* name);
    
    // ========================================================================
    // Event registration
    // Note: Callbacks may fire on mesh thread - use LVGL locking if updating UI
    // ========================================================================
    
    void setMessageCallback(MessageCallback callback);
    void setContactCallback(ContactCallback callback);
    void setStatusCallback(StatusCallback callback);
    void setAckCallback(AckCallback callback);

private:
    MeshService();
    ~MeshService();
    
    // Non-copyable
    MeshService(const MeshService&) = delete;
    MeshService& operator=(const MeshService&) = delete;
    
    // Thread entry point
    static int32_t meshThreadEntry(void* context);
    void meshThreadMain();
    
    // State
    IProtocol* _protocol;
    const char* _protocolId;
    bool _running;
    bool _threadInterrupted;
    
    // TODO: Replace with actual Tactility types when integrated
    // std::unique_ptr<Thread> _thread;
    // Mutex _mutex;
    void* _thread;  // Placeholder
    void* _mutex;   // Placeholder
    
    // Callbacks (forwarded from protocol)
    MessageCallback _messageCallback;
    ContactCallback _contactCallback;
    StatusCallback _statusCallback;
    AckCallback _ackCallback;
};

} // namespace meshola
