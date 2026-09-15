#pragma once

#include "lvgl.h"
#include "led/RGBLed.hpp"
#include "flash/WinbondFlash.hpp"

class UI
{
public:
    UI() = default;
    ~UI();

   bool init(RGBLed& rgbLed, WinbondFlash& flash);

private:
    RGBLed* rgbLed_ = nullptr;
    WinbondFlash* flash_ = nullptr;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* speedometerPlane_ = nullptr;
    lv_obj_t* speedometerCircle_ = nullptr;

    uint8_t* imageData_ = nullptr;
    lv_image_dsc_t speedometerImageDescriptor_ = {};
    lv_obj_t* speedometerImage_ = nullptr;

    uint8_t* clockImageData_ = nullptr;
    lv_image_dsc_t clockImageDescriptor_{};
    lv_obj_t* clockImage_ = nullptr;

    uint8_t* settingsImageData_ = nullptr;
    lv_image_dsc_t settingsImageDescriptor_{};
    lv_obj_t* settingsImage_ = nullptr;

    uint8_t* weighscaleImageData_ = nullptr;
    lv_image_dsc_t weighscaleImageDescriptor_{};
    lv_obj_t* weighscaleImage_ = nullptr;

};