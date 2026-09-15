#include "WinbondFlash.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char* TAG = "WinbondFlash";

esp_err_t WinbondFlash::init()
{
    ESP_LOGI(TAG, "init() START");

    // ---------------------------------------------------------
    // Configure CS FIRST
    // Same principle as Arduino SPIMemory constructor
    // ---------------------------------------------------------

    gpio_config_t cs_config = {};
    cs_config.pin_bit_mask = (1ULL << CS_GPIO);
    cs_config.mode = GPIO_MODE_OUTPUT;
    cs_config.pull_up_en = GPIO_PULLUP_DISABLE;
    cs_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cs_config.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&cs_config));

    // Flash must remain deselected
    gpio_set_level(CS_GPIO, 1);

    // ---------------------------------------------------------
    // Now initialize SPI bus
    // ---------------------------------------------------------

    spi_bus_config_t bus_config = {};

    bus_config.mosi_io_num = MOSI_GPIO;
    bus_config.miso_io_num = MISO_GPIO;
    bus_config.sclk_io_num = CLK_GPIO;
    bus_config.data2_io_num = -1;
    bus_config.data3_io_num = -1;
    bus_config.max_transfer_sz = 4096;

    esp_err_t err = spi_bus_initialize(
        SPI2_HOST,
        &bus_config,
        SPI_DMA_DISABLED);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "spi_bus_initialize failed: %s",
                 esp_err_to_name(err));

        return err;
    }

    bus_initialized_ = true;

    // ---------------------------------------------------------
    // Add SPI device
    // ---------------------------------------------------------

    spi_device_interface_config_t device_config = {};

    device_config.clock_speed_hz = 20000000;
    device_config.mode = 0;

    // Manual CS
    device_config.spics_io_num = -1;

    device_config.command_bits = 0;
    device_config.address_bits = 0;
    device_config.queue_size = 1;

    err = spi_bus_add_device(
        SPI2_HOST,
        &device_config,
        &device_);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "spi_bus_add_device failed: %s",
                 esp_err_to_name(err));

        spi_bus_free(SPI2_HOST);
        bus_initialized_ = false;

        return err;
    }

    gpio_set_level(CS_GPIO, 1);

    ESP_LOGI(TAG, "W25Q128 SPI device initialized");
    ESP_LOGI(TAG, "CS   = GPIO%d", CS_GPIO);
    ESP_LOGI(TAG, "CLK  = GPIO%d", CLK_GPIO);
    ESP_LOGI(TAG, "MOSI = GPIO%d", MOSI_GPIO);
    ESP_LOGI(TAG, "MISO = GPIO%d", MISO_GPIO);
    ESP_LOGI(TAG, "SPI frequency = %d Hz", 20000000);

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
spi_device_handle_t WinbondFlash::handle() const
{
    return device_;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
WinbondFlash::~WinbondFlash()
{
    if (device_ != nullptr)
    {
        spi_bus_remove_device(device_);
        device_ = nullptr;
    }

    if (bus_initialized_)
    {
        spi_bus_free(SPI2_HOST);
        bus_initialized_ = false;
    }
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::readJedecId(
    uint8_t* manufacturer,
    uint8_t* memory_type,
    uint8_t* capacity)
{
        uint8_t tx[4] = {
        0x9F,
        0x00,
        0x00,
        0x00
    };

    uint8_t rx[4] = {0};

    spi_transaction_t trans = {};
    trans.length = 32;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;

    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    gpio_set_level(CS_GPIO, 1);

    if (err != ESP_OK)
    {
        return err;
    }

    // rx[0] is the response while sending 0x9F.
    // The JEDEC bytes arrive while sending the three 0x00 bytes.
    *manufacturer = rx[1];
    *memory_type = rx[2];
    *capacity = rx[3];

    ESP_LOGI(TAG,
             "JEDEC ID: %02X %02X %02X",
             *manufacturer,
             *memory_type,
             *capacity);

    return ESP_OK;
}    
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::writeEnable()
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t tx = 0x06;

    spi_transaction_t trans = {};
    trans.length = 8;
    trans.tx_buffer = &tx;

    // CS LOW
    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    // CS HIGH
    gpio_set_level(CS_GPIO, 1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Write Enable failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "0x06 Write Enable sent");

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::writeDisable()
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t tx = 0x04;

    spi_transaction_t trans = {};
    trans.length = 8;
    trans.tx_buffer = &tx;

    // CS LOW
    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    // CS HIGH
    gpio_set_level(CS_GPIO, 1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Write Disable failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "0x04 Write Disable sent");

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void WinbondFlash::printStatusRegister1(uint8_t status)
{
    ESP_LOGI(TAG, "Status Register-1 = 0x%02X", status);

    ESP_LOGI(TAG, "BUSY = %d", (status & 0x01) ? 1 : 0);
    ESP_LOGI(TAG, "WEL  = %d", (status & 0x02) ? 1 : 0);

    uint8_t bp = (status >> 2) & 0x1F;
    ESP_LOGI(TAG, "BP bits = 0x%02X", bp);

    ESP_LOGI(TAG, "SRP0 = %d", (status & 0x80) ? 1 : 0);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::readStatusRegister1(uint8_t* status)
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t tx[2] = {
        0x05,
        0x00
    };

    uint8_t rx[2] = {
        0x00,
        0x00
    };

    spi_transaction_t trans = {};
    trans.length = 16;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;

    // CS LOW
    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    // CS HIGH
    gpio_set_level(CS_GPIO, 1);

    if (err != ESP_OK)
    {
        return err;
    }

    *status = rx[1];

    ESP_LOGI(TAG,
             "Status Register-1 = 0x%02X",
             *status);

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::waitUntilReady(uint32_t timeout_ms)
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t start_time = esp_log_timestamp();

    while (true)
    {
        uint8_t status = 0;

        esp_err_t err = readStatusRegister1(&status);

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "Failed to read Status Register-1: %s",
                     esp_err_to_name(err));
            return err;
        }

        // BUSY = bit 0
        if ((status & 0x01) == 0)
        {
            ESP_LOGI(TAG, "Flash ready, BUSY = 0");
            return ESP_OK;
        }

        // Timeout check
        if ((esp_log_timestamp() - start_time) >= timeout_ms)
        {
            ESP_LOGE(TAG,
                     "Timeout waiting for flash, BUSY = 1");
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::pageProgram(uint32_t address,
                                     const uint8_t* data,
                                     size_t length)
{
    if (device_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    if (data == nullptr || length == 0)
        return ESP_ERR_INVALID_ARG;

    if (length > 256)
        return ESP_ERR_INVALID_SIZE;

    // Page Program cannot cross a 256-byte page boundary.
    size_t pageOffset = address & 0xFF;

    if (pageOffset + length > 256)
    {
        ESP_LOGE(TAG, "Page Program crosses 256-byte page boundary");
        return ESP_ERR_INVALID_ARG;
    }

    // -----------------------------
    // Send command + 24-bit address
    // -----------------------------
    uint8_t header[4] = {
        0x02,
        static_cast<uint8_t>((address >> 16) & 0xFF),
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF)
    };

    spi_transaction_t trans = {};
    trans.length = 32;
    trans.tx_buffer = header;

    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);

        ESP_LOGE(TAG,
                 "0x02 Page Program header failed: %s",
                 esp_err_to_name(err));

        return err;
    }

    // -----------------------------
    // Send data in small chunks
    // CS stays LOW!
    // -----------------------------
    size_t offset = 0;

    while (offset < length)
    {
        size_t chunk = length - offset;

        if (chunk > 32)
            chunk = 32;

        spi_transaction_t data_trans = {};
        data_trans.length = chunk * 8;
        data_trans.tx_buffer = data + offset;

        err = spi_device_transmit(device_, &data_trans);

        if (err != ESP_OK)
        {
            gpio_set_level(CS_GPIO, 1);

            ESP_LOGE(TAG,
                     "0x02 Page Program data failed at offset %u: %s",
                     (unsigned)offset,
                     esp_err_to_name(err));

            return err;
        }

        offset += chunk;
    }

    // End the Page Program command.
    gpio_set_level(CS_GPIO, 1);

    ESP_LOGI(TAG,
             "0x02 Page Program sent: address=0x%06lX length=%u",
             (unsigned long)address,
             (unsigned)length);

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::readData(uint32_t address,
                                  uint8_t* data,
                                  size_t length)
{
    if (device_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    if (data == nullptr || length == 0)
        return ESP_ERR_INVALID_ARG;

    // -----------------------------
    // Send 0x03 + 24-bit address
    // -----------------------------
    uint8_t header[4] = {
        0x03,
        static_cast<uint8_t>((address >> 16) & 0xFF),
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF)
    };

    spi_transaction_t trans = {};
    trans.length = 32;
    trans.tx_buffer = header;

    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);

        ESP_LOGE(TAG,
                 "0x03 Read Data header failed: %s",
                 esp_err_to_name(err));

        return err;
    }

    // -----------------------------
    // Read data in 32-byte chunks
    // CS stays LOW
    // -----------------------------
    static const uint8_t dummy[32] = {0};

    size_t offset = 0;

    while (offset < length)
    {
        size_t chunk = length - offset;

        if (chunk > 32)
            chunk = 32;

        spi_transaction_t read_trans = {};
        read_trans.length = chunk * 8;
        read_trans.tx_buffer = dummy;
        read_trans.rx_buffer = data + offset;

        err = spi_device_transmit(device_, &read_trans);

        if (err != ESP_OK)
        {
            gpio_set_level(CS_GPIO, 1);

            ESP_LOGE(TAG,
                     "0x03 Read Data failed at offset %u: %s",
                     (unsigned)offset,
                     esp_err_to_name(err));

            return err;
        }

        offset += chunk;
    }

    gpio_set_level(CS_GPIO, 1);

    ESP_LOGI(TAG,
             "0x03 Read Data: address=0x%06lX length=%u",
             (unsigned long)address,
             (unsigned)length);

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::write(uint32_t address,
                              const uint8_t* data,
                              size_t length)
{
    if (device_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    if (data == nullptr || length == 0)
        return ESP_ERR_INVALID_ARG;

    size_t remaining = length;
    size_t dataOffset = 0;

    while (remaining > 0)
    {
        // Position inside current 256-byte page
        size_t pageOffset = address & 0xFF;

        // Number of bytes available until end of page
        size_t pageRemaining = 256 - pageOffset;

        // Write only what fits in this page
        size_t chunk = remaining;

        if (chunk > pageRemaining)
            chunk = pageRemaining;

        ESP_LOGI(TAG,
                 "write(): address=0x%06lX chunk=%u",
                 (unsigned long)address,
                 (unsigned)chunk);

        // -----------------------------------------
        // Write Enable
        // -----------------------------------------
        esp_err_t err = writeEnable();

        if (err != ESP_OK)
            return err;

        // -----------------------------------------
        // Verify WEL
        // -----------------------------------------
        uint8_t status = 0;

        err = readStatusRegister1(&status);

        if (err != ESP_OK)
            return err;

        if ((status & 0x02) == 0)
        {
            ESP_LOGE(TAG,
                     "write(): WEL was not set");

            return ESP_ERR_INVALID_STATE;
        }

        // -----------------------------------------
        // Program this page portion
        // -----------------------------------------
        err = pageProgram(address,
                          data + dataOffset,
                          chunk);

        if (err != ESP_OK)
            return err;

        // -----------------------------------------
        // Wait until programming is complete
        // -----------------------------------------
        err = waitUntilReady(5000);

        if (err != ESP_OK)
            return err;

        // Move to next portion
        address += chunk;
        dataOffset += chunk;
        remaining -= chunk;
    }

    ESP_LOGI(TAG,
             "write(): completed %u bytes",
             (unsigned)length);

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool WinbondFlash::isValidRange(uint32_t address,
                                size_t length) const
{
    if (length == 0)
        return false;

    if (address >= FLASH_SIZE)
        return false;

    if (length > FLASH_SIZE - address)
        return false;

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::read(uint32_t address,
                             uint8_t* data,
                             size_t length)
{
    if (device_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    if (data == nullptr || length == 0)
        return ESP_ERR_INVALID_ARG;

    if (!isValidRange(address, length))
    {
        ESP_LOGE(TAG,
                 "read(): invalid range address=0x%06lX length=%u",
                 (unsigned long)address,
                 (unsigned)length);

        return ESP_ERR_INVALID_ARG;
    }

    return readData(address, data, length);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::eraseAndWrite(uint32_t address,
                                      const uint8_t* data,
                                      size_t length)
{
    if (device_ == nullptr)
        return ESP_ERR_INVALID_STATE;

    if (data == nullptr || length == 0)
        return ESP_ERR_INVALID_ARG;

    if (!isValidRange(address, length))
    {
        ESP_LOGE(TAG,
                 "eraseAndWrite(): invalid range "
                 "address=0x%06lX length=%u",
                 (unsigned long)address,
                 (unsigned)length);

        return ESP_ERR_INVALID_ARG;
    }

    uint32_t firstSector =
        address & ~(SECTOR_SIZE - 1);

    uint32_t lastAddress =
        address + length - 1;

    uint32_t lastSector =
        lastAddress & ~(SECTOR_SIZE - 1);

    ESP_LOGI(TAG,
             "eraseAndWrite(): sectors 0x%06lX - 0x%06lX",
             (unsigned long)firstSector,
             (unsigned long)lastSector);

    // ---------------------------------------------------------
    // Erase all affected sectors
    // ---------------------------------------------------------

    for (uint32_t sector = firstSector;
         sector <= lastSector;
         sector += SECTOR_SIZE)
    {
        ESP_LOGI(TAG,
                 "Erasing sector 0x%06lX",
                 (unsigned long)sector);

        esp_err_t err = writeEnable();

        if (err != ESP_OK)
            return err;

        uint8_t status = 0;

        err = readStatusRegister1(&status);

        if (err != ESP_OK)
            return err;

        if ((status & 0x02) == 0)
        {
            ESP_LOGE(TAG,
                     "eraseAndWrite(): WEL was not set");

            return ESP_ERR_INVALID_STATE;
        }

        err = sectorErase(sector);

        if (err != ESP_OK)
            return err;

        err = waitUntilReady(5000);

        if (err != ESP_OK)
            return err;
    }

    // ---------------------------------------------------------
    // Program data
    // ---------------------------------------------------------

    ESP_LOGI(TAG,
             "eraseAndWrite(): programming %u bytes",
             (unsigned)length);

    return write(address, data, length);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------



#if 0
esp_err_t WinbondFlash::readData(uint32_t address,
                                  uint8_t* data,
                                  size_t length)
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == nullptr || length == 0)
    {
        ESP_LOGE(TAG, "Invalid data or length");
        return ESP_ERR_INVALID_ARG;
    }

    // Command + 24-bit address + dummy/read bytes
    uint8_t tx[4 + 256] = {0};
    uint8_t rx[4 + 256] = {0};

    if (length > 256)
    {
        ESP_LOGE(TAG, "Read length cannot exceed 256 bytes");
        return ESP_ERR_INVALID_SIZE;
    }

    tx[0] = 0x03;
    tx[1] = (address >> 16) & 0xFF;
    tx[2] = (address >> 8) & 0xFF;
    tx[3] = address & 0xFF;

    spi_transaction_t trans = {};
    trans.length = (4 + length) * 8;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;

    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    gpio_set_level(CS_GPIO, 1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "0x03 Read Data failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    memcpy(data, &rx[4], length);

    ESP_LOGI(TAG,
             "0x03 Read Data: address=0x%06lX length=%u",
             (unsigned long)address,
             (unsigned)length);

    return ESP_OK;
}
#endif
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::sectorErase(uint32_t address)
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // 0x20 + 24-bit address
    uint8_t tx[4] = {
        0x20,
        static_cast<uint8_t>((address >> 16) & 0xFF),
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF)
    };

    spi_transaction_t trans = {};
    trans.length = 32;
    trans.tx_buffer = tx;

    // CS LOW
    gpio_set_level(CS_GPIO, 0);

    esp_err_t err = spi_device_transmit(device_, &trans);

    // CS HIGH
    gpio_set_level(CS_GPIO, 1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "0x20 Sector Erase failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG,
             "0x20 Sector Erase sent: address=0x%06lX",
             (unsigned long)address);

    return ESP_OK;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
esp_err_t WinbondFlash::readManufacturerDeviceId(
    uint8_t* manufacturer,
    uint8_t* device_id)
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;
    spi_transaction_t trans = {};

    uint8_t command = 0x90;
    uint8_t address[3] = {0x00, 0x00, 0x00};
    uint8_t rx = 0;

    // CS LOW
    gpio_set_level(CS_GPIO, 0);

    // ---------------------------------------------------------
    // Send 0x90
    // ---------------------------------------------------------

    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.tx_buffer = &command;

    err = spi_device_transmit(device_, &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    // ---------------------------------------------------------
    // Send 24-bit address
    // ---------------------------------------------------------

    memset(&trans, 0, sizeof(trans));

    trans.length = 24;
    trans.tx_buffer = address;

    err = spi_device_transmit(device_, &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    // ---------------------------------------------------------
    // Read Manufacturer ID
    // ---------------------------------------------------------

    rx = 0;

    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.rxlength = 8;
    trans.rx_buffer = &rx;

    err = spi_device_transmit(device_, &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    *manufacturer = rx;

    // ---------------------------------------------------------
    // Read Device ID
    // ---------------------------------------------------------

    rx = 0;

    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.rxlength = 8;
    trans.rx_buffer = &rx;

    err = spi_device_transmit(device_, &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    *device_id = rx;

    // CS HIGH
    gpio_set_level(CS_GPIO, 1);

    ESP_LOGI(
        TAG,
        "0x90 ID: Manufacturer=%02X DeviceID=%02X",
        *manufacturer,
        *device_id);

    return ESP_OK;
}


/*
esp_err_t WinbondFlash::readJedecId(
    uint8_t* manufacturer,
    uint8_t* memory_type,
    uint8_t* capacity)
{
    if (device_ == nullptr)
    {
        ESP_LOGE(TAG, "SPI device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t rx = 0;

    spi_transaction_t trans = {};

    // ---------------------------------------------------------
    // Arduino:
    //
    // SPI.beginTransaction(SPISettings(
    //     20000000,
    //     MSBFIRST,
    //     SPI_MODE0));
    //
    // digitalWrite(CS, LOW);
    // SPI.transfer(0x9F);
    // SPI.transfer(0x00);
    // SPI.transfer(0x00);
    // SPI.transfer(0x00);
    // digitalWrite(CS, HIGH);
    // ---------------------------------------------------------

    gpio_set_level(CS_GPIO, 0);

    // Send 0x9F
    uint8_t command = 0x9F;

    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.tx_buffer = &command;

    esp_err_t err = spi_device_transmit(
        device_,
        &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    // Read manufacturer
    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.rxlength = 8;
    trans.rx_buffer = &rx;

    err = spi_device_transmit(
        device_,
        &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    *manufacturer = rx;

    // Read memory type
    rx = 0;

    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.rxlength = 8;
    trans.rx_buffer = &rx;

    err = spi_device_transmit(
        device_,
        &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    *memory_type = rx;

    // Read capacity
    rx = 0;

    memset(&trans, 0, sizeof(trans));

    trans.length = 8;
    trans.rxlength = 8;
    trans.rx_buffer = &rx;

    err = spi_device_transmit(
        device_,
        &trans);

    if (err != ESP_OK)
    {
        gpio_set_level(CS_GPIO, 1);
        return err;
    }

    *capacity = rx;

    // CS HIGH
    gpio_set_level(CS_GPIO, 1);

    ESP_LOGI(
        TAG,
        "JEDEC ID: %02X %02X %02X",
        *manufacturer,
        *memory_type,
        *capacity);

    return ESP_OK;
}
*/

