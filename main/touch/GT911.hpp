#pragma once

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

class GT911
{
public:
    static constexpr int DISP_WIDTH  = 800;
    static constexpr int DISP_HEIGHT = 480;

    // GT911 possible I2C addresses
    static constexpr uint16_t GT911_ADDR_5D = 0x5D;
    static constexpr uint16_t GT911_ADDR_14 = 0x14;

    // GT911 registers
    static constexpr uint16_t REG_COMMAND      = 0x8040;
    static constexpr uint16_t REG_CONFIG       = 0x8047;

    static constexpr uint16_t REG_PRODUCT_ID   = 0x8140;
    static constexpr uint16_t REG_FIRMWARE     = 0x8144;
    static constexpr uint16_t REG_X_RESOLUTION = 0x8146;
    static constexpr uint16_t REG_Y_RESOLUTION = 0x8148;

    static constexpr uint16_t REG_STATUS       = 0x814E;

    static constexpr uint16_t REG_POINT1       = 0x8150;
    static constexpr uint16_t REG_POINT2       = 0x8158;
    static constexpr uint16_t REG_POINT3       = 0x8160;
    static constexpr uint16_t REG_POINT4       = 0x8168;
    static constexpr uint16_t REG_POINT5       = 0x8170;

    static constexpr uint8_t MAX_TOUCH_POINTS = 5;

    struct TouchPoint
    {
        uint8_t  id = 0;
        uint16_t x = 0;
        uint16_t y = 0;
        uint16_t size = 0;
        bool     valid = false;
    };

    GT911() = default;
    ~GT911() = default;

    esp_err_t init(
        gpio_num_t sda_pin,
        gpio_num_t scl_pin,
        gpio_num_t rst_pin,
        gpio_num_t irq_pin
    );

    // Poll/read current GT911 touch state.
    // Returns ESP_OK when the controller was successfully read.
    esp_err_t read();

    bool isTouched() const
    {
        return touch_count_ > 0;
    }

    uint8_t touchCount() const
    {
        return touch_count_;
    }

    // First touch point, for simple applications.
    uint16_t rawX() const
    {
        return points_[0].x;
    }

    uint16_t rawY() const
    {
        return points_[0].y;
    }

    uint16_t displayX() const;
    uint16_t displayY() const;

    // Multi-touch access
    const TouchPoint &point(uint8_t index) const
    {
        return points_[index];
    }

    // Configuration
    void setXSwap(bool enable)
    {
        x_swap_ = enable;
    }

    void setYSwap(bool enable)
    {
        y_swap_ = enable;
    }

    void setXYSwap(bool enable)
    {
        xy_swap_ = enable;
    }

    void setXInvert(bool enable)
    {
        x_invert_ = enable;
    }

    void setYInvert(bool enable)
    {
        y_invert_ = enable;
    }

    uint16_t touchMaxX() const
    {
        return touch_max_x_;
    }

    uint16_t touchMaxY() const
    {
        return touch_max_y_;
    }

private:
    esp_err_t resetController(
        gpio_num_t rst_pin,
        gpio_num_t irq_pin
    );

    esp_err_t detectAddress();

    esp_err_t readRegister(
        uint16_t reg,
        uint8_t *data,
        size_t len
    );

    esp_err_t writeRegister(
        uint16_t reg,
        const uint8_t *data,
        size_t len
    );

    esp_err_t readPoint(
        uint16_t reg,
        TouchPoint &point
    );

    void clearPoints();

private:
    i2c_master_bus_handle_t bus_handle_ = nullptr;
    i2c_master_dev_handle_t dev_handle_ = nullptr;

    gpio_num_t irq_pin_ = GPIO_NUM_NC;

    uint16_t device_addr_ = 0;

    uint8_t touch_count_ = 0;

    TouchPoint points_[MAX_TOUCH_POINTS];

    uint16_t touch_max_x_ = DISP_WIDTH;
    uint16_t touch_max_y_ = DISP_HEIGHT;

    bool x_swap_   = false;
    bool y_swap_   = false;
    bool xy_swap_  = false;

    bool x_invert_ = false;
    bool y_invert_ = false;
};