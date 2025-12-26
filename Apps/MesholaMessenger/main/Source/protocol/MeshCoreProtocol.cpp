#include "MeshCoreProtocol.h"
#include <cstring>
#include <cstdio>
#include <ctime>
 
// ESP32 chip ID for unique node naming
#ifdef ESP_PLATFORM
#include <esp_mac.h>
#endif
 
// Integrate RadioLib for SX1262 on T-Deck
#include <RadioLib.h>
#if defined(RADIOLIB_BUILD_ARDUINO)
#include <SPI.h>
#endif
// NOTE: Full MeshCore integration is still TODO – for now we only use RadioLib
// to get packets on-air/off-air so that end-to-end testing is possible.

#include <Tactility/Log.h>

namespace meshola {

#define TAG "MeshCoreProtocol"

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

#ifdef ESP_PLATFORM
// T-Deck SX1262 pin map
static constexpr int PIN_LORA_NSS   = 9;
static constexpr int PIN_LORA_DIO1  = 45;
static constexpr int PIN_LORA_RST   = 17;
static constexpr int PIN_LORA_BUSY  = 13;
static constexpr int PIN_LORA_SCLK  = 40;
static constexpr int PIN_LORA_MISO  = 38;
static constexpr int PIN_LORA_MOSI  = 41;

static uint32_t sAckCounter = 1;
#endif

// Default MeshCore Public channel (provided by user)
static const char* DEFAULT_CHANNEL_NAME = "Public";
static const char* DEFAULT_CHANNEL_HEX = "8b3387e9c5cdea6ac9e5edbaa115cd72";

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
    memset(&_defaultChannel, 0, sizeof(_defaultChannel));
    memset(_selfPublicKey, 0, sizeof(_selfPublicKey));
    memset(_selfName, 0, sizeof(_selfName));
    
    // Generate unique node name from hardware ID
    generateUniqueNodeName(_nodeName, sizeof(_nodeName));
    strncpy(_defaultChannel.name, DEFAULT_CHANNEL_NAME, sizeof(_defaultChannel.name) - 1);
    _defaultChannel.isPublic = true;
    _defaultChannel.index = 0;
    // Populate channel ID from hex string
    auto hexToByte = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return 0;
    };
    for (size_t i = 0; i < CHANNEL_ID_SIZE && (i * 2 + 1) < strlen(DEFAULT_CHANNEL_HEX); i++) {
        _defaultChannel.id[i] = (hexToByte(DEFAULT_CHANNEL_HEX[i*2]) << 4) |
                                (hexToByte(DEFAULT_CHANNEL_HEX[i*2 + 1]));
    }

    // Synthetic default contact to allow immediate DM testing
    Contact broadcast = {};
    strncpy(broadcast.name, "Public Broadcast", sizeof(broadcast.name) - 1);
    broadcast.isOnline = true;
    broadcast.lastSeen = (uint32_t)time(nullptr);
    broadcast.lastRssi = 0;
    broadcast.pathLength = 1;
    _contacts.push_back(broadcast);
    
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
    
#ifdef ESP_PLATFORM
    // Clean up any previous instance
    if (_radio) {
        delete _radio;
        _radio = nullptr;
    }
    if (_module) {
        delete _module;
        _module = nullptr;
    }

    // Create RadioLib module for SX1262 on T-Deck
    _module = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY);
#if defined(RADIOLIB_BUILD_ARDUINO)
    // When building under Arduino HAL, push the T-Deck SPI pin map.
    _module->spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE0);
    _module->pinSCK = PIN_LORA_SCLK;
    _module->pinMISO = PIN_LORA_MISO;
    _module->pinMOSI = PIN_LORA_MOSI;
#endif

    _radio = new SX1262(_module);

    // Bring up radio with provided config
    int16_t state = _radio->begin(
        _config.frequency,
        _config.bandwidth,
        _config.spreadingFactor,
        _config.codingRate,
        RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
        _config.txPower,
        12  // default preamble
    );
    if (state != RADIOLIB_ERR_NONE) {
        TT_LOG_E(TAG, "Radio begin failed: %d", state);
        return false;
    }

    // Basic runtime tweaks
    _radio->setDio2AsRfSwitch(true);
    _radio->setCRC(2);
    _radio->setOutputPower(_config.txPower);
#if defined(ESP_PLATFORM) && !defined(RADIOLIB_BUILD_ARDUINO)
    // Ensure SPI is initialized when using ESP-IDF HAL
    _radio->getMod()->hal->init();
#endif

    // Prepare continuous RX loop
    _rxListening = false;
#endif
    return true;
}

bool MeshCoreProtocol::start() {
    if (_running) {
        return true;
    }

#ifdef ESP_PLATFORM
    if (!_radio) {
        return false;
    }
    // Kick RX into continuous mode
    if (_radio->startReceive() != RADIOLIB_ERR_NONE) {
        return false;
    }
    _rxListening = true;
#endif

    _running = true;
    return true;
}

void MeshCoreProtocol::stop() {
    if (!_running) {
        return;
    }

#ifdef ESP_PLATFORM
    if (_radio) {
        _radio->standby();
        delete _radio;
        _radio = nullptr;
    }
    if (_module) {
        delete _module;
        _module = nullptr;
    }
    _rxListening = false;
#endif

    _running = false;
}

bool MeshCoreProtocol::isRunning() const {
    return _running;
}

void MeshCoreProtocol::loop() {
    if (!_running) {
        return;
    }

#ifdef ESP_PLATFORM
    if (!_radio) {
        return;
    }

    // Ensure we're in RX mode
    if (!_rxListening) {
        _radio->startReceive();
        _rxListening = true;
    }

    uint32_t irq = _radio->getIrqFlags();
    if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR) {
        TT_LOG_W(TAG, "CRC error");
        _radio->clearIrqFlags(RADIOLIB_SX126X_IRQ_CRC_ERR);
        _radio->startReceive();
        return;
    }
    if (irq & RADIOLIB_SX126X_IRQ_TIMEOUT) {
        _radio->clearIrqFlags(RADIOLIB_SX126X_IRQ_TIMEOUT);
        _radio->startReceive();
        return;
    }

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE) {
        uint8_t rxBuf[RADIOLIB_SX126X_MAX_PACKET_LENGTH + 1] = {0};
        size_t packetLen = _radio->getPacketLength();
        if (packetLen >= sizeof(rxBuf)) {
            packetLen = sizeof(rxBuf) - 1;
        }

        int16_t state = _radio->readData(rxBuf, packetLen);
        _radio->clearIrqFlags(RADIOLIB_SX126X_IRQ_ALL);
        _radio->startReceive();

        if (state == RADIOLIB_ERR_NONE && _messageCallback) {
            Message msg = {};
            if (!parsePacket(rxBuf, packetLen, msg)) {
                // Fallback: treat as plain text
                msg.type = MessageType::Direct;
                msg.isChannel = false;
                msg.isOutgoing = false;
                msg.timestamp = (uint32_t)time(nullptr);
                strncpy(msg.text, reinterpret_cast<const char*>(rxBuf), sizeof(msg.text) - 1);
            }
            msg.rssi = _radio->getRSSI();
            msg.snr = _radio->getSNR();
            msg.status = MessageStatus::Received;
            _messageCallback(msg);
        }
    }
#endif
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
    if (_hasSelfKey) {
        memcpy(out, _selfPublicKey, PUBLIC_KEY_SIZE);
    } else {
        memset(out, 0, PUBLIC_KEY_SIZE);
    }
}

void MeshCoreProtocol::setLocalIdentity(const uint8_t publicKey[PUBLIC_KEY_SIZE],
                                        const char* name) {
    if (publicKey) {
        memcpy(_selfPublicKey, publicKey, PUBLIC_KEY_SIZE);
        _hasSelfKey = true;
    } else {
        _hasSelfKey = false;
        memset(_selfPublicKey, 0, sizeof(_selfPublicKey));
    }
    if (name) {
        strncpy(_selfName, name, sizeof(_selfName) - 1);
        _selfName[sizeof(_selfName) - 1] = '\0';
    }
}

bool MeshCoreProtocol::sendAdvertisement() {
    if (!_running) {
        return false;
    }
    
#ifdef ESP_PLATFORM
    if (!_radio) {
        return false;
    }
    const char* payload = "ADV";
    _radio->standby();
    int16_t state = _radio->transmit(reinterpret_cast<const uint8_t*>(payload), strlen(payload));
    _radio->startReceive();
    _rxListening = true;
    return state == RADIOLIB_ERR_NONE;
#else
    return true;
#endif
}

uint32_t MeshCoreProtocol::sendMessage(const Contact& to, const char* text) {
    if (!_running || !text) {
        return 0;
    }

#ifdef ESP_PLATFORM
    (void)to;
    if (!_radio) {
        return 0;
    }

    size_t len = strnlen(text, MAX_MESSAGE_LEN - 1);
    if (len == 0) {
        return 0;
    }

    uint8_t payload[RADIOLIB_SX126X_MAX_PACKET_LENGTH] = {0};
    size_t payloadLen = 0;
    if (!buildPacket(text, nullptr, to.publicKey, false, payload, payloadLen)) {
        TT_LOG_E(TAG, "Failed to build DM packet");
        return 0;
    }

    // Standby for TX
    _radio->standby();
    int16_t state = _radio->transmit(payload, payloadLen);
    _radio->startReceive();
    _rxListening = true;
    if (state != RADIOLIB_ERR_NONE) {
        return 0;
    }

    uint32_t ackId = sAckCounter++;
    return ackId;
#else
    (void)to;
    return 1;
#endif
}

bool MeshCoreProtocol::sendChannelMessage(const Channel& channel, const char* text) {
    if (!_running || !text) {
        return false;
    }
    
#ifdef ESP_PLATFORM
    if (!_radio) {
        return false;
    }

    uint8_t payload[RADIOLIB_SX126X_MAX_PACKET_LENGTH] = {0};
    size_t payloadLen = 0;
    if (!buildPacket(text, channel.id, nullptr, true, payload, payloadLen)) {
        TT_LOG_E(TAG, "Failed to build channel packet");
        return false;
    }

    _radio->standby();
    int16_t state = _radio->transmit(payload, payloadLen);
    _radio->startReceive();
    _rxListening = true;
    return state == RADIOLIB_ERR_NONE;
#else
    return true;
#endif
}

int MeshCoreProtocol::getContactCount() const {
    return (int)_contacts.size();
}

bool MeshCoreProtocol::getContact(int index, Contact& out) const {
    if (index < 0 || index >= (int)_contacts.size()) {
        memset(&out, 0, sizeof(Contact));
        return false;
    }
    out = _contacts[index];
    return true;
}

bool MeshCoreProtocol::findContact(const uint8_t publicKey[PUBLIC_KEY_SIZE], Contact& out) const {
    if (!publicKey) return false;
    for (const auto& c : _contacts) {
        if (memcmp(c.publicKey, publicKey, PUBLIC_KEY_SIZE) == 0) {
            out = c;
            return true;
        }
    }
    return false;
}

bool MeshCoreProtocol::addContact(const Contact& contact) {
    _contacts.push_back(contact);
    return true;
}

bool MeshCoreProtocol::removeContact(const uint8_t publicKey[PUBLIC_KEY_SIZE]) {
    if (!publicKey) return false;
    for (auto it = _contacts.begin(); it != _contacts.end(); ++it) {
        if (memcmp(it->publicKey, publicKey, PUBLIC_KEY_SIZE) == 0) {
            _contacts.erase(it);
            return true;
        }
    }
    return false;
}

void MeshCoreProtocol::resetPath(const uint8_t publicKey[PUBLIC_KEY_SIZE]) {
    // TODO: Reset path to contact
}

int MeshCoreProtocol::getChannelCount() const {
    return 1;
}

bool MeshCoreProtocol::getChannel(int index, Channel& out) const {
    if (index == 0) {
        out = _defaultChannel;
        return true;
    }
    memset(&out, 0, sizeof(Channel));
    return false;
}

bool MeshCoreProtocol::setChannel(int index, const Channel& channel) {
    if (index == 0) {
        _defaultChannel = channel;
        return true;
    }
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
#ifdef ESP_PLATFORM
    if (_radio) {
        status.lastRssi = (int16_t)_radio->getRSSI(true);
        status.lastSnr = (int8_t)_radio->getSNR();
    }
#endif
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

bool MeshCoreProtocol::buildPacket(const char* text,
                     const uint8_t channelId[CHANNEL_ID_SIZE],
                     const uint8_t recipientKey[PUBLIC_KEY_SIZE],
                     bool isChannel,
                     uint8_t* outBuf,
                     size_t& outLen) const {
    if (!text || !outBuf) return false;
    size_t textLen = strnlen(text, MAX_MESSAGE_LEN - 1);

    const size_t headerLen = 4 + CHANNEL_ID_SIZE + PUBLIC_KEY_SIZE + PUBLIC_KEY_SIZE; // magic(2)+ver+flags + channel + sender + recipient
    if (textLen + headerLen > RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
        return false;
    }

    size_t idx = 0;
    outBuf[idx++] = PACKET_MAGIC_0;
    outBuf[idx++] = PACKET_MAGIC_1;
    outBuf[idx++] = PACKET_VERSION;
    uint8_t flags = isChannel ? PACKET_FLAG_CHANNEL : 0;
    outBuf[idx++] = flags;

    if (channelId) {
        memcpy(&outBuf[idx], channelId, CHANNEL_ID_SIZE);
    } else {
        memset(&outBuf[idx], 0, CHANNEL_ID_SIZE);
    }
    idx += CHANNEL_ID_SIZE;

    // Sender key (self)
    if (_hasSelfKey) {
        memcpy(&outBuf[idx], _selfPublicKey, PUBLIC_KEY_SIZE);
    } else {
        memset(&outBuf[idx], 0, PUBLIC_KEY_SIZE);
    }
    idx += PUBLIC_KEY_SIZE;

    // Recipient key (for DMs)
    if (recipientKey) {
        memcpy(&outBuf[idx], recipientKey, PUBLIC_KEY_SIZE);
    } else {
        memset(&outBuf[idx], 0, PUBLIC_KEY_SIZE);
    }
    idx += PUBLIC_KEY_SIZE;

    memcpy(&outBuf[idx], text, textLen);
    idx += textLen;

    outLen = idx;
    return true;
}

bool MeshCoreProtocol::parsePacket(const uint8_t* data,
                     size_t len,
                     Message& outMsg) const {
    const size_t headerLen = 4 + CHANNEL_ID_SIZE + PUBLIC_KEY_SIZE + PUBLIC_KEY_SIZE;
    if (!data || len < headerLen) {
        return false;
    }
    if (data[0] != PACKET_MAGIC_0 || data[1] != PACKET_MAGIC_1 || data[2] != PACKET_VERSION) {
        return false;
    }
    uint8_t flags = data[3];
    bool isChannel = (flags & PACKET_FLAG_CHANNEL) != 0;

    memset(&outMsg, 0, sizeof(outMsg));
    outMsg.isChannel = isChannel;
    outMsg.isOutgoing = false;
    outMsg.timestamp = (uint32_t)time(nullptr);
    outMsg.type = isChannel ? MessageType::Channel : MessageType::Direct;
    memcpy(outMsg.channelId, &data[4], CHANNEL_ID_SIZE);
    memcpy(outMsg.senderKey, &data[4 + CHANNEL_ID_SIZE], PUBLIC_KEY_SIZE);
    memcpy(outMsg.recipientKey, &data[4 + CHANNEL_ID_SIZE + PUBLIC_KEY_SIZE], PUBLIC_KEY_SIZE);

    size_t textLen = len - headerLen;
    if (textLen >= sizeof(outMsg.text)) {
        textLen = sizeof(outMsg.text) - 1;
    }
    memcpy(outMsg.text, &data[headerLen], textLen);
    outMsg.text[textLen] = '\0';
    strncpy(outMsg.senderName, "Unknown", sizeof(outMsg.senderName) - 1);
    outMsg.status = MessageStatus::Received;

    return true;
}

IProtocol* MeshCoreProtocol::create() {
    return new MeshCoreProtocol();
}

void MeshCoreProtocol::registerSelf() {
    ProtocolRegistry::registerProtocol(meshCoreEntry);
}

} // namespace meshola
