#pragma once

#include "lvgl.h"
#include "touch/GT911.hpp"

class LVGLTouch
{
public:
    LVGLTouch() = default;
    ~LVGLTouch() = default;

    bool init(GT911& touch);

    lv_indev_t* handle() const;

private:
    GT911* touch_ = nullptr;
    lv_indev_t* indev_ = nullptr;

    static void readCallback(
        lv_indev_t* indev,
        lv_indev_data_t* data);
};