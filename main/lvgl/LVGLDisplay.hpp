#pragma once

#include "lvgl.h"
#include "display/Display.hpp"

class LVGLDisplay
{
public:
    LVGLDisplay() = default;
    ~LVGLDisplay() = default;

    bool init(Display& display);

    lv_display_t* handle() const;

private:
    Display* display_ = nullptr;
    lv_display_t* lv_display_ = nullptr;

    static void flushCallback(
        lv_display_t* display,
        const lv_area_t* area,
        uint8_t* px_map);
};