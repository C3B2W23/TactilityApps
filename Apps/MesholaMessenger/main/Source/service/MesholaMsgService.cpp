#include "MesholaMsgService.h"
#include "../protocol/MeshCoreProtocol.h"
#include "Tactility/Log.h"

namespace meshola::service {

#define TAG "MesholaMsgService"

// ============================================================================
// Service Manifest
// ============================================================================

extern const tt::service::ServiceManifest manifest = {
    .id = "MesholaMsg",
    .createService = tt::service::create<MesholaMsgService>
};

// ============================================================================
// Global Accessor
// ============================================================================

std::shared_ptr<MesholaMsgService> findMesholaMsgService() {
    auto service = tt::service::findServiceById(manifest.id);
    if (!service) {
        TT_LOG_E(TAG, "MesholaMsgService not found!");
        return nullptr;
    }
    return std::static_pointer_cast<MesholaMsgService>(service);
}

// ============================================================================
// Service Lifecycle
// ============================================================================

bool MesholaMsgService::onStart(tt::service::ServiceContext& serviceContext) {
    TT_LOG_I(TAG, "Starting MesholaMsgService...");
    
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    setState(ServiceState::Starting);
    
    // Get service paths for data storage
    _paths = serviceContext.getPaths();
    
    // Initialize PubSub channels
    _messagePubSub = std::make_shared<tt::PubSub<MessageEvent>>();
    _contactPubSub = std::make_shared<tt::PubSub<ContactEvent>>();
    _channelPubSub = std::make_shared<tt::PubSub<ChannelEvent>>();
    _statusPubSub = std::make_shared<tt::PubSub<StatusEvent>>();
    _ackPubSub = std::make_shared<tt::PubSub<AckEvent>>();
    
    // Register built-in protocols
    MeshCoreProtocol::registerSelf();
    
    // Initialize profile manager
    _profileManager = std::make_unique<ProfileManager>();
    if (!_profileManager->init()) {
        TT_LOG_E(TAG, "Failed to initialize ProfileManager");
        setState(ServiceState::Stopped);
        return false;
    }
    
    // Initialize message store
    _messageStore = std::make_unique<MessageStore>();
    
    // Get active profile
    const Profile* profile = _profileManager->getActiveProfile();
    if (!profile) {
        TT_LOG_W(TAG, "No active profile, service started but radio not initialized");
        setState(ServiceState::Running);
        return true;  // Service OK, just no profile yet
    }
    
    // Configure message store for this profile
    _messageStore->setActiveProfile(profile->id);
    
    // Initialize protocol
    if (!initializeProtocol(*profile)) {
        TT_LOG_W(TAG, "Failed to initialize protocol, service started but radio not ready");
        setState(ServiceState::Running);
        return true;  // Service OK, protocol failed
    }
    
    setState(ServiceState::Running);
    TT_LOG_I(TAG, "MesholaMsgService started successfully");
    
    return true;
}

void MesholaMsgService::onStop(tt::service::ServiceContext& serviceContext) {
    TT_LOG_I(TAG, "Stopping MesholaMsgService...");
    
    setState(ServiceState::Stopping);
    
    // Stop radio if running
    stopRadio();
    
    // Clean up
    {
        auto lock = _mutex.asScopedLock();
        lock.lock();
        
        _protocol.reset();
        _messageStore.reset();
        _profileManager.reset();
    }
    
    setState(ServiceState::Stopped);
    TT_LOG_I(TAG, "MesholaMsgService stopped");
}

// ============================================================================
// State Management
// ============================================================================

void MesholaMsgService::setState(ServiceState newState) {
    auto lock = _stateMutex.asScopedLock();
    lock.lock();
    _state = newState;
    lock.unlock();
    
    // Publish status change
    publishStatusEvent();
}

ServiceState MesholaMsgService::getState() const {
    auto lock = _stateMutex.asScopedLock();
    lock.lock();
    return _state;
}

// ============================================================================
// Protocol Initialization
// ============================================================================

bool MesholaMsgService::initializeProtocol(const Profile& profile) {
    TT_LOG_I(TAG, "Initializing protocol: %s", profile.protocolId);
    
    // Create protocol instance
    _protocol.reset(ProtocolRegistry::createProtocol(profile.protocolId));
    if (!_protocol) {
        TT_LOG_E(TAG, "Failed to create protocol: %s", profile.protocolId);
        return false;
    }
    
    _currentProtocolId = profile.protocolId;
    
    // Set node name
    _protocol->setNodeName(profile.nodeName);
    _protocol->setLocalIdentity(profile.publicKey, profile.nodeName);
    
    // Wire up callbacks
    _protocol->setMessageCallback([this](const Message& msg) {
        onMessageReceived(msg);
    });
    
    _protocol->setContactCallback([this](const Contact& contact, bool isNew) {
        onContactDiscovered(contact, isNew);
    });
    
    _protocol->setStatusCallback([this](const NodeStatus& status) {
        onStatusChanged(status);
    });
    
    _protocol->setAckCallback([this](uint32_t ackId, bool success) {
        onAckReceived(ackId, success);
    });
    
    // Initialize with radio config
    if (!_protocol->init(profile.radio)) {
        TT_LOG_E(TAG, "Failed to initialize protocol with radio config");
        _protocol.reset();
        return false;
    }
    
    TT_LOG_I(TAG, "Protocol initialized: %s", profile.protocolId);
    return true;
}

// ============================================================================
// Radio Control
// ============================================================================

bool MesholaMsgService::startRadio() {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (_threadRunning) {
        TT_LOG_W(TAG, "Radio already running");
        return true;
    }
    
    if (!_protocol) {
        TT_LOG_E(TAG, "Cannot start radio: no protocol initialized");
        return false;
    }
    
    TT_LOG_I(TAG, "Starting radio...");
    
    // Start protocol
    if (!_protocol->start()) {
        TT_LOG_E(TAG, "Protocol start failed");
        return false;
    }
    
    // Start background thread
    _threadRunning = true;
    _meshThread = std::make_unique<tt::Thread>(
        "MesholaMsgService",
        8192,  // Stack size
        meshThreadEntry,
        this
    );
    _meshThread->start();
    
    TT_LOG_I(TAG, "Radio started");
    publishStatusEvent();
    
    return true;
}

void MesholaMsgService::stopRadio() {
    TT_LOG_I(TAG, "Stopping radio...");
    
    // Signal thread to stop
    _threadRunning = false;
    
    // Wait for thread to finish
    if (_meshThread) {
        _meshThread->join();
        _meshThread.reset();
    }
    
    // Stop protocol
    {
        auto lock = _mutex.asScopedLock();
        lock.lock();
        
        if (_protocol) {
            _protocol->stop();
        }
    }
    
    TT_LOG_I(TAG, "Radio stopped");
    publishStatusEvent();
}

bool MesholaMsgService::isRadioRunning() const {
    return _threadRunning;
}

// ============================================================================
// Background Thread
// ============================================================================

int32_t MesholaMsgService::meshThreadEntry(void* context) {
    auto* self = static_cast<MesholaMsgService*>(context);
    self->meshThreadMain();
    return 0;
}

void MesholaMsgService::meshThreadMain() {
    TT_LOG_I(TAG, "Mesh thread started");
    
    while (_threadRunning) {
        {
            auto lock = _mutex.asScopedLock();
            lock.lock();
            
            if (_protocol) {
                _protocol->loop();
            }
        }
        
        // Small delay to prevent tight loop
        tt::kernel::delayMillis(10);
    }
    
    TT_LOG_I(TAG, "Mesh thread exiting");
}

// ============================================================================
// Protocol Callbacks
// ============================================================================

void MesholaMsgService::onMessageReceived(const Message& msg) {
    TT_LOG_D(TAG, "Message received from %s", msg.senderName);
    
    // Persist to storage
    if (_messageStore) {
        _messageStore->appendMessage(msg);
    }
    
    // Publish event to subscribers
    publishMessageEvent(msg, true, true);
}

void MesholaMsgService::onContactDiscovered(const Contact& contact, bool isNew) {
    TT_LOG_D(TAG, "Contact %s: %s", isNew ? "discovered" : "updated", contact.name);
    
    // Publish event to subscribers
    publishContactEvent(contact, isNew);
}

void MesholaMsgService::onStatusChanged(const NodeStatus& status) {
    TT_LOG_D(TAG, "Status changed: %s", status.isOnline ? "online" : "offline");
    publishStatusEvent();
}

void MesholaMsgService::onAckReceived(uint32_t ackId, bool success) {
    TT_LOG_D(TAG, "ACK %u: %s", ackId, success ? "success" : "failed");
    
    AckEvent event = {
        .ackId = ackId,
        .success = success
    };
    _ackPubSub->publish(event);
}

// ============================================================================
// Publish Helpers
// ============================================================================

void MesholaMsgService::publishMessageEvent(const Message& msg, bool isIncoming, bool isNew) {
    MessageEvent event = {
        .message = msg,
        .isIncoming = isIncoming,
        .isNew = isNew
    };
    _messagePubSub->publish(event);
}

void MesholaMsgService::publishContactEvent(const Contact& contact, bool isNew) {
    ContactEvent event = {
        .contact = contact,
        .isNew = isNew
    };
    _contactPubSub->publish(event);
}

void MesholaMsgService::publishStatusEvent() {
    StatusEvent event = {
        .radioRunning = _threadRunning,
        .contactCount = getContactCount(),
        .channelCount = getChannelCount(),
        .nodeStatus = getNodeStatus()
    };
    _statusPubSub->publish(event);
}

// ============================================================================
// Profile Management
// ============================================================================

ProfileManager* MesholaMsgService::getProfileManager() {
    return _profileManager.get();
}

const Profile* MesholaMsgService::getActiveProfile() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_profileManager) {
        return nullptr;
    }
    return _profileManager->getActiveProfile();
}

bool MesholaMsgService::switchProfile(const char* profileId, bool restartRadio) {
    TT_LOG_I(TAG, "Switching to profile: %s", profileId);
    
    bool wasRunning = _threadRunning;
    
    // Stop radio if running
    if (wasRunning) {
        stopRadio();
    }
    
    {
        auto lock = _mutex.asScopedLock();
        lock.lock();
        
        // Switch profile
        if (!_profileManager->setActiveProfile(profileId)) {
            TT_LOG_E(TAG, "Failed to switch profile");
            return false;
        }
        
        // Update message store
        _messageStore->setActiveProfile(profileId);
        
        // Get new profile
        const Profile* profile = _profileManager->getActiveProfile();
        if (!profile) {
            TT_LOG_E(TAG, "Active profile is null after switch");
            return false;
        }
        
        // Reinitialize protocol
        if (!initializeProtocol(*profile)) {
            TT_LOG_E(TAG, "Failed to initialize protocol for new profile");
            return false;
        }
    }
    
    // Restart radio if it was running
    if (restartRadio && wasRunning) {
        return startRadio();
    }
    
    return true;
}

// ============================================================================
// Messaging
// ============================================================================

bool MesholaMsgService::sendMessage(const uint8_t recipientKey[PUBLIC_KEY_SIZE], 
                              const char* text, 
                              uint32_t& outAckId) {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol || !_threadRunning) {
        TT_LOG_E(TAG, "Cannot send: radio not running");
        return false;
    }
    
    // Find the contact
    Contact recipient;
    if (!_protocol->findContact(recipientKey, recipient)) {
        TT_LOG_E(TAG, "Cannot send: recipient not found");
        return false;
    }
    
    // Send via protocol
    outAckId = _protocol->sendMessage(recipient, text);
    if (outAckId == 0) {
        return false;
    }
    
    // Create message record for our sent message
    Message sentMsg = {};
    sentMsg.type = MessageType::Direct;
    memcpy(sentMsg.senderKey, _profileManager->getActiveProfile()->publicKey, PUBLIC_KEY_SIZE);
    strncpy(sentMsg.senderName, _protocol->getNodeName(), sizeof(sentMsg.senderName) - 1);
    memcpy(sentMsg.recipientKey, recipientKey, PUBLIC_KEY_SIZE);
    strncpy(sentMsg.text, text, sizeof(sentMsg.text) - 1);
    sentMsg.timestamp = time(nullptr);
    sentMsg.status = MessageStatus::Sent;
    sentMsg.ackId = outAckId;
    
    // Persist our sent message
    if (_messageStore) {
        _messageStore->appendMessage(sentMsg);
    }
    
    // Publish event
    publishMessageEvent(sentMsg, false, true);
    
    return true;
}

bool MesholaMsgService::sendChannelMessage(const uint8_t channelId[CHANNEL_ID_SIZE], 
                                     const char* text) {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol || !_threadRunning) {
        TT_LOG_E(TAG, "Cannot send: radio not running");
        return false;
    }
    
    // Find the channel
    Channel channel;
    if (!findChannel(channelId, channel)) {
        TT_LOG_E(TAG, "Cannot send: channel not found");
        return false;
    }
    
    // Send via protocol
    if (!_protocol->sendChannelMessage(channel, text)) {
        return false;
    }
    
    // Create message record
    Message sentMsg = {};
    sentMsg.type = MessageType::Channel;
    memcpy(sentMsg.senderKey, _profileManager->getActiveProfile()->publicKey, PUBLIC_KEY_SIZE);
    strncpy(sentMsg.senderName, _protocol->getNodeName(), sizeof(sentMsg.senderName) - 1);
    memcpy(sentMsg.channelId, channelId, CHANNEL_ID_SIZE);
    strncpy(sentMsg.text, text, sizeof(sentMsg.text) - 1);
    sentMsg.timestamp = time(nullptr);
    sentMsg.status = MessageStatus::Sent;
    
    // Persist
    if (_messageStore) {
        _messageStore->appendMessage(sentMsg);
    }
    
    // Publish
    publishMessageEvent(sentMsg, false, true);
    
    return true;
}

bool MesholaMsgService::sendAdvertisement() {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol || !_threadRunning) {
        return false;
    }
    
    return _protocol->sendAdvertisement();
}

// ============================================================================
// Contacts
// ============================================================================

int MesholaMsgService::getContactCount() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return 0;
    }
    return _protocol->getContactCount();
}

bool MesholaMsgService::getContact(int index, Contact& out) const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return false;
    }
    return _protocol->getContact(index, out);
}

bool MesholaMsgService::findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return false;
    }
    return _protocol->findContact(publicKey, out);
}

std::vector<Contact> MesholaMsgService::getContacts() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    std::vector<Contact> contacts;
    if (!_protocol) {
        return contacts;
    }
    
    int count = _protocol->getContactCount();
    contacts.reserve(count);
    
    for (int i = 0; i < count; i++) {
        Contact c;
        if (_protocol->getContact(i, c)) {
            contacts.push_back(c);
        }
    }
    
    return contacts;
}

// ============================================================================
// Channels
// ============================================================================

int MesholaMsgService::getChannelCount() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return 0;
    }
    return _protocol->getChannelCount();
}

bool MesholaMsgService::getChannel(int index, Channel& out) const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return false;
    }
    return _protocol->getChannel(index, out);
}

bool MesholaMsgService::findChannel(const uint8_t channelId[CHANNEL_ID_SIZE], Channel& out) const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return false;
    }
    
    int count = _protocol->getChannelCount();
    for (int i = 0; i < count; i++) {
        Channel ch;
        if (_protocol->getChannel(i, ch)) {
            if (memcmp(ch.id, channelId, CHANNEL_ID_SIZE) == 0) {
                out = ch;
                return true;
            }
        }
    }
    return false;
}

std::vector<Channel> MesholaMsgService::getChannels() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    std::vector<Channel> channels;
    if (!_protocol) {
        return channels;
    }
    
    int count = _protocol->getChannelCount();
    channels.reserve(count);
    
    for (int i = 0; i < count; i++) {
        Channel ch;
        if (_protocol->getChannel(i, ch)) {
            channels.push_back(ch);
        }
    }
    
    return channels;
}

// ============================================================================
// Message History
// ============================================================================

std::vector<Message> MesholaMsgService::getContactMessages(const uint8_t contactKey[PUBLIC_KEY_SIZE], 
                                                      int maxCount) const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_messageStore) {
        return {};
    }
    
    return _messageStore->getContactMessages(contactKey, maxCount);
}

std::vector<Message> MesholaMsgService::getChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE], 
                                                      int maxCount) const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_messageStore) {
        return {};
    }
    
    return _messageStore->getChannelMessages(channelId, maxCount);
}

// ============================================================================
// Node Information
// ============================================================================

NodeStatus MesholaMsgService::getNodeStatus() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return NodeStatus{};
    }
    return _protocol->getStatus();
}

RadioConfig MesholaMsgService::getRadioConfig() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return RadioConfig{};
    }
    return _protocol->getRadioConfig();
}

const char* MesholaMsgService::getNodeName() const {
    auto lock = _mutex.asScopedLock();
    lock.lock();
    
    if (!_protocol) {
        return "Unknown";
    }
    return _protocol->getNodeName();
}

} // namespace meshola::service
