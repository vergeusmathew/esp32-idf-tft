#include "LVGLDisplay.hpp"

#include <cstring>

bool LVGLDisplay::init(Display& display)
{
    display_ = &display;

    lv_display_ = lv_display_create(
        Display::WIDTH,
        Display::HEIGHT);

    if (lv_display_ == nullptr) {
        return false;
    }

    lv_display_set_color_format(
        lv_display_,
        LV_COLOR_FORMAT_RGB565);

    lv_display_set_flush_cb(
        lv_display_,
        flushCallback);

    /*
     * Use the framebuffer already allocated by our
     * working ESP-IDF RGB panel.
     *
     * LVGL will render directly into it.
     */
    lv_display_set_buffers(
        lv_display_,
        display_->framebuffer(),
        nullptr,
        Display::WIDTH * Display::HEIGHT * sizeof(uint16_t),
        LV_DISPLAY_RENDER_MODE_DIRECT);

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
    /*
     * In DIRECT mode LVGL renders directly into the
     * RGB panel framebuffer, so there is nothing to copy.
     *
     * We only need to tell LVGL that the flush is complete.
     */
    lv_display_flush_ready(display);
}