#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_http_client.h>
#include <esp_wifi.h>
#include <esp_private/wifi_os_adapter.h>
#include <esp_now.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_system.h>
#include <esp_vfs_fat.h>
#include <driver/i2c.h>
#include <sys/types.h>
#include <cJSON.h>
#include <lvgl.h>
#include "minmea.h"

// Forward declare the tinyusb types to avoid pulling the component.
typedef struct tinyusb_config tinyusb_config_t;
typedef struct tinyusb_msc_sdmmc_config tinyusb_msc_sdmmc_config_t;
typedef struct tinyusb_msc_spiflash_config tinyusb_msc_spiflash_config_t;

// Forward declare minimal minitar types used by Tactility.
typedef struct minitar minitar_t;
typedef struct minitar_entry {
    const char* path;
    size_t size;
} minitar_entry_t;
#include <sys/types.h>

// FreeRTOS defines some of these as macros; undef so we can provide weak symbols.
#undef xEventGroupSetBitsFromISR
#undef xEventGroupClearBitsFromISR

// Weak stubs to satisfy the app ELF link step. Real implementations from
// ESP-IDF/Freertos will override these in the full firmware build.

__attribute__((weak)) EventGroupHandle_t xEventGroupCreate(void) {
    return NULL;
}

__attribute__((weak)) void vEventGroupDelete(EventGroupHandle_t xEventGroup) {
    (void)xEventGroup;
}

__attribute__((weak)) EventBits_t xEventGroupSetBitsFromISR(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet, BaseType_t* pxHigherPriorityTaskWoken) {
    (void)xEventGroup;
    (void)uxBitsToSet;
    (void)pxHigherPriorityTaskWoken;
    return 0;
}

__attribute__((weak)) EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet) {
    (void)xEventGroup;
    (void)uxBitsToSet;
    return 0;
}

__attribute__((weak)) EventBits_t xEventGroupGetBitsFromISR(EventGroupHandle_t xEventGroup) {
    (void)xEventGroup;
    return 0;
}

__attribute__((weak)) EventBits_t xEventGroupClearBitsFromISR(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear) {
    (void)xEventGroup;
    (void)uxBitsToClear;
    return 0;
}

__attribute__((weak)) EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear) {
    (void)xEventGroup;
    (void)uxBitsToClear;
    return 0;
}

__attribute__((weak)) EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                                     const EventBits_t uxBitsToWaitFor,
                                                     const BaseType_t xClearOnExit,
                                                     const BaseType_t xWaitForAllBits,
                                                     TickType_t xTicksToWait) {
    (void)xEventGroup;
    (void)uxBitsToWaitFor;
    (void)xClearOnExit;
    (void)xWaitForAllBits;
    (void)xTicksToWait;
    return 0;
}

__attribute__((weak)) esp_err_t httpd_resp_send_err(httpd_req_t* r, httpd_err_code_t error, const char* errmsg) {
    (void)r;
    (void)error;
    (void)errmsg;
    return ESP_ERR_INVALID_STATE;
}

__attribute__((weak)) ssize_t httpd_req_recv(httpd_req_t* r, char* buf, size_t buf_len) {
    (void)r;
    (void)buf;
    (void)buf_len;
    return -1;
}

__attribute__((weak)) size_t httpd_req_get_hdr_value_len(httpd_req_t* r, const char* field) {
    (void)r;
    (void)field;
    return 0;
}

__attribute__((weak)) esp_err_t httpd_req_get_hdr_value_str(httpd_req_t* r, const char* field, char* val, size_t val_size) {
    (void)r;
    (void)field;
    (void)val;
    (void)val_size;
    return ESP_ERR_NOT_FOUND;
}

__attribute__((weak)) size_t httpd_req_get_url_query_len(httpd_req_t* r) {
    (void)r;
    return 0;
}

__attribute__((weak)) esp_err_t httpd_req_get_url_query_str(httpd_req_t* r, char* buf, size_t buf_len) {
    (void)r;
    (void)buf;
    (void)buf_len;
    return ESP_ERR_NOT_FOUND;
}

__attribute__((weak)) esp_err_t httpd_resp_set_type(httpd_req_t* r, const char* type) {
    (void)r;
    (void)type;
    return ESP_OK;
}

__attribute__((weak)) esp_err_t httpd_resp_send(httpd_req_t* r, const char* buf, ssize_t buf_len) {
    (void)r;
    (void)buf;
    (void)buf_len;
    return ESP_OK;
}

__attribute__((weak)) bool httpd_uri_match_wildcard(const char* templ, const char* uri, size_t uri_len) {
    (void)templ;
    (void)uri;
    (void)uri_len;
    return false;
}

// System / QR stubs
__attribute__((weak)) esp_reset_reason_t esp_reset_reason(void) { return ESP_RST_UNKNOWN; }
__attribute__((weak)) int qrcode_getBufferSize(unsigned int version) { (void)version; return 0; }
__attribute__((weak)) void qrcode_initText(void* qr, uint8_t version, uint8_t ecc, const char* data) {
    (void)qr; (void)version; (void)ecc; (void)data;
}
__attribute__((weak)) int qrcode_getModule(const void* qr, unsigned int x, unsigned int y) {
    (void)qr; (void)x; (void)y; return 0;
}

// ESP-NOW stubs
__attribute__((weak)) esp_err_t esp_now_init(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_now_deinit(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) { (void)cb; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_now_set_pmk(const uint8_t* pmk) { (void)pmk; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_now_add_peer(const esp_now_peer_info_t* peer) { (void)peer; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_now_send(const uint8_t* peer_addr, const uint8_t* data, size_t len) {
    (void)peer_addr; (void)data; (void)len; return ESP_OK;
}

// Wi-Fi / Netif stubs
__attribute__((weak)) esp_err_t esp_wifi_init(const wifi_init_config_t* config) { (void)config; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_deinit(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_set_mode(wifi_mode_t mode) { (void)mode; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_set_storage(wifi_storage_t storage) { (void)storage; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_set_config(wifi_interface_t iface, wifi_config_t* conf) {
    (void)iface; (void)conf; return ESP_OK;
}
__attribute__((weak)) esp_err_t esp_wifi_connect(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_scan_start(const wifi_scan_config_t* config, bool block) {
    (void)config; (void)block; return ESP_OK;
}
__attribute__((weak)) esp_err_t esp_wifi_scan_stop(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_scan_get_ap_records(uint16_t* number, wifi_ap_record_t* ap_records) {
    (void)number; (void)ap_records; return ESP_OK;
}
__attribute__((weak)) esp_err_t esp_wifi_sta_get_rssi(int* rssi) { if (rssi) *rssi = 0; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_start(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_stop(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t second) { (void)primary; (void)second; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_wifi_set_protocol(wifi_interface_t ifx, uint8_t protocol_bitmap) { (void)ifx; (void)protocol_bitmap; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id,
                                                                     esp_event_handler_t event_handler, void* event_handler_arg,
                                                                     esp_event_handler_instance_t* instance) {
    (void)event_base; (void)event_id; (void)event_handler; (void)event_handler_arg; (void)instance; return ESP_OK;
}
__attribute__((weak)) esp_err_t esp_event_handler_instance_unregister(esp_event_base_t event_base, int32_t event_id,
                                                                       esp_event_handler_instance_t instance) {
    (void)event_base; (void)event_id; (void)instance; return ESP_OK;
}
__attribute__((weak)) esp_netif_t* esp_netif_create_default_wifi_sta(void) { return NULL; }
__attribute__((weak)) void esp_netif_destroy(esp_netif_t* h) { (void)h; }

// VFS / FAT stubs
__attribute__((weak)) esp_err_t esp_vfs_fat_info(const char* base_path, uint64_t* out_total_bytes, uint64_t* out_free_bytes) {
    (void)base_path; if (out_total_bytes) *out_total_bytes = 0; if (out_free_bytes) *out_free_bytes = 0; return ESP_OK;
}

// TinyUSB stubs
__attribute__((weak)) esp_err_t tinyusb_driver_install(const tinyusb_config_t* cfg) { (void)cfg; return ESP_OK; }
__attribute__((weak)) esp_err_t tinyusb_msc_storage_init_sdmmc(const tinyusb_msc_sdmmc_config_t* cfg) { (void)cfg; return ESP_OK; }
__attribute__((weak)) esp_err_t tinyusb_msc_storage_init_spiflash(const tinyusb_msc_spiflash_config_t* cfg) { (void)cfg; return ESP_OK; }

// HTTP client stubs
__attribute__((weak)) esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t* config) { (void)config; return NULL; }
__attribute__((weak)) esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client) { (void)client; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_http_client_close(esp_http_client_handle_t client) { (void)client; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_http_client_open(esp_http_client_handle_t client, int write_len) { (void)client; (void)write_len; return ESP_OK; }
__attribute__((weak)) int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client) { (void)client; return 0; }
__attribute__((weak)) int esp_http_client_get_status_code(esp_http_client_handle_t client) { (void)client; return 200; }
__attribute__((weak)) int64_t esp_http_client_get_content_length(esp_http_client_handle_t client) { (void)client; return 0; }
__attribute__((weak)) int esp_http_client_read(esp_http_client_handle_t client, char* buffer, int len) {
    (void)client; (void)buffer; (void)len; return 0;
}

// cJSON stubs
__attribute__((weak)) cJSON* cJSON_GetObjectItemCaseSensitive(const cJSON* const object, const char* const name) {
    (void)object; (void)name; return NULL;
}
__attribute__((weak)) cJSON_bool cJSON_IsString(const cJSON* const item) { (void)item; return false; }
__attribute__((weak)) char* cJSON_GetStringValue(const cJSON* const item) { (void)item; return NULL; }
__attribute__((weak)) cJSON_bool cJSON_IsArray(const cJSON* const item) { (void)item; return false; }
__attribute__((weak)) int cJSON_GetArraySize(const cJSON* array) { (void)array; return 0; }
__attribute__((weak)) cJSON* cJSON_GetArrayItem(const cJSON* array, int index) { (void)array; (void)index; return NULL; }
__attribute__((weak)) cJSON* cJSON_Parse(const char* value) { (void)value; return NULL; }
__attribute__((weak)) void cJSON_Delete(cJSON* item) { (void)item; }
__attribute__((weak)) cJSON_bool cJSON_IsNumber(const cJSON* const item) { (void)item; return false; }
__attribute__((weak)) double cJSON_GetNumberValue(const cJSON* const item) { (void)item; return 0.0; }

// minmea stubs
__attribute__((weak)) enum minmea_sentence_id minmea_sentence_id(const char* sentence, bool strict) {
    (void)sentence; (void)strict; return MINMEA_INVALID;
}
__attribute__((weak)) bool minmea_parse_rmc(struct minmea_sentence_rmc* frame, const char* sentence) {
    (void)frame; (void)sentence; return false;
}
__attribute__((weak)) bool minmea_parse_gga(struct minmea_sentence_gga* frame, const char* sentence) {
    (void)frame; (void)sentence; return false;
}

// LVGL screenshot stub
__attribute__((weak)) lv_img_dsc_t* lv_screenshot_create(lv_obj_t* obj, lv_coord_t w, lv_coord_t h) {
    (void)obj; (void)w; (void)h; return NULL;
}

// I2C stubs
__attribute__((weak)) esp_err_t i2c_param_config(i2c_port_t i2c_num, const i2c_config_t* i2c_conf) {
    (void)i2c_num; (void)i2c_conf; return ESP_OK;
}
__attribute__((weak)) esp_err_t i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode, size_t slv_rx_buf_len, size_t slv_tx_buf_len, int intr_alloc_flags) {
    (void)i2c_num; (void)mode; (void)slv_rx_buf_len; (void)slv_tx_buf_len; (void)intr_alloc_flags; return ESP_OK;
}
__attribute__((weak)) esp_err_t i2c_driver_delete(i2c_port_t i2c_num) { (void)i2c_num; return ESP_OK; }
__attribute__((weak)) esp_err_t i2c_master_write_to_device(i2c_port_t i2c_num, uint8_t device_address, const uint8_t* write_buffer, size_t write_size, TickType_t ticks_to_wait) {
    (void)i2c_num; (void)device_address; (void)write_buffer; (void)write_size; (void)ticks_to_wait; return ESP_OK;
}

// ELF loader stubs
__attribute__((weak)) esp_err_t esp_elf_init(void) { return ESP_OK; }
__attribute__((weak)) esp_err_t esp_elf_relocate(const char* elf_path) { (void)elf_path; return ESP_OK; }
__attribute__((weak)) esp_err_t esp_elf_request(const char* request, char** response) {
    (void)request; if (response) *response = NULL; return ESP_OK;
}
__attribute__((weak)) void esp_elf_deinit(void) {}

// RTOS info stubs
__attribute__((weak)) UBaseType_t uxTaskGetSystemState(TaskStatus_t* pxTaskStatusArray, UBaseType_t uxArraySize, uint32_t* pulTotalRunTime) {
    (void)pxTaskStatusArray; (void)uxArraySize; (void)pulTotalRunTime; return 0;
}

// FreeRTOS timer stubs
__attribute__((weak)) TimerHandle_t xTimerCreate(const char* const name, const TickType_t period, const BaseType_t autoReload, void* const id, TimerCallbackFunction_t cb) {
    (void)name; (void)period; (void)autoReload; (void)id; (void)cb; return NULL;
}
__attribute__((weak)) BaseType_t xTimerGenericCommand(TimerHandle_t xTimer, BaseType_t xCommandID, TickType_t xOptionalValue, BaseType_t* const pxHigherPriorityTaskWoken, TickType_t xTicksToWait) {
    (void)xTimer; (void)xCommandID; (void)xOptionalValue; (void)pxHigherPriorityTaskWoken; (void)xTicksToWait; return pdPASS;
}
__attribute__((weak)) BaseType_t xTimerIsTimerActive(TimerHandle_t xTimer) { (void)xTimer; return pdFALSE; }
__attribute__((weak)) TaskHandle_t xTimerGetTimerDaemonTaskHandle(void) { return NULL; }
__attribute__((weak)) void* pvTimerGetTimerID(const TimerHandle_t xTimer) { (void)xTimer; return NULL; }

// minitar stubs
__attribute__((weak)) minitar_t* minitar_open(const char* path) { (void)path; return NULL; }
__attribute__((weak)) void minitar_close(minitar_t* t) { (void)t; }
__attribute__((weak)) int minitar_read_entry(minitar_t* t, minitar_entry_t* entry) { (void)t; (void)entry; return -1; }
__attribute__((weak)) int minitar_read_contents_to_file(minitar_t* t, minitar_entry_t* entry, const char* out_path) {
    (void)t; (void)entry; (void)out_path; return -1;
}

// HTTP server control stubs
__attribute__((weak)) esp_err_t httpd_start(httpd_handle_t* handle, const httpd_config_t* config) {
    (void)handle; (void)config; return ESP_OK;
}
__attribute__((weak)) esp_err_t httpd_stop(httpd_handle_t handle) { (void)handle; return ESP_OK; }
__attribute__((weak)) esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t* uri_handler) {
    (void)handle; (void)uri_handler; return ESP_OK;
}

// Force this translation unit to be linked by referencing it from app_main.
void compat_force_link(void) {
    // no-op
}

// Provide weak global wifi function tables expected by esp_now glue.
__attribute__((weak)) wifi_osi_funcs_t g_wifi_osi_funcs = {0};
__attribute__((weak)) const wpa_crypto_funcs_t g_wifi_default_wpa_crypto_funcs = {0};

// Event base stubs for wifi/netif
__attribute__((weak)) const esp_event_base_t WIFI_EVENT = "WIFI_EVENT";
__attribute__((weak)) const esp_event_base_t IP_EVENT = "IP_EVENT";

