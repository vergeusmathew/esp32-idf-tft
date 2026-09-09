#include "GT911.hpp"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GT911";

// -----------------------------------------------------------------------------
// GT911 status bits
// -----------------------------------------------------------------------------

static constexpr uint8_t STATUS_TOUCH_MASK = 0x0F;
static constexpr uint8_t STATUS_READY_BIT  = 0x80;

// -----------------------------------------------------------------------------
// Constructor / initialization
// -----------------------------------------------------------------------------

esp_err_t GT911::init(
    gpio_num_t sda_pin,
    gpio_num_t scl_pin,
    gpio_num_t rst_pin,
    gpio_num_t irq_pin)
{
    irq_pin_ = irq_pin;

    clearPoints();

    // -------------------------------------------------------------------------
    // Configure IRQ as input.
    //
    // No internal pull-up/down.
    // -------------------------------------------------------------------------

    gpio_config_t irq_config = {};

    irq_config.pin_bit_mask = 1ULL << irq_pin_;
    irq_config.mode = GPIO_MODE_INPUT;
    irq_config.pull_up_en = GPIO_PULLUP_DISABLE;
    irq_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    irq_config.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&irq_config);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to configure IRQ GPIO: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    // -------------------------------------------------------------------------
    // I2C bus
    // -------------------------------------------------------------------------

    i2c_master_bus_config_t bus_config = {};

    bus_config.i2c_port = I2C_NUM_0;
    bus_config.sda_io_num = sda_pin;
    bus_config.scl_io_num = scl_pin;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = false;

    err = i2c_new_master_bus(
        &bus_config,
        &bus_handle_
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to create I2C bus: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "I2C bus initialized: SDA=%d SCL=%d",
        sda_pin,
        scl_pin
    );

    // -------------------------------------------------------------------------
    // Reset GT911
    // -------------------------------------------------------------------------

    err = resetController(
        rst_pin,
        irq_pin
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "GT911 reset failed: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    // -------------------------------------------------------------------------
    // Detect GT911 address
    // -------------------------------------------------------------------------

    err = detectAddress();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "GT911 address detection failed: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    // -------------------------------------------------------------------------
    // Add GT911 as I2C device
    // -------------------------------------------------------------------------

    i2c_device_config_t dev_config = {};

    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = device_addr_;
    dev_config.scl_speed_hz = 400000;

    err = i2c_master_bus_add_device(
        bus_handle_,
        &dev_config,
        &dev_handle_
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to add GT911 I2C device: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "GT911 device added at address 0x%02X",
        device_addr_
    );

    // -------------------------------------------------------------------------
    // Read Product ID
    // -------------------------------------------------------------------------

    uint8_t product_id[4] = {};

    err = readRegister(
        REG_PRODUCT_ID,
        product_id,
        sizeof(product_id)
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to read GT911 Product ID: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "GT911 Product ID: '%c%c%c%c'",
        product_id[0],
        product_id[1],
        product_id[2],
        product_id[3]
    );

    // -------------------------------------------------------------------------
    // Read firmware
    // -------------------------------------------------------------------------

    uint8_t firmware[2] = {};

    err = readRegister(
        REG_FIRMWARE,
        firmware,
        sizeof(firmware)
    );

    if (err == ESP_OK)
    {
        uint16_t version =
            static_cast<uint16_t>(firmware[0]) |
            (static_cast<uint16_t>(firmware[1]) << 8);

        ESP_LOGI(
            TAG,
            "GT911 Firmware: 0x%04X",
            version
        );
    }
    else
    {
        ESP_LOGW(
            TAG,
            "Could not read firmware: %s",
            esp_err_to_name(err)
        );
    }

    // -------------------------------------------------------------------------
    // Read touch resolution
    // -------------------------------------------------------------------------

    uint8_t resolution[4] = {};

    err = readRegister(
        REG_X_RESOLUTION,
        resolution,
        sizeof(resolution)
    );

    if (err == ESP_OK)
    {
        touch_max_x_ =
            static_cast<uint16_t>(resolution[0]) |
            (static_cast<uint16_t>(resolution[1]) << 8);

        touch_max_y_ =
            static_cast<uint16_t>(resolution[2]) |
            (static_cast<uint16_t>(resolution[3]) << 8);

        // Protect against invalid configuration values.

        if (touch_max_x_ == 0)
            touch_max_x_ = DISP_WIDTH;

        if (touch_max_y_ == 0)
            touch_max_y_ = DISP_HEIGHT;

        ESP_LOGI(
            TAG,
            "GT911 Touch Resolution: %ux%u",
            touch_max_x_,
            touch_max_y_
        );
    }
    else
    {
        touch_max_x_ = DISP_WIDTH;
        touch_max_y_ = DISP_HEIGHT;

        ESP_LOGW(
            TAG,
            "Could not read touch resolution: %s",
            esp_err_to_name(err)
        );

        ESP_LOGW(
            TAG,
            "Using default touch resolution: %ux%u",
            touch_max_x_,
            touch_max_y_
        );
    }

    ESP_LOGI(
        TAG,
        "GT911 initialization complete"
    );

    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Reset controller
// -----------------------------------------------------------------------------

esp_err_t GT911::resetController(
    gpio_num_t rst_pin,
    gpio_num_t irq_pin)
{
    esp_err_t err;

    // INT is temporarily controlled during reset/address selection.
    err = gpio_set_direction(irq_pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK)
        return err;

    err = gpio_set_level(irq_pin, 0);
    if (err != ESP_OK)
        return err;

    err = gpio_set_direction(rst_pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK)
        return err;

    err = gpio_set_level(rst_pin, 0);
    if (err != ESP_OK)
        return err;

    vTaskDelay(pdMS_TO_TICKS(20));

    err = gpio_set_level(rst_pin, 1);
    if (err != ESP_OK)
        return err;

    vTaskDelay(pdMS_TO_TICKS(50));

    err = gpio_set_level(irq_pin, 1);
    if (err != ESP_OK)
        return err;

    vTaskDelay(pdMS_TO_TICKS(10));

    err = gpio_set_direction(irq_pin, GPIO_MODE_INPUT);
    if (err != ESP_OK)
        return err;

    gpio_pullup_dis(irq_pin);
    gpio_pulldown_dis(irq_pin);

    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Detect I2C address
// -----------------------------------------------------------------------------

esp_err_t GT911::detectAddress()
{
    esp_err_t err;

    err = i2c_master_probe(
        bus_handle_,
        GT911_ADDR_5D,
        100
    );

    if (err == ESP_OK)
    {
        device_addr_ = GT911_ADDR_5D;

        ESP_LOGI(
            TAG,
            "GT911 responds at 0x5D"
        );

        return ESP_OK;
    }

    err = i2c_master_probe(
        bus_handle_,
        GT911_ADDR_14,
        100
    );

    if (err == ESP_OK)
    {
        device_addr_ = GT911_ADDR_14;

        ESP_LOGI(
            TAG,
            "GT911 responds at 0x14"
        );

        return ESP_OK;
    }

    ESP_LOGE(
        TAG,
        "GT911 not detected at 0x5D or 0x14"
    );

    return ESP_ERR_NOT_FOUND;
}

// -----------------------------------------------------------------------------
// Read register
// -----------------------------------------------------------------------------

esp_err_t GT911::readRegister(
    uint16_t reg,
    uint8_t *data,
    size_t len)
{
    if (dev_handle_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    uint8_t address[2];

    // GT911 register address is sent MSB first.
    address[0] = static_cast<uint8_t>((reg >> 8) & 0xFF);
    address[1] = static_cast<uint8_t>(reg & 0xFF);

    return i2c_master_transmit_receive(
        dev_handle_,
        address,
        sizeof(address),
        data,
        len,
        100
    );
}

// -----------------------------------------------------------------------------
// Write register
// -----------------------------------------------------------------------------

esp_err_t GT911::writeRegister(
    uint16_t reg,
    const uint8_t *data,
    size_t len)
{
    if (dev_handle_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    uint8_t buffer[2 + MAX_TOUCH_POINTS];

    if (len > MAX_TOUCH_POINTS)
        return ESP_ERR_INVALID_SIZE;

    buffer[0] = static_cast<uint8_t>((reg >> 8) & 0xFF);
    buffer[1] = static_cast<uint8_t>(reg & 0xFF);

    if (len > 0)
        memcpy(&buffer[2], data, len);

    return i2c_master_transmit(
        dev_handle_,
        buffer,
        2 + len,
        100
    );
}

// -----------------------------------------------------------------------------
// Clear local touch data
// -----------------------------------------------------------------------------

void GT911::clearPoints()
{
    touch_count_ = 0;

    for (auto &p : points_)
    {
        p.id = 0;
        p.x = 0;
        p.y = 0;
        p.size = 0;
        p.valid = false;
    }
}

// -----------------------------------------------------------------------------
// Read one touch point
// -----------------------------------------------------------------------------

esp_err_t GT911::readPoint(
    uint16_t reg,
    TouchPoint &point)
{
    uint8_t data[4] = {};

    esp_err_t err = readRegister(
        reg,
        data,
        sizeof(data)
    );

    if (err != ESP_OK)
        return err;

    point.x =
        static_cast<uint16_t>(data[0]) |
        (static_cast<uint16_t>(data[1]) << 8);

    point.y =
        static_cast<uint16_t>(data[2]) |
        (static_cast<uint16_t>(data[3]) << 8);

    point.id = 0;
    point.size = 0;
    point.valid = true;

    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Read GT911 touch state
// -----------------------------------------------------------------------------

esp_err_t GT911::read()
{
    uint8_t status = 0;

    esp_err_t err = readRegister(
        REG_STATUS,
        &status,
        1
    );

    if (err != ESP_OK)
        return err;

    /*
     * Status register:
     *
     * bit 7     = buffer/data ready
     * bits 3:0  = number of touch points
     */

    const bool data_ready =
        (status & STATUS_READY_BIT) != 0;

    const uint8_t count =
        status & STATUS_TOUCH_MASK;

    /*
     * No new data.
     *
     * Don't clear the register here.
     */
    if (!data_ready)
    {
        clearPoints();
        return ESP_OK;
    }

    /*
     * Protect against invalid controller data.
     */
    if (count > MAX_TOUCH_POINTS)
    {
        ESP_LOGW(
            TAG,
            "Invalid GT911 touch count: %u",
            count
        );

        clearPoints();

        uint8_t clear = 0;
        return writeRegister(
            REG_STATUS,
            &clear,
            1
        );
    }

    clearPoints();

    touch_count_ = count;

    // -------------------------------------------------------------------------
    // Read all reported points
    // -------------------------------------------------------------------------

    static constexpr uint16_t point_registers[MAX_TOUCH_POINTS] =
    {
        REG_POINT1,
        REG_POINT2,
        REG_POINT3,
        REG_POINT4,
        REG_POINT5
    };

    for (uint8_t i = 0; i < touch_count_; ++i)
    {
        err = readPoint(
            point_registers[i],
            points_[i]
        );

        if (err != ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Failed to read touch point %u",
                i
            );

            clearPoints();

            uint8_t clear = 0;
            writeRegister(
                REG_STATUS,
                &clear,
                1
            );

            return err;
        }
    }

    // -------------------------------------------------------------------------
    // Clear GT911 data-ready flag
    // -------------------------------------------------------------------------

    uint8_t clear = 0;

    err = writeRegister(
        REG_STATUS,
        &clear,
        1
    );

    if (err != ESP_OK)
        return err;

    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Transform X coordinate
// -----------------------------------------------------------------------------

uint16_t GT911::displayX() const
{
    if (!points_[0].valid)
        return 0;

    uint32_t x = points_[0].x;
    uint32_t y = points_[0].y;

    if (x_invert_)
    {
        if (x >= touch_max_x_)
            x = 0;
        else
            x = touch_max_x_ - 1 - x;
    }

    if (y_invert_)
    {
        if (y >= touch_max_y_)
            y = 0;
        else
            y = touch_max_y_ - 1 - y;
    }

    if (xy_swap_)
    {
        uint32_t temp = x;
        x = y;
        y = temp;
    }

    if (x_swap_)
    {
        if (x >= touch_max_x_)
            x = 0;
        else
            x = touch_max_x_ - 1 - x;
    }

    uint32_t max_x =
        xy_swap_ ? touch_max_y_ : touch_max_x_;

    if (max_x == 0)
        max_x = DISP_WIDTH;

    uint32_t display_x =
        (x * DISP_WIDTH) / max_x;

    if (display_x >= DISP_WIDTH)
        display_x = DISP_WIDTH - 1;

    return static_cast<uint16_t>(display_x);
}

// -----------------------------------------------------------------------------
// Transform Y coordinate
// -----------------------------------------------------------------------------

uint16_t GT911::displayY() const
{
    if (!points_[0].valid)
        return 0;

    uint32_t x = points_[0].x;
    uint32_t y = points_[0].y;

    if (x_invert_)
    {
        if (x >= touch_max_x_)
            x = 0;
        else
            x = touch_max_x_ - 1 - x;
    }

    if (y_invert_)
    {
        if (y >= touch_max_y_)
            y = 0;
        else
            y = touch_max_y_ - 1 - y;
    }

    if (xy_swap_)
    {
        uint32_t temp = x;
        x = y;
        y = temp;
    }

    if (y_swap_)
    {
        if (y >= touch_max_y_)
            y = 0;
        else
            y = touch_max_y_ - 1 - y;
    }

    uint32_t max_y =
        xy_swap_ ? touch_max_x_ : touch_max_y_;

    if (max_y == 0)
        max_y = DISP_HEIGHT;

    uint32_t display_y =
        (y * DISP_HEIGHT) / max_y;

    if (display_y >= DISP_HEIGHT)
        display_y = DISP_HEIGHT - 1;

    return static_cast<uint16_t>(display_y);
}