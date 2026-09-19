#pragma once

#include <stdint.h>
#include "esp_lcd_panel_rgb.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"   // must come before semphr.h
#include "freertos/semphr.h"
#include "esp_attr.h"   // for IRAM_ATTR

#define CL  1
class Display
{
public:
    static constexpr int WIDTH  = 800;
    static constexpr int HEIGHT = 480;

    Display() = default;
    ~Display() = default;

    esp_err_t init();
#ifdef G
    uint16_t *framebuffer();
#endif
#ifdef CL    
    uint16_t *framebuffer();
    uint16_t* framebuffer2();    // add this
#endif
    void fillTestPattern();
#ifdef CL
// Display.hpp
    esp_lcd_panel_handle_t panelHandle() const { return panel_handle_; }
    SemaphoreHandle_t vsyncSemaphore() const { return vsync_sem_; }
#endif

private:
//#ifdef G
    esp_lcd_panel_handle_t panel_handle_ = nullptr;
//#endif

    uint16_t *framebuffer_ = nullptr;
    uint16_t *framebuffer2_ = nullptr;
#ifdef CL
    SemaphoreHandle_t vsync_sem_ = nullptr;

    static bool IRAM_ATTR onVsync(
    esp_lcd_panel_handle_t panel,
    const esp_lcd_rgb_panel_event_data_t* edata,
    void* user_ctx);
#endif
};