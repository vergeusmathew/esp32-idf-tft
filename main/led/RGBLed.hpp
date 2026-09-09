#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_tx.h"

class RGBLed
{
public:
    RGBLed() = default;
    ~RGBLed() = default;

    esp_err_t init();

    void setRed();
    void setGreen();
    void setBlue();
    void off();

private:
    void setColor(uint8_t r, uint8_t g, uint8_t b);

private:
    static constexpr int RGB_LED_GPIO = GPIO_NUM_48; /* your existing RGB_LED_GPIO */;

    static constexpr uint32_t WS2812_T0H = 4;
    static constexpr uint32_t WS2812_T0L = 8;
    static constexpr uint32_t WS2812_T1H = 8;
    static constexpr uint32_t WS2812_T1L = 4;

    /*
    // WS2812 timing (10MHz RMT clock = 100ns per tick)
    #define WS2812_T0H  3   // 0.3us
    #define WS2812_T0L  9   // 0.9us
    #define WS2812_T1H  9   // 0.9us
    #define WS2812_T1L  3   // 0.3us
    #define WS2812_RESET 200 // 50us (>50us for WS2812)
    */
    rmt_channel_handle_t rmt_handle_ = nullptr;
    rmt_encoder_handle_t encoder_ = nullptr;

};