#include "UI.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "UI";

namespace
{
constexpr int32_t PLANE_X    = 160;
constexpr int32_t PLANE_Y    = 24;
constexpr int32_t PLANE_SIZE = 220;

constexpr int32_t CLKPLANE_X    = 440;
constexpr int32_t CLKPLANE_Y    = 24;

constexpr int32_t SETTINGSPLANE_X  = 440;
constexpr int32_t SETTINGSPLANE_Y  = 260;

constexpr int32_t WEIGHPLANE_X  = 160;
constexpr int32_t WEIGHPLANE_Y  = 260;

}


void UI::speedometerTouchCallback(lv_event_t* e)
{
    UI* ui = static_cast<UI*>(lv_event_get_user_data(e));

    if (ui == nullptr) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {

        ESP_LOGI(TAG, "Speedometer icon pressed");

        // Touch -> GREEN
        ui->rgbLed_->setGreen();

        lv_label_set_text( ui->dialTouchedLabel_, "Dial touched");

        lv_obj_clear_flag( ui->dialTouchedLabel_, LV_OBJ_FLAG_HIDDEN);
        
        ui->showTouchHalo(ui->speedometerImage_);
    }
    else if (code == LV_EVENT_RELEASED){ //|| code == LV_EVENT_PRESS_LOST) {

        ESP_LOGI(TAG, "Speedometer icon released");

        // Untouched -> BLUE
        ui->rgbLed_->setBlue();
        // Yellow ring OFF
        lv_obj_add_flag( ui->speedometerHalo_, LV_OBJ_FLAG_HIDDEN); 
    }
}


void UI::clockTouchCallback(lv_event_t* e)
{
    UI* ui = static_cast<UI*>(lv_event_get_user_data(e));

    if (ui == nullptr) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {

        ESP_LOGI(TAG, "Clock icon pressed");

        // Touch -> GREEN
        ui->rgbLed_->setGreen();

        lv_label_set_text( ui->clockTouchedLabel_, "Clock touched");

        lv_obj_clear_flag( ui->clockTouchedLabel_, LV_OBJ_FLAG_HIDDEN);
        
        ui->showTouchHalo(ui->clockImage_);
    }
    else if (code == LV_EVENT_RELEASED){ //|| code == LV_EVENT_PRESS_LOST) {

        ESP_LOGI(TAG, "Clock icon released");

        // Untouched -> BLUE
        ui->rgbLed_->setBlue();
        // Yellow ring OFF
        lv_obj_add_flag( ui->clockHalo_, LV_OBJ_FLAG_HIDDEN); 
    }
}

void UI::settingsTouchCallback(lv_event_t* e)
{
    UI* ui = static_cast<UI*>(lv_event_get_user_data(e));

    if (ui == nullptr) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {

        ESP_LOGI(TAG, "Settings icon pressed");

        // Touch -> GREEN
        ui->rgbLed_->setGreen();

        lv_label_set_text( ui->settingsTouchedLabel_, "Settings touched");

        lv_obj_clear_flag( ui->settingsTouchedLabel_, LV_OBJ_FLAG_HIDDEN);
        
        ui->showTouchHalo(ui->settingsImage_);
    }
    else if (code == LV_EVENT_RELEASED){ //|| code == LV_EVENT_PRESS_LOST) {

        ESP_LOGI(TAG, "Settings icon released");

        // Untouched -> BLUE
        ui->rgbLed_->setBlue();
        // Yellow ring OFF
        lv_obj_add_flag( ui->settingsHalo_, LV_OBJ_FLAG_HIDDEN); 
    }
}


void UI::weighTouchCallback(lv_event_t* e)
{
    UI* ui = static_cast<UI*>(lv_event_get_user_data(e));

    if (ui == nullptr) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {

        ESP_LOGI(TAG, "Weighscale icon pressed");

        // Touch -> GREEN
        ui->rgbLed_->setGreen();

        lv_label_set_text( ui->weighTouchedLabel_, "Weighscale touched");

        lv_obj_clear_flag( ui->weighTouchedLabel_, LV_OBJ_FLAG_HIDDEN);
        
        ui->showTouchHalo(ui->weighscaleImage_);
    }
    else if (code == LV_EVENT_RELEASED){ //|| code == LV_EVENT_PRESS_LOST) {

        ESP_LOGI(TAG, "Weighscale icon released");

        // Untouched -> BLUE
        ui->rgbLed_->setBlue();
        // Yellow ring OFF
        lv_obj_add_flag( ui->weighHalo_, LV_OBJ_FLAG_HIDDEN); 
    }
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
    lv_obj_set_style_bg_color( screen_, lv_color_hex(0x202020), 0);

    lv_obj_set_style_bg_opa( screen_, LV_OPA_COVER, 0);

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

    lv_obj_set_pos(clockImage_, CLKPLANE_X, CLKPLANE_Y);

    ESP_LOGI("UI", "Clock image loaded and positioned");

    //-----------------C L O C K H A L O-------------------------
    clockHalo_ = lv_obj_create(screen_);

    if (clockHalo_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Clock halo");
        return false;
    }

    lv_obj_set_size( clockHalo_, CLOCK_IMAGE_WIDTH + 4, CLOCK_IMAGE_HEIGHT + 4);

    lv_obj_set_pos( clockHalo_, CLKPLANE_X - 2, CLKPLANE_Y - 2);

    lv_obj_set_style_bg_opa(clockHalo_, LV_OPA_TRANSP, 0);

    lv_obj_set_style_border_color( clockHalo_, lv_color_hex(0xFFFF00), 0);

    lv_obj_set_style_border_width( clockHalo_, 1, 0);

    lv_obj_set_style_radius( clockHalo_, LV_RADIUS_CIRCLE,  0);

    lv_obj_add_flag( clockHalo_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag( clockHalo_, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_add_flag( clockHalo_, LV_OBJ_FLAG_FLOATING);

    lv_obj_remove_flag( clockHalo_, LV_OBJ_FLAG_CLICKABLE);    

    //--------------------------------------------

    // Make Clock touchable
    lv_obj_add_flag(clockImage_, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb( clockImage_, UI::clockTouchCallback, LV_EVENT_PRESSED, this);

    lv_obj_add_event_cb( clockImage_, UI::clockTouchCallback, LV_EVENT_RELEASED, this);

    //lv_obj_add_event_cb( clockImage_, UI::clockTouchCallback, LV_EVENT_PRESS_LOST, this);

    lv_obj_set_style_bg_opa(clockImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    lv_obj_set_style_image_recolor_opa(clockImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    // -------------------------------------------------
    // Clock touch test message
    // -------------------------------------------------

    clockTouchedLabel_ = lv_label_create(screen_);

    if (clockTouchedLabel_ == nullptr) { 
        ESP_LOGE(TAG, "Failed to create Clock touched label");
        return false;
    }

    lv_label_set_text(clockTouchedLabel_, "Clock touched");

    lv_obj_center(clockTouchedLabel_);

    lv_obj_add_flag( clockTouchedLabel_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "Clock touch callback ready");

    //-----------END OF--------C L O C K H A L O----------
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

    lv_obj_set_size(speedometerImage_, IMAGE_WIDTH, IMAGE_HEIGHT);

    lv_obj_set_pos(speedometerImage_, PLANE_X, PLANE_Y);

    ESP_LOGI(TAG, "Speedometer image centered");    

    //--------------H A L O-----------------------
    speedometerHalo_ = lv_obj_create(screen_);

    if (speedometerHalo_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Speedometer halo");
        return false;
    }

    lv_obj_set_size( speedometerHalo_, IMAGE_WIDTH + 4, IMAGE_HEIGHT + 4);

    lv_obj_set_pos( speedometerHalo_, PLANE_X - 2, PLANE_Y - 2);

    lv_obj_set_style_bg_opa(speedometerHalo_, LV_OPA_TRANSP, 0);

    lv_obj_set_style_border_color( speedometerHalo_, lv_color_hex(0xFFFF00), 0);

    lv_obj_set_style_border_width( speedometerHalo_, 1, 0);

    lv_obj_set_style_radius( speedometerHalo_, LV_RADIUS_CIRCLE,  0);

    lv_obj_add_flag( speedometerHalo_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag( speedometerHalo_, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_add_flag( speedometerHalo_, LV_OBJ_FLAG_FLOATING);

    lv_obj_remove_flag( speedometerHalo_, LV_OBJ_FLAG_CLICKABLE);    

    //--------------------------------------------

    // Make Speedometer touchable
    lv_obj_add_flag(speedometerImage_, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb( speedometerImage_, UI::speedometerTouchCallback, LV_EVENT_PRESSED, this);

    lv_obj_add_event_cb( speedometerImage_, UI::speedometerTouchCallback, LV_EVENT_RELEASED, this);

    //lv_obj_add_event_cb( speedometerImage_, UI::speedometerTouchCallback, LV_EVENT_PRESS_LOST, this);

    lv_obj_set_style_bg_opa(speedometerImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    lv_obj_set_style_image_recolor_opa(speedometerImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    // -------------------------------------------------
    // Speedometer touch test message
    // -------------------------------------------------

    dialTouchedLabel_ = lv_label_create(screen_);

    if (dialTouchedLabel_ == nullptr) { 
        ESP_LOGE(TAG, "Failed to create dial touched label");
        return false;
    }

    lv_label_set_text(dialTouchedLabel_, "Dial touched");

    lv_obj_center(dialTouchedLabel_);

    lv_obj_add_flag( dialTouchedLabel_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "Speedometer touch callback ready");
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

    lv_obj_set_pos(settingsImage_, SETTINGSPLANE_X, SETTINGSPLANE_Y);

    ESP_LOGI("UI", "Settings image loaded and positioned");

    //-----------------S E T T I N G S H A L O-------------------------
    settingsHalo_ = lv_obj_create(screen_);

    if (settingsHalo_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Settings halo");
        return false;
    }

    lv_obj_set_size( settingsHalo_, SETTINGS_IMAGE_WIDTH + 4, SETTINGS_IMAGE_HEIGHT + 4);

    lv_obj_set_pos( settingsHalo_, SETTINGSPLANE_X - 2, SETTINGSPLANE_Y - 2);

    lv_obj_set_style_bg_opa(settingsHalo_, LV_OPA_TRANSP, 0);

    lv_obj_set_style_border_color( settingsHalo_, lv_color_hex(0xFFFF00), 0);

    lv_obj_set_style_border_width( settingsHalo_, 1, 0);

    lv_obj_set_style_radius( settingsHalo_, LV_RADIUS_CIRCLE,  0);

    lv_obj_add_flag( settingsHalo_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag( settingsHalo_, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_add_flag( settingsHalo_, LV_OBJ_FLAG_FLOATING);

    lv_obj_remove_flag( settingsHalo_, LV_OBJ_FLAG_CLICKABLE);    

    //--------------------------------------------

    // Make Settings touchable
    lv_obj_add_flag(settingsImage_, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb( settingsImage_, UI::settingsTouchCallback, LV_EVENT_PRESSED, this);

    lv_obj_add_event_cb( settingsImage_, UI::settingsTouchCallback, LV_EVENT_RELEASED, this);

    //lv_obj_add_event_cb( clockImage_, UI::clockTouchCallback, LV_EVENT_PRESS_LOST, this);

    lv_obj_set_style_bg_opa(settingsImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    lv_obj_set_style_image_recolor_opa(clockImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    // -------------------------------------------------
    // Settings touch test message
    // -------------------------------------------------

    settingsTouchedLabel_ = lv_label_create(screen_);

    if (settingsTouchedLabel_ == nullptr) { 
        ESP_LOGE(TAG, "Failed to create Settings touched label");
        return false;
    }

    lv_label_set_text(settingsTouchedLabel_, "Settings touched");

    lv_obj_center(settingsTouchedLabel_);

    lv_obj_add_flag( settingsTouchedLabel_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "Settings touch callback ready");

    //-----------END OF--------S E T T I N G S H A L O----------
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
        ESP_LOGE("UI", "Failed to allocate Weighscale image buffer");
        return false;
    }

    if (flash_->read(WEIGH_IMAGE_ADDRESS,
                     weighscaleImageData_,
                     WEIGH_IMAGE_SIZE) != ESP_OK) {
        ESP_LOGE("UI", "Failed to read Weighscale image from W25Q128");
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

    lv_obj_set_size(weighscaleImage_, WEIGH_IMAGE_WIDTH, WEIGH_IMAGE_HEIGHT);

    lv_obj_set_pos(weighscaleImage_, WEIGHPLANE_X, WEIGHPLANE_Y);

    ESP_LOGI("UI", "Weighscale image loaded and positioned");

    //-----------------W E I G H S C A L E  H A L O-------------------------
    weighHalo_ = lv_obj_create(screen_);

    if (weighHalo_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Weighscale halo");
        return false;
    }

    lv_obj_set_size( weighHalo_, WEIGH_IMAGE_WIDTH + 4, WEIGH_IMAGE_HEIGHT + 4);

    lv_obj_set_pos( weighHalo_, SETTINGSPLANE_X - 2, SETTINGSPLANE_Y - 2);

    lv_obj_set_style_bg_opa(weighHalo_, LV_OPA_TRANSP, 0);

    lv_obj_set_style_border_color( weighHalo_, lv_color_hex(0xFFFF00), 0);

    lv_obj_set_style_border_width( weighHalo_, 1, 0);

    lv_obj_set_style_radius( weighHalo_, LV_RADIUS_CIRCLE,  0);

    lv_obj_add_flag( weighHalo_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag( weighHalo_, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_add_flag( weighHalo_, LV_OBJ_FLAG_FLOATING);

    lv_obj_remove_flag( weighHalo_, LV_OBJ_FLAG_CLICKABLE);    

    //--------------------------------------------

    // Make Weighscale touchable
    lv_obj_add_flag(weighscaleImage_, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb( weighscaleImage_, UI::weighTouchCallback, LV_EVENT_PRESSED, this);

    lv_obj_add_event_cb( weighscaleImage_, UI::weighTouchCallback, LV_EVENT_RELEASED, this);

    //lv_obj_add_event_cb( clockImage_, UI::clockTouchCallback, LV_EVENT_PRESS_LOST, this);

    lv_obj_set_style_bg_opa(weighscaleImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    lv_obj_set_style_image_recolor_opa(clockImage_, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

    // -------------------------------------------------
    // Weighscale touch test message
    // -------------------------------------------------

    weighTouchedLabel_ = lv_label_create(screen_);

    if (weighTouchedLabel_ == nullptr) { 
        ESP_LOGE(TAG, "Failed to create Weighscale touched label");
        return false;
    }

    lv_label_set_text(weighTouchedLabel_, "Weighscale touched");

    lv_obj_center(weighTouchedLabel_);

    lv_obj_add_flag( weighTouchedLabel_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "Weighscale touch callback ready");

    //-----------END OF--------W E I G H S C A L E H A L O----------

#endif
//---------------------------------------------------------

    return true;
}

void UI::showTouchHalo(lv_obj_t* icon)
{
    constexpr int32_t HALO_GAP = 2;
    constexpr int32_t HALO_BORDER = 3;

     if (icon == nullptr) {
        return;
    }

    // Hide both halos first
    if (speedometerHalo_ != nullptr) {
        lv_obj_add_flag(speedometerHalo_, LV_OBJ_FLAG_HIDDEN);
    }

    if (clockHalo_ != nullptr) {
        lv_obj_add_flag(clockHalo_, LV_OBJ_FLAG_HIDDEN);
    }

    if (settingsHalo_ != nullptr) {
        lv_obj_add_flag(settingsHalo_, LV_OBJ_FLAG_HIDDEN);
    }

    if (weighHalo_ != nullptr) {
        lv_obj_add_flag(weighHalo_, LV_OBJ_FLAG_HIDDEN);
    }

    if (icon == speedometerImage_) {
        // Hide clock halo
        if (clockHalo_ != nullptr) {
            lv_obj_add_flag(clockHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (settingsHalo_ != nullptr) {
            lv_obj_add_flag(settingsHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (weighHalo_ != nullptr) {
            lv_obj_add_flag(weighHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        //---------------------S P E E D H A L O-----------------------
        // Make sure halo follows the icon position
        lv_obj_set_pos( speedometerHalo_, lv_obj_get_x(icon) - HALO_GAP - HALO_BORDER , lv_obj_get_y(icon) - HALO_GAP - HALO_BORDER);

        lv_obj_set_size( speedometerHalo_, lv_obj_get_width(icon) + 2 * (HALO_GAP + HALO_BORDER), lv_obj_get_height(icon) + 2 * (HALO_GAP + HALO_BORDER));

        lv_obj_set_style_border_width(speedometerHalo_, HALO_BORDER, 0);
        
        // Start at full opacity
        lv_obj_set_style_border_opa( speedometerHalo_, LV_OPA_COVER, 0);

        lv_obj_clear_flag( speedometerHalo_, LV_OBJ_FLAG_HIDDEN);
     //---------------------S P E E D H A L O-----E N D S-----------
    }
    else if (icon == clockImage_) {

        // Hide Speedometer halo
        if (speedometerHalo_ != nullptr) {
            lv_obj_add_flag(speedometerHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (settingsHalo_ != nullptr) {
            lv_obj_add_flag(settingsHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (weighHalo_ != nullptr) {
            lv_obj_add_flag(weighHalo_, LV_OBJ_FLAG_HIDDEN);
        }
     //---------------------C L O C K H A L O-----------------------
        lv_obj_set_pos( clockHalo_, lv_obj_get_x(icon) - HALO_GAP - HALO_BORDER , lv_obj_get_y(icon) - HALO_GAP - HALO_BORDER);

        lv_obj_set_size( clockHalo_, lv_obj_get_width(icon) + 2 * (HALO_GAP + HALO_BORDER), lv_obj_get_height(icon) + 2 * (HALO_GAP + HALO_BORDER));

        lv_obj_set_style_border_width(clockHalo_, HALO_BORDER, 0);
        
        // Start at full opacity
        lv_obj_set_style_border_opa( clockHalo_, LV_OPA_COVER, 0);

        lv_obj_clear_flag( clockHalo_, LV_OBJ_FLAG_HIDDEN);
        //---------------------C L O C K H A L O-----E N D S-----------
    }    
    else if (icon == settingsImage_) {

        // Hide Speedometer halo
        if (speedometerHalo_ != nullptr) {
            lv_obj_add_flag(speedometerHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (clockHalo_ != nullptr) {
            lv_obj_add_flag(clockHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (weighHalo_ != nullptr) {
            lv_obj_add_flag(weighHalo_, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_pos( settingsHalo_, lv_obj_get_x(icon) - HALO_GAP - HALO_BORDER , lv_obj_get_y(icon) - HALO_GAP - HALO_BORDER);

        lv_obj_set_size( settingsHalo_, lv_obj_get_width(icon) + 2 * (HALO_GAP + HALO_BORDER), lv_obj_get_height(icon) + 2 * (HALO_GAP + HALO_BORDER));

        lv_obj_set_style_border_width(settingsHalo_, HALO_BORDER, 0);
        
        // Start at full opacity
        lv_obj_set_style_border_opa( settingsHalo_, LV_OPA_COVER, 0);

        lv_obj_clear_flag( settingsHalo_, LV_OBJ_FLAG_HIDDEN);
    }    
    else if (icon == weighscaleImage_) {

        // Hide Speedometer halo
        if (speedometerHalo_ != nullptr) {
            lv_obj_add_flag(speedometerHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (clockHalo_ != nullptr) {
            lv_obj_add_flag(clockHalo_, LV_OBJ_FLAG_HIDDEN);
        }
        else if (settingsHalo_ != nullptr) {
            lv_obj_add_flag(settingsHalo_, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_pos( weighHalo_, lv_obj_get_x(icon) - HALO_GAP - HALO_BORDER , lv_obj_get_y(icon) - HALO_GAP - HALO_BORDER);

        lv_obj_set_size( weighHalo_, lv_obj_get_width(icon) + 2 * (HALO_GAP + HALO_BORDER), lv_obj_get_height(icon) + 2 * (HALO_GAP + HALO_BORDER));

        lv_obj_set_style_border_width(weighHalo_, HALO_BORDER, 0);
        
        // Start at full opacity
        lv_obj_set_style_border_opa( weighHalo_, LV_OPA_COVER, 0);

        lv_obj_clear_flag( weighHalo_, LV_OBJ_FLAG_HIDDEN);
    }
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