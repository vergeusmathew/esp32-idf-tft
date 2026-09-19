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
    static void speedometerTouchCallback(lv_event_t* e);
    void showTouchHalo(lv_obj_t* icon);
    static void haloAnimationCallback(void* obj, int32_t value);

    lv_obj_t* speedometerHalo_ = nullptr;
    lv_anim_t haloAnim_;

    RGBLed* rgbLed_ = nullptr;
    WinbondFlash* flash_ = nullptr;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* speedometerPlane_ = nullptr;
    lv_obj_t* speedometerCircle_ = nullptr;

    uint8_t* imageData_ = nullptr;
    lv_image_dsc_t speedometerImageDescriptor_ = {};
    lv_obj_t* speedometerImage_ = nullptr;
    lv_obj_t* dialTouchedLabel_ = nullptr;

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