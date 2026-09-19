#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt_tx.h"
#include "esp_task_wdt.h"

#include "lvgl.h"

#include "display/Display.hpp"
#include "touch/GT911.hpp"
#include "lvgl/LVGLDisplay.hpp"
#include "lvgl/LVGLTouch.hpp"
#include "led/RGBLed.hpp"
#include "ui/UI.hpp"
#include "flash/WinbondFlash.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"

#define LCD_RST_GPIO GPIO_NUM_4
#define TOUCH_SDA    GPIO_NUM_1
#define TOUCH_SCL    GPIO_NUM_2
#define TOUCH_IRQ    GPIO_NUM_47
#define RGB_LED_GPIO GPIO_NUM_48

static const char *TAG = "TFT_TEST";

// Display Resolution
#define LCD_H_RES              800
#define LCD_V_RES              480

// Backlight PWM Setup
#define BACKLIGHT_PWM_PIN       GPIO_NUM_3
#define BACKLIGHT_PWM_CHAN      LEDC_CHANNEL_0
#define BACKLIGHT_PWM_TK        LEDC_TIMER_0
#define BACKLIGHT_PWM_FREQ      5000
#define BACKLIGHT_PWM_RES       LEDC_TIMER_8_BIT

// ---------------------------------------------------------------------------
// Backlight
// ---------------------------------------------------------------------------

static void initBacklight()
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num = BACKLIGHT_PWM_TK;
    ledc_timer.duty_resolution = BACKLIGHT_PWM_RES;
    ledc_timer.freq_hz = BACKLIGHT_PWM_FREQ;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num = BACKLIGHT_PWM_PIN;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = BACKLIGHT_PWM_CHAN;
    ledc_channel.timer_sel = BACKLIGHT_PWM_TK;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHAN, 77);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHAN);

    ESP_LOGI(TAG, "Backlight initialized");
}


static uint32_t crc32(const uint8_t* data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; ++i)
    {
        crc ^= data[i];

        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return ~crc;
}

static void testSpeedometerFlashImage(WinbondFlash& flash)
{
    constexpr uint32_t IMAGE_ADDRESS = 0x010000;
    constexpr size_t IMAGE_SIZE = 47628;
    constexpr uint32_t EXPECTED_CRC = 0x0E60F6E8;

    ESP_LOGI("FLASH_TEST", "================================");
    ESP_LOGI("FLASH_TEST", "Speedometer image flash test");
    ESP_LOGI("FLASH_TEST", "Address : 0x%06lX",
             (unsigned long)IMAGE_ADDRESS);
    ESP_LOGI("FLASH_TEST", "Size    : %u bytes",
             (unsigned)IMAGE_SIZE);
    ESP_LOGI("FLASH_TEST", "Expected CRC32 : %08lX",
             (unsigned long)EXPECTED_CRC);

    // Allocate the image buffer in PSRAM.
    uint8_t* image = static_cast<uint8_t*>(
        heap_caps_malloc(
            IMAGE_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (image == nullptr)
    {
        ESP_LOGE("FLASH_TEST",
                 "Failed to allocate %u bytes in PSRAM",
                 (unsigned)IMAGE_SIZE);
        return;
    }

    ESP_LOGI("FLASH_TEST", "PSRAM buffer allocated");

    // Read image from W25Q128.
    esp_err_t err = flash.read(
        IMAGE_ADDRESS,
        image,
        IMAGE_SIZE);

    if (err != ESP_OK)
    {
        ESP_LOGE("FLASH_TEST",
                 "Flash read failed: %s",
                 esp_err_to_name(err));

        heap_caps_free(image);
        return;
    }

    ESP_LOGI("FLASH_TEST", "Flash read complete");

    // Calculate CRC32.
    uint32_t crc = crc32(image, IMAGE_SIZE);

    ESP_LOGI("FLASH_TEST",
             "Read CRC32     : %08lX",
             (unsigned long)crc);

    ESP_LOGI("FLASH_TEST",
             "Expected CRC32 : %08lX",
             (unsigned long)EXPECTED_CRC);

    if (crc == EXPECTED_CRC)
    {
        ESP_LOGI("FLASH_TEST",
                 "================================");
        ESP_LOGI("FLASH_TEST",
                 "IMAGE CRC VERIFY SUCCESS!");
        ESP_LOGI("FLASH_TEST",
                 "W25Q128 data matches PC binary");
        ESP_LOGI("FLASH_TEST",
                 "================================");
    }
    else
    {
        ESP_LOGE("FLASH_TEST",
                 "================================");
        ESP_LOGE("FLASH_TEST",
                 "IMAGE CRC VERIFY FAILED!");
        ESP_LOGE("FLASH_TEST",
                 "================================");
    }

    // Show first 32 bytes for additional confirmation.
    ESP_LOGI("FLASH_TEST", "First 32 bytes:");

    for (int i = 0; i < 32; ++i)
    {
        printf("%02X ", image[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    heap_caps_free(image);
}

static const char* FLASH_TEST_TAG = "FLASH_TEST";

#if 1
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 09,2026 15:17...");
    ESP_LOGI(TAG, "PTLA Sept 13,2026 21:53...");

    initBacklight();
    //initRgbLed();

    // Power ON: Red
    //setRgbColor(32, 0, 0);
    // ---------------------------------------------------------
    // Display
    // ---------------------------------------------------------
#if 1
    ESP_LOGI(TAG, "Creating Display");

    Display display;

    ESP_ERROR_CHECK(display.init());

    RGBLed rgbLed;

    if (rgbLed.init() != ESP_OK) {
        ESP_LOGE(TAG, "RGB LED initialization failed");
        return;
    }
#endif    
    // ---------------------------------------------------------
    // LVGL
    // ---------------------------------------------------------
#if 1
    ESP_LOGI(TAG, "Initializing LVGL");

    lv_init();
    
    lv_tick_set_cb([]() -> uint32_t {
        return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    });
#endif
#if 1
    ESP_LOGI(TAG, "Creating LVGLDisplay object");
    LVGLDisplay lvglDisplay;

    bool lvgl_display_ok = lvglDisplay.init(display);


    if (!lvgl_display_ok)
    {
        ESP_LOGE(TAG, "LVGL display initialization failed");
        return;
    }
    ESP_LOGI(TAG, "LVGL display initialized");
#endif    
#if 0  //Filles the scrren with red color just for testing
    lv_obj_t* screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0xFF0000),
        0);

    lv_refr_now(lvglDisplay.handle());

    ESP_LOGI(TAG, "Red screen requested");
#endif
    //----------------------------------------------------------    
    // Winbond W25Q128
    //----------------------------------------------------------
#if 1    
    ESP_LOGI(TAG, "Initializing W25Q128...");

    WinbondFlash winbondFlash;

    esp_err_t flash_ret = winbondFlash.init();
    
    if (flash_ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "W25Q128 initialization failed: %s",
            esp_err_to_name(flash_ret));

        return;
    }

    ESP_LOGI(TAG, "W25Q128 SPI initialized done PTLA 13:57");
#endif    
#if 0
            ESP_LOGI(FLASH_TEST_TAG, "================================");
            ESP_LOGI(FLASH_TEST_TAG, "Step 1B-1: Speedometer flash test");

            constexpr uint32_t IMAGE_ADDRESS = 0x010000;
            constexpr size_t IMAGE_SIZE = 95580; //47628;
            constexpr uint32_t EXPECTED_CRC32 = 0x12F6E620; //0x0E60F6E8; 

            ESP_LOGI(FLASH_TEST_TAG, "Address         : 0x%06X", IMAGE_ADDRESS);
            ESP_LOGI(FLASH_TEST_TAG, "Size            : %u bytes",
                    static_cast<unsigned>(IMAGE_SIZE));
            ESP_LOGI(FLASH_TEST_TAG, "Expected CRC32  : %08X", EXPECTED_CRC32);

            // Allocate image buffer in PSRAM.
            // static avoids putting the large buffer on the main task stack.
            static uint8_t* imageData = nullptr;

            imageData = static_cast<uint8_t*>(
                heap_caps_malloc(
                    IMAGE_SIZE,
                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

            if (imageData == nullptr) {
                ESP_LOGE(FLASH_TEST_TAG, "PSRAM allocation failed");
                return;
            }

            ESP_LOGI(FLASH_TEST_TAG, "PSRAM buffer allocated");

            esp_err_t err = winbondFlash.read(
                IMAGE_ADDRESS,
                imageData,
                IMAGE_SIZE);

            if (err != ESP_OK) {
                ESP_LOGE(
                    FLASH_TEST_TAG,
                    "Flash read failed: %s",
                    esp_err_to_name(err));

                heap_caps_free(imageData);
                imageData = nullptr;
                return;
            }

            ESP_LOGI(FLASH_TEST_TAG, "Flash read complete");

    ///==========CRC=========================================
            uint32_t readCrc = crc32(
            imageData,
            IMAGE_SIZE);

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Read CRC32       : %08X",
            readCrc);

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Expected CRC32   : %08X",
            EXPECTED_CRC32);

        if (readCrc == EXPECTED_CRC32) {

            ESP_LOGI(
                FLASH_TEST_TAG,
                "================================");

            ESP_LOGI(
                FLASH_TEST_TAG,
                "IMAGE CRC VERIFY SUCCESS!");

            ESP_LOGI(
                FLASH_TEST_TAG,
                "W25Q128 data matches PC binary");

            ESP_LOGI(
                FLASH_TEST_TAG,
                "================================");

        } else {

            ESP_LOGE(
                FLASH_TEST_TAG,
                "================================");

            ESP_LOGE(
                FLASH_TEST_TAG,
                "IMAGE CRC VERIFY FAILED!");

            ESP_LOGE(
                FLASH_TEST_TAG,
                "Read    : %08X",
                readCrc);

            ESP_LOGE(
                FLASH_TEST_TAG,
                "Expected: %08X",
                EXPECTED_CRC32);

            ESP_LOGE(
                FLASH_TEST_TAG,
                "================================");

            heap_caps_free(imageData);
            imageData = nullptr;
            return;
        }

    ///======================================================
        constexpr uint32_t IMAGE_WIDTH = 177;
        constexpr uint32_t IMAGE_HEIGHT = 180;

        static lv_image_dsc_t speedometerImage = {};

        speedometerImage.header.cf = LV_COLOR_FORMAT_RGB565A8;
        speedometerImage.header.magic = LV_IMAGE_HEADER_MAGIC;
        speedometerImage.header.w = IMAGE_WIDTH;
        speedometerImage.header.h = IMAGE_HEIGHT;
        speedometerImage.data_size = IMAGE_SIZE;
        speedometerImage.data = imageData;
        //---------------descriptor info----------------------
        ESP_LOGI(
            FLASH_TEST_TAG,
            "================================");

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Step 1B-2: LVGL image descriptor");

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Color format : RGB565A8");

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Width        : %d",
            speedometerImage.header.w);

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Height       : %d",
            speedometerImage.header.h);

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Data size    : %u bytes",
            static_cast<unsigned>(speedometerImage.data_size));

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Data pointer : %p",
            speedometerImage.data);

        ESP_LOGI(
            FLASH_TEST_TAG,
            "Descriptor created successfully");

        ESP_LOGI(
            FLASH_TEST_TAG,
            "================================");
#endif            
    // ---------------------------------------------------------
    // UI
    // ---------------------------------------------------------
#if 1
    ESP_LOGI(TAG, "Creating UI object");
    UI ui;

    bool ui_ok = ui.init(rgbLed, winbondFlash);

    if (!ui_ok)
    {
        ESP_LOGE(TAG, "UI initialization failed");
        return;
    }
    ESP_LOGI(TAG, "UI initialized");
#endif
    // ---------------------------------------------------------
    // Touch
    // ---------------------------------------------------------
#if 1
    ESP_LOGI(TAG, "Initializing Touch...");

    GT911 touch;

    esp_err_t touch_ret =
        touch.init(
            TOUCH_SDA,
            TOUCH_SCL,
            LCD_RST_GPIO,
            TOUCH_IRQ);

    if (touch_ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "GT911 initialization failed: %s",
            esp_err_to_name(touch_ret));
    }
    else
    {
        ESP_LOGI(TAG, "GT911 initialized");
    }

//-------------------------------------------------------------
 /*while (true)
{
    uint8_t status = touch.debugReadStatus();

    ESP_LOGI(
        "TOUCH_TEST",
        "GT911 STATUS = 0x%02X",
        status);

    vTaskDelay(pdMS_TO_TICKS(500));
}*/
//-------------------------------------------------------------
    LVGLTouch lvglTouch;

    if (!lvglTouch.init(touch)) {
        ESP_LOGE(TAG, "LVGL touch initialization failed");
        return;
    }
#endif

    // ---------------------------------------------------------
    // Main LVGL loop
    // ---------------------------------------------------------
    static uint32_t last_report_flush_count = 0;
    static int64_t  last_report_time = 0;
    while (1)
    {
        // inside your while(1) loop, after lv_timer_handler():
        int64_t now = esp_timer_get_time();
        if (now - last_report_time > 10 * 1000 * 1000) {  // every 10 seconds
            uint32_t count = lvglDisplay.flushContext().flush_count;  // add a small getter
            uint32_t delta = count - last_report_flush_count;

            ESP_LOGI(TAG, "Flush stats: total=%lu, +%lu since last report, max_wait=%lld us",
                    (unsigned long)count, (unsigned long)delta,
                    (long long)lvglDisplay.flushContext().max_wait_us);

            last_report_flush_count = count;
            last_report_time = now;
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif







// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
#if 0
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Initializing W25Q128...");

    WinbondFlash winbondFlash;

    esp_err_t flash_ret = winbondFlash.init();

    testSpeedometerFlashImage(winbondFlash);
#if 0
    if (flash_ret != ESP_OK) {
        ESP_LOGE(TAG, "W25Q128 SPI initialization failed");
    } else {
        ESP_LOGI(TAG, "W25Q128 SPI initialized");

        uint8_t manufacturer;
        uint8_t memory_type;
        uint8_t capacity;

        for (int i = 1; i <= 1; i++)
        {
            //uint8_t mfr, type, cap;

            esp_err_t err = winbondFlash.readJedecId(&manufacturer, &memory_type, &capacity);

            ESP_LOGI(TAG,
                    "%d: %02X %02X %02X  err=%s",
                    i,
                    manufacturer,
                    memory_type,
                    capacity,
                    esp_err_to_name(err));

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        uint8_t manufacturer_;
        uint8_t device_id;

        for (int i = 1; i <= 1; i++)
        {
            //uint8_t mfr, type, cap;

            esp_err_t err = winbondFlash.readManufacturerDeviceId(&manufacturer_, &device_id);

            ESP_LOGI(TAG,
                    "W25Q128FV 0x90: Manufacturer=0x%02X DeviceID=0x%02X",
                    manufacturer_,
                    device_id,
                    esp_err_to_name(err));

            vTaskDelay(pdMS_TO_TICKS(100));
        }

    }

    uint8_t status;

    ESP_LOGI(TAG, "Testing 0x06 Write Enable...");

    esp_err_t err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "0x06 Write Enable OK");

        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK)
        {
            if (status & 0x02)
            {
                ESP_LOGI(TAG,
                        "WEL = 1 -> Write Enable Latch SET");
            }
            else
            {
                ESP_LOGE(TAG,
                        "WEL = 0 -> Write Enable Latch NOT SET");
            }
        }
    }

    ESP_LOGI(TAG, "Testing 0x04 Write Disable...");

    err = winbondFlash.writeDisable();

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "0x04 Write Disable OK");

        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK)
        {
            if (status & 0x02)
            {
                ESP_LOGE(TAG,
                        "WEL = 1 -> Write Enable Latch NOT CLEARED");
            }
            else
            {
                ESP_LOGI(TAG,
                        "WEL = 0 -> Write Enable Latch CLEARED");
            }
        }
    }   
    winbondFlash.printStatusRegister1(status);

    ESP_LOGI(TAG, "Testing waitUntilReady...");

    err = winbondFlash.waitUntilReady();

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "waitUntilReady OK");
    }
    else
    {
        ESP_LOGE(TAG, "waitUntilReady FAILED: %s",
                esp_err_to_name(err));
    }
//--------------------------------------------------------------------------------

//-------------------Erase Sector------------------------
    ESP_LOGI(TAG, "Testing 0x20 Sector Erase...");

    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        uint8_t status = 0;

        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK && (status & 0x02))
        {
            ESP_LOGI(TAG,
                    "WEL = 1 -> ready for Sector Erase");

            err = winbondFlash.sectorErase(0x000000);

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "0x20 Sector Erase OK");

                err = winbondFlash.waitUntilReady();

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG,
                            "Sector Erase complete");
                }
            }
        }
        else
        {
            ESP_LOGE(TAG,
                    "WEL = 0 -> Sector Erase NOT allowed");
        }
    }

//----------------Testing 0x02 Page Program----------------------------------------
    uint8_t testData[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    status = 0;

    ESP_LOGI(TAG, "Testing 0x02 Page Program...");

    // Step 1: Write Enable
    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        // Step 2: Verify WEL
        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK && (status & 0x02))
        {
            ESP_LOGI(TAG, "WEL = 1 -> ready for Page Program");

            // Step 3: Program 4 bytes
            err = winbondFlash.pageProgram(0x000000, testData, sizeof(testData));

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "0x02 Page Program OK");

                // Step 4: Wait for programming to finish
                err = winbondFlash.waitUntilReady();

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Page Program complete");
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "WEL = 0 -> Page Program NOT allowed");
        }
    }

//---------------------------Testing 0x03 Read Data------------------------------

    uint8_t readBuffer[4] = {0};

    ESP_LOGI(TAG, "Testing 0x03 Read Data...");

    err = winbondFlash.readData(0x000000, readBuffer, sizeof(readBuffer));

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG,
                "Read data: %02X %02X %02X %02X",
                readBuffer[0],
                readBuffer[1],
                readBuffer[2],
                readBuffer[3]);

        if (readBuffer[0] == 0xDE &&
            readBuffer[1] == 0xAD &&
            readBuffer[2] == 0xBE &&
            readBuffer[3] == 0xEF)
        {
            ESP_LOGI(TAG, "READ VERIFY SUCCESS!");
        }
        else
        {
            ESP_LOGE(TAG, "READ VERIFY FAILED!");
        }
    }
//-------------------Erase Sector------------------------
    ESP_LOGI(TAG, "Testing 0x20 Sector Erase...");

    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        uint8_t status = 0;

        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK && (status & 0x02))
        {
            ESP_LOGI(TAG,
                    "WEL = 1 -> ready for Sector Erase");

            err = winbondFlash.sectorErase(0x000000);

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "0x20 Sector Erase OK");

                err = winbondFlash.waitUntilReady();

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG,
                            "Sector Erase complete");
                }
            }
        }
        else
        {
            ESP_LOGE(TAG,
                    "WEL = 0 -> Sector Erase NOT allowed");
        }
    }
    //---------------------Verify Erase sector---------------------------
    uint8_t eraseCheck[4] = {0};

    ESP_LOGI(TAG, "Verifying erased data...");

    err = winbondFlash.readData(0x000000,
                        eraseCheck,
                        sizeof(eraseCheck));

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG,
                "After erase: %02X %02X %02X %02X",
                eraseCheck[0],
                eraseCheck[1],
                eraseCheck[2],
                eraseCheck[3]);

        if (eraseCheck[0] == 0xFF &&
            eraseCheck[1] == 0xFF &&
            eraseCheck[2] == 0xFF &&
            eraseCheck[3] == 0xFF)
        {
            ESP_LOGI(TAG, "SECTOR ERASE VERIFY SUCCESS!");
        }
        else
        {
            ESP_LOGE(TAG, "SECTOR ERASE VERIFY FAILED!");
        }
    }
//-------------------------------------------------------------------------

    ESP_LOGI(TAG, "Testing 16-byte Page Program...");

    uint8_t testData1[16] =
    {
        0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF
    };

    uint8_t readBuffer1[16] = {0};
    status = 0;

    // 1. Write Enable
    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        // 2. Check WEL
        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK && (status & 0x02))
        {
            ESP_LOGI(TAG, "WEL = 1 -> ready for Page Program");

            // 3. Program 16 bytes
            err = winbondFlash.pageProgram(0x000000,
                                    testData1,
                                    sizeof(testData1));

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "16-byte Page Program OK");

                // 4. Wait until programming finishes
                err = winbondFlash.waitUntilReady();

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Page Program complete");

                    // 5. Read back
                    ESP_LOGI(TAG, "Reading back 16 bytes...");

                    err = winbondFlash.readData(0x000000,
                                        readBuffer1,
                                        sizeof(readBuffer1));

                    if (err == ESP_OK)
                    {
                        ESP_LOGI(TAG,
                                "Read data:"
                                " %02X %02X %02X %02X"
                                " %02X %02X %02X %02X"
                                " %02X %02X %02X %02X"
                                " %02X %02X %02X %02X",
                                readBuffer1[0],
                                readBuffer1[1],
                                readBuffer1[2],
                                readBuffer1[3],
                                readBuffer1[4],
                                readBuffer1[5],
                                readBuffer1[6],
                                readBuffer1[7],
                                readBuffer1[8],
                                readBuffer1[9],
                                readBuffer1[10],
                                readBuffer1[11],
                                readBuffer1[12],
                                readBuffer1[13],
                                readBuffer1[14],
                                readBuffer1[15]);

                        // 6. Verify
                        if (memcmp(testData1,
                                readBuffer1,
                                sizeof(testData1)) == 0)
                        {
                            ESP_LOGI(TAG,
                                    "16-BYTE READ VERIFY SUCCESS!");
                        }
                        else
                        {
                            ESP_LOGE(TAG,
                                    "16-BYTE READ VERIFY FAILED!");
                        }
                    }
                }
            }
        }
        else
        {
            ESP_LOGE(TAG,
                    "WEL = 0 -> Page Program NOT allowed");
        }
    }
//-------------------------Testing 128-byte Page Program at 0x000100.-----------------------------
    ESP_LOGI(TAG, "Testing 128-byte Page Program at 0x000100...");

    uint8_t testData2[128];
    uint8_t readBuffer2[128] = {0};
    status = 0;

    // Generate test pattern
    for (int i = 0; i < 128; i++)
    {
        testData2[i] = (uint8_t)i;
    }

    // 1. Write Enable
    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        // 2. Check WEL
        err = winbondFlash.readStatusRegister1(&status);

        if (err == ESP_OK && (status & 0x02))
        {
            ESP_LOGI(TAG,
                    "WEL = 1 -> ready for 128-byte Page Program");

            // 3. Program exactly one page
            err = winbondFlash.pageProgram(0x000100,
                                    testData2,
                                    sizeof(testData2));

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG,
                        "128-byte Page Program OK");

                // 4. Wait until programming completes
                err = winbondFlash.waitUntilReady();

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG,
                            "128-byte Page Program complete");

                    // 5. Read back
                    ESP_LOGI(TAG,
                            "Reading back 128 bytes...");

                    err = winbondFlash.readData(0x000100,
                                        readBuffer2,
                                        sizeof(readBuffer2));

                    if (err == ESP_OK)
                    {
                        // 6. Verify
                        if (memcmp(testData2,
                                readBuffer2,
                                sizeof(testData2)) == 0)
                        {
                            ESP_LOGI(TAG,
                                    "128-BYTE READ VERIFY SUCCESS!");
                        }
                        else
                        {
                            ESP_LOGE(TAG,
                                    "128-BYTE READ VERIFY FAILED!");

                            // Find first mismatch
                            for (int i = 0; i < 256; i++)
                            {
                                if (testData2[i] != readBuffer2[i])
                                {
                                    ESP_LOGE(TAG,
                                            "Mismatch at offset 0x%02X: "
                                            "expected %02X, got %02X",
                                            i,
                                            testData2[i],
                                            readBuffer2[i]);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ESP_LOGE(TAG,
                    "WEL = 0 -> Page Program NOT allowed");
        }
    }
//---------------------Erase--Then--256 byte program and verifiy------------------------------------------------

//--------------------------- Testing 256-byte Page Program ------------------------------

    uint8_t testData256[256];
    uint8_t readBuffer256[256] = {0};

    // Pattern: 00 01 02 03 ... FE FF
    for (int i = 0; i < 256; i++)
    {
        testData256[i] = (uint8_t)i;
    }

    ESP_LOGI(TAG, "Testing 256-byte Page Program at 0x000100...");

    // --------------------------------------------------
    // Step 1: Erase sector containing 0x000100
    // --------------------------------------------------

    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        uint8_t eraseStatus = 0;

        err = winbondFlash.readStatusRegister1(&eraseStatus);

        if (err == ESP_OK && (eraseStatus & 0x02))
        {
            ESP_LOGI(TAG, "WEL = 1 -> ready for Sector Erase");

            err = winbondFlash.sectorErase(0x000100);

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "Sector Erase OK");

                err = winbondFlash.waitUntilReady(5000);

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Sector Erase complete");
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "WEL = 0 -> Sector Erase NOT allowed");
        }
    }

    // --------------------------------------------------
    // Step 2: Write Enable for Page Program
    // --------------------------------------------------

    if (err == ESP_OK)
    {
        err = winbondFlash.writeEnable();
    }

    // --------------------------------------------------
    // Step 3: Verify WEL
    // --------------------------------------------------

    if (err == ESP_OK)
    {
        uint8_t programStatus = 0;

        err = winbondFlash.readStatusRegister1(&programStatus);

        if (err == ESP_OK && (programStatus & 0x02))
        {
            ESP_LOGI(TAG,
                     "WEL = 1 -> ready for 256-byte Page Program");

            // --------------------------------------------------
            // Step 4: Program complete 256-byte page
            // --------------------------------------------------

            err = winbondFlash.pageProgram(
                0x000100,
                testData256,
                sizeof(testData256));

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "256-byte Page Program OK");

                // --------------------------------------------------
                // Step 5: Wait for BUSY to clear
                // --------------------------------------------------

                err = winbondFlash.waitUntilReady(5000);

                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG,
                             "256-byte Page Program complete");

                    // --------------------------------------------------
                    // Step 6: Read back entire 256-byte page
                    // --------------------------------------------------

                    ESP_LOGI(TAG,
                             "Reading back 256 bytes...");

                    err = winbondFlash.readData(
                        0x000100,
                        readBuffer256,
                        sizeof(readBuffer256));

                    if (err == ESP_OK)
                    {
                        // --------------------------------------------------
                        // Step 7: Verify every byte
                        // --------------------------------------------------

                        bool match = true;

                        for (int i = 0; i < 256; i++)
                        {
                            if (readBuffer256[i] != testData256[i])
                            {
                                ESP_LOGE(
                                    TAG,
                                    "Mismatch at offset %d: "
                                    "expected 0x%02X, got 0x%02X",
                                    i,
                                    testData256[i],
                                    readBuffer256[i]);

                                match = false;
                                break;
                            }
                        }

                        if (match)
                        {
                            ESP_LOGI(
                                TAG,
                                "256-BYTE READ VERIFY SUCCESS!");
                        }
                        else
                        {
                            ESP_LOGE(
                                TAG,
                                "256-BYTE READ VERIFY FAILED!");
                        }
                    }
                }
            }
        }
        else
        {
            ESP_LOGE(TAG,
                     "WEL = 0 -> 256-byte Page Program NOT allowed");
        }
    }
//-------------------------Erase before 256 boundary test-------------------------------------------------
    ESP_LOGI(TAG, "Erasing sector 0x000000...");

    err = winbondFlash.writeEnable();

    if (err == ESP_OK)
    {
        err = winbondFlash.sectorErase(0x000000);
    }

    if (err == ESP_OK)
    {
        err = winbondFlash.waitUntilReady(5000);
    }

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Sector erase complete");
    }
    else
    {
        ESP_LOGE(TAG,
                "Sector erase failed: %s",
                esp_err_to_name(err));
    }

//-------------------------------Testing write() across 256-byte page boundary---------------------------------
    uint8_t boundaryData[32];

    for (int i = 0; i < 32; i++)
    {
        boundaryData[i] = 0xA0 + i;
    }

    ESP_LOGI(TAG,
            "Testing write() across 256-byte page boundary...");

    err = winbondFlash.write(
        0x0000F0,
        boundaryData,
        sizeof(boundaryData));

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Boundary write OK");

        uint8_t verify[32] = {0};

        err = winbondFlash.readData(
            0x0000F0,
            verify,
            sizeof(verify));

        if (err == ESP_OK)
        {
            bool match = true;

            for (int i = 0; i < 32; i++)
            {
                if (verify[i] != boundaryData[i])
                {
                    ESP_LOGE(TAG,
                            "Mismatch at offset %d: expected 0x%02X, got 0x%02X",
                            i,
                            boundaryData[i],
                            verify[i]);

                    match = false;
                    break;
                }
            }

            if (match)
            {
                ESP_LOGI(TAG,
                        "PAGE-BOUNDARY WRITE VERIFY SUCCESS!");
            }
            else
            {
                ESP_LOGE(TAG,
                        "PAGE-BOUNDARY WRITE VERIFY FAILED!");
            }
        }
    }
//-------------------------------------------------------------------------------------------------------------
    uint8_t multiPageData[512];

    for (int i = 0; i < 512; i++)
        multiPageData[i] = i & 0xFF;

    ESP_LOGI(TAG,
            "Testing 512-byte write across multiple page boundaries...");

    err = winbondFlash.sectorErase(0x000000);

    if (err == ESP_OK)
        err = winbondFlash.waitUntilReady(5000);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                "Sector erase failed: %s",
                esp_err_to_name(err));
    }
    else
    {
        err = winbondFlash.write(
            0x0001F0,
            multiPageData,
            sizeof(multiPageData));

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "512-byte multi-page write OK");

            uint8_t verify[512] = {0};

            err = winbondFlash.readData(
                0x0001F0,
                verify,
                sizeof(verify));

            if (err == ESP_OK)
            {
                bool match = true;

                for (int i = 0; i < 512; i++)
                {
                    if (verify[i] != multiPageData[i])
                    {
                        ESP_LOGE(TAG,
                                "Mismatch at offset %d: "
                                "expected 0x%02X, got 0x%02X",
                                i,
                                multiPageData[i],
                                verify[i]);

                        match = false;
                        break;
                    }
                }

                if (match)
                    ESP_LOGI(TAG,
                            "512-BYTE MULTI-PAGE WRITE VERIFY SUCCESS!");
                else
                    ESP_LOGE(TAG,
                            "512-BYTE MULTI-PAGE WRITE VERIFY FAILED!");
            }
        }
    }

//-------------------------------------------------------------------------------------------------------------
    uint8_t testData3[512];

    for (int i = 0; i < 512; i++)
        testData3[i] = i & 0xFF;

    ESP_LOGI(TAG, "Testing eraseAndWrite()...");

    err = winbondFlash.eraseAndWrite(
        0x0007F0,
        testData3,
        sizeof(testData3));

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "eraseAndWrite() OK");

        uint8_t verify[512] = {0};

        err = winbondFlash.read(
            0x0007F0,
            verify,
            sizeof(verify));

        if (err == ESP_OK)
        {
            bool match = true;

            for (int i = 0; i < 512; i++)
            {
                if (verify[i] != testData3[i])
                {
                    ESP_LOGE(TAG,
                            "Mismatch at offset %d: "
                            "expected 0x%02X got 0x%02X",
                            i,
                            testData3[i],
                            verify[i]);

                    match = false;
                    break;
                }
            }

            if (match)
                ESP_LOGI(TAG,
                        "ERASE-AND-WRITE VERIFY SUCCESS!");
            else
                ESP_LOGE(TAG,
                        "ERASE-AND-WRITE VERIFY FAILED!");
        }
    }
//-------------------------------------------------------------------------------------------------------------
    ESP_LOGI("TFT_TEST", "Testing true sector-boundary eraseAndWrite()...");

    static uint8_t testData4[512];
    static uint8_t verifyData[512];

    for (int i = 0; i < 512; i++) {
        testData4[i] = i & 0xFF;
    }

    err = winbondFlash.eraseAndWrite(
        0x000FF0,
        testData4,
        sizeof(testData4)
    );

    if (err != ESP_OK) {
        ESP_LOGE("TFT_TEST",
                "eraseAndWrite() FAILED: %s",
                esp_err_to_name(err));
    } else {
        ESP_LOGI("TFT_TEST", "eraseAndWrite() OK");

        memset(verifyData, 0, sizeof(verifyData));

        err = winbondFlash.read(
            0x000FF0,
            verifyData,
            sizeof(verifyData)
        );

        if (err != ESP_OK) {
            ESP_LOGE("TFT_TEST",
                    "Read-back FAILED: %s",
                    esp_err_to_name(err));
        } else {
            bool match = true;

            for (int i = 0; i < 512; i++) {
                if (verifyData[i] != testData4[i]) {
                    ESP_LOGE("TFT_TEST",
                            "Mismatch at offset %d: expected 0x%02X, got 0x%02X",
                            i,
                            testData4[i],
                            verifyData[i]);
                    match = false;
                    break;
                }
            }

            if (match) {
                ESP_LOGI("TFT_TEST",
                        "TRUE SECTOR-BOUNDARY VERIFY SUCCESS!");
            }
        }
    }
#endif  
//-------------------------------------------------------------------------------------------------------------
    ESP_LOGI(TAG, "W25Q128 SPI initialized done PTLA Sept 12,2026 12:55");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif


#if 0
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 09,2026 15:17...");

    initBacklight();
    //initRgbLed();

    // Power ON: Red
    //setRgbColor(32, 0, 0);

    // Reset the display controller via RST pin
    ESP_LOGI(TAG, "Resetting display...");

    gpio_set_direction(LCD_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // ---------------------------------------------------------
    // Display
    // ---------------------------------------------------------
#if 0
    ESP_LOGI(TAG, "Creating Display");

    Display display;

    ESP_LOGI(TAG, "Initializing Display");

    ESP_ERROR_CHECK(display.init());

    RGBLed rgbLed;

    if (rgbLed.init() != ESP_OK) {
        ESP_LOGE(TAG, "RGB LED initialization failed");
        return;
    }
#endif    
    // ---------------------------------------------------------
    // LVGL
    // ---------------------------------------------------------
#if 0
    ESP_LOGI(TAG, "Initializing LVGL");

    ESP_LOGI(TAG, "Before lv_init()");
    lv_init();
    ESP_LOGI(TAG, "After lv_init()");

    ESP_LOGI(TAG, "Before lv_tick_set_cb()");
    lv_tick_set_cb([]() -> uint32_t {
        return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    });
    ESP_LOGI(TAG, "After lv_tick_set_cb()");
#endif
#if 0
    ESP_LOGI(TAG, "Creating LVGLDisplay object");
    LVGLDisplay lvglDisplay;

    ESP_LOGI(TAG, "Before lvglDisplay.init()");

    bool lvgl_display_ok = lvglDisplay.init(display);

    ESP_LOGI(TAG, "After lvglDisplay.init()");

    if (!lvgl_display_ok)
    {
        ESP_LOGE(TAG, "LVGL display initialization failed");
        return;
    }


    ESP_LOGI(TAG, "LVGL display initialized");
#endif    
#if 0
    lv_obj_t* screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0xFF0000),
        0);

    lv_refr_now(lvglDisplay.handle());

    ESP_LOGI(TAG, "Red screen requested");
#endif
    // ---------------------------------------------------------
    // UI
    // ---------------------------------------------------------
#if 0
    ESP_LOGI(TAG, "Creating UI object");
    UI ui;

    ESP_LOGI(TAG, "Before ui.init()");
    bool ui_ok = ui.init(rgbLed);

    ESP_LOGI(TAG, "After ui.init()");

    if (!ui_ok)
    {
        ESP_LOGE(TAG, "UI initialization failed");
        return;
    }
    ESP_LOGI(TAG, "UI initialized");
#endif
    // ---------------------------------------------------------
    // Touch
    // ---------------------------------------------------------
#if 0
    ESP_LOGI(TAG, "Initializing Touch...");

    GT911 touch;

    esp_err_t touch_ret =
        touch.init(
            TOUCH_SDA,
            TOUCH_SCL,
            LCD_RST_GPIO,
            TOUCH_IRQ);

    if (touch_ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "GT911 initialization failed: %s",
            esp_err_to_name(touch_ret));
    }
    else
    {
        ESP_LOGI(TAG, "GT911 initialized");
    }

    LVGLTouch lvglTouch;

    if (!lvglTouch.init(touch)) {
        ESP_LOGE(TAG, "LVGL touch initialization failed");
        return;
    }
#endif
#if 1    
    ESP_LOGI(TAG, "Initializing W25Q128...");

    WinbondFlash winbondFlash;

    esp_err_t flash_ret = winbondFlash.init();

    if (flash_ret != ESP_OK) {
        ESP_LOGE(TAG, "W25Q128 SPI initialization failed");
    } else {
        ESP_LOGI(TAG, "W25Q128 SPI initialized");

        uint8_t manufacturer;
        uint8_t memory_type;
        uint8_t capacity;

        for (int i = 1; i <= 10; i++)
        {
            //uint8_t mfr, type, cap;

            esp_err_t err = winbondFlash.readJedecId(&manufacturer, &memory_type, &capacity);

            ESP_LOGI(TAG,
                    "%d: %02X %02X %02X  err=%s",
                    i,
                    manufacturer,
                    memory_type,
                    capacity,
                    esp_err_to_name(err));

            vTaskDelay(pdMS_TO_TICKS(100));
        }


        flash_ret = winbondFlash.readJedecId(
            &manufacturer,
            &memory_type,
            &capacity
        );

        if (flash_ret == ESP_OK) {
            ESP_LOGI(TAG,
                    "W25Q128 JEDEC: Manufacturer=0x%02X "
                    "MemoryType=0x%02X Capacity=0x%02X",
                    manufacturer,
                    memory_type,
                    capacity);
        }

        uint8_t manufacturer_;
        uint8_t device_id;

        for (int i = 1; i <= 10; i++)
        {
            //uint8_t mfr, type, cap;

            esp_err_t err = winbondFlash.readManufacturerDeviceId(&manufacturer_, &device_id);

            ESP_LOGI(TAG,
                    "W25Q128FV 0x90: Manufacturer=0x%02X DeviceID=0x%02X",
                    manufacturer_,
                    device_id);

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        flash_ret = winbondFlash.readManufacturerDeviceId(
            &manufacturer_,
            &device_id
        );

        if (flash_ret == ESP_OK) {
            ESP_LOGI(TAG,
                    "W25Q128FV 0x90: Manufacturer=0x%02X DeviceID=0x%02X",
                    manufacturer_,
                    device_id);
        }
    }

    ESP_LOGI(TAG, "W25Q128 SPI initialized done PTLA");
#endif    
    // ---------------------------------------------------------
    // Main LVGL loop
    // ---------------------------------------------------------

    while (1)
    {
         //ESP_LOGI(TAG, "Before lv_timer_handler()");

    //uint32_t next = lv_timer_handler();

    //ESP_LOGI(TAG, "After lv_timer_handler(): %lu",
    //         static_cast<unsigned long>(next));

    vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

#if 0
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "PTLA Sept 09,2026 14:05...");

    initBacklight();
    initRgbLed();

    // Power ON: Red
    setRgbColor(32, 0, 0);

    // Reset the display controller via RST pin
    ESP_LOGI(TAG, "Resetting display...");
    gpio_set_direction(LCD_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    ESP_LOGI(TAG, "Creating Display");
    Display display;

    ESP_LOGI(TAG, "Initializing Display");
    ESP_ERROR_CHECK(display.init());

    display.fillTestPattern();

    // Initialize GT911 Touch Controller
    ESP_LOGI(TAG, "Initializing Touch...");

    GT911 touch;

    esp_err_t touch_ret = touch.init(TOUCH_SDA, TOUCH_SCL, LCD_RST_GPIO, TOUCH_IRQ);
    

    // Main loop: only read touch when IRQ signals data ready
    while (1)
    {
        if (touch_ret == ESP_OK)
        {
            esp_err_t err = touch.read();

            if (err == ESP_OK)
            {
                if (touch.isTouched())
                {
                    setRgbColor(0, 32, 0);

                    ESP_LOGI(
                        TAG,
                        "Touch: count=%d raw=(%d,%d) disp=(%d,%d)",
                        touch.touchCount(),
                        touch.rawX(),
                        touch.rawY(),
                        touch.displayX(),
                        touch.displayY()
                    );
                }
                else
                {
                    setRgbColor(0, 0, 32);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#endif