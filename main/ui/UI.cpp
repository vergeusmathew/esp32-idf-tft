#include "UI.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "UI";

namespace
{
constexpr int32_t PLANE_X    = 160;
constexpr int32_t PLANE_Y    = 24;
constexpr int32_t PLANE_SIZE = 220;
}

bool UI::init(RGBLed& rgbLed, WinbondFlash& flash)
{
    rgbLed_ = &rgbLed;
    flash_ = &flash;
    screen_ = lv_screen_active();

    if (screen_ == nullptr) {
        ESP_LOGE(TAG, "Could not get active LVGL screen");
        return false;
    }

    // Dark background
    lv_obj_set_style_bg_color(
        screen_,
        lv_color_hex(0x202020),
        0);

    lv_obj_set_style_bg_opa(
        screen_,
        LV_OPA_COVER,
        0);

    // -------------------------------------------------
    // Step 1A: 200 x 200 solid white circle
    // -------------------------------------------------
#if 0
    speedometerPlane_ = lv_obj_create(screen_);

    if (speedometerPlane_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Speedometer plane");
        return false;
    }

    lv_obj_set_size(
        speedometerPlane_,
        PLANE_SIZE,
        PLANE_SIZE);

    lv_obj_set_pos(
        speedometerPlane_,
        PLANE_X,
        PLANE_Y);

    // Transparent rectangular plane
    lv_obj_set_style_border_width(
        speedometerPlane_,
        0,
        0);

    lv_obj_set_style_pad_all(
        speedometerPlane_,
        0,
        0);

    lv_obj_set_style_bg_opa(
        speedometerPlane_,
        LV_OPA_TRANSP,
        0);

    lv_obj_set_scrollbar_mode(
        speedometerPlane_,
        LV_SCROLLBAR_MODE_OFF);
#endif
    // -------------------------------------------------
    // White circle
    // -------------------------------------------------
#if 0
    //speedometerCircle_ = lv_obj_create(speedometerPlane_);
    speedometerCircle_ = lv_obj_create(screen_);
    if (speedometerCircle_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Speedometer circle");
        return false;
    }

    ESP_LOGI(TAG, "LVGL image object created");

    lv_obj_set_size(
        speedometerCircle_,
        PLANE_SIZE,
        PLANE_SIZE);

    lv_obj_center(speedometerCircle_);

    lv_obj_set_style_border_width(
        speedometerCircle_,
        0,
        0);

    lv_obj_set_style_pad_all(
        speedometerCircle_,
        0,
        0);

    lv_obj_set_style_bg_color(
        speedometerCircle_,
        lv_color_white(),
        0);

    lv_obj_set_style_bg_opa(
        speedometerCircle_,
        LV_OPA_COVER, //lv_pct(75), //LV_OPA_TRANSP,//LV_OPA_COVER,
        0);

    lv_obj_set_style_radius(
        speedometerCircle_,
        LV_RADIUS_CIRCLE,   // 0 for square
        0);

    lv_obj_set_scrollbar_mode(
        speedometerCircle_,
        LV_SCROLLBAR_MODE_OFF);

    /*lv_obj_set_style_clip_corner(
        speedometerCircle_,
        true,
        0);
    */    
    ESP_LOGI(TAG, "Step 1A: solid circle created");
    ESP_LOGI(TAG, "Circle: %d x %d @ (%d,%d)",
             PLANE_SIZE,
             PLANE_SIZE,
             PLANE_X,
             PLANE_Y);
#endif
//-----------------------CLOCK------------------------------
#if 1
    const uint32_t CLOCK_IMAGE_ADDRESS = 0x030000;
    const size_t CLOCK_IMAGE_SIZE = 120000;
    const uint32_t CLOCK_IMAGE_WIDTH = 200;
    const uint32_t CLOCK_IMAGE_HEIGHT = 200;
/*
    clockImageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(CLOCK_IMAGE_SIZE,
                         MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
*/
    clockImageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(
            CLOCK_IMAGE_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (clockImageData_ == nullptr) {
        ESP_LOGE("UI", "Failed to allocate clock image buffer");
        return false;
    }

    if (flash_->read(CLOCK_IMAGE_ADDRESS,
                     clockImageData_,
                     CLOCK_IMAGE_SIZE) != ESP_OK) {
        ESP_LOGE("UI", "Failed to read clock image from W25Q128");
        return false;
    }

    clockImageDescriptor_.header.cf = LV_COLOR_FORMAT_RGB565A8;
    clockImageDescriptor_.header.magic = LV_IMAGE_HEADER_MAGIC;
    clockImageDescriptor_.header.w = CLOCK_IMAGE_WIDTH;
    clockImageDescriptor_.header.h = CLOCK_IMAGE_HEIGHT;
    clockImageDescriptor_.header.stride = CLOCK_IMAGE_WIDTH * 2;
    clockImageDescriptor_.data_size = CLOCK_IMAGE_SIZE;
    clockImageDescriptor_.data = clockImageData_;

    clockImage_ = lv_image_create(screen_);

    lv_image_set_src(clockImage_, &clockImageDescriptor_);

    lv_obj_set_size(clockImage_,
                    CLOCK_IMAGE_WIDTH,
                    CLOCK_IMAGE_HEIGHT);

    lv_obj_set_pos(clockImage_, 440, 24);

    ESP_LOGI("UI", "Clock image loaded and positioned");
#endif
//---------------------------------------------------------
#if 1             
        // -------------------------------------------------
    // Step 1B-3: Speedometer image
    // -------------------------------------------------

    constexpr uint32_t IMAGE_ADDRESS = 0x010000;
    constexpr size_t IMAGE_SIZE = 120000; //95580; //47628;
    constexpr uint32_t IMAGE_WIDTH = 200; //126;
    constexpr uint32_t IMAGE_HEIGHT = 200; //126;

    
    imageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(
            IMAGE_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    
    /*
    imageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(IMAGE_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

    */    
    if (imageData_ == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate image buffer in PSRAM");
        return false;
    }

    ESP_LOGI(TAG, "Speedometer image buffer allocated");

    esp_err_t err = flash_->read(
        IMAGE_ADDRESS,
        imageData_,
        IMAGE_SIZE);

    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "Failed to read Speedometer image: %s",
                 esp_err_to_name(err));

        heap_caps_free(imageData_);
        imageData_ = nullptr;

        return false;
    }

    ESP_LOGI(TAG, "Speedometer image read: %u bytes",
             static_cast<unsigned>(IMAGE_SIZE));

    // Create LVGL image descriptor
    speedometerImageDescriptor_.header.cf =     LV_COLOR_FORMAT_RGB565A8;
    speedometerImageDescriptor_.header.magic =  LV_IMAGE_HEADER_MAGIC;
    speedometerImageDescriptor_.header.w =      IMAGE_WIDTH;
    speedometerImageDescriptor_.header.h =      IMAGE_HEIGHT;
    speedometerImageDescriptor_.header.stride = IMAGE_WIDTH * 2;
    speedometerImageDescriptor_.data_size =     IMAGE_SIZE;
    speedometerImageDescriptor_.data =          imageData_;
    ESP_LOGI(TAG, "Speedometer image descriptor ready");

    // Create LVGL image object
    speedometerImage_ =  lv_image_create(screen_);

    //speedometerImage_ =    lv_image_create(speedometerPlane_);

    if (speedometerImage_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Speedometer image object");
        return false;
    }

    ESP_LOGI(TAG, "Speedometer image object created");

    // Give LVGL the image descriptor
    lv_image_set_src(speedometerImage_, &speedometerImageDescriptor_);

    lv_obj_set_size(speedometerImage_,
                    IMAGE_WIDTH,
                    IMAGE_HEIGHT);

    lv_obj_set_pos(speedometerImage_, PLANE_X, PLANE_Y);

    ESP_LOGI(TAG, "Speedometer image centered");    
#endif
//-----------------------SETTINGS------------------------------
#if 1
    const uint32_t SETTINGS_IMAGE_ADDRESS = 0x050000;
    const size_t SETTINGS_IMAGE_SIZE = 120000;
    const uint32_t SETTINGS_IMAGE_WIDTH = 200;
    const uint32_t SETTINGS_IMAGE_HEIGHT = 200;
/*
    clockImageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(CLOCK_IMAGE_SIZE,
                         MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
*/
    settingsImageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(
            SETTINGS_IMAGE_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (settingsImageData_ == nullptr) {
        ESP_LOGE("UI", "Failed to allocate Settings image buffer");
        return false;
    }

    if (flash_->read(SETTINGS_IMAGE_ADDRESS,
                     settingsImageData_,
                     SETTINGS_IMAGE_SIZE) != ESP_OK) {
        ESP_LOGE("UI", "Failed to read Settings image from W25Q128");
        return false;
    }

    settingsImageDescriptor_.header.cf = LV_COLOR_FORMAT_RGB565A8;
    settingsImageDescriptor_.header.magic = LV_IMAGE_HEADER_MAGIC;
    settingsImageDescriptor_.header.w = SETTINGS_IMAGE_WIDTH;
    settingsImageDescriptor_.header.h = SETTINGS_IMAGE_HEIGHT;
    settingsImageDescriptor_.header.stride = SETTINGS_IMAGE_WIDTH * 2;
    settingsImageDescriptor_.data_size = SETTINGS_IMAGE_SIZE;
    settingsImageDescriptor_.data = settingsImageData_;

    settingsImage_ = lv_image_create(screen_);

    lv_image_set_src(settingsImage_, &settingsImageDescriptor_);

    lv_obj_set_size(settingsImage_,
                    SETTINGS_IMAGE_WIDTH,
                    SETTINGS_IMAGE_HEIGHT);

    lv_obj_set_pos(settingsImage_, 440, 260);

    ESP_LOGI("UI", "Settings image loaded and positioned");
#endif
//---------------------------------------------------------

//-----------------------WEIGHSCALE------------------------------
#if 1
    const uint32_t WEIGH_IMAGE_ADDRESS = 0x070000;
    const size_t WEIGH_IMAGE_SIZE = 120000;
    const uint32_t WEIGH_IMAGE_WIDTH = 200;
    const uint32_t WEIGH_IMAGE_HEIGHT = 200;
/*
    clockImageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(CLOCK_IMAGE_SIZE,
                         MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
*/
    weighscaleImageData_ = static_cast<uint8_t*>(
        heap_caps_malloc(
            WEIGH_IMAGE_SIZE,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (weighscaleImageData_ == nullptr) {
        ESP_LOGE("UI", "Failed to allocate Settings image buffer");
        return false;
    }

    if (flash_->read(WEIGH_IMAGE_ADDRESS,
                     weighscaleImageData_,
                     WEIGH_IMAGE_SIZE) != ESP_OK) {
        ESP_LOGE("UI", "Failed to read Settings image from W25Q128");
        return false;
    }

    weighscaleImageDescriptor_.header.cf = LV_COLOR_FORMAT_RGB565A8;
    weighscaleImageDescriptor_.header.magic = LV_IMAGE_HEADER_MAGIC;
    weighscaleImageDescriptor_.header.w = WEIGH_IMAGE_WIDTH;
    weighscaleImageDescriptor_.header.h = WEIGH_IMAGE_HEIGHT;
    weighscaleImageDescriptor_.header.stride = WEIGH_IMAGE_WIDTH * 2;
    weighscaleImageDescriptor_.data_size = WEIGH_IMAGE_SIZE;
    weighscaleImageDescriptor_.data = weighscaleImageData_;

    weighscaleImage_ = lv_image_create(screen_);

    lv_image_set_src(weighscaleImage_, &weighscaleImageDescriptor_);

    lv_obj_set_size(weighscaleImage_,
                    WEIGH_IMAGE_WIDTH,
                    WEIGH_IMAGE_HEIGHT);

    lv_obj_set_pos(weighscaleImage_, 160, 260);

    ESP_LOGI("UI", "Settings image loaded and positioned");
#endif
//---------------------------------------------------------

    return true;
}

UI::~UI()
{
    if (imageData_ != nullptr) {
        heap_caps_free(imageData_);
        imageData_ = nullptr;
    }
    if (clockImageData_ != nullptr) {
        heap_caps_free(clockImageData_);
        clockImageData_ = nullptr;
    }
}