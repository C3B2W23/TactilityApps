#include "MeshService.h"
#include "protocol/MeshCoreProtocol.h"
#include "profile/Profile.h"
#include "storage/MessageStore.h"
#include <cstring>

// TODO: Include actual Tactility headers when integrated
// #include <TactilityCpp/Thread.h>
// #include <TactilityCpp/Mutex.h>
// #include <tt_kernel.h>

namespace meshola {

// Singleton instance
static MeshService* s_instance = nullptr;

MeshService& MeshService::getInstance() {
    if (!s_instance) {
        s_instance = new MeshService();
    }
    return *s_instance;
}

MeshService::MeshService()
    : _protocol(nullptr)
    , _protocolId(nullptr)
    , _running(false)
    , _threadInterrupted(false)
    , _thread(nullptr)
    , _mutex(nullptr)
    , _messageCallback(nullptr)
    , _contactCallback(nullptr)
    , _statusCallback(nullptr)
    , _ackCallback(nullptr)
{
    // Register built-in protocols
    MeshCoreProtocol::registerSelf();
    
    // TODO: Initialize mutex
    // _mutex = Mutex(MutexTypeRecursive);
}

MeshService::~MeshService() {
    stop();
    if (_protocol) {
        delete _protocol;
        _protocol = nullptr;
    }
}

bool MeshService::init() {
    // Use ProfileManager to get active profile
    auto& profileMgr = ProfileManager::getInstance();
    profileMgr.init();
    
    const Profile* profile = profileMgr.getActiveProfile();
    if (!profile) {
        return false;
    }
    
    return initWithProfile(*profile);
}

bool MeshService::initWithProfile(const Profile& profile) {
    if (_running) {
        return false;  // Must stop first
    }
    
    // Clean up existing protocol
    if (_protocol) {
        delete _protocol;
        _protocol = nullptr;
    }
    
    // Create new protocol instance
    _protocol = ProtocolRegistry::createProtocol(profile.protocolId);
    if (!_protocol) {
        return false;
    }
    
    _protocolId = profile.protocolId;
    
    // Set node name from profile
    _protocol->setNodeName(profile.nodeName);
    
    // Configure MessageStore for this profile
    MessageStore::getInstance().setActiveProfile(profile.id);
    
    // Apply protocol-specific settings from profile
    for (int i = 0; i < profile.protocolSettingCount; i++) {
        // TODO: Protocol needs API to accept these
        // _protocol->setSetting(profile.protocolSettings[i].key, 
        //                       profile.protocolSettings[i].value);
    }
    
    // Wire up callbacks to forward to our registered callbacks
    // AND persist messages
    _protocol->setMessageCallback([this](const Message& msg) {
        // Persist incoming message immediately
        MessageStore::getInstance().appendMessage(msg);
        
        // Forward to UI callback
        if (_messageCallback) {
            _messageCallback(msg);
        }
    });
    
    _protocol->setContactCallback([this](const Contact& contact, bool isNew) {
        if (_contactCallback) {
            _contactCallback(contact, isNew);
        }
    });
    
    _protocol->setStatusCallback([this](const NodeStatus& status) {
        if (_statusCallback) {
            _statusCallback(status);
        }
    });
    
    _protocol->setAckCallback([this](uint32_t ackId, bool success) {
        if (_ackCallback) {
            _ackCallback(ackId, success);
        }
    });
    
    // Initialize with profile's radio config
    return _protocol->init(profile.radio);
}

bool MeshService::reinitWithProfile(const Profile& profile) {
    bool wasRunning = _running;
    
    if (wasRunning) {
        stop();
    }
    
    if (!initWithProfile(profile)) {
        return false;
    }
    
    if (wasRunning) {
        return start();
    }
    
    return true;
}

bool MeshService::start() {
    if (_running || !_protocol) {
        return false;
    }
    
    if (!_protocol->start()) {
        return false;
    }
    
    _threadInterrupted = false;
    _running = true;
    
    // TODO: Start background thread
    // _thread = std::make_unique<Thread>("MeshService", 8192, meshThreadEntry, this);
    // _thread->start();
    
    return true;
}

void MeshService::stop() {
    if (!_running) {
        return;
    }
    
    _threadInterrupted = true;
    
    // TODO: Wait for thread to finish
    // if (_thread) {
    //     _thread->join();
    //     _thread.reset();
    // }
    
    if (_protocol) {
        _protocol->stop();
    }
    
    _running = false;
}

bool MeshService::isRunning() const {
    return _running;
}

IProtocol* MeshService::getProtocol() {
    // TODO: Lock mutex for thread safety
    return _protocol;
}

bool MeshService::switchProtocol(const char* protocolId) {
    if (_running) {
        return false;  // Must stop first
    }
    return init(protocolId);
}

const char* MeshService::getCurrentProtocolId() const {
    return _protocolId;
}

int32_t MeshService::meshThreadEntry(void* context) {
    auto* self = static_cast<MeshService*>(context);
    self->meshThreadMain();
    return 0;
}

void MeshService::meshThreadMain() {
    while (!_threadInterrupted) {
        // TODO: Lock mutex
        if (_protocol && _running) {
            _protocol->loop();
        }
        // TODO: Unlock mutex
        
        // Small delay to prevent tight loop
        // tt_kernel_delay_ticks(10);
    }
}

// ============================================================================
// Convenience methods
// ============================================================================

bool MeshService::sendMessage(const Contact& to, const char* text, uint32_t& outAckId) {
    // TODO: Lock mutex
    if (!_protocol || !_running) {
        return false;
    }
    outAckId = _protocol->sendMessage(to, text);
    return outAckId != 0;
}

bool MeshService::sendChannelMessage(const Channel& channel, const char* text) {
    // TODO: Lock mutex
    if (!_protocol || !_running) {
        return false;
    }
    return _protocol->sendChannelMessage(channel, text);
}

bool MeshService::sendAdvertisement() {
    // TODO: Lock mutex
    if (!_protocol || !_running) {
        return false;
    }
    return _protocol->sendAdvertisement();
}

int MeshService::getContactCount() {
    // TODO: Lock mutex
    if (!_protocol) {
        return 0;
    }
    return _protocol->getContactCount();
}

bool MeshService::getContact(int index, Contact& out) {
    // TODO: Lock mutex
    if (!_protocol) {
        return false;
    }
    return _protocol->getContact(index, out);
}

bool MeshService::findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) {
    // TODO: Lock mutex
    if (!_protocol) {
        return false;
    }
    return _protocol->findContact(publicKey, out);
}

int MeshService::getChannelCount() {
    // TODO: Lock mutex
    if (!_protocol) {
        return 0;
    }
    return _protocol->getChannelCount();
}

bool MeshService::getChannel(int index, Channel& out) {
    // TODO: Lock mutex
    if (!_protocol) {
        return false;
    }
    return _protocol->getChannel(index, out);
}

NodeStatus MeshService::getStatus() {
    // TODO: Lock mutex
    if (!_protocol) {
        return NodeStatus{};
    }
    return _protocol->getStatus();
}

RadioConfig MeshService::getRadioConfig() {
    // TODO: Lock mutex
    if (!_protocol) {
        return RadioConfig{};
    }
    return _protocol->getRadioConfig();
}

bool MeshService::setRadioConfig(const RadioConfig& config) {
    // TODO: Lock mutex
    if (!_protocol) {
        return false;
    }
    return _protocol->setRadioConfig(config);
}

const char* MeshService::getNodeName() {
    // TODO: Lock mutex
    if (!_protocol) {
        return "Unknown";
    }
    return _protocol->getNodeName();
}

bool MeshService::setNodeName(const char* name) {
    // TODO: Lock mutex
    if (!_protocol) {
        return false;
    }
    return _protocol->setNodeName(name);
}

void MeshService::setMessageCallback(MessageCallback callback) {
    _messageCallback = callback;
}

void MeshService::setContactCallback(ContactCallback callback) {
    _contactCallback = callback;
}

void MeshService::setStatusCallback(StatusCallback callback) {
    _statusCallback = callback;
}

void MeshService::setAckCallback(AckCallback callback) {
    _ackCallback = callback;
}

} // namespace meshola
