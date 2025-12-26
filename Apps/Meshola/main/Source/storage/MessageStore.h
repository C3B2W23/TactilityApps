#pragma once

#include "protocol/IProtocol.h"
#include <vector>
#include <cstdint>

namespace meshola {

/**
 * MessageStore - Persistent message storage per profile.
 * 
 * Uses JSON Lines format (one JSON object per line) for append-only writes.
 * Each profile has separate message storage.
 * 
 * Storage format:
 * /data/meshola/messenger/profiles/{profileId}/messages/
 *   ├── dm_{contactKeyHex}.jsonl    # DMs with specific contact
 *   └── ch_{channelIdHex}.jsonl     # Channel messages
 */
class MessageStore {
public:
    /**
     * Get singleton instance.
     */
    static MessageStore& getInstance();
    
    /**
     * Set the active profile ID.
     * Call this when profile switches.
     */
    void setActiveProfile(const char* profileId);
    
    /**
     * Append a message to storage.
     * Called immediately on message receive/send.
     * Returns true on success.
     */
    bool appendMessage(const Message& msg);
    
    /**
     * Load message history for a contact (DMs).
     * Returns messages in chronological order.
     * @param publicKey Contact's public key
     * @param maxMessages Maximum messages to load (0 = all)
     * @param outMessages Vector to populate
     */
    bool loadContactMessages(const uint8_t publicKey[PUBLIC_KEY_SIZE], 
                             int maxMessages,
                             std::vector<Message>& outMessages);
    
    /**
     * Load message history for a channel.
     * Returns messages in chronological order.
     * @param channelId Channel ID
     * @param maxMessages Maximum messages to load (0 = all)
     * @param outMessages Vector to populate
     */
    bool loadChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE],
                             int maxMessages,
                             std::vector<Message>& outMessages);
    
    /**
     * Get count of messages for a contact.
     */
    int getContactMessageCount(const uint8_t publicKey[PUBLIC_KEY_SIZE]);
    
    /**
     * Get count of messages for a channel.
     */
    int getChannelMessageCount(const uint8_t channelId[CHANNEL_ID_SIZE]);
    
    /**
     * Delete all messages for a contact.
     */
    bool deleteContactMessages(const uint8_t publicKey[PUBLIC_KEY_SIZE]);
    
    /**
     * Delete all messages for a channel.
     */
    bool deleteChannelMessages(const uint8_t channelId[CHANNEL_ID_SIZE]);
    
    /**
     * Delete all messages for the active profile.
     */
    bool deleteAllMessages();

private:
    MessageStore();
    ~MessageStore();
    
    // Non-copyable
    MessageStore(const MessageStore&) = delete;
    MessageStore& operator=(const MessageStore&) = delete;
    
    // Helper to get file path for contact DMs
    void getContactFilePath(const uint8_t publicKey[PUBLIC_KEY_SIZE], 
                            char* dest, size_t maxLen);
    
    // Helper to get file path for channel messages
    void getChannelFilePath(const uint8_t channelId[CHANNEL_ID_SIZE],
                            char* dest, size_t maxLen);
    
    // Helper to convert bytes to hex string
    static void bytesToHex(const uint8_t* data, size_t len, char* dest);
    
    // Helper to convert hex string to bytes
    static bool hexToBytes(const char* hex, uint8_t* dest, size_t maxLen);
    
    // Serialize message to JSON line
    bool serializeMessage(const Message& msg, char* dest, size_t maxLen);
    
    // Deserialize message from JSON line
    bool deserializeMessage(const char* json, Message& msg);
    
    // Ensure directory exists
    bool ensureDirectory(const char* path);
    
    // Active profile ID
    char _profileId[32];
    bool _hasProfile;
    
    // Base path for current profile's messages
    char _basePath[128];
};

} // namespace meshola
