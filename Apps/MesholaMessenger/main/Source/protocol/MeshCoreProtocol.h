#pragma once

#include "IProtocol.h"
#include <cstring>
#include <cstdint>
#include <array>
#include <vector>

namespace meshola {

/**
 * MeshCore protocol implementation.
 * 
 * This wraps the actual MeshCore library (Mesh, BaseChatMesh, etc.)
 * and exposes it through the IProtocol interface.
 */
class MeshCoreProtocol : public IProtocol {
public:
    MeshCoreProtocol();
    ~MeshCoreProtocol() override;

    // Lifecycle
    bool init(const RadioConfig& config) override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    void loop() override;

    // Protocol Information
    ProtocolInfo getInfo() const override;
    bool hasFeature(ProtocolFeature feature) const override;

    // Identity
    const char* getNodeName() const override;
    bool setNodeName(const char* name) override;
    void getPublicKey(uint8_t out[PUBLIC_KEY_SIZE]) const override;
    bool sendAdvertisement() override;
    void setLocalIdentity(const uint8_t publicKey[PUBLIC_KEY_SIZE],
                          const char* name) override;

    // Messaging
    uint32_t sendMessage(const Contact& to, const char* text) override;
    bool sendChannelMessage(const Channel& channel, const char* text) override;

    // Contacts
    int getContactCount() const override;
    bool getContact(int index, Contact& out) const override;
    bool findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const override;
    bool addContact(const Contact& contact) override;
    bool removeContact(const uint8_t publicKey[PUBLIC_KEY_SIZE]) override;
    void resetPath(const uint8_t publicKey[PUBLIC_KEY_SIZE]) override;

    // Channels
    int getChannelCount() const override;
    bool getChannel(int index, Channel& out) const override;
    bool setChannel(int index, const Channel& channel) override;

    // Radio Configuration
    RadioConfig getRadioConfig() const override;
    bool setRadioConfig(const RadioConfig& config) override;
    NodeStatus getStatus() const override;

    // Event Callbacks
    void setMessageCallback(MessageCallback callback) override;
    void setContactCallback(ContactCallback callback) override;
    void setStatusCallback(StatusCallback callback) override;
    void setAckCallback(AckCallback callback) override;
    void setErrorCallback(ErrorCallback callback) override;

    // Persistence
    bool saveState() override;
    bool loadState() override;

    // Factory function for registration
    static IProtocol* create();
    
    // Protocol registration helper
    static void registerSelf();

private:
    // Internal state
    bool _running;
    RadioConfig _config;
    char _nodeName[MAX_NODE_NAME_LEN];
    
    // Callbacks
    MessageCallback _messageCallback;
    ContactCallback _contactCallback;
    StatusCallback _statusCallback;
    AckCallback _ackCallback;
    ErrorCallback _errorCallback;
    
#ifdef ESP_PLATFORM
    Module* _module = nullptr;
    SX1262* _radio = nullptr;
    bool _rxListening = false;
#endif

    // Local identity cached for framing
    uint8_t _selfPublicKey[PUBLIC_KEY_SIZE]{};
    char _selfName[MAX_NODE_NAME_LEN]{};
    bool _hasSelfKey = false;

    // Packet framing helpers
    static constexpr uint8_t PACKET_MAGIC_0 = 0x4d; // 'M'
    static constexpr uint8_t PACKET_MAGIC_1 = 0x4c; // 'L'
    static constexpr uint8_t PACKET_VERSION = 0x01;
    static constexpr uint8_t PACKET_FLAG_CHANNEL = 0x01;

    bool buildPacket(const char* text,
                     const uint8_t channelId[CHANNEL_ID_SIZE],
                     const uint8_t recipientKey[PUBLIC_KEY_SIZE],
                     bool isChannel,
                     uint8_t* outBuf,
                     size_t& outLen) const;
    bool parsePacket(const uint8_t* data,
                     size_t len,
                     Message& outMsg) const;

    // Default public channel (MeshCore default)
    Channel _defaultChannel{};
    // Synthetic contacts list (e.g., defaults)
    std::vector<Contact> _contacts;
    // MeshCore integration placeholder
    // BaseChatMesh* _mesh = nullptr;
};

} // namespace meshola
