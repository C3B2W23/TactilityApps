#include "ChatView.h"
#include "service/MesholaMsgService.h"
#include <cstring>
#include <cstdio>

namespace meshola {

// Color scheme (matching MesholaApp)
static const uint32_t COLOR_BG_DARK = 0x1a1a1a;
static const uint32_t COLOR_BG_CARD = 0x2d2d2d;
static const uint32_t COLOR_ACCENT = 0x0066cc;
static const uint32_t COLOR_ACCENT_LIGHT = 0x3399ff;
static const uint32_t COLOR_TEXT = 0xffffff;
static const uint32_t COLOR_TEXT_DIM = 0x888888;
static const uint32_t COLOR_SUCCESS = 0x00aa55;
static const uint32_t COLOR_WARNING = 0xffaa00;
static const uint32_t COLOR_ERROR = 0xcc3333;
static const uint32_t COLOR_MSG_OUTGOING = 0x0055aa;
static const uint32_t COLOR_MSG_INCOMING = 0x3d3d3d;

ChatView::ChatView()
    : _parent(nullptr)
    , _container(nullptr)
    , _headerBar(nullptr)
    , _headerLabel(nullptr)
    , _headerStatus(nullptr)
    , _messageList(nullptr)
    , _inputRow(nullptr)
    , _inputTextarea(nullptr)
    , _sendButton(nullptr)
    , _welcomeView(nullptr)
    , _hasActiveContact(false)
    , _hasActiveChannel(false)
    , _sendCallback(nullptr)
    , _sendCallbackUserData(nullptr)
    , _service(nullptr)
{
    memset(&_activeContact, 0, sizeof(_activeContact));
    memset(&_activeChannel, 0, sizeof(_activeChannel));
}

ChatView::~ChatView() {
    destroy();
}

void ChatView::setService(std::shared_ptr<service::MesholaMsgService> service) {
    _service = service;
}

void ChatView::create(lv_obj_t* parent) {
    _parent = parent;
    
    // Main container
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(_container, 0, LV_STATE_DEFAULT);
    
    // Show welcome view by default
    createWelcomeView();
}

void ChatView::destroy() {
    _messages.clear();
    _parent = nullptr;
    _container = nullptr;
    _headerBar = nullptr;
    _headerLabel = nullptr;
    _headerStatus = nullptr;
    _messageList = nullptr;
    _inputRow = nullptr;
    _inputTextarea = nullptr;
    _sendButton = nullptr;
    _welcomeView = nullptr;
}

void ChatView::createWelcomeView() {
    if (_welcomeView) {
        return;  // Already showing
    }
    
    // Hide conversation UI if present
    if (_headerBar) lv_obj_add_flag(_headerBar, LV_OBJ_FLAG_HIDDEN);
    if (_messageList) lv_obj_add_flag(_messageList, LV_OBJ_FLAG_HIDDEN);
    if (_inputRow) lv_obj_add_flag(_inputRow, LV_OBJ_FLAG_HIDDEN);
    
    _welcomeView = lv_obj_create(_container);
    lv_obj_set_size(_welcomeView, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(_welcomeView, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_welcomeView, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(_welcomeView, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_welcomeView, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(_welcomeView, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(_welcomeView, 20, LV_STATE_DEFAULT);
    
    // Logo/icon placeholder
    auto* icon = lv_label_create(_welcomeView);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(icon, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
    
    // Welcome text
    auto* title = lv_label_create(_welcomeView);
    lv_label_set_text(title, "Welcome to Meshola Messenger");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_TEXT), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(title, 10, LV_STATE_DEFAULT);
    
    // Instructions
    auto* instructions = lv_label_create(_welcomeView);
    lv_label_set_text(instructions, 
        "Select a peer from the Peers tab\n"
        "or a channel from the Channels tab\n"
        "to start messaging");
    lv_obj_set_style_text_align(instructions, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(instructions, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(instructions, 10, LV_STATE_DEFAULT);
    lv_label_set_long_mode(instructions, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(instructions, LV_PCT(90));
    
    // Node info (only if we have a service)
    if (_service) {
        auto* nodeInfo = lv_label_create(_welcomeView);
        char nodeBuf[64];
        snprintf(nodeBuf, sizeof(nodeBuf), "\nYour node: %s", _service->getNodeName());
        lv_label_set_text(nodeInfo, nodeBuf);
        lv_obj_set_style_text_color(nodeInfo, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(nodeInfo, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    }
}

void ChatView::createConversationView() {
    // Remove welcome view if present
    if (_welcomeView) {
        lv_obj_del(_welcomeView);
        _welcomeView = nullptr;
    }
    
    // Create header bar if needed
    if (!_headerBar) {
        _headerBar = lv_obj_create(_container);
        lv_obj_set_size(_headerBar, LV_PCT(100), 44);
        lv_obj_set_flex_flow(_headerBar, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_bg_color(_headerBar, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(_headerBar, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(_headerBar, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(_headerBar, 6, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(_headerBar, 2, LV_STATE_DEFAULT);
        
        _headerLabel = lv_label_create(_headerBar);
        lv_obj_set_style_text_font(_headerLabel, &lv_font_montserrat_14, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(_headerLabel, lv_color_hex(COLOR_TEXT), LV_STATE_DEFAULT);
        
        _headerStatus = lv_label_create(_headerBar);
        lv_obj_set_style_text_font(_headerStatus, &lv_font_montserrat_10, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(_headerStatus, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    }
    lv_obj_clear_flag(_headerBar, LV_OBJ_FLAG_HIDDEN);
    
    // Create message list if needed
    if (!_messageList) {
        _messageList = lv_obj_create(_container);
        lv_obj_set_width(_messageList, LV_PCT(100));
        lv_obj_set_flex_grow(_messageList, 1);
        lv_obj_set_flex_flow(_messageList, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_bg_color(_messageList, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(_messageList, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(_messageList, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(_messageList, 8, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(_messageList, 6, LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(_messageList, LV_SCROLLBAR_MODE_AUTO);
    }
    lv_obj_clear_flag(_messageList, LV_OBJ_FLAG_HIDDEN);
    
    // Create input row if needed
    if (!_inputRow) {
        _inputRow = lv_obj_create(_container);
        lv_obj_set_size(_inputRow, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(_inputRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(_inputRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_color(_inputRow, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(_inputRow, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(_inputRow, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(_inputRow, 6, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(_inputRow, 6, LV_STATE_DEFAULT);
        
        _inputTextarea = lv_textarea_create(_inputRow);
        lv_obj_set_flex_grow(_inputTextarea, 1);
        lv_obj_set_height(_inputTextarea, 36);
        lv_textarea_set_placeholder_text(_inputTextarea, "Type a message...");
        lv_textarea_set_one_line(_inputTextarea, true);
        lv_textarea_set_max_length(_inputTextarea, MAX_MESSAGE_LEN - 1);
        lv_obj_add_event_cb(_inputTextarea, onInputFocused, LV_EVENT_FOCUSED, this);
        lv_obj_add_event_cb(_inputTextarea, onInputDefocused, LV_EVENT_DEFOCUSED, this);
        
        _sendButton = lv_btn_create(_inputRow);
        lv_obj_set_size(_sendButton, 50, 36);
        lv_obj_set_style_bg_color(_sendButton, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
        lv_obj_set_style_radius(_sendButton, 6, LV_STATE_DEFAULT);
        lv_obj_add_event_cb(_sendButton, onSendClicked, LV_EVENT_CLICKED, this);
        
        auto* sendLabel = lv_label_create(_sendButton);
        lv_label_set_text(sendLabel, LV_SYMBOL_OK " Send");
        lv_obj_center(sendLabel);
    }
    lv_obj_clear_flag(_inputRow, LV_OBJ_FLAG_HIDDEN);
    
    // Update header and refresh messages
    updateHeader();
}

void ChatView::createMessageBubble(const Message& msg) {
    if (!_messageList) return;
    
    bool isOutgoing = msg.isOutgoing;
    
    // Bubble container (for alignment)
    auto* bubbleWrapper = lv_obj_create(_messageList);
    lv_obj_set_size(bubbleWrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bubbleWrapper, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bubbleWrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(bubbleWrapper, 0, LV_STATE_DEFAULT);
    
    // Message bubble
    auto* bubble = lv_obj_create(bubbleWrapper);
    lv_obj_set_size(bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(bubble, LV_PCT(80), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bubble, 
        lv_color_hex(isOutgoing ? COLOR_MSG_OUTGOING : COLOR_MSG_INCOMING), 
        LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bubble, 12, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(bubble, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bubble, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(bubble, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(bubble, 4, LV_STATE_DEFAULT);
    
    // Align bubble left or right
    if (isOutgoing) {
        lv_obj_align(bubble, LV_ALIGN_RIGHT_MID, 0, 0);
    } else {
        lv_obj_align(bubble, LV_ALIGN_LEFT_MID, 0, 0);
    }
    
    // Sender name (for incoming channel messages)
    if (!isOutgoing && _hasActiveChannel && strlen(msg.senderName) > 0) {
        auto* senderLabel = lv_label_create(bubble);
        lv_label_set_text(senderLabel, msg.senderName);
        lv_obj_set_style_text_font(senderLabel, &lv_font_montserrat_10, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(senderLabel, lv_color_hex(COLOR_ACCENT_LIGHT), LV_STATE_DEFAULT);
    }
    
    // Message text
    auto* textLabel = lv_label_create(bubble);
    lv_label_set_text(textLabel, msg.text);
    lv_label_set_long_mode(textLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(textLabel, lv_color_hex(COLOR_TEXT), LV_STATE_DEFAULT);
    lv_obj_set_width(textLabel, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(textLabel, lv_pct(100), LV_STATE_DEFAULT);
    
    // Timestamp and status row
    auto* metaRow = lv_obj_create(bubble);
    lv_obj_set_size(metaRow, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(metaRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(metaRow, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(metaRow, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(metaRow, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(metaRow, 0, LV_STATE_DEFAULT);
    
    // Time (simplified - just show HH:MM)
    // TODO: Proper timestamp formatting
    auto* timeLabel = lv_label_create(metaRow);
    char timeBuf[16];
    uint32_t hours = (msg.timestamp / 3600) % 24;
    uint32_t mins = (msg.timestamp / 60) % 60;
    snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", (unsigned long)hours, (unsigned long)mins);
    lv_label_set_text(timeLabel, timeBuf);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(timeLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    
    // Status icon (for outgoing messages)
    if (isOutgoing) {
        auto* statusIcon = lv_label_create(metaRow);
        const char* statusText;
        uint32_t statusColor;
        
        switch (msg.status) {
            case MessageStatus::Pending:
                statusText = LV_SYMBOL_REFRESH;
                statusColor = COLOR_TEXT_DIM;
                break;
            case MessageStatus::Sent:
                statusText = LV_SYMBOL_OK;
                statusColor = COLOR_TEXT_DIM;
                break;
            case MessageStatus::Delivered:
                statusText = LV_SYMBOL_OK LV_SYMBOL_OK;
                statusColor = COLOR_SUCCESS;
                break;
            case MessageStatus::Failed:
                statusText = LV_SYMBOL_CLOSE;
                statusColor = COLOR_ERROR;
                break;
            default:
                statusText = "";
                statusColor = COLOR_TEXT_DIM;
        }
        
        lv_label_set_text(statusIcon, statusText);
        lv_obj_set_style_text_font(statusIcon, &lv_font_montserrat_10, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(statusIcon, lv_color_hex(statusColor), LV_STATE_DEFAULT);
    }
    
    // RSSI (for incoming messages)
    if (!isOutgoing && msg.rssi != 0) {
        auto* rssiLabel = lv_label_create(metaRow);
        char rssiBuf[16];
        snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", msg.rssi);
        lv_label_set_text(rssiLabel, rssiBuf);
        lv_obj_set_style_text_font(rssiLabel, &lv_font_montserrat_10, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(rssiLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    }
}

void ChatView::scrollToBottom() {
    if (_messageList) {
        lv_obj_scroll_to_y(_messageList, LV_COORD_MAX, LV_ANIM_ON);
    }
}

void ChatView::clearMessageList() {
    if (_messageList) {
        lv_obj_clean(_messageList);
    }
    _messages.clear();
}

void ChatView::updateHeader() {
    if (!_headerLabel || !_headerStatus) return;
    
    if (_hasActiveContact) {
        lv_label_set_text(_headerLabel, _activeContact.name);
        
        char statusBuf[64];
        if (_activeContact.isOnline) {
            snprintf(statusBuf, sizeof(statusBuf), "Online • %d dBm • %d hop%s",
                _activeContact.lastRssi,
                _activeContact.pathLength,
                _activeContact.pathLength == 1 ? "" : "s");
        } else {
            snprintf(statusBuf, sizeof(statusBuf), "Last seen: offline");
        }
        lv_label_set_text(_headerStatus, statusBuf);
        
    } else if (_hasActiveChannel) {
        char headerBuf[48];
        snprintf(headerBuf, sizeof(headerBuf), "# %s", _activeChannel.name);
        lv_label_set_text(_headerLabel, headerBuf);
        
        lv_label_set_text(_headerStatus, _activeChannel.isPublic ? "Public channel" : "Private channel");
    }
}

void ChatView::setActiveContact(const Contact* contact) {
    _hasActiveChannel = false;
    
    if (contact) {
        memcpy(&_activeContact, contact, sizeof(Contact));
        _hasActiveContact = true;
        createConversationView();
        clearMessageList();
        
        // Load message history for this contact (via service)
        if (_service) {
            std::vector<Message> history = _service->getContactMessages(contact->publicKey, 50);
            for (const auto& msg : history) {
                _messages.push_back(msg);
                createMessageBubble(msg);
            }
            scrollToBottom();
        }
    } else {
        _hasActiveContact = false;
        clearActiveConversation();
    }
}

void ChatView::setActiveChannel(const Channel* channel) {
    _hasActiveContact = false;
    
    if (channel) {
        memcpy(&_activeChannel, channel, sizeof(Channel));
        _hasActiveChannel = true;
        createConversationView();
        clearMessageList();
        
        // Load message history for this channel (via service)
        if (_service) {
            std::vector<Message> history = _service->getChannelMessages(channel->id, 50);
            for (const auto& msg : history) {
                _messages.push_back(msg);
                createMessageBubble(msg);
            }
            scrollToBottom();
        }
    } else {
        _hasActiveChannel = false;
        clearActiveConversation();
    }
}

void ChatView::clearActiveConversation() {
    _hasActiveContact = false;
    _hasActiveChannel = false;
    clearMessageList();
    createWelcomeView();
}

void ChatView::addMessage(const Message& msg) {
    _messages.push_back(msg);
    createMessageBubble(msg);
    scrollToBottom();
}

void ChatView::updateMessageStatus(uint32_t ackId, MessageStatus status) {
    // Find message by ackId and update status
    for (auto& msg : _messages) {
        if (msg.ackId == ackId) {
            msg.status = status;
            // TODO: Update the visual bubble (need to track bubble objects)
            break;
        }
    }
    // For now, just refresh the whole list
    // refresh();
}

void ChatView::refresh() {
    if (!hasActiveConversation()) return;
    
    clearMessageList();
    for (const auto& msg : _messages) {
        createMessageBubble(msg);
    }
    scrollToBottom();
}

bool ChatView::hasActiveConversation() const {
    return _hasActiveContact || _hasActiveChannel;
}

const Contact* ChatView::getActiveContact() const {
    return _hasActiveContact ? &_activeContact : nullptr;
}

const Channel* ChatView::getActiveChannel() const {
    return _hasActiveChannel ? &_activeChannel : nullptr;
}

void ChatView::setSendCallback(SendMessageCallback callback, void* userData) {
    _sendCallback = callback;
    _sendCallbackUserData = userData;
}

// Event handlers

void ChatView::onSendClicked(lv_event_t* event) {
    auto* view = static_cast<ChatView*>(lv_event_get_user_data(event));
    if (!view || !view->_inputTextarea) return;
    
    const char* text = lv_textarea_get_text(view->_inputTextarea);
    if (!text || strlen(text) == 0) return;
    
    // Call the send callback
    if (view->_sendCallback) {
        view->_sendCallback(text, view->_sendCallbackUserData);
    }
    
    // Clear input
    lv_textarea_set_text(view->_inputTextarea, "");
}

void ChatView::onInputFocused(lv_event_t* event) {
    // Could show keyboard here on touchscreen devices
    // T-Deck has physical keyboard so not needed
}

void ChatView::onInputDefocused(lv_event_t* event) {
    // Could hide keyboard here
}

} // namespace meshola
