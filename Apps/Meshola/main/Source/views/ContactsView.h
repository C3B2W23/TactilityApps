#pragma once

#include "protocol/IProtocol.h"
#include <lvgl.h>
#include <vector>
#include <functional>
#include <memory>

// Forward declaration to avoid circular include
namespace meshola::service {
    class MesholaMsgService;
}

namespace meshola {

/**
 * ContactsView - Displays discovered peers and allows selection for chat.
 * 
 * Features:
 * - List of discovered contacts with name, signal, status
 * - Tap to open chat with contact
 * - Broadcast advertisement button
 * - Refresh button
 * - Sort options (name, signal, last seen)
 * 
 * NOTE: This view receives its service pointer from MesholaApp.
 * It does NOT access the service via singleton/getInstance().
 */
class ContactsView {
public:
    using ContactSelectedCallback = std::function<void(const Contact& contact)>;
    
    ContactsView();
    ~ContactsView();
    
    /**
     * Set the service pointer. Must be called before create().
     * The service is owned by MesholaApp, not this view.
     */
    void setService(std::shared_ptr<service::MesholaMsgService> service);
    
    /**
     * Create the view UI.
     */
    void create(lv_obj_t* parent);
    
    /**
     * Destroy the view UI.
     */
    void destroy();
    
    /**
     * Refresh the contact list from MesholaMsgService.
     */
    void refresh();
    
    /**
     * Set callback when a contact is selected.
     */
    void setContactSelectedCallback(ContactSelectedCallback callback);
    
    /**
     * Update a single contact in the list (for real-time updates).
     */
    void updateContact(const Contact& contact);
    
    /**
     * Add a new contact to the list.
     */
    void addContact(const Contact& contact);

private:
    // UI elements
    lv_obj_t* _container;
    lv_obj_t* _headerRow;
    lv_obj_t* _contactList;
    lv_obj_t* _emptyLabel;
    lv_obj_t* _broadcastBtn;
    lv_obj_t* _refreshBtn;
    
    // Data
    std::vector<Contact> _contacts;
    ContactSelectedCallback _contactSelectedCallback;
    
    // UI creation helpers
    void createHeader();
    void createContactList();
    void createEmptyState();
    
    // Create a single contact row
    lv_obj_t* createContactRow(const Contact& contact, int index);
    
    // Update the list display
    void updateListDisplay();
    
    // Get signal strength indicator
    const char* getSignalIcon(int16_t rssi);
    
    // Get online status color
    uint32_t getStatusColor(bool isOnline);
    
    // Format last seen time
    void formatLastSeen(uint32_t timestamp, char* dest, size_t maxLen);
    
    // Event handlers
    static void onContactPressed(lv_event_t* event);
    static void onBroadcastPressed(lv_event_t* event);
    static void onRefreshPressed(lv_event_t* event);
    
    // Service pointer (owned by MesholaApp, not us)
    std::shared_ptr<service::MesholaMsgService> _service;
};

} // namespace meshola
