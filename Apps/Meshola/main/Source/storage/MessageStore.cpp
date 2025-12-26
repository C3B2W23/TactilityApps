#include "MessageStore.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

// For directory creation
#ifdef ESP_PLATFORM
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#endif

namespace meshola {

static const char* STORAGE_BASE = "/data/meshola/messenger";

// Singleton instance
static MessageStore* s_instance = nullptr;

MessageStore& MessageStore::getInstance() {
    if (!s_instance) {
        s_instance = new MessageStore();
    }
    return *s_instance;
}

MessageStore::MessageStore()
    : _hasProfile(false)
{
    memset(_profileId, 0, sizeof(_profileId));
    memset(_basePath, 0, sizeof(_basePath));
}

MessageStore::~MessageStore() {
}

void MessageStore::setActiveProfile(const char* profileId) {
    if (!profileId) {
        _hasProfile = false;
        return;
    }
    
    strncpy(_profileId, profileId, sizeof(_profileId) - 1);
    snprintf(_basePath, sizeof(_basePath), "%s/profiles/%s/messages", 
             STORAGE_BASE, profileId);
    _hasProfile = true;
    
    // Ensure messages directory exists
    ensureDirectory(_basePath);
}

bool MessageStore::appendMessage(const Message& msg) {
    if (!_hasProfile) {
        return false;
    }
    
    // Get file path based on message type
    char filePath[192];
    if (msg.isChannel) {
        getChannelFilePath(msg.channelId, filePath, sizeof(filePath));
    } else {
        // For DMs, use sender key for incoming, or we need recipient key for outgoing
        // For outgoing, senderKey should be set to recipient's key
        getContactFilePath(msg.senderKey, filePath, sizeof(filePath));
    }
    
    // Serialize message to JSON
    char jsonLine[512];
    if (!serializeMessage(msg, jsonLine, sizeof(jsonLine))) {
        return false;
    }
    
    // Append to file
    FILE* f = fopen(filePath, "a");
    if (!f) {
        // Try creating parent directory
        ensureDirectory(_basePath);
        f = fopen(filePath, "a");
        if (!f) {
            return false;
        }
    }
    
    fprintf(f, "%s\n", jsonLine);
    fclose(f);
    
    return true;
}

bool MessageStore::loadContactMessages(const uint8_t publicKey[PUBLIC_KEY_SIZE],
                                       int maxMessages,
                                       std::vector<Message>& outMessages) {
    if (!_hasProfile) {
        return false;
    }
    
    char filePath[192];
    getContactFilePath(publicKey, filePath, sizeof(filePath));
    
    FILE* f = fopen(filePath, "r");
    if (!f) {
        // No messages yet - not an error
        return true;
    }
    
    char line[512];
    std::vector<Message> allMessages;
    
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        Message msg;
        if (deserializeMessage(line, msg)) {
            allMessages.push_back(msg);
        }
    }
    
    fclose(f);
    
    // Return last N messages if maxMessages specified
    if (maxMessages > 0 && (int)allMessages.size() > maxMessages) {
        outMessages.assign(allMessages.end() - maxMessages, allMessages.end());
    } else {
        outMessages = std::move(allMessages);
    }
    
    return true;
}

bool MessageStore::loadChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE],
                                       int maxMessages,
                                       std::vector<Message>& outMessages) {
    if (!_hasProfile) {
        return false;
    }
    
    char filePath[192];
    getChannelFilePath(channelId, filePath, sizeof(filePath));
    
    FILE* f = fopen(filePath, "r");
    if (!f) {
        return true;  // No messages yet
    }
    
    char line[512];
    std::vector<Message> allMessages;
    
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        Message msg;
        if (deserializeMessage(line, msg)) {
            allMessages.push_back(msg);
        }
    }
    
    fclose(f);
    
    if (maxMessages > 0 && (int)allMessages.size() > maxMessages) {
        outMessages.assign(allMessages.end() - maxMessages, allMessages.end());
    } else {
        outMessages = std::move(allMessages);
    }
    
    return true;
}

int MessageStore::getContactMessageCount(const uint8_t publicKey[PUBLIC_KEY_SIZE]) {
    std::vector<Message> msgs;
    loadContactMessages(publicKey, 0, msgs);
    return (int)msgs.size();
}

int MessageStore::getChannelMessageCount(const uint8_t channelId[CHANNEL_ID_SIZE]) {
    std::vector<Message> msgs;
    loadChannelMessages(channelId, 0, msgs);
    return (int)msgs.size();
}

bool MessageStore::deleteContactMessages(const uint8_t publicKey[PUBLIC_KEY_SIZE]) {
    if (!_hasProfile) {
        return false;
    }
    
    char filePath[192];
    getContactFilePath(publicKey, filePath, sizeof(filePath));
    return remove(filePath) == 0;
}

bool MessageStore::deleteChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE]) {
    if (!_hasProfile) {
        return false;
    }
    
    char filePath[192];
    getChannelFilePath(channelId, filePath, sizeof(filePath));
    return remove(filePath) == 0;
}

bool MessageStore::deleteAllMessages() {
    if (!_hasProfile) {
        return false;
    }
    
    // TODO: Iterate directory and delete all .jsonl files
    // For now, this is a stub
    return false;
}

void MessageStore::getContactFilePath(const uint8_t publicKey[PUBLIC_KEY_SIZE],
                                      char* dest, size_t maxLen) {
    char keyHex[PUBLIC_KEY_SIZE * 2 + 1];
    bytesToHex(publicKey, PUBLIC_KEY_SIZE, keyHex);
    snprintf(dest, maxLen, "%s/dm_%s.jsonl", _basePath, keyHex);
}

void MessageStore::getChannelFilePath(const uint8_t channelId[CHANNEL_ID_SIZE],
                                      char* dest, size_t maxLen) {
    char idHex[CHANNEL_ID_SIZE * 2 + 1];
    bytesToHex(channelId, CHANNEL_ID_SIZE, idHex);
    snprintf(dest, maxLen, "%s/ch_%s.jsonl", _basePath, idHex);
}

void MessageStore::bytesToHex(const uint8_t* data, size_t len, char* dest) {
    for (size_t i = 0; i < len; i++) {
        sprintf(dest + i * 2, "%02x", data[i]);
    }
    dest[len * 2] = '\0';
}

bool MessageStore::hexToBytes(const char* hex, uint8_t* dest, size_t maxLen) {
    size_t hexLen = strlen(hex);
    if (hexLen % 2 != 0 || hexLen / 2 > maxLen) {
        return false;
    }
    
    for (size_t i = 0; i < hexLen / 2; i++) {
        char byte[3] = { hex[i*2], hex[i*2+1], '\0' };
        dest[i] = (uint8_t)strtol(byte, nullptr, 16);
    }
    return true;
}

bool MessageStore::serializeMessage(const Message& msg, char* dest, size_t maxLen) {
    // Simple JSON serialization
    // Escape any quotes in text
    char escapedText[MAX_MESSAGE_LEN * 2];
    size_t j = 0;
    for (size_t i = 0; msg.text[i] && j < sizeof(escapedText) - 2; i++) {
        if (msg.text[i] == '"' || msg.text[i] == '\\') {
            escapedText[j++] = '\\';
        }
        if (msg.text[i] == '\n') {
            escapedText[j++] = '\\';
            escapedText[j++] = 'n';
        } else if (msg.text[i] == '\r') {
            escapedText[j++] = '\\';
            escapedText[j++] = 'r';
        } else {
            escapedText[j++] = msg.text[i];
        }
    }
    escapedText[j] = '\0';
    
    // Convert keys to hex
    char senderKeyHex[PUBLIC_KEY_SIZE * 2 + 1];
    char channelIdHex[CHANNEL_ID_SIZE * 2 + 1];
    bytesToHex(msg.senderKey, PUBLIC_KEY_SIZE, senderKeyHex);
    bytesToHex(msg.channelId, CHANNEL_ID_SIZE, channelIdHex);
    
    int written = snprintf(dest, maxLen,
        "{\"ts\":%lu,\"sk\":\"%s\",\"ch\":\"%s\",\"sn\":\"%s\","
        "\"txt\":\"%s\",\"st\":%d,\"ack\":%lu,\"isCh\":%s,\"isOut\":%s,"
        "\"rssi\":%d,\"snr\":%d}",
        (unsigned long)msg.timestamp,
        senderKeyHex,
        channelIdHex,
        msg.senderName,
        escapedText,
        (int)msg.status,
        (unsigned long)msg.ackId,
        msg.isChannel ? "true" : "false",
        msg.isOutgoing ? "true" : "false",
        msg.rssi,
        msg.snr
    );
    
    return written > 0 && (size_t)written < maxLen;
}

bool MessageStore::deserializeMessage(const char* json, Message& msg) {
    // Simple JSON parsing - look for known fields
    // This is a minimal parser, not a full JSON implementation
    
    memset(&msg, 0, sizeof(msg));
    
    // Helper lambda to find value after key
    auto findValue = [json](const char* key, char* dest, size_t maxLen) -> bool {
        char searchKey[64];
        snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);
        const char* pos = strstr(json, searchKey);
        if (!pos) return false;
        pos += strlen(searchKey);
        
        // Skip whitespace
        while (*pos == ' ') pos++;
        
        if (*pos == '"') {
            // String value
            pos++;
            size_t i = 0;
            while (*pos && *pos != '"' && i < maxLen - 1) {
                if (*pos == '\\' && *(pos+1)) {
                    pos++;
                    if (*pos == 'n') dest[i++] = '\n';
                    else if (*pos == 'r') dest[i++] = '\r';
                    else dest[i++] = *pos;
                } else {
                    dest[i++] = *pos;
                }
                pos++;
            }
            dest[i] = '\0';
            return true;
        } else {
            // Number or boolean
            size_t i = 0;
            while (*pos && *pos != ',' && *pos != '}' && i < maxLen - 1) {
                dest[i++] = *pos++;
            }
            dest[i] = '\0';
            return true;
        }
    };
    
    char buf[256];
    
    // Parse timestamp
    if (findValue("ts", buf, sizeof(buf))) {
        msg.timestamp = (uint32_t)strtoul(buf, nullptr, 10);
    }
    
    // Parse sender key
    if (findValue("sk", buf, sizeof(buf))) {
        hexToBytes(buf, msg.senderKey, PUBLIC_KEY_SIZE);
    }
    
    // Parse channel ID
    if (findValue("ch", buf, sizeof(buf))) {
        hexToBytes(buf, msg.channelId, CHANNEL_ID_SIZE);
    }
    
    // Parse sender name
    if (findValue("sn", buf, sizeof(buf))) {
        strncpy(msg.senderName, buf, MAX_NODE_NAME_LEN - 1);
    }
    
    // Parse text
    if (findValue("txt", buf, sizeof(buf))) {
        strncpy(msg.text, buf, MAX_MESSAGE_LEN - 1);
    }
    
    // Parse status
    if (findValue("st", buf, sizeof(buf))) {
        msg.status = (MessageStatus)atoi(buf);
    }
    
    // Parse ack ID
    if (findValue("ack", buf, sizeof(buf))) {
        msg.ackId = (uint32_t)strtoul(buf, nullptr, 10);
    }
    
    // Parse isChannel
    if (findValue("isCh", buf, sizeof(buf))) {
        msg.isChannel = (strcmp(buf, "true") == 0);
    }
    
    // Parse isOutgoing
    if (findValue("isOut", buf, sizeof(buf))) {
        msg.isOutgoing = (strcmp(buf, "true") == 0);
    }
    
    // Parse RSSI
    if (findValue("rssi", buf, sizeof(buf))) {
        msg.rssi = (int16_t)atoi(buf);
    }
    
    // Parse SNR
    if (findValue("snr", buf, sizeof(buf))) {
        msg.snr = (int8_t)atoi(buf);
    }
    
    return true;
}

bool MessageStore::ensureDirectory(const char* path) {
#ifdef ESP_PLATFORM
    struct stat st;
    if (stat(path, &st) == 0) {
        return true;  // Already exists
    }
    
    // Create parent directories recursively
    char tmp[256];
    strncpy(tmp, path, sizeof(tmp) - 1);
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#else
    // Stub for non-ESP builds
    return true;
#endif
}

} // namespace meshola
