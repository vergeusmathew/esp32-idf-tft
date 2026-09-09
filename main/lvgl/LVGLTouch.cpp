#include "LVGLTouch.hpp"

bool LVGLTouch::init(GT911& touch)
{
    touch_ = &touch;

    indev_ = lv_indev_create();

    if (indev_ == nullptr) {
        return false;
    }

    lv_indev_set_type(
        indev_,
        LV_INDEV_TYPE_POINTER);

    lv_indev_set_user_data(
        indev_,
        this);

    lv_indev_set_read_cb(
        indev_,
        readCallback);

    return true;
}

lv_indev_t* LVGLTouch::handle() const
{
    return indev_;
}

void LVGLTouch::readCallback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    auto* self = static_cast<LVGLTouch*>(
        lv_indev_get_user_data(indev));

    if (self == nullptr || self->touch_ == nullptr) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    /*
     * GT911 is intentionally polled here.
     *
     * We don't depend on IRQ because our working GT911
     * implementation already handles the controller
     * state through read().
     */
    esp_err_t err = self->touch_->read();

    if (err != ESP_OK) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if (self->touch_->isTouched()) {
        data->state = LV_INDEV_STATE_PRESSED;

        data->point.x =
            static_cast<int32_t>(self->touch_->displayX());

        data->point.y =
            static_cast<int32_t>(self->touch_->displayY());
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}