#pragma once

#include "lvgl.h"
#include "display/Display.hpp"
#include "freertos/FreeRTOS.h"   // must come first
#include "freertos/semphr.h"
#include "esp_lcd_panel_ops.h"

class LVGLDisplay
{
public:
    LVGLDisplay() = default;
    ~LVGLDisplay() = default;

    bool init(Display& display);

    lv_display_t* handle() const;
    //const FlushContext& flushContext() const { return flush_ctx_; }
    private:
    struct FlushContext {
        esp_lcd_panel_handle_t panel_handle;
        SemaphoreHandle_t vsync_sem;
        uint32_t flush_count = 0;
        int64_t  max_wait_us = 0;
        int64_t  total_wait_us = 0;
    };

    public:
        const FlushContext& flushContext() const { return flush_ctx_; }   // now comes after the struct is known


private:
    Display* display_ = nullptr;
    lv_display_t* lv_display_ = nullptr;

    static void flushCallback(
        lv_display_t* display,
        const lv_area_t* area,
        uint8_t* px_map);
/*
    struct FlushContext {
        esp_lcd_panel_handle_t panel_handle;
        SemaphoreHandle_t vsync_sem;
    };
    */
    FlushContext flush_ctx_{};
};