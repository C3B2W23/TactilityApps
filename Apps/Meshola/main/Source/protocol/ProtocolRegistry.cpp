#include "IProtocol.h"
#include <cstring>

namespace meshola {

// Static storage for protocol registry
ProtocolEntry ProtocolRegistry::protocols[MAX_PROTOCOLS] = {};
int ProtocolRegistry::protocolCount = 0;

bool ProtocolRegistry::registerProtocol(const ProtocolEntry& entry) {
    if (protocolCount >= MAX_PROTOCOLS) {
        return false;
    }
    
    // Check for duplicate ID
    for (int i = 0; i < protocolCount; i++) {
        if (strcmp(protocols[i].id, entry.id) == 0) {
            return false;  // Already registered
        }
    }
    
    protocols[protocolCount++] = entry;
    return true;
}

int ProtocolRegistry::getProtocolCount() {
    return protocolCount;
}

const ProtocolEntry* ProtocolRegistry::getProtocol(int index) {
    if (index < 0 || index >= protocolCount) {
        return nullptr;
    }
    return &protocols[index];
}

const ProtocolEntry* ProtocolRegistry::findProtocol(const char* id) {
    for (int i = 0; i < protocolCount; i++) {
        if (strcmp(protocols[i].id, id) == 0) {
            return &protocols[i];
        }
    }
    return nullptr;
}

IProtocol* ProtocolRegistry::createProtocol(const char* id) {
    const ProtocolEntry* entry = findProtocol(id);
    if (entry && entry->create) {
        return entry->create();
    }
    return nullptr;
}

} // namespace meshola
