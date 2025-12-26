#pragma once

#include "protocol/IProtocol.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <string>
#include <functional>

namespace meshola {

constexpr size_t PROFILE_ID_LEN = 16;
constexpr size_t PROFILE_NAME_LEN = 32;
constexpr size_t MAX_PROFILES = 16;
constexpr size_t MAX_PROTOCOL_SETTINGS = 32;

/**
 * Protocol-specific setting stored as key-value string pair.
 */
struct ProtocolSettingValue {
    char key[32];
    char value[64];
};

/**
 * Profile - A complete configuration for a mesh network identity.
 * 
 * Each profile has:
 * - Unique identity (keypair)
 * - Protocol selection
 * - Radio configuration
 * - Protocol-specific settings
 * - Separate chat history, contacts, channels
 */
struct Profile {
    // Profile metadata
    char id[PROFILE_ID_LEN];          // UUID (e.g., "a1b2c3d4")
    char name[PROFILE_NAME_LEN];      // Display name (e.g., "Home", "CustomFork")
    uint32_t createdAt;               // Unix timestamp
    uint32_t lastUsedAt;              // Unix timestamp
    
    // Protocol selection
    char protocolId[32];              // "meshcore", "customfork", "meshtastic"
    
    // Radio configuration
    RadioConfig radio;
    
    // Node identity
    char nodeName[MAX_NODE_NAME_LEN];
    uint8_t publicKey[PUBLIC_KEY_SIZE];
    uint8_t privateKey[PUBLIC_KEY_SIZE];  // Same size as public for most crypto
    bool hasKeys;                     // true if keys have been generated/imported
    
    // Protocol-specific settings (key-value pairs)
    ProtocolSettingValue protocolSettings[MAX_PROTOCOL_SETTINGS];
    int protocolSettingCount;
    
    /**
     * Initialize with defaults.
     */
    void initDefaults() {
        memset(this, 0, sizeof(Profile));
        
        // Default radio config
        radio.frequency = 906.875f;
        radio.bandwidth = 250.0f;
        radio.spreadingFactor = 11;
        radio.codingRate = 5;
        radio.txPower = 22;
        
        strcpy(protocolId, "meshcore");
        strcpy(nodeName, "Meshola");
        hasKeys = false;
        protocolSettingCount = 0;
    }
    
    /**
     * Get a protocol-specific setting value.
     */
    const char* getProtocolSetting(const char* key) const {
        for (int i = 0; i < protocolSettingCount; i++) {
            if (strcmp(protocolSettings[i].key, key) == 0) {
                return protocolSettings[i].value;
            }
        }
        return nullptr;
    }
    
    /**
     * Set a protocol-specific setting value.
     */
    bool setProtocolSetting(const char* key, const char* value) {
        // Look for existing
        for (int i = 0; i < protocolSettingCount; i++) {
            if (strcmp(protocolSettings[i].key, key) == 0) {
                strncpy(protocolSettings[i].value, value, sizeof(protocolSettings[i].value) - 1);
                return true;
            }
        }
        // Add new
        if (protocolSettingCount < MAX_PROTOCOL_SETTINGS) {
            strncpy(protocolSettings[protocolSettingCount].key, key, 
                    sizeof(protocolSettings[protocolSettingCount].key) - 1);
            strncpy(protocolSettings[protocolSettingCount].value, value,
                    sizeof(protocolSettings[protocolSettingCount].value) - 1);
            protocolSettingCount++;
            return true;
        }
        return false;
    }
};

/**
 * ProfileManager - Manages profiles and persistence.
 * 
 * Handles:
 * - Creating, editing, deleting profiles
 * - Loading/saving profiles to storage
 * - Switching active profile
 * - Generating unique profile IDs
 */
class ProfileManager {
public:
    using ProfileSwitchCallback = std::function<void(const Profile& newProfile)>;
    
    /**
     * Get singleton instance.
     */
    static ProfileManager& getInstance();
    
    /**
     * Initialize - load profiles from storage.
     * Call this at app startup.
     */
    bool init();
    
    /**
     * Get the currently active profile.
     */
    const Profile* getActiveProfile() const;
    
    /**
     * Get active profile for modification.
     * Call saveActiveProfile() after changes.
     */
    Profile* getActiveProfileMutable();
    
    /**
     * Get number of profiles.
     */
    int getProfileCount() const;
    
    /**
     * Get profile by index.
     */
    const Profile* getProfile(int index) const;
    
    /**
     * Find profile by ID.
     */
    const Profile* findProfileById(const char* id) const;
    
    /**
     * Find profile by name.
     */
    const Profile* findProfileByName(const char* name) const;
    
    /**
     * Create a new profile with defaults.
     * Returns the new profile, or nullptr if max profiles reached.
     */
    Profile* createProfile(const char* name);
    
    /**
     * Delete a profile by ID.
     * Cannot delete the active profile if it's the only one.
     */
    bool deleteProfile(const char* id);
    
    /**
     * Switch to a different profile.
     * This will:
     * - Save current profile data
     * - Load new profile
     * - Trigger callback for protocol reinitialization
     */
    bool switchToProfile(const char* id);
    
    /**
     * Save the active profile to storage.
     */
    bool saveActiveProfile();
    
    /**
     * Save all profiles metadata (list + active ID).
     */
    bool saveProfileList();
    
    /**
     * Set callback for profile switches.
     * The callback should reinitialize the protocol with new settings.
     */
    void setProfileSwitchCallback(ProfileSwitchCallback callback);
    
    /**
     * Generate unique node name for a profile.
     * Uses hardware ID to create "Meshola-XXXX" format.
     */
    void generateNodeName(char* dest, size_t maxLen);
    
    /**
     * Generate new keypair for a profile.
     */
    bool generateKeys(Profile& profile);
    
    /**
     * Get the storage path for a profile's data.
     * e.g., "/data/meshola/profiles/abc123/"
     */
    void getProfileDataPath(const char* profileId, char* dest, size_t maxLen);

private:
    ProfileManager();
    ~ProfileManager();
    
    // Non-copyable
    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;
    
    // Generate a unique profile ID
    void generateProfileId(char* dest, size_t maxLen);
    
    // Storage helpers
    bool loadProfileList();
    bool loadProfile(Profile& profile, const char* id);
    bool saveProfile(const Profile& profile);
    bool createProfileDirectory(const char* id);
    
    // Data
    Profile _profiles[MAX_PROFILES];
    int _profileCount;
    int _activeProfileIndex;
    bool _initialized;
    
    ProfileSwitchCallback _switchCallback;
    
    // Storage base path
    static constexpr const char* STORAGE_BASE = "/data/meshola";
};

} // namespace meshola
