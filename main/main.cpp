#include "esp_timer.h"
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

#include "lvgl.h"

#include "display/Display.hpp"
#include "touch/GT911.hpp"
#include "lvgl/LVGLDisplay.hpp"
#include "lvgl/LVGLTouch.hpp"
#include "led/RGBLed.hpp"
#include "ui/UI.hpp"

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
// Main
// ---------------------------------------------------------------------------

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 09,2026 15:17...");

    initBacklight();
    //initRgbLed();

    // Power ON: Red
    //setRgbColor(32, 0, 0);

    // Reset the display controller via RST pin
    ESP_LOGI(TAG, "Resetting display...");

    gpio_set_direction(LCD_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // ---------------------------------------------------------
    // Display
    // ---------------------------------------------------------

    ESP_LOGI(TAG, "Creating Display");

    Display display;

    ESP_LOGI(TAG, "Initializing Display");

    ESP_ERROR_CHECK(display.init());

    RGBLed rgbLed;

    if (rgbLed.init() != ESP_OK) {
        ESP_LOGE(TAG, "RGB LED initialization failed");
        return;
    }
    // ---------------------------------------------------------
    // LVGL
    // ---------------------------------------------------------

    ESP_LOGI(TAG, "Initializing LVGL");

    lv_init();

    lv_tick_set_cb([]() -> uint32_t {
        return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    });

    LVGLDisplay lvglDisplay;

    if (!lvglDisplay.init(display))
    {
        ESP_LOGE(TAG, "LVGL display initialization failed");
        return;
    }

    ESP_LOGI(TAG, "LVGL display initialized");
#if 0
    lv_obj_t* screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0xFF0000),
        0);

    lv_refr_now(lvglDisplay.handle());

    ESP_LOGI(TAG, "Red screen requested");
#endif

    // ---------------------------------------------------------
    // UI
    // ---------------------------------------------------------
#if 1
    UI ui;

    if (!ui.init(rgbLed))
    {
        ESP_LOGE(TAG, "UI initialization failed");
        return;
    }

    ESP_LOGI(TAG, "UI initialized");
#endif
    // ---------------------------------------------------------
    // Touch
    // ---------------------------------------------------------
#if 1
    ESP_LOGI(TAG, "Initializing Touch...");

    GT911 touch;

    esp_err_t touch_ret =
        touch.init(
            TOUCH_SDA,
            TOUCH_SCL,
            LCD_RST_GPIO,
            TOUCH_IRQ);

    if (touch_ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "GT911 initialization failed: %s",
            esp_err_to_name(touch_ret));
    }
    else
    {
        ESP_LOGI(TAG, "GT911 initialized");
    }
#endif
    LVGLTouch lvglTouch;

    if (!lvglTouch.init(touch)) {
        ESP_LOGE(TAG, "LVGL touch initialization failed");
        return;
    }
    // ---------------------------------------------------------
    // Main LVGL loop
    // ---------------------------------------------------------

    while (1)
    {
        //ESP_LOGI(TAG, "LVGL handler START");

        uint32_t start = esp_log_timestamp();

        lv_timer_handler();

        uint32_t elapsed = esp_log_timestamp() - start;

        //ESP_LOGI(TAG, "LVGL handler END: %lu ms",
        //        static_cast<unsigned long>(elapsed));

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


#if 0
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
#endif