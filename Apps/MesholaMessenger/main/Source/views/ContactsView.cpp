#include "ContactsView.h"
#include "service/MesholaMsgService.h"
#include <cstring>
#include <cstdio>
#include <ctime>
#include <algorithm>

namespace meshola {

// Colors (match MesholaApp)
static constexpr uint32_t COLOR_BG_DARK = 0x1a1a1a;
static constexpr uint32_t COLOR_BG_CARD = 0x2d2d2d;
static constexpr uint32_t COLOR_ACCENT = 0x0066cc;
static constexpr uint32_t COLOR_TEXT = 0xffffff;
static constexpr uint32_t COLOR_TEXT_DIM = 0x888888;
static constexpr uint32_t COLOR_SUCCESS = 0x00aa55;
static constexpr uint32_t COLOR_WARNING = 0xffaa00;
static constexpr uint32_t COLOR_ERROR = 0xcc3333;

ContactsView::ContactsView()
    : _container(nullptr)
    , _headerRow(nullptr)
    , _contactList(nullptr)
    , _emptyLabel(nullptr)
    , _broadcastBtn(nullptr)
    , _refreshBtn(nullptr)
    , _contactSelectedCallback(nullptr)
    , _service(nullptr)
{
}

ContactsView::~ContactsView() {
    destroy();
}

void ContactsView::setService(std::shared_ptr<service::MesholaMsgService> service) {
    _service = service;
}

void ContactsView::create(lv_obj_t* parent) {
    // Main container
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(_container, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    
    createHeader();
    createContactList();
    createEmptyState();
    
    // Initial refresh
    refresh();
}

void ContactsView::destroy() {
    _container = nullptr;
    _headerRow = nullptr;
    _contactList = nullptr;
    _emptyLabel = nullptr;
    _broadcastBtn = nullptr;
    _refreshBtn = nullptr;
    _contacts.clear();
}

void ContactsView::createHeader() {
    _headerRow = lv_obj_create(_container);
    lv_obj_set_width(_headerRow, LV_PCT(100));
    lv_obj_set_height(_headerRow, 44);
    lv_obj_set_flex_flow(_headerRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_headerRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(_headerRow, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(_headerRow, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(_headerRow, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_headerRow, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(_headerRow, 0, LV_STATE_DEFAULT);
    
    // Title
    auto* title = lv_label_create(_headerRow);
    lv_label_set_text(title, "Peers");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    // Button container
    auto* btnRow = lv_obj_create(_headerRow);
    lv_obj_set_height(btnRow, LV_SIZE_CONTENT);
    lv_obj_set_width(btnRow, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btnRow, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btnRow, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btnRow, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btnRow, 0, LV_STATE_DEFAULT);
    
    // Broadcast button
    _broadcastBtn = lv_btn_create(btnRow);
    lv_obj_set_size(_broadcastBtn, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_bg_color(_broadcastBtn, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(_broadcastBtn, 6, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(_broadcastBtn, onBroadcastPressed, LV_EVENT_CLICKED, this);
    
    auto* broadcastLabel = lv_label_create(_broadcastBtn);
    lv_label_set_text(broadcastLabel, LV_SYMBOL_WIFI " Broadcast");
    lv_obj_set_style_text_font(broadcastLabel, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    
    // Refresh button
    _refreshBtn = lv_btn_create(btnRow);
    lv_obj_set_size(_refreshBtn, 28, 28);
    lv_obj_set_style_bg_color(_refreshBtn, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(_refreshBtn, 4, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(_refreshBtn, onRefreshPressed, LV_EVENT_CLICKED, this);
    
    auto* refreshLabel = lv_label_create(_refreshBtn);
    lv_label_set_text(refreshLabel, LV_SYMBOL_REFRESH);
    lv_obj_center(refreshLabel);
}

void ContactsView::createContactList() {
    _contactList = lv_obj_create(_container);
    lv_obj_set_width(_contactList, LV_PCT(100));
    lv_obj_set_flex_grow(_contactList, 1);
    lv_obj_set_flex_flow(_contactList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(_contactList, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(_contactList, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(_contactList, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(_contactList, 0, LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(_contactList, LV_SCROLLBAR_MODE_AUTO);
}

void ContactsView::createEmptyState() {
    _emptyLabel = lv_label_create(_contactList);
    lv_label_set_text(_emptyLabel, "No peers discovered yet.\n\nTap 'Broadcast' to announce\nyour presence to nearby nodes.");
    lv_obj_set_style_text_color(_emptyLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(_emptyLabel, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_width(_emptyLabel, LV_PCT(100));
    lv_obj_set_style_pad_top(_emptyLabel, 40, LV_STATE_DEFAULT);
}

void ContactsView::refresh() {
    if (!_contactList || !_service) return;
    
    // Get contacts from MesholaMsgService via service pointer
    int count = _service->getContactCount();
    
    _contacts.clear();
    for (int i = 0; i < count; i++) {
        Contact contact;
        if (_service->getContact(i, contact)) {
            _contacts.push_back(contact);
        }
    }
    
    updateListDisplay();
}

void ContactsView::updateListDisplay() {
    if (!_contactList) return;
    
    // Clear existing items (except empty label)
    uint32_t childCount = lv_obj_get_child_cnt(_contactList);
    for (int i = childCount - 1; i >= 0; i--) {
        lv_obj_t* child = lv_obj_get_child(_contactList, i);
        if (child != _emptyLabel) {
            lv_obj_del(child);
        }
    }
    
    // Show/hide empty state
    if (_contacts.empty()) {
        lv_obj_clear_flag(_emptyLabel, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    
    lv_obj_add_flag(_emptyLabel, LV_OBJ_FLAG_HIDDEN);
    
    auto makeSection = [&](const char* title, const std::vector<Contact>& list) {
        if (list.empty()) return;
        auto* sectionLabel = lv_label_create(_contactList);
        lv_label_set_text(sectionLabel, title);
        lv_obj_set_style_text_color(sectionLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(sectionLabel, 6, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(sectionLabel, 4, LV_STATE_DEFAULT);
        for (size_t i = 0; i < list.size(); i++) {
            createContactRow(list[i], static_cast<int>(i));
        }
    };

    auto sorter = [](const Contact& a, const Contact& b) {
        if (a.isFavorite != b.isFavorite) {
            return a.isFavorite; // favorites first
        }
        return strcmp(a.name, b.name) < 0;
    };

    std::vector<Contact> companions, repeaters, rooms, unknowns;
    for (const auto& c : _contacts) {
        switch (c.role) {
            case NodeRole::Companion: companions.push_back(c); break;
            case NodeRole::Repeater: repeaters.push_back(c); break;
            case NodeRole::Room: rooms.push_back(c); break;
            default: unknowns.push_back(c); break;
        }
    }
    std::sort(companions.begin(), companions.end(), sorter);
    std::sort(repeaters.begin(), repeaters.end(), sorter);
    std::sort(rooms.begin(), rooms.end(), sorter);
    std::sort(unknowns.begin(), unknowns.end(), sorter);

    makeSection("Companions", companions);
    makeSection("Repeaters", repeaters);
    makeSection("Rooms", rooms);
    makeSection("Unknown", unknowns);
}

lv_obj_t* ContactsView::createContactRow(const Contact& contact, int index) {
    auto* row = lv_obj_create(_contactList);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(row, 52, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(row, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_BG_CARD), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 0, LV_STATE_DEFAULT);
    
    // Make clickable
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x3d3d3d), LV_STATE_PRESSED);
    
    // Store index in user data for callback
    lv_obj_set_user_data(row, (void*)(intptr_t)index);
    lv_obj_add_event_cb(row, onContactPressed, LV_EVENT_CLICKED, this);
    
    // Online indicator dot
    auto* statusDot = lv_obj_create(row);
    lv_obj_set_size(statusDot, 10, 10);
    lv_obj_set_style_radius(statusDot, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(statusDot, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(statusDot, 
        lv_color_hex(contact.isOnline ? COLOR_SUCCESS : COLOR_TEXT_DIM), 
        LV_STATE_DEFAULT);
    
    // Name and info column
    auto* infoCol = lv_obj_create(row);
    lv_obj_set_flex_grow(infoCol, 1);
    lv_obj_set_height(infoCol, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(infoCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(infoCol, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(infoCol, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(infoCol, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(infoCol, 0, LV_STATE_DEFAULT);
    
    // Name
    auto* nameLabel = lv_label_create(infoCol);
    lv_label_set_text(nameLabel, contact.name[0] ? contact.name : "(Unknown)");
    lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(nameLabel, lv_color_hex(COLOR_TEXT), LV_STATE_DEFAULT);
    
    // Status line: last seen / hops
    char statusBuf[64];
    if (contact.isOnline) {
        if (contact.pathLength > 1) {
            snprintf(statusBuf, sizeof(statusBuf), "Online • %d hops", contact.pathLength);
        } else {
            snprintf(statusBuf, sizeof(statusBuf), "Online • Direct");
        }
    } else {
        char timeBuf[32];
        formatLastSeen(contact.lastSeen, timeBuf, sizeof(timeBuf));
        snprintf(statusBuf, sizeof(statusBuf), "Last seen %s", timeBuf);
    }
    
    auto* statusLabel = lv_label_create(infoCol);
    lv_label_set_text(statusLabel, statusBuf);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    
    // Right-side controls and signal
    auto* rightCol = lv_obj_create(row);
    lv_obj_set_flex_flow(rightCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(rightCol, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(rightCol, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(rightCol, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(rightCol, 4, LV_STATE_DEFAULT);

    auto* signalLabel = lv_label_create(rightCol);
    char signalBuf[16];
    snprintf(signalBuf, sizeof(signalBuf), "%s %d", getSignalIcon(contact.lastRssi), contact.lastRssi);
    lv_label_set_text(signalLabel, signalBuf);
    lv_obj_set_style_text_font(signalLabel, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(signalLabel, lv_color_hex(COLOR_TEXT_DIM), LV_STATE_DEFAULT);

    // Favorite toggle
    auto* favBtn = lv_btn_create(rightCol);
    lv_obj_set_size(favBtn, 28, 28);
    lv_obj_set_style_bg_color(favBtn, lv_color_hex(COLOR_BG_DARK), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(favBtn, onFavoriteToggled, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(favBtn, (void*)(intptr_t)index);
    auto* favLabel = lv_label_create(favBtn);
    lv_label_set_text(favLabel, contact.isFavorite ? "*" : " ");
    lv_obj_set_style_text_color(favLabel, lv_color_hex(contact.isFavorite ? COLOR_ACCENT : COLOR_TEXT_DIM), LV_STATE_DEFAULT);
    lv_obj_center(favLabel);

    // Add button for discovered contacts
    if (contact.isDiscovered) {
        auto* addBtn = lv_btn_create(rightCol);
        lv_obj_set_size(addBtn, 48, 24);
        lv_obj_set_style_bg_color(addBtn, lv_color_hex(COLOR_ACCENT), LV_STATE_DEFAULT);
        lv_obj_add_event_cb(addBtn, onAddPressed, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(addBtn, (void*)(intptr_t)index);
        auto* addLbl = lv_label_create(addBtn);
        lv_label_set_text(addLbl, "Add");
        lv_obj_center(addLbl);
    }
    
    return row;
}

void ContactsView::setContactSelectedCallback(ContactSelectedCallback callback) {
    _contactSelectedCallback = callback;
}

void ContactsView::updateContact(const Contact& contact) {
    // Find and update existing, or add new
    for (size_t i = 0; i < _contacts.size(); i++) {
        if (memcmp(_contacts[i].publicKey, contact.publicKey, PUBLIC_KEY_SIZE) == 0) {
            _contacts[i] = contact;
            updateListDisplay();
            return;
        }
    }
    // Not found, add it
    addContact(contact);
}

void ContactsView::addContact(const Contact& contact) {
    _contacts.push_back(contact);
    updateListDisplay();
}

const char* ContactsView::getSignalIcon(int16_t rssi) {
    if (rssi >= -70) return LV_SYMBOL_WIFI;      // Strong
    if (rssi >= -85) return LV_SYMBOL_WIFI;      // Medium (could use different icon)
    if (rssi >= -100) return LV_SYMBOL_WIFI;     // Weak
    return LV_SYMBOL_WIFI;                        // Very weak
}

uint32_t ContactsView::getStatusColor(bool isOnline) {
    return isOnline ? COLOR_SUCCESS : COLOR_TEXT_DIM;
}

void ContactsView::formatLastSeen(uint32_t timestamp, char* dest, size_t maxLen) {
    if (timestamp == 0) {
        snprintf(dest, maxLen, "never");
        return;
    }
    
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t diff = now - timestamp;
    
    if (diff < 60) {
        snprintf(dest, maxLen, "just now");
    } else if (diff < 3600) {
        snprintf(dest, maxLen, "%lum ago", (unsigned long)(diff / 60));
    } else if (diff < 86400) {
        snprintf(dest, maxLen, "%luh ago", (unsigned long)(diff / 3600));
    } else {
        snprintf(dest, maxLen, "%lud ago", (unsigned long)(diff / 86400));
    }
}

// Event handlers
void ContactsView::onRefreshPressed(lv_event_t* event) {
    auto* view = static_cast<ContactsView*>(lv_event_get_user_data(event));
    if (view) {
        view->refresh();
    }
}

void ContactsView::onBroadcastPressed(lv_event_t* event) {
    auto* view = static_cast<ContactsView*>(lv_event_get_user_data(event));
    if (view && view->_service) {
        view->_service->sendAdvertisement();
        // Could show a brief "Sent!" indicator here
    }
}

void ContactsView::onContactPressed(lv_event_t* event) {
    auto* view = static_cast<ContactsView*>(lv_event_get_user_data(event));
    lv_obj_t* row = static_cast<lv_obj_t*>(lv_event_get_target(event));
    
    if (view && view->_contactSelectedCallback) {
        int index = (int)(intptr_t)lv_obj_get_user_data(row);
        if (index >= 0 && index < (int)view->_contacts.size()) {
            view->_contactSelectedCallback(view->_contacts[index]);
        }
    }
}

void ContactsView::onFavoriteToggled(lv_event_t* event) {
    auto* view = static_cast<ContactsView*>(lv_event_get_user_data(event));
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (!view || !btn || !view->_service) return;
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (index < 0 || index >= (int)view->_contacts.size()) return;
    const auto& contact = view->_contacts[index];
    bool newFav = !contact.isFavorite;
    view->_service->setContactFavorite(contact.publicKey, newFav);
    view->refresh();
}

void ContactsView::onAddPressed(lv_event_t* event) {
    auto* view = static_cast<ContactsView*>(lv_event_get_user_data(event));
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (!view || !btn || !view->_service) return;
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (index < 0 || index >= (int)view->_contacts.size()) return;
    const auto& contact = view->_contacts[index];
    view->_service->promoteContact(contact.publicKey);
    view->refresh();
}

} // namespace meshola
