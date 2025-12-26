#include "Profile.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>

// TODO: Replace with actual ESP32/Tactility includes when integrated
#ifdef ESP_PLATFORM
#include <esp_mac.h>
#include <esp_random.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace meshola {

ProfileManager::ProfileManager()
    : _profileCount(0)
    , _activeProfileIndex(-1)
    , _initialized(false)
    , _switchCallback(nullptr)
{
    memset(_profiles, 0, sizeof(_profiles));
}

ProfileManager::~ProfileManager() {
    // Save on shutdown
    if (_initialized && _activeProfileIndex >= 0) {
        saveActiveProfile();
        saveProfileList();
    }
}

bool ProfileManager::init() {
    if (_initialized) {
        return true;
    }
    
    // Try to load existing profiles
    if (!loadProfileList()) {
        // No profiles exist - create default profile
        Profile* defaultProfile = createProfile("Default");
        if (defaultProfile) {
            generateNodeName(defaultProfile->nodeName, sizeof(defaultProfile->nodeName));
            generateKeys(*defaultProfile);
            _activeProfileIndex = 0;
            saveProfile(*defaultProfile);
            saveProfileList();
        }
    }
    
    _initialized = true;
    return _profileCount > 0;
}

const Profile* ProfileManager::getActiveProfile() const {
    if (_activeProfileIndex >= 0 && _activeProfileIndex < _profileCount) {
        return &_profiles[_activeProfileIndex];
    }
    return nullptr;
}

Profile* ProfileManager::getActiveProfileMutable() {
    if (_activeProfileIndex >= 0 && _activeProfileIndex < _profileCount) {
        return &_profiles[_activeProfileIndex];
    }
    return nullptr;
}

int ProfileManager::getProfileCount() const {
    return _profileCount;
}

const Profile* ProfileManager::getProfile(int index) const {
    if (index >= 0 && index < _profileCount) {
        return &_profiles[index];
    }
    return nullptr;
}

const Profile* ProfileManager::findProfileById(const char* id) const {
    for (int i = 0; i < _profileCount; i++) {
        if (strcmp(_profiles[i].id, id) == 0) {
            return &_profiles[i];
        }
    }
    return nullptr;
}

const Profile* ProfileManager::findProfileByName(const char* name) const {
    for (int i = 0; i < _profileCount; i++) {
        if (strcmp(_profiles[i].name, name) == 0) {
            return &_profiles[i];
        }
    }
    return nullptr;
}

Profile* ProfileManager::createProfile(const char* name) {
    if (_profileCount >= MAX_PROFILES) {
        return nullptr;
    }
    
    Profile& profile = _profiles[_profileCount];
    profile.initDefaults();
    
    // Generate unique ID
    generateProfileId(profile.id, sizeof(profile.id));
    
    // Set name
    strncpy(profile.name, name, sizeof(profile.name) - 1);
    
    // Set timestamps
    profile.createdAt = time(nullptr);
    profile.lastUsedAt = profile.createdAt;
    
    // Generate node name and keys
    generateNodeName(profile.nodeName, sizeof(profile.nodeName));
    generateKeys(profile);
    
    // Create storage directory
    createProfileDirectory(profile.id);
    
    _profileCount++;
    return &profile;
}

bool ProfileManager::deleteProfile(const char* id) {
    // Find profile index
    int deleteIndex = -1;
    for (int i = 0; i < _profileCount; i++) {
        if (strcmp(_profiles[i].id, id) == 0) {
            deleteIndex = i;
            break;
        }
    }
    
    if (deleteIndex < 0) {
        return false;  // Profile not found
    }
    
    // Can't delete if it's the only profile
    if (_profileCount == 1) {
        return false;
    }
    
    // If deleting active profile, switch to another first
    if (deleteIndex == _activeProfileIndex) {
        int newActive = (deleteIndex == 0) ? 1 : 0;
        switchToProfile(_profiles[newActive].id);
    }
    
    // TODO: Delete profile directory and files
    // For now, just remove from memory
    
    // Shift remaining profiles down
    for (int i = deleteIndex; i < _profileCount - 1; i++) {
        memcpy(&_profiles[i], &_profiles[i + 1], sizeof(Profile));
    }
    _profileCount--;
    
    // Adjust active index if needed
    if (_activeProfileIndex > deleteIndex) {
        _activeProfileIndex--;
    }
    
    saveProfileList();
    return true;
}

bool ProfileManager::switchToProfile(const char* id) {
    // Find profile
    int newIndex = -1;
    for (int i = 0; i < _profileCount; i++) {
        if (strcmp(_profiles[i].id, id) == 0) {
            newIndex = i;
            break;
        }
    }
    
    if (newIndex < 0) {
        return false;  // Profile not found
    }
    
    if (newIndex == _activeProfileIndex) {
        return true;  // Already active
    }
    
    // Save current profile data
    if (_activeProfileIndex >= 0) {
        saveActiveProfile();
    }
    
    // Switch
    _activeProfileIndex = newIndex;
    _profiles[_activeProfileIndex].lastUsedAt = time(nullptr);
    
    // Save profile list (to persist lastUsedAt and active ID)
    saveProfileList();
    
    // Notify callback
    if (_switchCallback) {
        _switchCallback(_profiles[_activeProfileIndex]);
    }
    
    return true;
}

bool ProfileManager::setActiveProfile(const char* id) {
    // Alias for switchToProfile
    return switchToProfile(id);
}

bool ProfileManager::saveActiveProfile() {
    if (_activeProfileIndex < 0 || _activeProfileIndex >= _profileCount) {
        return false;
    }
    return saveProfile(_profiles[_activeProfileIndex]);
}

void ProfileManager::setProfileSwitchCallback(ProfileSwitchCallback callback) {
    _switchCallback = callback;
}

void ProfileManager::generateNodeName(char* dest, size_t maxLen) {
#ifdef ESP_PLATFORM
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(dest, maxLen, "Meshola-%02X%02X", mac[4], mac[5]);
#else
    // Fallback for non-ESP32 builds
    snprintf(dest, maxLen, "Meshola-%04X", (unsigned)(rand() & 0xFFFF));
#endif
}

bool ProfileManager::generateKeys(Profile& profile) {
    // TODO: Use proper crypto library (Crypto from MeshCore dependencies)
    // For now, generate random bytes as placeholder
    
#ifdef ESP_PLATFORM
    // Use ESP32 hardware RNG
    for (int i = 0; i < PUBLIC_KEY_SIZE; i++) {
        profile.privateKey[i] = esp_random() & 0xFF;
        profile.publicKey[i] = esp_random() & 0xFF;  // Placeholder - real impl derives from private
    }
#else
    // Fallback for testing
    for (int i = 0; i < PUBLIC_KEY_SIZE; i++) {
        profile.privateKey[i] = rand() & 0xFF;
        profile.publicKey[i] = rand() & 0xFF;
    }
#endif
    
    profile.hasKeys = true;
    return true;
}

void ProfileManager::getProfileDataPath(const char* profileId, char* dest, size_t maxLen) {
    snprintf(dest, maxLen, "%s/profiles/%s", STORAGE_BASE, profileId);
}

void ProfileManager::generateProfileId(char* dest, size_t maxLen) {
    // Generate 8-character hex ID
#ifdef ESP_PLATFORM
    uint32_t rand1 = esp_random();
    snprintf(dest, maxLen, "%08lx", (unsigned long)rand1);
#else
    snprintf(dest, maxLen, "%08x", (unsigned)rand());
#endif
}

bool ProfileManager::createProfileDirectory(const char* id) {
    char path[128];
    
    // Create base directory
    snprintf(path, sizeof(path), "%s", STORAGE_BASE);
    // TODO: mkdir(path, 0755);
    
    // Create profiles directory
    snprintf(path, sizeof(path), "%s/profiles", STORAGE_BASE);
    // TODO: mkdir(path, 0755);
    
    // Create this profile's directory
    snprintf(path, sizeof(path), "%s/profiles/%s", STORAGE_BASE, id);
    // TODO: mkdir(path, 0755);
    
    return true;
}

// ============================================================================
// Storage - JSON serialization
// ============================================================================

// Simple JSON writing helpers (no external library dependency)
static void writeJsonString(FILE* f, const char* key, const char* value, bool comma = true) {
    fprintf(f, "  \"%s\": \"%s\"%s\n", key, value, comma ? "," : "");
}

static void writeJsonInt(FILE* f, const char* key, int value, bool comma = true) {
    fprintf(f, "  \"%s\": %d%s\n", key, value, comma ? "," : "");
}

static void writeJsonUint(FILE* f, const char* key, uint32_t value, bool comma = true) {
    fprintf(f, "  \"%s\": %lu%s\n", key, (unsigned long)value, comma ? "," : "");
}

static void writeJsonFloat(FILE* f, const char* key, float value, bool comma = true) {
    fprintf(f, "  \"%s\": %.3f%s\n", key, value, comma ? "," : "");
}

static void writeJsonBool(FILE* f, const char* key, bool value, bool comma = true) {
    fprintf(f, "  \"%s\": %s%s\n", key, value ? "true" : "false", comma ? "," : "");
}

static void writeJsonHex(FILE* f, const char* key, const uint8_t* data, size_t len, bool comma = true) {
    fprintf(f, "  \"%s\": \"", key);
    for (size_t i = 0; i < len; i++) {
        fprintf(f, "%02x", data[i]);
    }
    fprintf(f, "\"%s\n", comma ? "," : "");
}

bool ProfileManager::saveProfile(const Profile& profile) {
    char path[128];
    snprintf(path, sizeof(path), "%s/profiles/%s/config.json", STORAGE_BASE, profile.id);
    
    FILE* f = fopen(path, "w");
    if (!f) {
        // Directory might not exist yet in development
        return false;
    }
    
    fprintf(f, "{\n");
    writeJsonString(f, "id", profile.id);
    writeJsonString(f, "name", profile.name);
    writeJsonUint(f, "createdAt", profile.createdAt);
    writeJsonUint(f, "lastUsedAt", profile.lastUsedAt);
    writeJsonString(f, "protocolId", profile.protocolId);
    
    // Radio config
    writeJsonFloat(f, "frequency", profile.radio.frequency);
    writeJsonFloat(f, "bandwidth", profile.radio.bandwidth);
    writeJsonInt(f, "spreadingFactor", profile.radio.spreadingFactor);
    writeJsonInt(f, "codingRate", profile.radio.codingRate);
    writeJsonInt(f, "txPower", profile.radio.txPower);
    
    // Identity
    writeJsonString(f, "nodeName", profile.nodeName);
    writeJsonBool(f, "hasKeys", profile.hasKeys);
    if (profile.hasKeys) {
        writeJsonHex(f, "publicKey", profile.publicKey, PUBLIC_KEY_SIZE);
        writeJsonHex(f, "privateKey", profile.privateKey, PUBLIC_KEY_SIZE);
    }
    
    // Protocol settings
    fprintf(f, "  \"protocolSettings\": {\n");
    for (int i = 0; i < profile.protocolSettingCount; i++) {
        fprintf(f, "    \"%s\": \"%s\"%s\n", 
                profile.protocolSettings[i].key,
                profile.protocolSettings[i].value,
                (i < profile.protocolSettingCount - 1) ? "," : "");
    }
    fprintf(f, "  }\n");
    
    fprintf(f, "}\n");
    fclose(f);
    
    return true;
}

bool ProfileManager::saveProfileList() {
    char path[128];
    snprintf(path, sizeof(path), "%s/profiles.json", STORAGE_BASE);
    
    FILE* f = fopen(path, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "{\n");
    
    // Active profile ID
    if (_activeProfileIndex >= 0 && _activeProfileIndex < _profileCount) {
        writeJsonString(f, "activeProfileId", _profiles[_activeProfileIndex].id);
    }
    
    // Profile list (just IDs and names for quick loading)
    fprintf(f, "  \"profiles\": [\n");
    for (int i = 0; i < _profileCount; i++) {
        fprintf(f, "    {\"id\": \"%s\", \"name\": \"%s\"}%s\n",
                _profiles[i].id,
                _profiles[i].name,
                (i < _profileCount - 1) ? "," : "");
    }
    fprintf(f, "  ]\n");
    
    fprintf(f, "}\n");
    fclose(f);
    
    return true;
}

bool ProfileManager::loadProfileList() {
    char path[128];
    snprintf(path, sizeof(path), "%s/profiles.json", STORAGE_BASE);
    
    FILE* f = fopen(path, "r");
    if (!f) {
        return false;  // No profiles file exists yet
    }
    
    // TODO: Proper JSON parsing
    // For now, this is a stub - in real implementation, parse the JSON
    // and load each profile by ID
    
    fclose(f);
    
    // Placeholder: return false to trigger default profile creation
    return false;
}

bool ProfileManager::loadProfile(Profile& profile, const char* id) {
    char path[128];
    snprintf(path, sizeof(path), "%s/profiles/%s/config.json", STORAGE_BASE, id);
    
    FILE* f = fopen(path, "r");
    if (!f) {
        return false;
    }
    
    // TODO: Proper JSON parsing
    // For now, this is a stub
    
    fclose(f);
    return false;
}

} // namespace meshola
