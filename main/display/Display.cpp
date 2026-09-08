#include "Display.hpp"

#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

#include "driver/gpio.h"

// -----------------------------------------------------------------------------
// Display GPIO definitions
// -----------------------------------------------------------------------------

#define R_GPIO_3  GPIO_NUM_13
#define R_GPIO_4  GPIO_NUM_12
#define R_GPIO_5  GPIO_NUM_11
#define R_GPIO_6  GPIO_NUM_10
#define R_GPIO_7  GPIO_NUM_9

#define G_GPIO_2  GPIO_NUM_21
#define G_GPIO_3  GPIO_NUM_18
#define G_GPIO_4  GPIO_NUM_17
#define G_GPIO_5  GPIO_NUM_16
#define G_GPIO_6  GPIO_NUM_15
#define G_GPIO_7  GPIO_NUM_14

#define B_GPIO_3  GPIO_NUM_42
#define B_GPIO_4  GPIO_NUM_41
#define B_GPIO_5  GPIO_NUM_40
#define B_GPIO_6  GPIO_NUM_39
#define B_GPIO_7  GPIO_NUM_38

#define PCLK_GPIO  GPIO_NUM_5
#define VSYNC_GPIO GPIO_NUM_6
#define HSYNC_GPIO GPIO_NUM_7
#define DE_GPIO    GPIO_NUM_8

static const char *TAG = "DISPLAY";

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

esp_err_t Display::init()
{
    esp_lcd_rgb_panel_config_t rgb_config = {};

    rgb_config.data_width = 16;
    rgb_config.clk_src = LCD_CLK_SRC_DEFAULT;

    // No separate DISP control GPIO.
    rgb_config.disp_gpio_num = GPIO_NUM_NC;

    rgb_config.pclk_gpio_num = PCLK_GPIO;
    rgb_config.vsync_gpio_num = VSYNC_GPIO;
    rgb_config.hsync_gpio_num = HSYNC_GPIO;
    rgb_config.de_gpio_num = DE_GPIO;

    // -------------------------------------------------------------------------
    // Validated 800x480 RGB565 timing
    // -------------------------------------------------------------------------

    rgb_config.timings.pclk_hz = 40 * 1000 * 1000;
    rgb_config.timings.h_res = WIDTH;
    rgb_config.timings.v_res = HEIGHT;

    rgb_config.timings.hsync_back_porch = 7;
    rgb_config.timings.hsync_front_porch = 4;
    rgb_config.timings.hsync_pulse_width = 30;

    rgb_config.timings.vsync_back_porch = 15;
    rgb_config.timings.vsync_front_porch = 22;
    rgb_config.timings.vsync_pulse_width = 10;

    rgb_config.timings.flags.pclk_active_neg = true;
    rgb_config.timings.flags.hsync_idle_low = true;
    rgb_config.timings.flags.vsync_idle_low = true;
    rgb_config.timings.flags.de_idle_high = true;

    // -------------------------------------------------------------------------
    // RGB565
    // -------------------------------------------------------------------------

    rgb_config.in_color_format = LCD_COLOR_FMT_RGB565;

    // Keep the validated DMA/bounce settings.
    rgb_config.dma_burst_size = 64;
    rgb_config.bounce_buffer_size_px = WIDTH * 20;

    // Framebuffer lives in PSRAM.
    rgb_config.flags.fb_in_psram = 1;

    // -------------------------------------------------------------------------
    // Initialize all RGB data GPIOs as disconnected first.
    // -------------------------------------------------------------------------

    for (int i = 0; i < ESP_LCD_RGB_BUS_WIDTH_MAX; ++i) {
        rgb_config.data_gpio_nums[i] = GPIO_NUM_NC;
    }

    // Blue
    rgb_config.data_gpio_nums[0] = B_GPIO_3;
    rgb_config.data_gpio_nums[1] = B_GPIO_4;
    rgb_config.data_gpio_nums[2] = B_GPIO_5;
    rgb_config.data_gpio_nums[3] = B_GPIO_6;
    rgb_config.data_gpio_nums[4] = B_GPIO_7;

    // Green
    rgb_config.data_gpio_nums[5]  = G_GPIO_2;
    rgb_config.data_gpio_nums[6]  = G_GPIO_3;
    rgb_config.data_gpio_nums[7]  = G_GPIO_4;
    rgb_config.data_gpio_nums[8]  = G_GPIO_5;
    rgb_config.data_gpio_nums[9]  = G_GPIO_6;
    rgb_config.data_gpio_nums[10] = G_GPIO_7;

    // Red
    rgb_config.data_gpio_nums[11] = R_GPIO_3;
    rgb_config.data_gpio_nums[12] = R_GPIO_4;
    rgb_config.data_gpio_nums[13] = R_GPIO_5;
    rgb_config.data_gpio_nums[14] = R_GPIO_6;
    rgb_config.data_gpio_nums[15] = R_GPIO_7;

    // -------------------------------------------------------------------------
    // Create and initialize RGB panel
    // -------------------------------------------------------------------------

    ESP_ERROR_CHECK(
        esp_lcd_new_rgb_panel(&rgb_config, &panel_handle_)
    );

    ESP_ERROR_CHECK(
        esp_lcd_panel_reset(panel_handle_)
    );

    ESP_ERROR_CHECK(
        esp_lcd_panel_init(panel_handle_)
    );

    // -------------------------------------------------------------------------
    // Get framebuffer
    // -------------------------------------------------------------------------

    ESP_ERROR_CHECK(
        esp_lcd_rgb_panel_get_frame_buffer(
            panel_handle_,
            1,
            reinterpret_cast<void **>(&framebuffer_)
        )
    );

    ESP_LOGI(TAG, "Display initialized: %dx%d RGB565", WIDTH, HEIGHT);

    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Framebuffer access
// -----------------------------------------------------------------------------

uint16_t *Display::framebuffer()
{
    return framebuffer_;
}

// -----------------------------------------------------------------------------
// Test pattern
// -----------------------------------------------------------------------------

void Display::fillTestPattern()
{
    if (framebuffer_ == nullptr) {
        ESP_LOGE(TAG, "Framebuffer is not initialized");
        return;
    }

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {

            if (x < 80) {
                framebuffer_[y * WIDTH + x] = 0x8410;
            }
            else if (x < 160) {
                framebuffer_[y * WIDTH + x] = 0xFFE0;
            }
            else if (x < 240) {
                framebuffer_[y * WIDTH + x] = 0x07FF;
            }
            else if (x < 320) {
                framebuffer_[y * WIDTH + x] = 0x07E0;
            }
            else if (x < 400) {
                framebuffer_[y * WIDTH + x] = 0xF8B2;
            }
            else if (x < 480) {
                framebuffer_[y * WIDTH + x] = 0xFD20;
            }
            else if (x < 560) {
                framebuffer_[y * WIDTH + x] = 0xF800;
            }
            else if (x < 640) {
                framebuffer_[y * WIDTH + x] = 0x867D;
            }
            else if (x < 720) {
                framebuffer_[y * WIDTH + x] = 0x001F;
            }
            else {
                framebuffer_[y * WIDTH + x] = 0xFFFF;
            }
        }
    }

    ESP_LOGI(TAG, "Test pattern written to framebuffer");
}