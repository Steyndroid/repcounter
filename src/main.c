#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "helbur_small.h"
#include "helbur_splash.h"
#include "display/esp32_s3.h"
#include "display/matouch_7inch_1024x600.h"
#include "task/counter_task.h"

static const char *TAG = "MAIN";

// LVGL mutex (from your original main.c and esp32_s3.c)
extern SemaphoreHandle_t lvgl_mux;

// Display configuration (from matouch_7inch_1024x600.h)
#define LCD_WIDTH LCD_H_RES  // 1024
#define LCD_HEIGHT LCD_V_RES // 600

// Function prototypes
void display_init(void);
void lvgl_init(void);

// Toggle switch callback
void mode_switch_event_cb(lv_event_t *e)
{
    lv_obj_t *mode_switch = lv_event_get_target(e);
    lv_obj_t *kg_slider = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *kg_value_label = kg_slider ? kg_slider->user_data : NULL;
    lv_obj_t *weight_bar = kg_value_label ? kg_value_label->user_data : NULL;

    if (lv_obj_has_state(mode_switch, LV_STATE_CHECKED))
    {
        // ADP mode
        if (kg_slider)
            lv_obj_add_flag(kg_slider, LV_OBJ_FLAG_HIDDEN);
        if (weight_bar)
            lv_obj_clear_flag(weight_bar, LV_OBJ_FLAG_HIDDEN);
        if (kg_value_label)
            lv_label_set_text(kg_value_label, "15"); // Example value
    }
    else
    {
        // CNS mode
        if (kg_slider)
            lv_obj_clear_flag(kg_slider, LV_OBJ_FLAG_HIDDEN);
        if (weight_bar)
            lv_obj_add_flag(weight_bar, LV_OBJ_FLAG_HIDDEN);
        if (kg_value_label)
            lv_label_set_text(kg_value_label, "0");
    }
}

void app_main(void)
{
    printf("Starting app_main\n");
    ESP_LOGI(TAG, "Starting app_main");

    display_init();

    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    lvgl_init();
    xSemaphoreGiveRecursive(lvgl_mux);

    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    ESP_LOGI(TAG, "Setting background color");
    lv_disp_t *disp = lv_disp_get_default();
    lv_obj_set_style_bg_color(disp->screens[0], lv_color_hex(0x223A44), LV_PART_MAIN);
    xSemaphoreGiveRecursive(lvgl_mux);

    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    ESP_LOGI(TAG, "Creating splash screen");
    lv_obj_t *splash_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x223A44), LV_PART_MAIN);

    lv_obj_t *splash_img = lv_img_create(splash_screen);
    lv_img_set_src(splash_img, &helbur_splash);
    lv_obj_set_pos(splash_img, (LCD_WIDTH - 800) / 2, (LCD_HEIGHT - 139) / 2);

    ESP_LOGI(TAG, "Loading splash screen");
    lv_scr_load(splash_screen);
    ESP_LOGI(TAG, "Splash screen loaded");
    xSemaphoreGiveRecursive(lvgl_mux);

    ESP_LOGI(TAG, "Delaying for 3 seconds");
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "Delay complete");

    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    ESP_LOGI(TAG, "Creating main UI");
    lv_obj_t *main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x223A44), LV_PART_MAIN);

    // Helbur logo (100x17), at (60, 60)
    ESP_LOGI(TAG, "Adding small logo");
    lv_obj_t *small_logo = lv_img_create(main_screen);
    lv_img_set_src(small_logo, &helbur_small);
    lv_obj_set_pos(small_logo, 60, 60);

    // Toggle switch with "CNS" and "ADP" labels on either side
    lv_obj_t *cns_label = lv_label_create(main_screen);
    lv_label_set_text(cns_label, "CNS");
    lv_obj_set_style_text_color(cns_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(cns_label, &lv_font_montserrat_36, LV_PART_MAIN); // Larger font
    lv_obj_align(cns_label, LV_ALIGN_BOTTOM_MID, -100, -50);                     // Left of switch

    lv_obj_t *mode_switch = lv_switch_create(main_screen);
    lv_obj_set_style_bg_color(mode_switch, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mode_switch, lv_color_hex(0xBCD24B), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_size(mode_switch, 100, 40);
    lv_obj_align(mode_switch, LV_ALIGN_BOTTOM_MID, 0, -50);

    lv_obj_t *adp_label = lv_label_create(main_screen);
    lv_label_set_text(adp_label, "ADP");
    lv_obj_set_style_text_color(adp_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(adp_label, &lv_font_montserrat_36, LV_PART_MAIN); // Larger font
    lv_obj_align(adp_label, LV_ALIGN_BOTTOM_MID, 100, -50);                      // Right of switch

    // KG Label (left side, above value)
    lv_obj_t *kg_label = lv_label_create(main_screen);
    lv_label_set_text(kg_label, "KG");
    lv_obj_set_style_text_color(kg_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(kg_label, &lv_font_montserrat_48, LV_PART_MAIN); // Larger font
    lv_obj_set_pos(kg_label, 200, 230);

    // KG Value Label (below "KG")
    lv_obj_t *kg_value_label = lv_label_create(main_screen);
    lv_label_set_text(kg_value_label, "0");
    lv_obj_set_style_text_color(kg_value_label, lv_color_hex(0xBCD24B), LV_PART_MAIN);
    lv_obj_set_style_text_font(kg_value_label, &lv_font_montserrat_48, LV_PART_MAIN); // Adjusted to 48 for ~7-8mm height
    lv_obj_set_pos(kg_value_label, 200, 280);

    // CNS Mode: Vertical Slider (0 to 30)
    ESP_LOGI(TAG, "Adding KG slider");
    lv_obj_t *kg_slider = lv_slider_create(main_screen);
    lv_slider_set_range(kg_slider, 0, 30);
    lv_obj_set_size(kg_slider, 20, 300); // Vertical
    lv_obj_set_style_bg_color(kg_slider, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_bg_color(kg_slider, lv_color_hex(0xBCD24B), LV_PART_KNOB);
    lv_obj_align(kg_slider, LV_ALIGN_CENTER, 0, 0);

    // ADP Mode: Vertical Weight Indicator Bar (0 to 30)
    lv_obj_t *weight_bar = lv_bar_create(main_screen);
    lv_bar_set_range(weight_bar, 0, 30);
    lv_bar_set_value(weight_bar, 15, LV_ANIM_OFF);
    lv_obj_set_size(weight_bar, 20, 300);
    lv_obj_set_style_bg_color(weight_bar, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_bg_color(weight_bar, lv_color_hex(0xBCD24B), LV_PART_INDICATOR);
    lv_obj_align(weight_bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(weight_bar, LV_OBJ_FLAG_HIDDEN);

    // Rep Counter Label (right side, aligned with "KG")
    ESP_LOGI(TAG, "Adding rep counter");
    lv_obj_t *rep_label = lv_label_create(main_screen);
    lv_label_set_text(rep_label, "REPS");
    lv_obj_set_style_text_color(rep_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(rep_label, &lv_font_montserrat_48, LV_PART_MAIN); // Larger font
    lv_obj_set_pos(rep_label, 700, 230);

    // Rep Value Label (below "REPS")
    lv_obj_t *rep_value_label = lv_label_create(main_screen);
    lv_label_set_text(rep_value_label, "0");
    lv_obj_set_style_text_color(rep_value_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(rep_value_label, &lv_font_montserrat_48, LV_PART_MAIN); // Adjusted to 48 for ~7-8mm height
    lv_obj_set_pos(rep_value_label, 700, 280);

    // Link objects for mode switch callback
    kg_slider->user_data = kg_value_label;
    kg_value_label->user_data = weight_bar;

    lv_obj_add_event_cb(mode_switch, mode_switch_event_cb, LV_EVENT_VALUE_CHANGED, kg_slider);

    ESP_LOGI(TAG, "Loading main UI");
    lv_scr_load(main_screen);
    ESP_LOGI(TAG, "Main UI loaded");
    xSemaphoreGiveRecursive(lvgl_mux);

    ESP_LOGI(TAG, "Entering main loop");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void display_init(void)
{
    // Directly call the existing display initialization functions
    ESP_LOGI(TAG, "Calling init_display");
    init_display();
    ESP_LOGI(TAG, "Calling set_backlight_brightness");
    set_backlight_brightness(2);
    ESP_LOGI(TAG, "Display initialized");
}

void lvgl_init(void)
{
    // This function is already handled by init_display() in esp32_s3.c
    // init_display() calls init_lvgl(), which sets up LVGL, the display driver, and the input device driver
    ESP_LOGI(TAG, "LVGL initialized (via init_display)");
}