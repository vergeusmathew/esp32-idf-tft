#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "display/Display.hpp"

#define LCD_RST_GPIO GPIO_NUM_4  // Your RST pin

static const char *TAG = "TFT_TEST";

#define WHITE               0xFFFF
#define RED                 0xF800
#define GREEN               0x07E0
#define BLUE                0x001F    
// Display Resolution
#define LCD_H_RES              800
#define LCD_V_RES              480

// Backlight PWM Setup
#define BACKLIGHT_PWM_PIN       GPIO_NUM_3
#define BACKLIGHT_PWM_CHAN      LEDC_CHANNEL_0
#define BACKLIGHT_PWM_TK        LEDC_TIMER_0
#define BACKLIGHT_PWM_FREQ      5000
#define BACKLIGHT_PWM_RES       LEDC_TIMER_8_BIT

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


extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 08,2026 14:47...");

    initBacklight();

    ESP_LOGI(TAG, "Creating Display");

    Display display;

    ESP_LOGI(TAG, "Initializing Display");

    ESP_ERROR_CHECK(display.init());

    display.fillTestPattern();

#if 0    // vertical 10 strips
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {

            if (x < 80) {
                fb0[y * 800 + x] = 0x8410;   // Grey
            }
            else if (x < 160) {
                fb0[y * 800 + x] = 0xFFE0;   // YELLOW
            }
            else if (x < 240) {
                fb0[y * 800 + x] = 0x0451;   // 0x07FF;   // CYAN
            }
            else if (x < 320) {
                fb0[y * 800 + x] = 0x07E0;   // Green
            }
            else if (x < 400) {
                fb0[y * 800 + x] = 0xF8B2;   // Pink
            }
            else if (x < 480) {
                fb0[y * 800 + x] = 0xFD20;   // Orange 
            }
            else if (x < 560) {
                fb0[y * 800 + x] = 0xF800;   // Red
            }
            else if (x < 640) {
                fb0[y * 800 + x] = 0x867D;   // Sky Blue
            }
            else if (x < 720) {
                fb0[y * 800 + x] = 0x001F;   // Blue
            }
            else {
                fb0[y * 800 + x] = 0xFFFF;   // WHITE
            }
        }
    }
#endif
     
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
