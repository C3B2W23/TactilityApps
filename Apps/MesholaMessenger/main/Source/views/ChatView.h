#pragma once

#include "protocol/IProtocol.h"
#include <lvgl.h>
#include <vector>
#include <cstdint>
#include <memory>

// Forward declaration to avoid circular include
namespace meshola::service {
    class MesholaMsgService;
}

namespace meshola {

/**
 * ChatView - Main messaging interface
 * 
 * Displays conversation with selected contact or channel,
 * handles message composition and sending.
 * 
 * NOTE: This view receives its service pointer from MesholaApp.
 * It does NOT access the service via singleton/getInstance().
 */
class ChatView {
public:
    ChatView();
    ~ChatView();
    
    /**
     * Set the service pointer. Must be called before create().
     * The service is owned by MesholaApp, not this view.
     */
    void setService(std::shared_ptr<service::MesholaMsgService> service);
    
    /**
     * Create the view UI as a child of parent.
     */
    void create(lv_obj_t* parent);
    
    /**
     * Destroy the view UI.
     */
    void destroy();
    
    /**
     * Set the current conversation target (contact).
     * Pass nullptr to clear selection.
     */
    void setActiveContact(const Contact* contact);
    
    /**
     * Set the current conversation target (channel).
     * Pass nullptr to clear selection.
     */
    void setActiveChannel(const Channel* channel);
    
    /**
     * Clear active conversation (show welcome screen).
     */
    void clearActiveConversation();
    
    /**
     * Add a message to the current conversation.
     * Called by MesholaApp when messages are received.
     */
    void addMessage(const Message& msg);
    
    /**
     * Update message status (sent, delivered, failed).
     */
    void updateMessageStatus(uint32_t ackId, MessageStatus status);
    
    /**
     * Refresh the message list from storage.
     */
    void refresh();
    
    /**
     * Check if view is currently showing a conversation.
     */
    bool hasActiveConversation() const;
    
    /**
     * Get the currently selected contact (or nullptr).
     */
    const Contact* getActiveContact() const;
    
    /**
     * Get the currently selected channel (or nullptr).
     */
    const Channel* getActiveChannel() const;

    // Callbacks for parent app
    using SendMessageCallback = void(*)(const char* text, void* userData);
    void setSendCallback(SendMessageCallback callback, void* userData);

private:
    // UI creation helpers
    void createWelcomeView();
    void createConversationView();
    void createMessageBubble(const Message& msg);
    void scrollToBottom();
    void clearMessageList();
    void updateHeader();
    
    // Event handlers
    static void onSendClicked(lv_event_t* event);
    static void onInputFocused(lv_event_t* event);
    static void onInputDefocused(lv_event_t* event);
    
    // UI elements
    lv_obj_t* _parent;
    lv_obj_t* _container;
    lv_obj_t* _headerBar;
    lv_obj_t* _headerLabel;
    lv_obj_t* _headerStatus;
    lv_obj_t* _messageList;
    lv_obj_t* _inputRow;
    lv_obj_t* _inputTextarea;
    lv_obj_t* _sendButton;
    lv_obj_t* _welcomeView;
    
    // State
    Contact _activeContact;
    Channel _activeChannel;
    bool _hasActiveContact;
    bool _hasActiveChannel;
    
    // Message cache for current conversation
    std::vector<Message> _messages;
    
    // Callback
    SendMessageCallback _sendCallback;
    void* _sendCallbackUserData;
    
    // Service pointer (owned by MesholaApp, not us)
    std::shared_ptr<service::MesholaMsgService> _service;
};

} // namespace meshola
