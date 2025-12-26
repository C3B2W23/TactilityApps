# Meshola Ecosystem - Integration Guide

**Version:** 1.0.0  
**Last Updated:** December 26, 2024

---

## Overview

The Meshola ecosystem is designed to support multiple independent apps that can interoperate. This document defines the integration architecture and contracts for building additional Meshola apps.

---

## Architecture Principles

### 1. Messenger as Mesh Gateway

**Meshola Messenger is the single point of contact with the mesh network.**

All other Meshola apps communicate with Messenger, not directly with the underlying MesholaMsgService. This provides:

- **Single source of truth** for mesh state
- **Centralized profile management** - Other apps don't handle profiles
- **Simplified development** - Apps just request data
- **Consistent user experience** - One app manages mesh settings

### 2. Independent Apps

Each Meshola app is:

- **Standalone installable** - No forced dependencies (though Messenger is recommended)
- **Independently versioned** - Can update separately
- **Self-contained storage** - Own data directory
- **Own namespace** - No code conflicts

### 3. Shared Data Contract

Apps communicate via well-defined data contracts, not code dependencies.

---

## Namespace Conventions

| App/Component | Namespace | App ID |
|---------------|-----------|--------|
| Meshola Messenger | `meshola::messenger` | `com.meshola.messenger` |
| MesholaMsgService | `meshola::service` | N/A (service, not app) |
| Meshola Maps | `meshola::maps` | `com.meshola.maps` |
| Future App X | `meshola::x` | `com.meshola.x` |

**Shared types** (Contact, Message, etc.) are in `meshola::` namespace and defined in Messenger's headers. Other apps can copy these definitions or include them if building together.

---

## Storage Layout

```
/data/meshola/
│
├── messenger/                    # Meshola Messenger (PRIVATE)
│   ├── profiles/                 # User profiles
│   │   └── {profileId}/
│   │       ├── config.json       # Profile settings
│   │       └── messages/         # Chat history
│   │           ├── dm_*.jsonl    # Direct messages
│   │           └── ch_*.jsonl    # Channel messages
│   └── state.json                # Runtime state
│
├── shared/                       # Cross-app data (MESSENGER WRITES)
│   ├── contacts.json             # Contact list with locations
│   ├── locations.json            # Real-time location cache
│   ├── node_info.json            # Current node identity
│   └── requests/                 # Request queue from other apps
│       └── *.json                # Pending requests
│
├── maps/                         # Meshola Maps (PRIVATE)
│   ├── tracks/                   # Saved routes
│   ├── waypoints/                # Saved locations
│   └── settings.json             # Maps preferences
│
└── {future_app}/                 # Future apps follow same pattern
```

### Ownership Rules

| Path | Owner | Other Apps |
|------|-------|------------|
| `/data/meshola/messenger/` | Messenger | NO ACCESS |
| `/data/meshola/shared/` | Messenger (writes) | READ ONLY |
| `/data/meshola/shared/requests/` | Other apps (write) | Messenger reads & deletes |
| `/data/meshola/maps/` | Maps | NO ACCESS from others |

---

## Shared Data Formats

### contacts.json

Written by Messenger when contacts change.

```json
{
  "version": 1,
  "updated": 1703520000,
  "profile": "default",
  "contacts": [
    {
      "publicKey": "a1b2c3d4...",
      "name": "Node-A3F7",
      "lastSeen": 1703519900,
      "isOnline": true,
      "hasLocation": true,
      "latitude": 33.8818,
      "longitude": -112.7298,
      "locationAge": 120,
      "rssi": -85,
      "snr": 8,
      "pathLength": 1
    }
  ]
}
```

### locations.json

Lightweight location-only data, updated more frequently.

```json
{
  "version": 1,
  "updated": 1703520000,
  "locations": [
    {
      "publicKey": "a1b2c3d4...",
      "name": "Node-A3F7",
      "latitude": 33.8818,
      "longitude": -112.7298,
      "altitude": 450.5,
      "speed": 0.0,
      "heading": 0,
      "timestamp": 1703519900,
      "age": 120
    }
  ]
}
```

### node_info.json

Current node identity (for display, not security-sensitive).

```json
{
  "version": 1,
  "nodeName": "Meshola-A3F7",
  "publicKeyPrefix": "a1b2c3d4",
  "profileName": "Default",
  "isOnline": true,
  "hasLocation": true,
  "latitude": 33.9234,
  "longitude": -112.6543
}
```

### Request Format

Other apps write request files to `/data/meshola/shared/requests/`.

**Filename:** `{app}_{timestamp}_{action}.json`

**Example:** `maps_1703520000_send_waypoint.json`

```json
{
  "version": 1,
  "app": "maps",
  "action": "send_waypoint",
  "timestamp": 1703520000,
  "data": {
    "recipientKey": "a1b2c3d4...",
    "name": "Meeting Point",
    "latitude": 33.8818,
    "longitude": -112.7298,
    "message": "Meet here at 3pm"
  }
}
```

**Supported Actions:**

| Action | Description | Data Fields |
|--------|-------------|-------------|
| `send_waypoint` | Send location to contact | recipientKey, name, lat, lon, message |
| `send_location` | Broadcast current location | (none - uses device GPS) |
| `request_refresh` | Ask Messenger to update shared data | (none) |

Messenger processes requests, performs the action, then deletes the request file.

---

## Integration Checklist for New Apps

When building a new Meshola app:

### 1. Namespace
- [ ] Use `meshola::{appname}::` namespace
- [ ] App ID: `com.meshola.{appname}`

### 2. Storage
- [ ] Create `/data/meshola/{appname}/` for private data
- [ ] NEVER write to `/data/meshola/messenger/`
- [ ] Read from `/data/meshola/shared/` for mesh data
- [ ] Write requests to `/data/meshola/shared/requests/`

### 3. User Experience
- [ ] Handle "Messenger not installed" gracefully
- [ ] Show "No mesh data" if shared files missing
- [ ] Don't require Messenger to be running (read cached data)

### 4. Data Freshness
- [ ] Check `updated` timestamp in shared files
- [ ] Show data age to user if stale
- [ ] Allow manual refresh (write `request_refresh` action)

---

## Implementation Timeline

### Phase 1: Current (Messenger Only)
- MesholaMsgService runs in background
- All mesh operations in Messenger
- No shared data export yet

### Phase 2: Shared Data Export
- Messenger writes to `/data/meshola/shared/`
- Export on contact updates
- Export on location advertisements

### Phase 3: Request Handling  
- Messenger watches requests directory
- Process and execute requests
- Delete processed requests

### Phase 4: Maps Integration
- Maps reads shared data
- Maps writes waypoint requests
- Full bidirectional integration

---

## Security Considerations

### Private Keys
- **NEVER** exposed in shared data
- Only public key prefixes shared for identification
- Full public keys only when needed for addressing

### Profile Data
- Profile details stay in Messenger
- Shared data only includes node name and online status
- Other apps cannot switch profiles or access profile settings

### Request Validation
- Messenger validates all incoming requests
- Malformed requests logged and deleted
- Rate limiting on request processing

---

## Testing Integration

### Mock Data
For developing Maps before shared export exists:

```bash
# Create mock shared data for testing
mkdir -p /data/meshola/shared
cat > /data/meshola/shared/contacts.json << 'EOF'
{
  "version": 1,
  "updated": 1703520000,
  "profile": "test",
  "contacts": [
    {
      "publicKey": "test1234567890abcdef",
      "name": "TestNode-1",
      "lastSeen": 1703519900,
      "isOnline": true,
      "hasLocation": true,
      "latitude": 33.8818,
      "longitude": -112.7298
    }
  ]
}
EOF
```

### Integration Tests
- Verify shared file format parsing
- Test request file creation
- Test handling of missing/corrupt shared data
- Test stale data detection

---

## Versioning

All shared data includes a `version` field. Apps should:

1. Check version before parsing
2. Handle unknown versions gracefully (warn, use what's understood)
3. Never fail hard on version mismatch

Version increments:
- **Patch (1.0.x):** New optional fields
- **Minor (1.x.0):** New required fields, new actions
- **Major (x.0.0):** Breaking format changes

---

*This document is the contract for Meshola ecosystem integration. Changes require coordination across all apps.*
