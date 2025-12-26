#pragma once

#ifdef ESP_PLATFORM

#include <RadioLib.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_timer.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace meshola {

/**
 * Minimal RadioLib HAL for ESP-IDF on ESP32-S3.
 * Implements the required SPI/GPIO/timing hooks using ESP-IDF drivers.
 */
class Esp32S3Hal : public RadioLibHal {
public:
    Esp32S3Hal(int8_t sck, int8_t miso, int8_t mosi, spi_host_device_t host = SPI2_HOST)
    : RadioLibHal(GPIO_MODE_INPUT, GPIO_MODE_OUTPUT,
                  0, 1, GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE),
      _sck(sck), _miso(miso), _mosi(mosi), _host(host) {}

    void init() override { spiBegin(); }
    void term() override { spiEnd(); }

    void pinMode(uint32_t pin, uint32_t mode) override {
        if (pin == RADIOLIB_NC) return;
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = (1ULL << pin);
        cfg.mode = static_cast<gpio_mode_t>(mode);
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) return;
        gpio_set_level(static_cast<gpio_num_t>(pin), value ? 1 : 0);
    }

    uint32_t digitalRead(uint32_t pin) override {
        if (pin == RADIOLIB_NC) return 0;
        return gpio_get_level(static_cast<gpio_num_t>(pin));
    }

    void attachInterrupt(uint32_t pin, void (*interruptCb)(void), uint32_t mode) override {
        if (pin == RADIOLIB_NC) return;
        ensureIsrService();
        gpio_set_intr_type(static_cast<gpio_num_t>(pin), static_cast<gpio_int_type_t>(mode));
        gpio_isr_handler_add(static_cast<gpio_num_t>(pin), reinterpret_cast<gpio_isr_t>(interruptCb), nullptr);
    }

    void detachInterrupt(uint32_t pin) override {
        if (pin == RADIOLIB_NC) return;
        gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
        gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
    }

    void delay(RadioLibTime_t ms) override { vTaskDelay(ms / portTICK_PERIOD_MS); }

    void delayMicroseconds(RadioLibTime_t us) override { esp_rom_delay_us(us); }

    RadioLibTime_t millis() override { return esp_timer_get_time() / 1000; }

    RadioLibTime_t micros() override { return esp_timer_get_time(); }

    long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override {
        if (pin == RADIOLIB_NC) return 0;
        int target = state ? 1 : 0;
        int level = 0;
        uint64_t start = esp_timer_get_time();
        // wait for state to start
        while ((level = gpio_get_level(static_cast<gpio_num_t>(pin))) != target) {
            if (esp_timer_get_time() - start >= timeout) return 0;
        }
        uint64_t pulseStart = esp_timer_get_time();
        // measure until state ends
        while ((level = gpio_get_level(static_cast<gpio_num_t>(pin))) == target) {
            if (esp_timer_get_time() - start >= timeout) break;
        }
        return static_cast<long>(esp_timer_get_time() - pulseStart);
    }

    void spiBegin() override {
        if (_spiInited) return;
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = _mosi;
        buscfg.miso_io_num = _miso;
        buscfg.sclk_io_num = _sck;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 0;
        buscfg.intr_flags = 0;
        esp_err_t err = spi_bus_initialize(_host, &buscfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            return;
        }
        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 0;
        devcfg.clock_speed_hz = 2000000; // 2 MHz for SX1262 default
        devcfg.spics_io_num = -1;        // CS is driven manually by RadioLib
        devcfg.queue_size = 1;
        devcfg.flags = 0;
        if (spi_bus_add_device(_host, &devcfg, &_device) == ESP_OK) {
            _spiInited = true;
        }
    }

    void spiBeginTransaction() override {
        // No-op; spi_device_transmit is already serialized.
    }

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override {
        if (!_spiInited || !_device) return;
        spi_transaction_t t = {};
        t.length = len * 8;
        t.tx_buffer = out;
        t.rx_buffer = in;
        spi_device_transmit(_device, &t);
    }

    void spiEndTransaction() override {
        // No-op
    }

    void spiEnd() override {
        if (_device) {
            spi_bus_remove_device(_device);
            _device = nullptr;
        }
        if (_spiInited) {
            spi_bus_free(_host);
            _spiInited = false;
        }
    }

private:
    void ensureIsrService() {
        static bool installed = false;
        if (!installed) {
            gpio_install_isr_service(0);
            installed = true;
        }
    }

    int8_t _sck;
    int8_t _miso;
    int8_t _mosi;
    spi_host_device_t _host;
    spi_device_handle_t _device = nullptr;
    bool _spiInited = false;
};

} // namespace meshola

#endif // ESP_PLATFORM

