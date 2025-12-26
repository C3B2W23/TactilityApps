#include "MesholaApp.h"
#include "mesh/MeshService.h"
#include "profile/Profile.h"
#include "storage/MessageStore.h"
#include "views/ChatView.h"

#include <tt_lvgl_toolbar.h>
#include <tt_lvgl.h>
#include <lvgl.h>
#include <cstring>
#include <cstdio>
#include <ctime>

namespace meshola {

// Color scheme
static const uint32_t COLOR_BG_DARK = 0x1a1a1a;
static const uint32_t COLOR_BG_CARD = 0x2d2d2d;
static const uint32_t COLOR_ACCENT = 0x0066cc;
static const uint32_t COLOR_ACCENT_DIM = 0x333333;
static const uint32_t COLOR_TEXT = 0xffffff;
static const uint32_t COLOR_TEXT_DIM = 0x888888;
static const uint32_t COLOR_SUCCESS = 0x00aa55;
static const uint32_t COLOR_WARNING = 0xffaa00;

MesholaApp::MesholaApp()
    : _handle(nullptr)
    , _parent(nullptr)
    , _contentContainer(nullptr)
    , _navBar(nullptr)
    , _btnChat(nullptr)
    , _btnContacts(nullptr)
    , _btnChannels(nullptr)
    , _btnSettings(nullptr)
    , _currentView(ViewType::Chat)
{
}

MesholaApp::~MesholaApp() {
    // Views are children of LVGL objects, cleaned up automatically
}

void MesholaApp::onShow(AppHandle handle, lv_obj_t* parent) {
    _handle = handle;
    _parent = parent;
    
    // Set up flex layout for main container
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);
    
    // Create toolbar
    auto* toolbar = tt_lvgl_toolbar_create_for_app(parent, handle);
    
    // Create content container (takes remaining space)
    _contentContainer = lv_obj_create(parent);
    lv_obj_set_width(_contentContainer, LV_PCT(100));
    lv_obj_set_flex_grow(_contentContainer, 1);
    lv_obj_set_style_pad_all(_contentContainer, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_contentContainer, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(_contentContainer, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    
    // Create bottom navigation bar
    createNavBar(parent);
    
    // Initialize profile manager
    auto& profileMgr = ProfileManager::getInstance();
    profileMgr.init();
    
    // Set up profile switch callback
    profileMgr.setProfileSwitchCallback([this](const Profile& newProfile) {
        onProfileSwitch(newProfile);
    });
    
    // Initialize mesh service if not already running
    auto& meshService = MeshService::getInstance();
    if (!meshService.isRunning()) {
        if (meshService.init()) {
            // Set up callbacks
            meshService.setMessageCallback([this](const Message& msg) {
                onMessageReceived(msg);
            });
            meshService.setContactCallback([this](const Contact& contact, bool isNew) {
                onContactUpdated(contact, isNew);
            });
            meshService.setAckCallback([this](uint32_t ackId, bool success) {
                onAckReceived(ackId, success);
            });
            
            meshService.start();
        }
    }
    
    // Show default view
    showView(ViewType::Chat);
}

void MesholaApp::onHide(AppHandle handle) {
    // Clean up views
    _chatView.destroy();
    
    // Note: We don't stop the mesh service here - it continues running
    // in the background so we can receive messages even when app is hidden
    
    _handle = nullptr;
    _parent = nullptr;
    _contentContainer = nullptr;
    _navBar = nullptr;
    _btnChat = nullptr;
    _btnContacts = nullptr;
    _btnChannels = nullptr;
    _btnSettings = nullptr;
}

void MesholaApp::createNavBar(lv_obj_t* parent) {
    _navBar = lv_obj_create(parent);
    lv_obj_set_size(_navBar, LV_PCT(100), 44);
    lv_obj_set_flex_flow(_navBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_navBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(_navBar, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(_navBar, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_navBar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(_navBar, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(_navBar, 0, LV_STATE_DEFAULT);
    
    // Chat button
    _btnChat = lv_btn_create(_navBar);
    lv_obj_set_flex_grow(_btnChat, 1);
    lv_obj_set_height(_btnChat, 36);
    lv_obj_set_style_radius(_btnChat, 6, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(_btnChat, onNavChatPressed, LV_EVENT_CLICKED, this);
    auto* lblChat = lv_label_create(_btnChat);
    lv_label_set_text(lblChat, LV_SYMBOL_ENVELOPE " Chat");
    lv_obj_center(lblChat);
    
    // Contacts button
    _btnContacts = lv_btn_create(_navBar);
    lv_obj_set_flex_grow(_btnContacts, 1);
    lv_obj_set_height(_btnContacts, 36);
    lv_obj_set_style_radius(_btnContacts, 6, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(_btnContacts, onNavContactsPressed, LV_EVENT_CLICKED, this);
    auto* lblContacts = lv_label_create(_btnContacts);
    lv_label_set_text(lblContacts, LV_SYMBOL_LIST " Peers");
    lv_obj_center(lblContacts);
    
    // Channels button
    _btnChannels = lv_btn_create(_navBar);
    lv_obj_set_flex_grow(_btnChannels, 1);
    lv_obj_set_height(_btnChannels, 36);
    lv_obj_set_style_radius(_btnChannels, 6, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(_btnChannels, onNavChannelsPressed, LV_EVENT_CLICKED, this);
    auto* lblChannels = lv_label_create(_btnChannels);
    lv_label_set_text(lblChannels, LV_SYMBOL_CALL " Ch");
    lv_obj_center(lblChannels);
    
    // Settings button
    _btnSettings = lv_btn_create(_navBar);
    lv_obj_set_flex_grow(_btnSettings, 1);
    lv_obj_set_height(_btnSettings, 36);
    lv_obj_set_style_radius(_btnSettings, 6, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(_btnSettings, onNavSettingsPressed, LV_EVENT_CLICKED, this);
    auto* lblSettings = lv_label_create(_btnSettings);
    lv_label_set_text(lblSettings, LV_SYMBOL_SETTINGS);
    lv_obj_center(lblSettings);
}

void MesholaApp::updateNavButtonStates() {
    auto setButtonState = [](lv_obj_t* btn, bool active) {
        lv_obj_set_style_bg_color(btn, 
            lv_color_hex(active ? COLOR_ACCENT : COLOR_ACCENT_DIM), 
            LV_STATE_DEFAULT);
    };
    
    setButtonState(_btnChat, _currentView == ViewType::Chat);
    setButtonState(_btnContacts, _currentView == ViewType::Contacts);
    setButtonState(_btnChannels, _currentView == ViewType::Channels);
    setButtonState(_btnSettings, _currentView == ViewType::Settings);
}

void MesholaApp::showView(ViewType view) {
    if (!_contentContainer) {
        return;
    }
    
    // Clean up previous view
    if (_currentView == ViewType::Chat) {
        _chatView.destroy();
    }
    
    // Clear content container
    lv_obj_clean(_contentContainer);
    
    _currentView = view;
    updateNavButtonStates();
    
    // Create view content
    switch (view) {
        case ViewType::Chat:
            _chatView.create(_contentContainer);
            _chatView.setSendCallback(onSendMessage, this);
            break;
        case ViewType::Contacts:
            createContactsViewPlaceholder();
            break;
        case ViewType::Channels:
            createChannelsViewPlaceholder();
            break;
        case ViewType::Settings:
            createSettingsViewPlaceholder();
            break;
    }
}

// ============================================================================
// Send message handler
// ============================================================================

void MesholaApp::onSendMessage(const char* text, void* userData) {
    auto* app = static_cast<MesholaApp*>(userData);
    if (!app) return;
    
    auto& meshService = MeshService::getInstance();
    auto& msgStore = MessageStore::getInstance();
    
    const Contact* contact = app->_chatView.getActiveContact();
    const Channel* channel = app->_chatView.getActiveChannel();
    
    if (contact) {
        // Send direct message
        uint32_t ackId = 0;
        if (meshService.sendMessage(*contact, text, ackId)) {
            // Create outgoing message for display and storage
            Message outMsg = {};
            memcpy(outMsg.senderKey, contact->publicKey, PUBLIC_KEY_SIZE);  // Store recipient key
            strncpy(outMsg.text, text, MAX_MESSAGE_LEN - 1);
            outMsg.isOutgoing = true;
            outMsg.isChannel = false;
            outMsg.status = MessageStatus::Pending;
            outMsg.ackId = ackId;
            outMsg.timestamp = (uint32_t)time(nullptr);
            
            // Persist immediately
            msgStore.appendMessage(outMsg);
            
            // Add to UI
            app->_chatView.addMessage(outMsg);
        }
    } else if (channel) {
        // Send channel message
        if (meshService.sendChannelMessage(*channel, text)) {
            // Create outgoing message for display and storage
            Message outMsg = {};
            memcpy(outMsg.channelId, channel->id, CHANNEL_ID_SIZE);
            strncpy(outMsg.text, text, MAX_MESSAGE_LEN - 1);
            strncpy(outMsg.senderName, meshService.getNodeName(), MAX_NODE_NAME_LEN - 1);
            outMsg.isOutgoing = true;
            outMsg.isChannel = true;
            outMsg.status = MessageStatus::Sent;
            outMsg.timestamp = (uint32_t)time(nullptr);
            
            // Persist immediately
            msgStore.appendMessage(outMsg);
            
            // Add to UI
            app->_chatView.addMessage(outMsg);
        }
    }
}

// ============================================================================
// Placeholder Views (to be replaced with full view classes)
// ============================================================================

void MesholaApp::createContactsViewPlaceholder() {
    auto* container = lv_obj_create(_contentContainer);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(container, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    
    // Header
    auto* header = lv_label_create(container);
    lv_label_set_text(header, "Discovered Peers");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    // Get contacts from mesh service
    auto& meshService = MeshService::getInstance();
    int count = meshService.getContactCount();
    
    if (count == 0) {
        auto* placeholder = lv_label_create(container);
        lv_label_set_text(placeholder, 
            LV_SYMBOL_REFRESH " Scanning for peers...\n\n"
            "Other nodes will appear here\n"
            "as they broadcast advertisements");
        lv_obj_set_style_text_align(placeholder, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(placeholder, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
        lv_obj_set_flex_grow(placeholder, 1);
        lv_obj_set_style_pad_top(placeholder, 40, LV_STATE_DEFAULT);
    } else {
        auto* list = lv_list_create(container);
        lv_obj_set_width(list, LV_PCT(100));
        lv_obj_set_flex_grow(list, 1);
        lv_obj_set_style_bg_color(list, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
        
        for (int i = 0; i < count; i++) {
            Contact contact;
            if (meshService.getContact(i, contact)) {
                char label[64];
                snprintf(label, sizeof(label), "%s  %ddBm", 
                    contact.name, contact.lastRssi);
                auto* btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, label);
                lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
            }
        }
    }
    
    // Advertise button
    auto* advertBtn = lv_btn_create(container);
    lv_obj_set_width(advertBtn, LV_PCT(100));
    lv_obj_set_height(advertBtn, 40);
    lv_obj_set_style_bg_color(advertBtn, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(advertBtn, [](lv_event_t* e) {
        MeshService::getInstance().sendAdvertisement();
    }, LV_EVENT_CLICKED, nullptr);
    
    auto* advertLbl = lv_label_create(advertBtn);
    lv_label_set_text(advertLbl, LV_SYMBOL_UPLOAD " Broadcast Advertisement");
    lv_obj_center(advertLbl);
}

void MesholaApp::createChannelsViewPlaceholder() {
    auto* container = lv_obj_create(_contentContainer);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(container, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    
    // Header
    auto* header = lv_label_create(container);
    lv_label_set_text(header, "Channels");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    // Get channels from mesh service
    auto& meshService = MeshService::getInstance();
    int count = meshService.getChannelCount();
    
    if (count == 0) {
        auto* placeholder = lv_label_create(container);
        lv_label_set_text(placeholder, 
            LV_SYMBOL_PLUS " No channels configured\n\n"
            "Channels allow group messaging\n"
            "with shared encryption keys");
        lv_obj_set_style_text_align(placeholder, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(placeholder, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
        lv_obj_set_flex_grow(placeholder, 1);
        lv_obj_set_style_pad_top(placeholder, 40, LV_STATE_DEFAULT);
    } else {
        auto* list = lv_list_create(container);
        lv_obj_set_width(list, LV_PCT(100));
        lv_obj_set_flex_grow(list, 1);
        lv_obj_set_style_bg_color(list, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
        
        for (int i = 0; i < count; i++) {
            Channel channel;
            if (meshService.getChannel(i, channel)) {
                auto* btn = lv_list_add_btn(list, LV_SYMBOL_CALL, channel.name);
                lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
            }
        }
    }
    
    // Add channel button
    auto* addBtn = lv_btn_create(container);
    lv_obj_set_width(addBtn, LV_PCT(100));
    lv_obj_set_height(addBtn, 40);
    lv_obj_set_style_bg_color(addBtn, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
    
    auto* addLbl = lv_label_create(addBtn);
    lv_label_set_text(addLbl, LV_SYMBOL_PLUS " Add Channel");
    lv_obj_center(addLbl);
}

void MesholaApp::createSettingsViewPlaceholder() {
    auto* container = lv_obj_create(_contentContainer);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(container, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_AUTO);
    
    auto& profileMgr = ProfileManager::getInstance();
    auto& meshService = MeshService::getInstance();
    const Profile* activeProfile = profileMgr.getActiveProfile();
    
    // === Profile Section ===
    auto* profileSection = lv_label_create(container);
    lv_label_set_text(profileSection, "Active Profile");
    lv_obj_set_style_text_font(profileSection, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    auto* profileCard = lv_obj_create(container);
    lv_obj_set_width(profileCard, LV_PCT(100));
    lv_obj_set_height(profileCard, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(profileCard, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(profileCard, 8, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(profileCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(profileCard, 4, LV_STATE_DEFAULT);
    
    if (activeProfile) {
        // Profile dropdown
        auto* profileRow = lv_obj_create(profileCard);
        lv_obj_set_width(profileRow, LV_PCT(100));
        lv_obj_set_height(profileRow, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(profileRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(profileRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(profileRow, LV_OPA_TRANSP, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(profileRow, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(profileRow, 0, LV_STATE_DEFAULT);
        
        auto* profileLabel = lv_label_create(profileRow);
        lv_label_set_text(profileLabel, "Profile:");
        
        auto* profileDropdown = lv_dropdown_create(profileRow);
        lv_obj_set_width(profileDropdown, 140);
        
        // Build dropdown options from profiles
        char dropdownOpts[256] = "";
        int selectedIdx = 0;
        for (int i = 0; i < profileMgr.getProfileCount(); i++) {
            const Profile* p = profileMgr.getProfile(i);
            if (p) {
                if (i > 0) strcat(dropdownOpts, "\n");
                strcat(dropdownOpts, p->name);
                if (strcmp(p->id, activeProfile->id) == 0) {
                    selectedIdx = i;
                }
            }
        }
        lv_dropdown_set_options(profileDropdown, dropdownOpts);
        lv_dropdown_set_selected(profileDropdown, selectedIdx);
        
        // Profile switch handler
        lv_obj_add_event_cb(profileDropdown, [](lv_event_t* e) {
            auto* dropdown = lv_event_get_target(e);
            int sel = lv_dropdown_get_selected(dropdown);
            auto& pm = ProfileManager::getInstance();
            const Profile* p = pm.getProfile(sel);
            if (p) {
                pm.switchToProfile(p->id);
            }
        }, LV_EVENT_VALUE_CHANGED, nullptr);
        
        // Protocol info
        char protoBuf[64];
        snprintf(protoBuf, sizeof(protoBuf), "Protocol: %s", activeProfile->protocolId);
        auto* protoLabel = lv_label_create(profileCard);
        lv_label_set_text(protoLabel, protoBuf);
        lv_obj_set_style_text_color(protoLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    }
    
    // New/Edit profile buttons
    auto* profileBtnRow = lv_obj_create(profileCard);
    lv_obj_set_width(profileBtnRow, LV_PCT(100));
    lv_obj_set_height(profileBtnRow, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(profileBtnRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(profileBtnRow, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(profileBtnRow, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(profileBtnRow, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(profileBtnRow, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(profileBtnRow, 8, LV_STATE_DEFAULT);
    
    auto* newProfileBtn = lv_btn_create(profileBtnRow);
    lv_obj_set_style_bg_color(newProfileBtn, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
    auto* newProfileLbl = lv_label_create(newProfileBtn);
    lv_label_set_text(newProfileLbl, LV_SYMBOL_PLUS " New");
    // TODO: Wire up new profile creation
    
    auto* editProfileBtn = lv_btn_create(profileBtnRow);
    lv_obj_set_style_bg_color(editProfileBtn, lv_color_hex(COLOR_ACCENT_DIM), LV_STATE_DEFAULT);
    auto* editProfileLbl = lv_label_create(editProfileBtn);
    lv_label_set_text(editProfileLbl, LV_SYMBOL_EDIT " Edit");
    // TODO: Wire up profile editing
    
    // === Radio Section ===
    auto* radioSection = lv_label_create(container);
    lv_label_set_text(radioSection, "Radio Configuration");
    lv_obj_set_style_text_font(radioSection, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    auto* radioCard = lv_obj_create(container);
    lv_obj_set_width(radioCard, LV_PCT(100));
    lv_obj_set_height(radioCard, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(radioCard, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(radioCard, 8, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(radioCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(radioCard, 4, LV_STATE_DEFAULT);
    
    if (activeProfile) {
        char buf[64];
        
        snprintf(buf, sizeof(buf), "Frequency: %.3f MHz", activeProfile->radio.frequency);
        auto* freqLabel = lv_label_create(radioCard);
        lv_label_set_text(freqLabel, buf);
        
        snprintf(buf, sizeof(buf), "Bandwidth: %.1f kHz", activeProfile->radio.bandwidth);
        auto* bwLabel = lv_label_create(radioCard);
        lv_label_set_text(bwLabel, buf);
        
        snprintf(buf, sizeof(buf), "SF: %d  CR: 4/%d  TX: %d dBm", 
            activeProfile->radio.spreadingFactor, 
            activeProfile->radio.codingRate, 
            activeProfile->radio.txPower);
        auto* paramLabel = lv_label_create(radioCard);
        lv_label_set_text(paramLabel, buf);
    }
    
    // === Node Section ===
    auto* nodeSection = lv_label_create(container);
    lv_label_set_text(nodeSection, "This Node");
    lv_obj_set_style_text_font(nodeSection, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    auto* nodeCard = lv_obj_create(container);
    lv_obj_set_width(nodeCard, LV_PCT(100));
    lv_obj_set_height(nodeCard, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(nodeCard, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(nodeCard, 8, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(nodeCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(nodeCard, 4, LV_STATE_DEFAULT);
    
    if (activeProfile) {
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "Name: %s", activeProfile->nodeName);
        auto* nodeNameLabel = lv_label_create(nodeCard);
        lv_label_set_text(nodeNameLabel, nameBuf);
        
        // Public key (first 8 bytes as hex)
        char keyBuf[64];
        snprintf(keyBuf, sizeof(keyBuf), "Key: %02x%02x%02x%02x...",
            activeProfile->publicKey[0], activeProfile->publicKey[1],
            activeProfile->publicKey[2], activeProfile->publicKey[3]);
        auto* keyLabel = lv_label_create(nodeCard);
        lv_label_set_text(keyLabel, keyBuf);
        lv_obj_set_style_text_color(keyLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    }
    
    auto status = meshService.getStatus();
    char statusBuf[64];
    snprintf(statusBuf, sizeof(statusBuf), "Radio: %s", 
        status.radioRunning ? "Running" : "Stopped");
    auto* statusLabel = lv_label_create(nodeCard);
    lv_label_set_text(statusLabel, statusBuf);
    lv_obj_set_style_text_color(statusLabel, 
        lv_color_hex(status.radioRunning ? COLOR_SUCCESS : COLOR_WARNING), 
        LV_STATE_DEFAULT);
}

// ============================================================================
// Navigation event handlers
// ============================================================================

void MesholaApp::onNavChatPressed(lv_event_t* event) {
    auto* app = static_cast<MesholaApp*>(lv_event_get_user_data(event));
    app->showView(ViewType::Chat);
}

void MesholaApp::onNavContactsPressed(lv_event_t* event) {
    auto* app = static_cast<MesholaApp*>(lv_event_get_user_data(event));
    app->showView(ViewType::Contacts);
}

void MesholaApp::onNavChannelsPressed(lv_event_t* event) {
    auto* app = static_cast<MesholaApp*>(lv_event_get_user_data(event));
    app->showView(ViewType::Channels);
}

void MesholaApp::onNavSettingsPressed(lv_event_t* event) {
    auto* app = static_cast<MesholaApp*>(lv_event_get_user_data(event));
    app->showView(ViewType::Settings);
}

// ============================================================================
// Mesh event handlers
// ============================================================================

void MesholaApp::onMessageReceived(const Message& msg) {
    // Check if this message belongs to the current conversation
    if (_currentView == ViewType::Chat) {
        const Contact* activeContact = _chatView.getActiveContact();
        const Channel* activeChannel = _chatView.getActiveChannel();
        
        if (msg.isChannel && activeChannel) {
            // Check if message is for active channel
            if (memcmp(msg.channelId, activeChannel->id, CHANNEL_ID_SIZE) == 0) {
                // TODO: Use LVGL lock for thread safety
                // tt_lvgl_lock(TT_MAX_TICKS);
                _chatView.addMessage(msg);
                // tt_lvgl_unlock();
            }
        } else if (!msg.isChannel && activeContact) {
            // Check if message is from active contact
            if (memcmp(msg.senderKey, activeContact->publicKey, PUBLIC_KEY_SIZE) == 0) {
                // TODO: Use LVGL lock for thread safety
                _chatView.addMessage(msg);
            }
        }
    }
    // TODO: Store message for later viewing even if not in active conversation
}

void MesholaApp::onContactUpdated(const Contact& contact, bool isNew) {
    // Refresh contacts view if visible
    if (_currentView == ViewType::Contacts) {
        showView(ViewType::Contacts);  // Refresh
    }
    
    // Update chat header if this is the active contact
    if (_currentView == ViewType::Chat) {
        const Contact* activeContact = _chatView.getActiveContact();
        if (activeContact && memcmp(contact.publicKey, activeContact->publicKey, PUBLIC_KEY_SIZE) == 0) {
            _chatView.setActiveContact(&contact);  // Update with new info
        }
    }
}

void MesholaApp::onAckReceived(uint32_t ackId, bool success) {
    if (_currentView == ViewType::Chat) {
        _chatView.updateMessageStatus(ackId, 
            success ? MessageStatus::Delivered : MessageStatus::Failed);
    }
}

void MesholaApp::onProfileSwitch(const Profile& newProfile) {
    // Reinitialize mesh service with new profile
    auto& meshService = MeshService::getInstance();
    meshService.reinitWithProfile(newProfile);
    
    // Clear chat view - new profile means new conversation context
    _chatView.clearActiveConversation();
    
    // If we're on chat view, refresh to show welcome screen
    if (_currentView == ViewType::Chat) {
        showView(ViewType::Chat);
    }
    
    // Refresh other views if visible
    if (_currentView == ViewType::Contacts) {
        showView(ViewType::Contacts);
    } else if (_currentView == ViewType::Channels) {
        showView(ViewType::Channels);
    } else if (_currentView == ViewType::Settings) {
        showView(ViewType::Settings);
    }
}

} // namespace meshola
