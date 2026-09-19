#include "LVGLDisplay.hpp"

#include <cstring>
#include "esp_log.h"
#include "esp_timer.h"
#define CL  1

bool LVGLDisplay::init(Display& display)
{
    ESP_LOGI("LVGLDisplay", "init() START");
    display_ = &display;

    ESP_LOGI("LVGLDisplay", "Before lv_display_create()");
    lv_display_ = lv_display_create(
        Display::WIDTH,
        Display::HEIGHT);

    ESP_LOGI(
        "LVGLDisplay",
        "After lv_display_create(): %p",
        lv_display_);

    if (!lv_display_)
    {
        ESP_LOGE(
            "LVGLDisplay",
            "lv_display_create() failed");

        return false;
    }

    if (lv_display_ == nullptr) {
        return false;
    }

    //lv_display_set_user_data(lv_display_, display_->panelHandle());

    ESP_LOGI(
        "LVGLDisplay",
        "Before lv_display_set_color_format()");

    lv_display_set_color_format(
        lv_display_,
        LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(
        "LVGLDisplay",
        "After lv_display_set_color_format()");

    ESP_LOGI(
        "LVGLDisplay",
        "Before lv_display_set_flush_cb()");

    lv_display_set_flush_cb(
        lv_display_,
        flushCallback);

     ESP_LOGI(
        "LVGLDisplay",
        "After lv_display_set_flush_cb()");

    /*
     * Use the framebuffer already allocated by our
     * working ESP-IDF RGB panel.
     *
     * LVGL will render directly into it.
     */
    void* framebuffer = display_->framebuffer();

    ESP_LOGI(
        "LVGLDisplay",
        "Framebuffer address: %p",
        framebuffer);

    ESP_LOGI(
        "LVGLDisplay",
        "Before lv_display_set_buffers()");

    lv_display_set_buffers(
        lv_display_,
        display_->framebuffer(),
#ifdef CL        
        display_->framebuffer2(),
#endif
#ifdef G        
        nullptr,
#endif        
        Display::WIDTH * Display::HEIGHT * sizeof(uint16_t),
        LV_DISPLAY_RENDER_MODE_DIRECT);

        flush_ctx_.panel_handle = display_->panelHandle();
        flush_ctx_.vsync_sem    = display_->vsyncSemaphore();
        lv_display_set_user_data(lv_display_, &flush_ctx_);

    ESP_LOGI(
        "LVGLDisplay",
        "After lv_display_set_buffers()");

    ESP_LOGI("LVGLDisplay", "init() END");

    return true;
}

lv_display_t* LVGLDisplay::handle() const
{
    return lv_display_;
}

void LVGLDisplay::flushCallback(
    lv_display_t* display,
    const lv_area_t* area,
    uint8_t* px_map)
{
    auto* ctx = static_cast<FlushContext*>(lv_display_get_user_data(display));

    int64_t t0 = esp_timer_get_time();
    xSemaphoreTake(ctx->vsync_sem, portMAX_DELAY);
    int64_t wait_us = esp_timer_get_time() - t0;

    ctx->flush_count++;
    ctx->total_wait_us += wait_us;
    if (wait_us > ctx->max_wait_us) {
        ctx->max_wait_us = wait_us;
    }

    // Log anything suspicious immediately, not just periodically:
    // a wait longer than ~1 full frame period (≈16-20ms at typical refresh)
    // suggests a missed/late vsync rather than normal blanking-window timing.
    if (wait_us > 20000) {
        ESP_LOGW("LVGLDisplay", "Long vsync wait: %lld us (flush #%lu)",
                 (long long)wait_us, (unsigned long)ctx->flush_count);
    }

    esp_lcd_panel_draw_bitmap(
        ctx->panel_handle,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        px_map);

    lv_display_flush_ready(display);
    /*
#ifdef CL
    esp_lcd_panel_handle_t panel_handle =
        static_cast<esp_lcd_panel_handle_t>(lv_display_get_user_data(display));

    esp_lcd_panel_draw_bitmap(
        panel_handle,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        px_map);

    lv_display_flush_ready(display);
#endif
#ifdef G
    //In DIRECT mode LVGL renders directly into the
    // RGB panel framebuffer, so there is nothing to copy.
    // We only need to tell LVGL that the flush is complete.
    lv_display_flush_ready(display);
#endif    
      */
}