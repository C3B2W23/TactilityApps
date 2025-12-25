#pragma once

#include <TactilityCpp/App.h>
#include "protocol/IProtocol.h"

namespace meshola {

// Forward declarations
class ChatView;
class ContactsView;
class ChannelsView;
class SettingsView;

/**
 * Meshola - Multi-protocol mesh messaging for Tactility
 * 
 * A protocol-agnostic mesh messaging client supporting:
 * - MeshCore (standard and forks)
 * - Meshtastic (future)
 * - Custom protocols
 */
class MesholaApp final : public App {
public:
    MesholaApp();
    ~MesholaApp() override;

    // App lifecycle
    void onShow(AppHandle handle, lv_obj_t* parent) override;
    void onHide(AppHandle handle) override;

private:
    // Navigation
    enum class ViewType {
        Chat,
        Contacts,
        Channels,
        Settings
    };
    
    void showView(ViewType view);
    void createNavBar(lv_obj_t* parent);
    void updateNavButtonStates();
    
    // Event handlers
    static void onNavChatPressed(lv_event_t* event);
    static void onNavContactsPressed(lv_event_t* event);
    static void onNavChannelsPressed(lv_event_t* event);
    static void onNavSettingsPressed(lv_event_t* event);
    
    // Mesh event handlers
    void onMessageReceived(const Message& msg);
    void onContactUpdated(const Contact& contact, bool isNew);
    void onAckReceived(uint32_t ackId, bool success);
    
    // Placeholder view creators (temporary until full views)
    void createChatViewPlaceholder();
    void createContactsViewPlaceholder();
    void createChannelsViewPlaceholder();
    void createSettingsViewPlaceholder();
    
    // UI state
    AppHandle _handle;
    lv_obj_t* _parent;
    lv_obj_t* _contentContainer;
    lv_obj_t* _navBar;
    
    // Navigation buttons
    lv_obj_t* _btnChat;
    lv_obj_t* _btnContacts;
    lv_obj_t* _btnChannels;
    lv_obj_t* _btnSettings;
    
    ViewType _currentView;
};

} // namespace meshola
