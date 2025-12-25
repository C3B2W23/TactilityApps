#include "MesholaApp.h"
#include "mesh/MeshService.h"

#include <tt_lvgl_toolbar.h>
#include <tt_lvgl.h>
#include <lvgl.h>
#include <cstring>
#include <cstdio>

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
    
    // Initialize mesh service if not already running
    auto& meshService = MeshService::getInstance();
    if (!meshService.isRunning()) {
        if (meshService.init("meshcore")) {
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
    
    // Clear current content
    lv_obj_clean(_contentContainer);
    
    _currentView = view;
    updateNavButtonStates();
    
    // Create view content
    switch (view) {
        case ViewType::Chat:
            createChatViewPlaceholder();
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
// Placeholder Views (to be replaced with full view classes)
// ============================================================================

void MesholaApp::createChatViewPlaceholder() {
    auto* container = lv_obj_create(_contentContainer);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    
    // Message list area
    auto* msgList = lv_obj_create(container);
    lv_obj_set_width(msgList, LV_PCT(100));
    lv_obj_set_flex_grow(msgList, 1);
    lv_obj_set_style_bg_color(msgList, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(msgList, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(msgList, 0, LV_STATE_DEFAULT);
    
    auto* placeholder = lv_label_create(msgList);
    lv_label_set_text(placeholder, 
        "Welcome to Meshola!\n\n"
        LV_SYMBOL_WIFI " No messages yet\n\n"
        "Select a peer or channel to start chatting");
    lv_obj_set_style_text_align(placeholder, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(placeholder, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    lv_obj_center(placeholder);
    
    // Input area
    auto* inputRow = lv_obj_create(container);
    lv_obj_set_size(inputRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(inputRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(inputRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(inputRow, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(inputRow, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(inputRow, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(inputRow, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(inputRow, 0, LV_STATE_DEFAULT);
    
    auto* input = lv_textarea_create(inputRow);
    lv_obj_set_flex_grow(input, 1);
    lv_obj_set_height(input, 36);
    lv_textarea_set_placeholder_text(input, "Type a message...");
    lv_textarea_set_one_line(input, true);
    
    auto* sendBtn = lv_btn_create(inputRow);
    lv_obj_set_size(sendBtn, 44, 36);
    lv_obj_set_style_bg_color(sendBtn, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
    auto* sendLbl = lv_label_create(sendBtn);
    lv_label_set_text(sendLbl, LV_SYMBOL_OK);
    lv_obj_center(sendLbl);
}

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
    
    auto& meshService = MeshService::getInstance();
    auto* protocol = meshService.getProtocol();
    
    // === Protocol Section ===
    auto* protoSection = lv_label_create(container);
    lv_label_set_text(protoSection, "Protocol");
    lv_obj_set_style_text_font(protoSection, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    auto* protoCard = lv_obj_create(container);
    lv_obj_set_width(protoCard, LV_PCT(100));
    lv_obj_set_height(protoCard, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(protoCard, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(protoCard, 8, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(protoCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(protoCard, 4, LV_STATE_DEFAULT);
    
    if (protocol) {
        auto info = protocol->getInfo();
        
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s v%s", info.name, info.version);
        auto* nameLabel = lv_label_create(protoCard);
        lv_label_set_text(nameLabel, nameBuf);
        
        auto* descLabel = lv_label_create(protoCard);
        lv_label_set_text(descLabel, info.description);
        lv_obj_set_style_text_color(descLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
        lv_label_set_long_mode(descLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(descLabel, LV_PCT(100));
    }
    
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
    
    if (protocol) {
        auto config = meshService.getRadioConfig();
        char buf[64];
        
        snprintf(buf, sizeof(buf), "Frequency: %.3f MHz", config.frequency);
        auto* freqLabel = lv_label_create(radioCard);
        lv_label_set_text(freqLabel, buf);
        
        snprintf(buf, sizeof(buf), "Bandwidth: %.1f kHz", config.bandwidth);
        auto* bwLabel = lv_label_create(radioCard);
        lv_label_set_text(bwLabel, buf);
        
        snprintf(buf, sizeof(buf), "SF: %d  CR: 4/%d  TX: %d dBm", 
            config.spreadingFactor, config.codingRate, config.txPower);
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
    
    char nameBuf[64];
    snprintf(nameBuf, sizeof(nameBuf), "Name: %s", meshService.getNodeName());
    auto* nodeNameLabel = lv_label_create(nodeCard);
    lv_label_set_text(nodeNameLabel, nameBuf);
    
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
    // TODO: Update UI on LVGL thread
    // Need to use tt_lvgl_lock() and queue UI update
}

void MesholaApp::onContactUpdated(const Contact& contact, bool isNew) {
    // TODO: Refresh contacts view if visible
}

void MesholaApp::onAckReceived(uint32_t ackId, bool success) {
    // TODO: Update message status in chat view
}

} // namespace meshola
