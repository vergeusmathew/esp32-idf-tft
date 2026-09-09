#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt_tx.h"
#include "esp_task_wdt.h"

#include "display/Display.hpp"
#include "GT911.hpp"

#define LCD_RST_GPIO GPIO_NUM_4
#define TOUCH_SDA    GPIO_NUM_1
#define TOUCH_SCL    GPIO_NUM_2
#define TOUCH_IRQ    GPIO_NUM_47
#define RGB_LED_GPIO GPIO_NUM_48

static const char *TAG = "TFT_TEST";

// Display Resolution
#define LCD_H_RES              800
#define LCD_V_RES              480

// Backlight PWM Setup
#define BACKLIGHT_PWM_PIN       GPIO_NUM_3
#define BACKLIGHT_PWM_CHAN      LEDC_CHANNEL_0
#define BACKLIGHT_PWM_TK        LEDC_TIMER_0
#define BACKLIGHT_PWM_FREQ      5000
#define BACKLIGHT_PWM_RES       LEDC_TIMER_8_BIT

// WS2812 timing (10MHz RMT clock = 100ns per tick)
#define WS2812_T0H  3   // 0.3us
#define WS2812_T0L  9   // 0.9us
#define WS2812_T1H  9   // 0.9us
#define WS2812_T1L  3   // 0.3us
#define WS2812_RESET 200 // 50us (>50us for WS2812)

static rmt_channel_handle_t rmt_handle_ = nullptr;
static rmt_encoder_handle_t encoder_ = nullptr;

// ---------------------------------------------------------------------------
// Backlight
// ---------------------------------------------------------------------------

static void initBacklight()
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num = BACKLIGHT_PWM_TK;
    ledc_timer.duty_resolution = BACKLIGHT_PWM_RES;
    ledc_timer.freq_hz = BACKLIGHT_PWM_FREQ;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num = BACKLIGHT_PWM_PIN;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = BACKLIGHT_PWM_CHAN;
    ledc_channel.timer_sel = BACKLIGHT_PWM_TK;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHAN, 77);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHAN);

    ESP_LOGI(TAG, "Backlight initialized");
}

// ---------------------------------------------------------------------------
// WS2812 RGB LED
// ---------------------------------------------------------------------------

static void initRgbLed()
{
    rmt_tx_channel_config_t tx_config = {};
    tx_config.gpio_num = RGB_LED_GPIO;
    tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_config.resolution_hz = 10 * 1000 * 1000; // 10MHz
    tx_config.mem_block_symbols = 48;
    tx_config.trans_queue_depth = 4;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_config, &rmt_handle_));

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
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_config, &encoder_));
    ESP_ERROR_CHECK(rmt_enable(rmt_handle_));

    ESP_LOGI(TAG, "RGB LED initialized on GPIO%d", RGB_LED_GPIO);
}

static void setRgbColor(uint8_t r, uint8_t g, uint8_t b)
{
    // WS2812 order: Green, Red, Blue
    uint8_t data[3] = { g, r, b };
    rmt_transmit_config_t tx_config = {};
    ESP_ERROR_CHECK(rmt_transmit(rmt_handle_, encoder_, data, 3, &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(rmt_handle_, 100));
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 09,2026 14:05...");

    initBacklight();
    initRgbLed();

    // Power ON: Red
    setRgbColor(32, 0, 0);

    // Reset the display controller via RST pin
    ESP_LOGI(TAG, "Resetting display...");
    gpio_set_direction(LCD_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    ESP_LOGI(TAG, "Creating Display");
    Display display;

    ESP_LOGI(TAG, "Initializing Display");
    ESP_ERROR_CHECK(display.init());

    display.fillTestPattern();

    // Initialize GT911 Touch Controller
    ESP_LOGI(TAG, "Initializing Touch...");

    GT911 touch;

    esp_err_t touch_ret = touch.init(TOUCH_SDA, TOUCH_SCL, LCD_RST_GPIO, TOUCH_IRQ);
    

    // Main loop: only read touch when IRQ signals data ready
    while (1)
    {
        if (touch_ret == ESP_OK)
        {
            esp_err_t err = touch.read();

            if (err == ESP_OK)
            {
                if (touch.isTouched())
                {
                    setRgbColor(0, 32, 0);

                    ESP_LOGI(
                        TAG,
                        "Touch: count=%d raw=(%d,%d) disp=(%d,%d)",
                        touch.touchCount(),
                        touch.rawX(),
                        touch.rawY(),
                        touch.displayX(),
                        touch.displayY()
                    );
                }
                else
                {
                    setRgbColor(0, 0, 32);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
