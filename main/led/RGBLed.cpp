#include "RGBLed.hpp"

#include "esp_log.h"

static const char* TAG = "RGBLed";

esp_err_t RGBLed::init()
{
    rmt_tx_channel_config_t tx_config = {};

    tx_config.gpio_num = static_cast<gpio_num_t>(RGB_LED_GPIO);
    tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_config.resolution_hz = 10 * 1000 * 1000;
    tx_config.mem_block_symbols = 48;
    tx_config.trans_queue_depth = 4;

    esp_err_t err = rmt_new_tx_channel(
        &tx_config,
        &rmt_handle_);

    if (err != ESP_OK) {
        return err;
    }

    rmt_bytes_encoder_config_t enc_config = {};

    enc_config.bit0.duration0 = WS2812_T0H;
    enc_config.bit0.duration1 = WS2812_T0L;

    enc_config.bit1.duration0 = WS2812_T1H;
    enc_config.bit1.duration1 = WS2812_T1L;

    enc_config.bit0.level0 = 1;
    enc_config.bit0.level1 = 0;

    enc_config.bit1.level0 = 1;
    enc_config.bit1.level1 = 0;

    enc_config.flags.msb_first = 1;

    err = rmt_new_bytes_encoder(
        &enc_config,
        &encoder_);

    if (err != ESP_OK) {
        return err;
    }

    err = rmt_enable(rmt_handle_);

    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(
        TAG,
        "RGB LED initialized on GPIO%d",
        RGB_LED_GPIO);

    setRed();
    //setGreen();

    return ESP_OK;
}

void RGBLed::setColor(
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    // WS2812 byte order: Green, Red, Blue
    uint8_t data[3] = {
        g,
        r,
        b
    };

    rmt_transmit_config_t tx_config = {};

    ESP_ERROR_CHECK(
        rmt_transmit(
            rmt_handle_,
            encoder_,
            data,
            sizeof(data),
            &tx_config));

    ESP_ERROR_CHECK(
        rmt_tx_wait_all_done(
            rmt_handle_,
            100));
}

void RGBLed::setRed()
{
    setColor(255, 0, 0);
}

void RGBLed::setGreen()
{
    setColor(0, 255, 0);
}

void RGBLed::setBlue()
{
    setColor(0, 0, 255);
}

void RGBLed::off()
{
    setColor(0, 0, 0);
}