# Meshola - User Guide

**Version:** 0.1.0  
**Last Updated:** December 25, 2024

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Main Interface](#main-interface)
4. [Profiles](#profiles)
5. [Chatting](#chatting)
6. [Peers](#peers)
7. [Channels](#channels)
8. [Settings](#settings)
9. [Troubleshooting](#troubleshooting)

---

## Introduction

### What is Meshola?

Meshola is a mesh networking chat application for the LilyGo T-Deck. It allows you to send and receive messages over LoRa radio without needing cell towers, internet, or any infrastructure.

### What is Mesh Networking?

In a mesh network, every device can relay messages for other devices. This means:
- **No infrastructure required** - Works anywhere
- **Extended range** - Messages hop through other nodes
- **Resilient** - Network adapts if nodes go offline
- **Private** - End-to-end encrypted messages

### Supported Protocols

Meshola currently supports:
- **MeshCore** - Primary protocol for off-grid mesh messaging

Future support planned for:
- Custom Protocol (community fork)
- Meshtastic

---

## Getting Started

### First Launch

When you first open Meshola:

1. A **default profile** is automatically created
2. Your node is assigned a unique name like "Meshola-A3F7"
3. A unique keypair is generated for your identity
4. You'll see the welcome screen

### Your Node Name

Your node name is displayed to other users when they see your messages or discover you on the network. The default format is:

```
Meshola-XXXX
```

Where XXXX is derived from your device's unique hardware ID, ensuring no two devices have the same default name.

You can customize your node name in Settings.

---

## Main Interface

### Navigation Bar

At the bottom of the screen, you'll find four tabs:

| Icon | Tab | Purpose |
|------|-----|---------|
| ✉️ | **Chat** | Send and receive messages |
| 📋 | **Peers** | View discovered nodes |
| 📞 | **Channels** | Join group channels |
| ⚙️ | **Settings** | Configure profiles and radio |

Tap any tab to switch views. The active tab is highlighted.

### Toolbar

At the top of the screen is the Tactility toolbar showing:
- App name (Meshola)
- Back button (to exit app)

---

## Profiles

### What is a Profile?

A profile is a complete configuration for connecting to a mesh network. Each profile has:

- **Unique identity** (public/private keypair)
- **Protocol selection** (MeshCore, etc.)
- **Radio settings** (frequency, bandwidth, etc.)
- **Separate chat history** (messages don't mix between profiles)
- **Separate contacts** (peers discovered on that network)

### Why Use Multiple Profiles?

- **Different networks** - Home vs. travel locations
- **Different protocols** - MeshCore vs. Meshtastic
- **Different identities** - Personal vs. group use
- **Testing** - Experiment without affecting main profile

### Creating a Profile

1. Go to **Settings** tab
2. Tap **+ New** button
3. Enter a name (e.g., "Home", "Travel", "Testing")
4. Configure protocol and radio settings
5. Save

### Switching Profiles

1. Go to **Settings** tab
2. Use the **Profile** dropdown
3. Select the desired profile
4. The app will reinitialize with the new settings

**Note:** Switching profiles clears the current chat view. Your messages are saved and will reappear when you switch back.

### Editing a Profile

1. Go to **Settings** tab
2. Ensure the profile you want to edit is selected
3. Tap **Edit** button
4. Modify settings
5. Save

---

## Chatting

### Chat View

The Chat tab shows your message conversations.

**When no conversation is selected:**
- Welcome screen with your node name
- Instructions to select a peer or channel

**When a conversation is active:**
- Header showing contact/channel name and status
- Message history (scrollable)
- Input field at bottom
- Send button

### Message Bubbles

Messages are displayed as bubbles:
- **Right side (blue)** - Messages you sent
- **Left side (gray)** - Messages you received

Each message shows:
- Message text
- Timestamp (HH:MM)
- Status icons (for sent messages):
  - 🔄 Pending - Queued to send
  - ✓ Sent - Transmitted
  - ✓✓ Delivered - Acknowledged by recipient
  - ✗ Failed - Send failed
- Signal strength (for received messages)

### Sending a Message

1. Select a contact from Peers tab, or a channel from Channels tab
2. Type your message in the input field
3. Tap **Send** or press Enter

### Message Persistence

All messages are saved automatically. You will never lose messages due to:
- Closing the app
- Switching profiles
- Restarting the device
- Power loss

Messages are saved immediately when sent or received.

---

## Peers

### Discovering Peers

Other mesh nodes are discovered automatically when they:
- Send an advertisement (beacon)
- Send or relay messages

### Peer List

The Peers tab shows all discovered nodes with:
- Node name
- Signal strength (dBm)
- Online/offline status
- Last seen time

### Starting a Chat

1. Go to **Peers** tab
2. Tap on a peer's name
3. You'll be taken to Chat view with that peer selected

### Broadcasting Advertisement

To announce your presence to nearby nodes:
1. Go to **Peers** tab
2. Tap **Broadcast Advertisement** button

This sends a beacon that other nodes will receive, allowing them to discover you.

---

## Channels

### What is a Channel?

A channel is a group chat where multiple nodes can participate. Channels have:
- A name
- A shared encryption key
- Public or private visibility

### Channel Types

- **Public channels** - Anyone with the channel name can join
- **Private channels** - Requires the encryption key to join

### Joining a Channel

*(Feature in development)*

### Creating a Channel

*(Feature in development)*

---

## Settings

### Profile Section

- **Profile dropdown** - Switch between profiles
- **+ New** - Create a new profile
- **Edit** - Modify current profile

### Radio Configuration

View current radio settings:
- **Frequency** - Operating frequency in MHz
- **Bandwidth** - Channel width in kHz
- **Spreading Factor** - SF7-SF12 (higher = more range, slower)
- **Coding Rate** - Error correction level
- **TX Power** - Transmit power in dBm

### Node Information

- **Name** - Your node's display name
- **Key** - First bytes of your public key (for verification)
- **Radio Status** - Running or Stopped

---

## Troubleshooting

### No Peers Discovered

**Possible causes:**
1. No other nodes in range
2. Different frequency/bandwidth settings
3. Radio not started

**Solutions:**
1. Check that other nodes are powered on and transmitting
2. Verify radio settings match the network
3. Try broadcasting an advertisement
4. Move to a location with better line-of-sight

### Messages Not Sending

**Possible causes:**
1. No path to recipient
2. Recipient offline
3. Radio configuration mismatch

**Solutions:**
1. Check if recipient appears in Peers list
2. Wait for recipient to come online
3. Verify both nodes use same frequency/bandwidth

### Profile Won't Switch

**Possible causes:**
1. Radio busy with transmission

**Solutions:**
1. Wait for current operation to complete
2. Restart the app

### App Crashes

If Meshola crashes:
1. Your messages are safe (persisted immediately)
2. Restart the app
3. If problem persists, try switching profiles
4. Report the issue with steps to reproduce

---

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Enter | Send message (when input focused) |
| Trackball Click | Select/activate |
| Trackball Scroll | Navigate lists |

---

## Technical Specifications

### Radio Parameters (Default)

| Parameter | Value |
|-----------|-------|
| Frequency | 906.875 MHz |
| Bandwidth | 250 kHz |
| Spreading Factor | 11 |
| Coding Rate | 4/5 |
| TX Power | 22 dBm |

### Storage Location

```
/data/meshola/
├── profiles.json
└── profiles/{id}/
    ├── config.json
    └── messages/*.jsonl
```

### Message Limits

- Maximum message length: 256 characters
- Node name length: 32 characters
- Channel name length: 32 characters

---

## Getting Help

- **Documentation:** This guide
- **Community:** Meshola Community
- **Issues:** GitHub repository

---

*This guide is updated with each release.*
