#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

class Display
{
public:
    static constexpr int WIDTH  = 800;
    static constexpr int HEIGHT = 480;

    Display() = default;
    ~Display() = default;

    esp_err_t init();

    uint16_t *framebuffer();

    void fillTestPattern();

private:
    esp_lcd_panel_handle_t panel_handle_ = nullptr;
    uint16_t *framebuffer_ = nullptr;
};