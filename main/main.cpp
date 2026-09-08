#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#define LCD_RST_GPIO GPIO_NUM_4  // Your RST pin

static const char *TAG = "TFT_TEST";

#define WHITE               0xFFFF
#define RED                 0xF800
#define GREEN               0x07E0
#define BLUE                0x001F    
// Display Resolution
#define LCD_H_RES              800
#define LCD_V_RES              480

// --- PIN MAPPING (From your ESP32-S3 list) ---
// Red Pins
#define R_GPIO_7  GPIO_NUM_9
#define R_GPIO_6  GPIO_NUM_10
#define R_GPIO_5  GPIO_NUM_11
#define R_GPIO_4  GPIO_NUM_12
#define R_GPIO_3  GPIO_NUM_13

// Green Pins
#define G_GPIO_7  GPIO_NUM_14
#define G_GPIO_6  GPIO_NUM_15
#define G_GPIO_5  GPIO_NUM_16
#define G_GPIO_4  GPIO_NUM_17
#define G_GPIO_3  GPIO_NUM_18
#define G_GPIO_2  GPIO_NUM_21

// Blue Pins
#define B_GPIO_7  GPIO_NUM_38
#define B_GPIO_6  GPIO_NUM_39
#define B_GPIO_5  GPIO_NUM_40
#define B_GPIO_4  GPIO_NUM_41
#define B_GPIO_3  GPIO_NUM_42

// Sync/Timing Pins
#define PCLK_GPIO  GPIO_NUM_5
#define VSYNC_GPIO GPIO_NUM_6
#define HSYNC_GPIO GPIO_NUM_7
#define DE_GPIO    GPIO_NUM_8

// Backlight PWM Setup
#define BACKLIGHT_PWM_PIN       GPIO_NUM_3
#define BACKLIGHT_PWM_CHAN      LEDC_CHANNEL_0
#define BACKLIGHT_PWM_TK        LEDC_TIMER_0
#define BACKLIGHT_PWM_FREQ      5000
#define BACKLIGHT_PWM_RES       LEDC_TIMER_8_BIT


extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 08,2026 14:47...");

    // 1. PWM Timer Configuration
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num        = BACKLIGHT_PWM_TK;
    ledc_timer.duty_resolution  = BACKLIGHT_PWM_RES;
    ledc_timer.freq_hz          = BACKLIGHT_PWM_FREQ;
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. PWM Channel Configuration (Cleaned for v6.1)
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num       = BACKLIGHT_PWM_PIN;
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = BACKLIGHT_PWM_CHAN;
    ledc_channel.timer_sel      = BACKLIGHT_PWM_TK;
    ledc_channel.duty           = 0; // Logic: 0 duty = Transistor ON = PWM Pin GND = Backlight OFF
    ledc_channel.hpoint         = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

     // 1. Manually Reset the Display Chip
    /* 
    gpio_set_direction(LCD_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST_GPIO, 0); // Pull RST low
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait 100ms
    gpio_set_level(LCD_RST_GPIO, 1);  // Pull RST high
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait 100ms for chip to stabilize

    ESP_LOGI(TAG, "Reset sequence complete. Initializing RGB driver...");
    */
    esp_lcd_panel_handle_t panel_handle = NULL;

    esp_lcd_rgb_panel_config_t rgb_config = {};
    rgb_config.data_width = 16;
    rgb_config.clk_src = LCD_CLK_SRC_DEFAULT;
    rgb_config.disp_gpio_num = GPIO_NUM_NC;
    rgb_config.pclk_gpio_num = PCLK_GPIO;
    rgb_config.vsync_gpio_num = VSYNC_GPIO;
    rgb_config.hsync_gpio_num = HSYNC_GPIO;
    rgb_config.de_gpio_num = DE_GPIO;
    rgb_config.timings.pclk_hz = 40 * 1000 * 1000;   // does not work for value below 40
    rgb_config.timings.h_res = LCD_H_RES;
    rgb_config.timings.v_res = LCD_V_RES;
    rgb_config.timings.hsync_back_porch = 7;  //onverted: (STM32 HBP 46 - HSync 30)
    rgb_config.timings.hsync_front_porch = 4; //44 and 39 also works 
    rgb_config.timings.hsync_pulse_width = 30;

    rgb_config.timings.vsync_back_porch = 15;   // Converted: (STM32 VBP 23 - VSync 10)
    rgb_config.timings.vsync_front_porch = 22;
    rgb_config.timings.vsync_pulse_width = 10;  // Direct Map: (STM32 VSync)
    rgb_config.timings.flags.pclk_active_neg = true; // Sampling on falling clock edge
    rgb_config.timings.flags.hsync_idle_low = true;  // Match STM32 Active Low HSYNC
    rgb_config.timings.flags.vsync_idle_low = true;  // Match STM32 Active Low VSYNC
    rgb_config.timings.flags.de_idle_high = true;    // Match STM32 Active Low DE behavior (Idles high, active low)

    rgb_config.in_color_format = LCD_COLOR_FMT_RGB565; 
    rgb_config.dma_burst_size = 64;                    
    rgb_config.bounce_buffer_size_px = LCD_H_RES * 20;
    rgb_config.flags.fb_in_psram = 1;
   

    // Instead of using the struct members, we will set the array elements directly.
    // We must initialize the array with -1 (not connected) first to avoid junk values.
    for (int i = 0; i < ESP_LCD_RGB_BUS_WIDTH_MAX; i++) {
        rgb_config.data_gpio_nums[i] = GPIO_NUM_NC;
    }
    
    // Red (R0-R4)
    rgb_config.data_gpio_nums[0] = B_GPIO_3;
    rgb_config.data_gpio_nums[1] = B_GPIO_4;
    rgb_config.data_gpio_nums[2] = B_GPIO_5;
    rgb_config.data_gpio_nums[3] = B_GPIO_6;
    rgb_config.data_gpio_nums[4] = B_GPIO_7;

    // Green (D5 - D10)
    rgb_config.data_gpio_nums[5] = G_GPIO_2;
    rgb_config.data_gpio_nums[6] = G_GPIO_3;
    rgb_config.data_gpio_nums[7] = G_GPIO_4;
    rgb_config.data_gpio_nums[8] = G_GPIO_5;
    rgb_config.data_gpio_nums[9] = G_GPIO_6;
    rgb_config.data_gpio_nums[10] = G_GPIO_7;

    // Red (D11 - D15)
    rgb_config.data_gpio_nums[11] = R_GPIO_3;
    rgb_config.data_gpio_nums[12] = R_GPIO_4;
    rgb_config.data_gpio_nums[13] = R_GPIO_5;
    rgb_config.data_gpio_nums[14] = R_GPIO_6;
    rgb_config.data_gpio_nums[15] = R_GPIO_7;

    // Initialize the panel
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Allocate buffer in PSRAM
    // 5. Fetch the auto-allocated frame buffer pointer from the driver
    uint16_t *fb0 = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, (void **)&fb0));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHAN, 77); // towards zero dim
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHAN);
    
    // Fill with Sky Blue
#if 0    
    //uint32_t sky_blue =  0x867D;
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) {
        fb0[i] = 0xF800;  //Red
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 2. 100x100 BLUE square in the center
    for (int y = 190; y < 290; y++) {
        for (int x = 350; x < 450; x++) {
            fb0[y * 800 + x] = 0x001F;
        }
    }
#endif

#if 0
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 3. Change square back to RED
    for (int y = 190; y < 290; y++) {
        for (int x = 350; x < 450; x++) {
            fb0[y * 800 + x] = 0xF800;
        }
    }    

    // Top half RED, bottom half BLUE
    for (int y = 0; y < 480; y++) {
        uint16_t color = (y < 240) ? 0xF800 : 0x867D;

        for (int x = 0; x < 800; x++) {
            fb0[y * 800 + x] = color;
        }
    }
#endif   
#if 0   //VIBGYOR
    const uint16_t colors[8] = {
        /*
        0x801F,  // Violet
        0x4810,  // Indigo
        0x001F,  // Blue
        0x07E0,  // Green
        0xFFE0,  // Yellow
        0xFD20,  // Orange
        0xF800,  // Red
        0xFFFF   // White
        */
        0xFFFF,  // White
        0xFFE0,  // Yellow
        0x07FF,  // Cyan
        0x07E0,  // Green
        0xF8B2,  // Pink  0x8010,  // Purple
        0xF800,  // Red
        0x001F,  // Blue
        0x8410   // Grey
    };

    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {

            int strip = x / 100;
            fb0[y * 800 + x] = colors[strip];
        }
    }

#endif

#if 0
/*
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {
            if (x < 10) {
                fb0[y * 800 + x] = 0xFFFF;   // WHITE: first 10 pixels
            } else if (x < 20) {
                fb0[y * 800 + x] = 0xF800;   // RED: next 10 pixels
            } else {
                fb0[y * 800 + x] = 0x001F;   // BLUE: rest
            }
        }
    }
        */
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {
            if (x < 10) {
                fb0[(y * 800) + x] = 0xFFFF;   // WHITE: first 10 pixels
            } else if( (x >= 10) && (x < 20) ){
                fb0[(y * 800) + x] = 0xF800;   // RED: next 10 pixels
            } else if (x >= 20){
                fb0[(y * 800) + x] = 0x001F;   // BLUE: rest
            }
        }
    }
#endif

#if 0

    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {
            fb0[(y * 800) + x] = (x < 400) ? 0xF800 : 0x001F;
        }
    }
/*
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 400; x++) {
            fb0[y * 800 + x] = 0xF800;
        }
    }
        for (int y = 0; y < 480; y++) {
        for (int x = 400; x < 800; x++) {
            fb0[y * 800 + x] = 0x001F;
        }
    }
*/        
#endif

#if 0
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {

            if (x < 200) {
                fb0[y * 800 + x] = 0xF800;   // RED
            }
            else if (x < 600) {
                fb0[y * 800 + x] = 0x07E0;   // GREEN
            }
            else {
                fb0[y * 800 + x] = 0x001F;   // BLUE
            }
        }
    }
#endif
#if 0   // testing left right boundaries
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {

            if (x < 10) {
                fb0[y * 800 + x] = 0xFFFF;   // WHITE
            }
            else if (x < 20) {
                fb0[y * 800 + x] = 0xF800;   // RED
            }
            else if (x < 780) {
                fb0[y * 800 + x] = 0x07E0;   // GREEN
            }
            else if (x < 790) {
                fb0[y * 800 + x] = 0x001F;   // Blue
            }
            else {
                fb0[y * 800 + x] = 0xFFFF;   // WHITE
            }
        }
    }
#endif
#if 1    // vertical 10 strips
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
#if 0    // testing top bottom boundaries
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 800; x++) {

            uint16_t color;

            if (y < 10) {
                color = WHITE;
            } else if (y < 20) {
                color = RED;
            } else if (y < 460) {
                color = GREEN;
            } else if (y < 470) {
                color = BLUE;
            } else {
                color = WHITE;
            }

            fb0[y * 800 + x] = color;
        }
    }
#endif


    // Draw to screen
    ESP_LOGI(TAG, "Drawing Sky Blue to screen...");
   
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
