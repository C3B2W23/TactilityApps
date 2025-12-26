#pragma once

#include <Tactility/App.h>
#include <Tactility/PubSub.h>

#include "service/MesholaMsgService.h"
#include "views/ChatView.h"
#include "views/ContactsView.h"

namespace meshola {

// Forward declarations
class ChatView;
class ContactsView;
class ChannelsView;
class SettingsView;

/**
 * Meshola Messenger - Multi-protocol mesh messaging for Tactility
 * 
 * This is a Tactility APP - it provides the user interface.
 * The actual mesh operations happen in MesholaMsgService (a Tactility SERVICE).
 * 
 * The app:
 * - Subscribes to MesholaMsgService PubSub on show
 * - Unsubscribes on hide
 * - Fetches current state from MesholaMsgService
 * - Calls MesholaMsgService methods to send messages
 * 
 * Supports:
 * - MeshCore (standard and forks)
 * - Meshtastic (future)
 * - Custom protocols
 */
class MesholaApp final : public tt::App {
public:
    MesholaApp();
    ~MesholaApp() override;

    // App lifecycle
    void onShow(tt::AppHandle handle, lv_obj_t* parent) override;
    void onHide(tt::AppHandle handle) override;

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
    
    // Navigation event handlers
    static void onNavChatPressed(lv_event_t* event);
    static void onNavContactsPressed(lv_event_t* event);
    static void onNavChannelsPressed(lv_event_t* event);
    static void onNavSettingsPressed(lv_event_t* event);
    
    // PubSub event handlers
    void onMessageEvent(const service::MessageEvent& event);
    void onContactEvent(const service::ContactEvent& event);
    void onAckEvent(const service::AckEvent& event);
    void onStatusEvent(const service::StatusEvent& event);
    
    // Contact selection handler (from ContactsView)
    void onContactSelected(const Contact& contact);
    
    // Send message handler (from ChatView)
    static void onSendMessage(const char* text, void* userData);
    
    // Placeholder view creators (temporary until full views)
    void createChannelsViewPlaceholder();
    void createSettingsViewPlaceholder();
    
    // Refresh views with data from service
    void refreshContactList();
    void refreshChatHistory();
    
    // UI state
    tt::AppHandle _handle;
    lv_obj_t* _parent = nullptr;
    lv_obj_t* _contentContainer = nullptr;
    lv_obj_t* _navBar = nullptr;
    
    // Navigation buttons
    lv_obj_t* _btnChat = nullptr;
    lv_obj_t* _btnContacts = nullptr;
    lv_obj_t* _btnChannels = nullptr;
    lv_obj_t* _btnSettings = nullptr;
    
    ViewType _currentView = ViewType::Chat;
    
    // Views
    ChatView _chatView;
    ContactsView _contactsView;
    
    // MesholaMsgService connection
    std::shared_ptr<service::MesholaMsgService> _mesholaMsgService;
    
    // PubSub subscription IDs (for unsubscribing on hide)
    tt::PubSub<service::MessageEvent>::SubscriptionId _messageSubId = 0;
    tt::PubSub<service::ContactEvent>::SubscriptionId _contactSubId = 0;
    tt::PubSub<service::AckEvent>::SubscriptionId _ackSubId = 0;
    tt::PubSub<service::StatusEvent>::SubscriptionId _statusSubId = 0;
    
    // Currently selected contact for chat
    Contact _activeContact;
    bool _hasActiveContact = false;
};

} // namespace meshola
