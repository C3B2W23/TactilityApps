/**
 * Meshola Messenger - App Implementation
 * 
 * This is a Tactility APP that provides UI for mesh messaging.
 * The actual mesh operations happen in MesholaMsgService (a Tactility SERVICE).
 */

#include "MesholaApp.h"
#include "service/MesholaMsgService.h"

#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/lvgl/Lvgl.h>
#include <Tactility/Log.h>
#include <lvgl.h>
#include <cstring>
#include <cstdio>

namespace meshola {

#define TAG "MesholaApp"

// Color scheme
static const uint32_t COLOR_BG_DARK = 0x1a1a1a;
static const uint32_t COLOR_BG_CARD = 0x2d2d2d;
static const uint32_t COLOR_ACCENT = 0x0066cc;
static const uint32_t COLOR_ACCENT_DIM = 0x333333;
static const uint32_t COLOR_TEXT = 0xffffff;
static const uint32_t COLOR_TEXT_DIM = 0x888888;

MesholaApp::MesholaApp() {}
MesholaApp::~MesholaApp() {}

void MesholaApp::onShow(tt::app::AppContext& appContext, lv_obj_t* parent) {
    TT_LOG_I(TAG, "onShow");
    _parent = parent;
    
    // Get MesholaMsgService
    _mesholaMsgService = service::findMesholaMsgService();
    if (!_mesholaMsgService) {
        TT_LOG_E(TAG, "MesholaMsgService not found!");
        auto* label = lv_label_create(parent);
        lv_label_set_text(label, "Error: MesholaMsgService not running");
        lv_obj_center(label);
        return;
    }
    
    // Subscribe to service events
    _messageSubId = _mesholaMsgService->getMessagePubSub()->subscribe(
        [this](const service::MessageEvent& e) { onMessageEvent(e); });
    _contactSubId = _mesholaMsgService->getContactPubSub()->subscribe(
        [this](const service::ContactEvent& e) { onContactEvent(e); });
    _ackSubId = _mesholaMsgService->getAckPubSub()->subscribe(
        [this](const service::AckEvent& e) { onAckEvent(e); });
    _statusSubId = _mesholaMsgService->getStatusPubSub()->subscribe(
        [this](const service::StatusEvent& e) { onStatusEvent(e); });
    
    // Layout
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, LV_STATE_DEFAULT);
    
    tt::lvgl::toolbar_create(parent, appContext);
    
    _contentContainer = lv_obj_create(parent);
    lv_obj_set_width(_contentContainer, LV_PCT(100));
    lv_obj_set_flex_grow(_contentContainer, 1);
    lv_obj_set_style_pad_all(_contentContainer, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_contentContainer, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(_contentContainer, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    
    createNavBar(parent);
    refreshContactList();
    showView(ViewType::Chat);
}

void MesholaApp::onHide(tt::app::AppContext& appContext) {
    TT_LOG_I(TAG, "onHide");
    (void)appContext;
    
    if (_mesholaMsgService) {
        _mesholaMsgService->getMessagePubSub()->unsubscribe(_messageSubId);
        _mesholaMsgService->getContactPubSub()->unsubscribe(_contactSubId);
        _mesholaMsgService->getAckPubSub()->unsubscribe(_ackSubId);
        _mesholaMsgService->getStatusPubSub()->unsubscribe(_statusSubId);
    }
    
    _chatView.destroy();
    _contactsView.destroy();
    _mesholaMsgService = nullptr;
    _parent = nullptr;
    _contentContainer = nullptr;
}

void MesholaApp::onMessageEvent(const service::MessageEvent& event) {
    if (event.isNew && _currentView == ViewType::Chat && _hasActiveContact) {
        bool relevant = event.isIncoming 
            ? (memcmp(event.message.senderKey, _activeContact.publicKey, PUBLIC_KEY_SIZE) == 0)
            : (memcmp(event.message.recipientKey, _activeContact.publicKey, PUBLIC_KEY_SIZE) == 0);
        if (relevant) {
            _chatView.addMessage(event.message);
        }
    }
}

void MesholaApp::onContactEvent(const service::ContactEvent& event) {
    if (_currentView == ViewType::Contacts) {
        refreshContactList();
    }
}

void MesholaApp::onAckEvent(const service::AckEvent& event) {
    _chatView.updateMessageStatus(event.ackId, 
        event.success ? MessageStatus::Delivered : MessageStatus::Failed);
}

void MesholaApp::onStatusEvent(const service::StatusEvent& event) {
    // Status bar updates could go here
}

void MesholaApp::createNavBar(lv_obj_t* parent) {
    _navBar = lv_obj_create(parent);
    lv_obj_set_size(_navBar, LV_PCT(100), 44);
    lv_obj_set_flex_flow(_navBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_navBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(_navBar, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_navBar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(_navBar, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    
    auto makeBtn = [this](const char* text) {
        auto* btn = lv_btn_create(_navBar);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, 36);
        lv_obj_set_style_radius(btn, 6, LV_STATE_DEFAULT);
        auto* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, text);
        lv_obj_center(lbl);
        return btn;
    };
    
    _btnChat = makeBtn(LV_SYMBOL_ENVELOPE " Chat");
    lv_obj_add_event_cb(_btnChat, onNavChatPressed, LV_EVENT_CLICKED, this);
    
    _btnContacts = makeBtn(LV_SYMBOL_LIST " Peers");
    lv_obj_add_event_cb(_btnContacts, onNavContactsPressed, LV_EVENT_CLICKED, this);
    
    _btnChannels = makeBtn(LV_SYMBOL_CALL " Ch");
    lv_obj_add_event_cb(_btnChannels, onNavChannelsPressed, LV_EVENT_CLICKED, this);
    
    _btnSettings = makeBtn(LV_SYMBOL_SETTINGS);
    lv_obj_add_event_cb(_btnSettings, onNavSettingsPressed, LV_EVENT_CLICKED, this);
}

void MesholaApp::updateNavButtonStates() {
    auto set = [](lv_obj_t* btn, bool active) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(active ? COLOR_ACCENT : COLOR_ACCENT_DIM), LV_STATE_DEFAULT);
    };
    set(_btnChat, _currentView == ViewType::Chat);
    set(_btnContacts, _currentView == ViewType::Contacts);
    set(_btnChannels, _currentView == ViewType::Channels);
    set(_btnSettings, _currentView == ViewType::Settings);
}

void MesholaApp::showView(ViewType view) {
    if (!_contentContainer) return;
    
    if (_currentView == ViewType::Chat) _chatView.destroy();
    else if (_currentView == ViewType::Contacts) _contactsView.destroy();
    
    lv_obj_clean(_contentContainer);
    _currentView = view;
    updateNavButtonStates();
    
    switch (view) {
        case ViewType::Chat:
            _chatView.setService(_mesholaMsgService);
            _chatView.create(_contentContainer);
            _chatView.setSendCallback(onSendMessage, this);
            if (_hasActiveContact) {
                _chatView.setActiveContact(&_activeContact);
            } else {
                _chatView.clearActiveConversation();
            }
            break;
        case ViewType::Contacts:
            _contactsView.setService(_mesholaMsgService);
            _contactsView.create(_contentContainer);
            _contactsView.setContactSelectedCallback([this](const Contact& c) { onContactSelected(c); });
            break;
        case ViewType::Channels:
            createChannelsViewPlaceholder();
            break;
        case ViewType::Settings:
            createSettingsViewPlaceholder();
            break;
    }
}

void MesholaApp::onNavChatPressed(lv_event_t* e) {
    static_cast<MesholaApp*>(lv_event_get_user_data(e))->showView(ViewType::Chat);
}
void MesholaApp::onNavContactsPressed(lv_event_t* e) {
    static_cast<MesholaApp*>(lv_event_get_user_data(e))->showView(ViewType::Contacts);
}
void MesholaApp::onNavChannelsPressed(lv_event_t* e) {
    static_cast<MesholaApp*>(lv_event_get_user_data(e))->showView(ViewType::Channels);
}
void MesholaApp::onNavSettingsPressed(lv_event_t* e) {
    static_cast<MesholaApp*>(lv_event_get_user_data(e))->showView(ViewType::Settings);
}

void MesholaApp::onContactSelected(const Contact& contact) {
    _activeContact = contact;
    _hasActiveContact = true;
    showView(ViewType::Chat);
}

void MesholaApp::onSendMessage(const char* text, void* userData) {
    auto* app = static_cast<MesholaApp*>(userData);
    if (!app->_mesholaMsgService || !app->_hasActiveContact) return;
    
    uint32_t ackId;
    app->_mesholaMsgService->sendMessage(app->_activeContact.publicKey, text, ackId);
}

void MesholaApp::refreshContactList() {
    if (!_mesholaMsgService) return;
    
    // ContactsView handles its own refresh via service pointer
    if (_currentView == ViewType::Contacts) {
        _contactsView.refresh();
    }
}

void MesholaApp::refreshChatHistory() {
    // ChatView handles loading history internally via setActiveContact
    // This method is now a no-op but kept for potential future use
}

void MesholaApp::createChannelsViewPlaceholder() {
    auto* c = lv_obj_create(_contentContainer);
    lv_obj_set_size(c, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(c, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(c, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    auto* lbl = lv_label_create(c);
    lv_label_set_text(lbl, "Channels\nComing soon...");
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
}

void MesholaApp::createSettingsViewPlaceholder() {
    auto* c = lv_obj_create(_contentContainer);
    lv_obj_set_size(c, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(c, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(c, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    auto* lbl = lv_label_create(c);
    lv_label_set_text(lbl, "Settings\nComing soon...");
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
}

} // namespace meshola
