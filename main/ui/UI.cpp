#include "UI.hpp"

bool UI::init(RGBLed& rgbLed)
{
    rgbLed_ = &rgbLed;

    screen_ = lv_screen_active();

    if (screen_ == nullptr) {
        return false;
    }

    lv_obj_set_style_bg_color(
        screen_,
        lv_color_hex(0x202020),
        0);

    title_ = lv_label_create(screen_);

    if (title_ == nullptr) {
        return false;
    }

    lv_label_set_text(
        title_,
        "ESP32-S3 + LVGL");

    lv_obj_set_style_text_color(
        title_,
        lv_color_hex(0xFFFFFF),
        0);

    lv_obj_align(
        title_,
        LV_ALIGN_TOP_MID,
        0,
        40);

    status_ = lv_label_create(screen_);

    if (status_ == nullptr) {
        return false;
    }

    lv_label_set_text(
        status_,
        "LVGL display initialized");

    lv_obj_set_style_text_color(
        status_,
        lv_color_hex(0xFFFFFF),
        0);

    lv_obj_align(
        status_,
        LV_ALIGN_CENTER,
        0,
        0);

    /*
     * Receive pointer events from LVGL.
     *
     * We attach the callback to the screen itself so
     * touching anywhere on the display is detected.
     */
    lv_obj_add_event_cb(
        screen_,
        touchEventCallback,
        LV_EVENT_PRESSED,
        this);

    lv_obj_add_event_cb(
        screen_,
        touchEventCallback,
        LV_EVENT_RELEASED,
        this);

    return true;
}

void UI::touchEventCallback(lv_event_t* event)
{
    auto* self =
        static_cast<UI*>(lv_event_get_user_data(event));

    if (self == nullptr ||
        self->rgbLed_ == nullptr) {
        return;
    }

    switch (lv_event_get_code(event)) {

        case LV_EVENT_PRESSED:
            self->rgbLed_->setGreen();

            if (self->status_ != nullptr) {
                lv_label_set_text(
                    self->status_,
                    "TOUCH PRESSED");
            }
            break;

        case LV_EVENT_RELEASED:
            self->rgbLed_->setBlue();

            if (self->status_ != nullptr) {
                lv_label_set_text(
                    self->status_,
                    "TOUCH RELEASED");
            }
            break;

        default:
            break;
    }
}