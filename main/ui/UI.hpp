#pragma once

#include "lvgl.h"
#include "led/RGBLed.hpp"

class UI
{
public:
    UI() = default;
    ~UI() = default;

    bool init(RGBLed& rgbLed);

private:
    static void touchEventCallback(
        lv_event_t* event);

private:
    RGBLed* rgbLed_ = nullptr;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* status_ = nullptr;
};