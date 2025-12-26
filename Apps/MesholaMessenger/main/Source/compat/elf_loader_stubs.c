#include <esp_err.h>

typedef struct esp_elf {
    void* reserved;
} esp_elf_t;

int esp_elf_init(esp_elf_t* elf) {
    (void)elf;
    return ESP_OK;
}

int esp_elf_relocate(esp_elf_t* elf) {
    (void)elf;
    return ESP_OK;
}

int esp_elf_request(esp_elf_t* elf, const char* name, void** dst, size_t* size) {
    (void)elf;
    (void)name;
    (void)dst;
    (void)size;
    return ESP_ERR_NOT_SUPPORTED;
}

void esp_elf_deinit(esp_elf_t* elf) {
    (void)elf;
}

