#include "MeshCoreProtocol.h"
#include <cstring>
#include <cstdio>

// ESP32 chip ID for unique node naming
#ifdef ESP_PLATFORM
#include <esp_mac.h>
#endif

// TODO: Include actual MeshCore headers when integrated
// #include <Mesh.h>
// #include <helpers/BaseChatMesh.h>
// #include <RadioLib.h>

namespace meshola {

/**
 * Generate a unique node name based on ESP32 MAC address.
 * Format: "Meshola-XXXX" where XXXX is last 2 bytes of MAC in hex.
 */
static void generateUniqueNodeName(char* dest, size_t maxLen) {
#ifdef ESP_PLATFORM
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(dest, maxLen, "Meshola-%02X%02X", mac[4], mac[5]);
#else
    // Fallback for non-ESP32 builds (testing/simulation)
    snprintf(dest, maxLen, "Meshola-0000");
#endif
}

// Protocol registration entry
static const ProtocolEntry meshCoreEntry = {
    .id = "meshcore",
    .name = "MeshCore (Standard)",
    .create = MeshCoreProtocol::create
};

MeshCoreProtocol::MeshCoreProtocol()
    : _running(false)
    , _messageCallback(nullptr)
    , _contactCallback(nullptr)
    , _statusCallback(nullptr)
    , _ackCallback(nullptr)
    , _errorCallback(nullptr)
{
    memset(&_config, 0, sizeof(_config));
    memset(_nodeName, 0, sizeof(_nodeName));
    
    // Generate unique node name from hardware ID
    generateUniqueNodeName(_nodeName, sizeof(_nodeName));
    
    // Default radio config for MeshCore
    _config.frequency = 906.875f;      // US default
    _config.bandwidth = 250.0f;        // kHz
    _config.spreadingFactor = 11;
    _config.codingRate = 5;            // 4/5
    _config.txPower = 22;              // dBm
}

MeshCoreProtocol::~MeshCoreProtocol() {
    stop();
}

bool MeshCoreProtocol::init(const RadioConfig& config) {
    _config = config;
    
    // TODO: Initialize RadioLib SX1262 with T-Deck pins
    // P_LORA_NSS = 9
    // P_LORA_DIO_1 = 45
    // P_LORA_RESET = 17
    // P_LORA_BUSY = 13
    // P_LORA_SCLK = 40
    // P_LORA_MISO = 38
    // P_LORA_MOSI = 41
    
    // TODO: Initialize MeshCore objects
    // _radio = new SX1262(...);
    // _mesh = new MesholaMessengerMesh(_radio, ...);
    
    return true;
}

bool MeshCoreProtocol::start() {
    if (_running) {
        return true;
    }
    
    // TODO: Start radio and mesh
    // _radio->begin();
    // _mesh->begin();
    
    _running = true;
    return true;
}

void MeshCoreProtocol::stop() {
    if (!_running) {
        return;
    }
    
    // TODO: Stop mesh and radio
    
    _running = false;
}

bool MeshCoreProtocol::isRunning() const {
    return _running;
}

void MeshCoreProtocol::loop() {
    if (!_running) {
        return;
    }
    
    // TODO: Call mesh loop
    // _mesh->loop();
}

ProtocolInfo MeshCoreProtocol::getInfo() const {
    return ProtocolInfo{
        .id = "meshcore",
        .name = "MeshCore",
        .version = "1.0.0",
        .description = "Standard MeshCore protocol for off-grid mesh messaging",
        .capabilities = (1 << static_cast<int>(ProtocolFeature::DirectMessages)) |
                       (1 << static_cast<int>(ProtocolFeature::Channels)) |
                       (1 << static_cast<int>(ProtocolFeature::SignedMessages)) |
                       (1 << static_cast<int>(ProtocolFeature::LocationSharing)) |
                       (1 << static_cast<int>(ProtocolFeature::PathRouting)) |
                       (1 << static_cast<int>(ProtocolFeature::Encryption))
    };
}

bool MeshCoreProtocol::hasFeature(ProtocolFeature feature) const {
    ProtocolInfo info = getInfo();
    return (info.capabilities & (1 << static_cast<int>(feature))) != 0;
}

const char* MeshCoreProtocol::getNodeName() const {
    return _nodeName;
}

bool MeshCoreProtocol::setNodeName(const char* name) {
    if (!name || strlen(name) >= MAX_NODE_NAME_LEN) {
        return false;
    }
    strncpy(_nodeName, name, MAX_NODE_NAME_LEN - 1);
    _nodeName[MAX_NODE_NAME_LEN - 1] = '\0';
    return true;
}

void MeshCoreProtocol::getPublicKey(uint8_t out[PUBLIC_KEY_SIZE]) const {
    // TODO: Get from mesh identity
    // memcpy(out, _mesh->self_id.pub_key, PUBLIC_KEY_SIZE);
    memset(out, 0, PUBLIC_KEY_SIZE);
}

bool MeshCoreProtocol::sendAdvertisement() {
    if (!_running) {
        return false;
    }
    
    // TODO: Create and send advertisement
    // auto* pkt = _mesh->createSelfAdvert(_nodeName);
    // _mesh->sendFlood(pkt);
    
    return true;
}

uint32_t MeshCoreProtocol::sendMessage(const Contact& to, const char* text) {
    if (!_running || !text) {
        return 0;
    }
    
    // TODO: Send message via mesh
    // ContactInfo contact;
    // memcpy(contact.pub_key, to.publicKey, PUBLIC_KEY_SIZE);
    // uint32_t ack_id, timeout;
    // _mesh->sendMessage(contact, timestamp, 0, text, ack_id, timeout);
    // return ack_id;
    
    return 0;  // Placeholder
}

bool MeshCoreProtocol::sendChannelMessage(const Channel& channel, const char* text) {
    if (!_running || !text) {
        return false;
    }
    
    // TODO: Send channel message
    // mesh::GroupChannel ch;
    // memcpy(ch.hash, channel.id, CHANNEL_ID_SIZE);
    // _mesh->sendGroupMessage(timestamp, ch, _nodeName, text, strlen(text));
    
    return true;
}

int MeshCoreProtocol::getContactCount() const {
    // TODO: return _mesh->getNumContacts();
    return 0;
}

bool MeshCoreProtocol::getContact(int index, Contact& out) const {
    // TODO: Get contact from mesh
    // ContactInfo info;
    // if (!_mesh->getContactByIdx(index, info)) return false;
    // Convert ContactInfo to Contact
    
    memset(&out, 0, sizeof(Contact));
    return false;
}

bool MeshCoreProtocol::findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const {
    // TODO: Look up contact by public key
    return false;
}

bool MeshCoreProtocol::addContact(const Contact& contact) {
    // TODO: Add contact to mesh
    return false;
}

bool MeshCoreProtocol::removeContact(const uint8_t publicKey[PUBLIC_KEY_SIZE]) {
    // TODO: Remove contact from mesh
    return false;
}

void MeshCoreProtocol::resetPath(const uint8_t publicKey[PUBLIC_KEY_SIZE]) {
    // TODO: Reset path to contact
}

int MeshCoreProtocol::getChannelCount() const {
    // TODO: Return channel count
    return 0;
}

bool MeshCoreProtocol::getChannel(int index, Channel& out) const {
    // TODO: Get channel by index
    memset(&out, 0, sizeof(Channel));
    return false;
}

bool MeshCoreProtocol::setChannel(int index, const Channel& channel) {
    // TODO: Set channel
    return false;
}

RadioConfig MeshCoreProtocol::getRadioConfig() const {
    return _config;
}

bool MeshCoreProtocol::setRadioConfig(const RadioConfig& config) {
    _config = config;
    
    if (_running) {
        // TODO: Apply new radio settings
        // _radio->setFrequency(config.frequency);
        // _radio->setBandwidth(config.bandwidth);
        // etc.
    }
    
    return true;
}

NodeStatus MeshCoreProtocol::getStatus() const {
    NodeStatus status = {};
    status.radioRunning = _running;
    
    // TODO: Get actual status
    // status.batteryMillivolts = board.getBattMilliVolts();
    // status.uptime = millis() / 1000;
    // status.freeHeap = ESP.getFreeHeap();
    
    return status;
}

void MeshCoreProtocol::setMessageCallback(MessageCallback callback) {
    _messageCallback = callback;
}

void MeshCoreProtocol::setContactCallback(ContactCallback callback) {
    _contactCallback = callback;
}

void MeshCoreProtocol::setStatusCallback(StatusCallback callback) {
    _statusCallback = callback;
}

void MeshCoreProtocol::setAckCallback(AckCallback callback) {
    _ackCallback = callback;
}

void MeshCoreProtocol::setErrorCallback(ErrorCallback callback) {
    _errorCallback = callback;
}

bool MeshCoreProtocol::saveState() {
    // TODO: Save contacts and channels to flash/SD
    return true;
}

bool MeshCoreProtocol::loadState() {
    // TODO: Load contacts and channels from flash/SD
    return true;
}

IProtocol* MeshCoreProtocol::create() {
    return new MeshCoreProtocol();
}

void MeshCoreProtocol::registerSelf() {
    ProtocolRegistry::registerProtocol(meshCoreEntry);
}

} // namespace meshola
