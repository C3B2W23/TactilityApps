#pragma once

/**
 * MesholaMsgService - Tactility Service for mesh networking
 * 
 * This is a TACTILITY SERVICE, not an app component. It:
 * - Runs continuously in the background
 * - Persists across app switches
 * - Provides radio RX/TX regardless of which app is in foreground
 * - Uses PubSub to notify apps of events
 * 
 * Apps (like Meshola Messenger) subscribe to this service's PubSub
 * to receive notifications about new messages, contacts, etc.
 * 
 * Reference: GpsService in Tactility
 */

#include "Tactility/Mutex.h"
#include "Tactility/PubSub.h"
#include "Tactility/Thread.h"
#include "Tactility/service/Service.h"
#include "Tactility/service/ServiceContext.h"
#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistration.h"

#include "../protocol/IProtocol.h"
#include "../profile/Profile.h"
#include "../storage/MessageStore.h"

#include <memory>
#include <vector>

namespace meshola::service {

// ============================================================================
// Event Types for PubSub
// ============================================================================

/**
 * Event published when a message is received or sent.
 */
struct MessageEvent {
    Message message;
    bool isIncoming;    // true = received, false = sent by us
    bool isNew;         // true = just happened, false = loaded from storage
};

/**
 * Event published when a contact is discovered or updated.
 */
struct ContactEvent {
    Contact contact;
    bool isNew;         // true = newly discovered, false = updated
};

/**
 * Event published when a channel is added or updated.
 */
struct ChannelEvent {
    Channel channel;
    bool isNew;
};

/**
 * Event published when service status changes.
 */
struct StatusEvent {
    bool radioRunning;
    int contactCount;
    int channelCount;
    NodeStatus nodeStatus;
};

/**
 * Event published when an ACK is received.
 */
struct AckEvent {
    uint32_t ackId;
    bool success;
};

// ============================================================================
// Service State
// ============================================================================

enum class ServiceState {
    Stopped,
    Starting,
    Running,
    Stopping
};

// ============================================================================
// MesholaMsgService Class
// ============================================================================

class MesholaMsgService final : public tt::service::Service {

public:
    // Tactility Service lifecycle
    bool onStart(tt::service::ServiceContext& serviceContext) override;
    void onStop(tt::service::ServiceContext& serviceContext) override;

    // ========================================================================
    // Radio Control
    // ========================================================================
    
    /**
     * Start the radio and begin RX/TX operations.
     */
    bool startRadio();
    
    /**
     * Stop the radio.
     */
    void stopRadio();
    
    /**
     * Check if radio is currently running.
     */
    bool isRadioRunning() const;
    
    /**
     * Get current service state.
     */
    ServiceState getState() const;

    // ========================================================================
    // Profile Management
    // ========================================================================
    
    /**
     * Get the ProfileManager (for profile CRUD operations).
     */
    ProfileManager* getProfileManager();
    
    /**
     * Get the currently active profile.
     */
    const Profile* getActiveProfile() const;
    
    /**
     * Switch to a different profile.
     * This will stop the radio, reinitialize with the new profile,
     * and optionally restart the radio.
     */
    bool switchProfile(const char* profileId, bool restartRadio = true);

    // ========================================================================
    // Messaging
    // ========================================================================
    
    /**
     * Send a direct message to a contact.
     * @param recipientKey The recipient's public key
     * @param text The message text
     * @param outAckId Receives the ACK ID for tracking delivery
     * @return true if message was queued for sending
     */
    bool sendMessage(const uint8_t recipientKey[PUBLIC_KEY_SIZE], 
                     const char* text, 
                     uint32_t& outAckId);
    
    /**
     * Send a message to a channel.
     */
    bool sendChannelMessage(const uint8_t channelId[CHANNEL_ID_SIZE], 
                            const char* text);
    
    /**
     * Broadcast an advertisement packet.
     */
    bool sendAdvertisement();

    // ========================================================================
    // Contacts
    // ========================================================================
    
    /**
     * Get number of known contacts.
     */
    int getContactCount() const;
    
    /**
     * Get contact by index.
     */
    bool getContact(int index, Contact& out) const;
    
    /**
     * Find contact by public key.
     */
    bool findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const;
    
    /**
     * Get all contacts.
     */
    std::vector<Contact> getContacts() const;

    // ========================================================================
    // Channels
    // ========================================================================
    
    /**
     * Get number of channels.
     */
    int getChannelCount() const;
    
    /**
     * Get channel by index.
     */
    bool getChannel(int index, Channel& out) const;
    
    /**
     * Find channel by ID.
     */
    bool findChannel(const uint8_t channelId[CHANNEL_ID_SIZE], Channel& out) const;
    
    /**
     * Get all channels.
     */
    std::vector<Channel> getChannels() const;

    // ========================================================================
    // Contact Management Helpers (favorites/promotion)
    // ========================================================================
    bool setContactFavorite(const uint8_t publicKey[PUBLIC_KEY_SIZE], bool favorite);
    bool promoteContact(const uint8_t publicKey[PUBLIC_KEY_SIZE]);

    // ========================================================================
    // Message History
    // ========================================================================
    
    /**
     * Get message history for a contact.
     * @param contactKey The contact's public key
     * @param maxCount Maximum number of messages to retrieve (0 = all)
     * @return Vector of messages, newest last
     */
    std::vector<Message> getContactMessages(const uint8_t contactKey[PUBLIC_KEY_SIZE], 
                                            int maxCount = 0) const;
    
    /**
     * Get message history for a channel.
     */
    std::vector<Message> getChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE], 
                                            int maxCount = 0) const;

    // ========================================================================
    // Node Information
    // ========================================================================
    
    /**
     * Get current node status.
     */
    NodeStatus getNodeStatus() const;
    
    /**
     * Get current radio configuration.
     */
    RadioConfig getRadioConfig() const;
    
    /**
     * Get this node's name.
     */
    const char* getNodeName() const;

    // ========================================================================
    // PubSub Accessors
    // Apps subscribe to these to receive real-time updates
    // ========================================================================
    
    std::shared_ptr<tt::PubSub<MessageEvent>> getMessagePubSub() const { 
        return _messagePubSub; 
    }
    
    std::shared_ptr<tt::PubSub<ContactEvent>> getContactPubSub() const { 
        return _contactPubSub; 
    }
    
    std::shared_ptr<tt::PubSub<ChannelEvent>> getChannelPubSub() const { 
        return _channelPubSub; 
    }
    
    std::shared_ptr<tt::PubSub<StatusEvent>> getStatusPubSub() const { 
        return _statusPubSub; 
    }
    
    std::shared_ptr<tt::PubSub<AckEvent>> getAckPubSub() const { 
        return _ackPubSub; 
    }

private:
    // Thread safety
    mutable tt::Mutex _mutex{tt::Mutex::Type::Recursive};
    mutable tt::Mutex _stateMutex;
    
    // Service state
    ServiceState _state = ServiceState::Stopped;
    std::unique_ptr<tt::service::ServicePaths> _paths;
    
    // Core components
    std::unique_ptr<ProfileManager> _profileManager;
    std::unique_ptr<MessageStore> _messageStore;
    std::unique_ptr<IProtocol> _protocol;
    const char* _currentProtocolId = nullptr;
    
    // Background thread for radio operations
    std::unique_ptr<tt::Thread> _meshThread;
    bool _threadRunning = false;
    
    // PubSub channels for event broadcasting
    std::shared_ptr<tt::PubSub<MessageEvent>> _messagePubSub;
    std::shared_ptr<tt::PubSub<ContactEvent>> _contactPubSub;
    std::shared_ptr<tt::PubSub<ChannelEvent>> _channelPubSub;
    std::shared_ptr<tt::PubSub<StatusEvent>> _statusPubSub;
    std::shared_ptr<tt::PubSub<AckEvent>> _ackPubSub;
    
    // Internal methods
    void setState(ServiceState newState);
    bool initializeProtocol(const Profile& profile);
    void meshThreadMain();
    static int32_t meshThreadEntry(void* context);
    
    // Protocol callbacks
    void onMessageReceived(const Message& msg);
    void onContactDiscovered(const Contact& contact, bool isNew);
    void onStatusChanged(const NodeStatus& status);
    void onAckReceived(uint32_t ackId, bool success);
    
    // Publish helpers
    void publishMessageEvent(const Message& msg, bool isIncoming, bool isNew);
    void publishContactEvent(const Contact& contact, bool isNew);
    void publishStatusEvent();
};

// ============================================================================
// Global Access
// ============================================================================

/**
 * Find the MesholaMsgService instance.
 * 
 * Usage from apps:
 *   auto mesholaMsgService = meshola::service::findMesholaMsgService();
 *   if (mesholaMsgService) {
 *       mesholaMsgService->getMessagePubSub()->subscribe(...);
 *   }
 */
std::shared_ptr<MesholaMsgService> findMesholaMsgService();

/**
 * Service manifest - used for registration.
 */
extern const tt::service::ServiceManifest manifest;

} // namespace meshola::service
