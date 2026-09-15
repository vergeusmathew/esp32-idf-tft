#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"


class WinbondFlash
{
public:
    static constexpr uint32_t FLASH_SIZE = 16 * 1024 * 1024;
    static constexpr uint32_t SECTOR_SIZE = 4 * 1024;
    static constexpr uint32_t PAGE_SIZE   = 256;

    WinbondFlash() = default;
    ~WinbondFlash();

    esp_err_t init();

    esp_err_t readData(uint32_t address,
                       uint8_t* data, 
                       size_t length);

    esp_err_t write(uint32_t address,
                    const uint8_t* data,
                    size_t length);
    
    esp_err_t sectorErase(uint32_t address);

    esp_err_t read(uint32_t address,
                  uint8_t* data,
                  size_t length);

    esp_err_t eraseAndWrite(uint32_t address,
                            const uint8_t* data,
                            size_t length);
    
    bool isValidRange(uint32_t address,
                    size_t length) const;


    esp_err_t readJedecId(uint8_t* manufacturer,
                         uint8_t* memory_type,
                         uint8_t* capacity);
    
    esp_err_t readManufacturerDeviceId(uint8_t* manufacturer,
                                    uint8_t* device_id);

    spi_device_handle_t handle() const;
    esp_err_t writeEnable();
    esp_err_t writeDisable();
    esp_err_t readStatusRegister1(uint8_t* status);
    void printStatusRegister1(uint8_t status);
    esp_err_t waitUntilReady(uint32_t timeout_ms = 5000);
    esp_err_t pageProgram(uint32_t address, const uint8_t* data, size_t length);
    
private:
    static constexpr gpio_num_t CS_GPIO   = GPIO_NUM_4;
    static constexpr gpio_num_t CLK_GPIO  = GPIO_NUM_46;  //44
    static constexpr gpio_num_t MOSI_GPIO = GPIO_NUM_45;  //GPIO_NUM_43/3/21; //DI
    static constexpr gpio_num_t MISO_GPIO = GPIO_NUM_44;  //DO 46

    //static constexpr gpio_num_t MOSI_GPIO = GPIO_NUM_NC; //DI

    static constexpr int SPI_FREQUENCY_HZ = 400000; // 1 * 1000 * 100;   //10 000 00

    spi_device_handle_t device_ = nullptr;
    bool bus_initialized_ = false;
};