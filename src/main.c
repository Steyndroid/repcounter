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
#include "lvgl/lv_font_montserrat_72.h"
#include "driver/uart.h"

static const char *TAG = "MAIN";

// LVGL mutex (from esp32_s3.c)
extern SemaphoreHandle_t lvgl_mux;

// Display configuration (from matouch_7inch_1024x600.h)
#define LCD_WIDTH LCD_H_RES  // 1024
#define LCD_HEIGHT LCD_V_RES // 600

// Function prototypes
void display_init(void);

// Slider (Arc) event callback
static void kg_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    int32_t value = lv_arc_get_value(slider);
    // Constrain value to 15–50 kg
    value = (value < 15) ? 15 : (value > 50) ? 50 : value;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)value);
    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    lv_label_set_text(label, buf);
    lv_arc_set_value(slider, value); // Ensure slider reflects constrained value
    xSemaphoreGiveRecursive(lvgl_mux);
    // Send weight to Arduino
    char uart_buf[32];
    snprintf(uart_buf, sizeof(uart_buf), "WEIGHT:%d\n", (int)value);
    int bytes_written = uart_write_bytes(UART_NUM_1, uart_buf, strlen(uart_buf));
    ESP_LOGI(TAG, "Sent weight command: %s, bytes written: %d", uart_buf, bytes_written);
}

// Toggle switch callback
void mode_switch_event_cb(lv_event_t *e)
{
    lv_obj_t *mode_switch = lv_event_get_target(e);
    lv_obj_t *kg_slider = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *kg_value_label = kg_slider ? kg_slider->user_data : NULL;
    lv_obj_t *weight_bar = kg_value_label ? kg_value_label->user_data : NULL;

    xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
    if (lv_obj_has_state(mode_switch, LV_STATE_CHECKED))
    {
        // ADP mode
        if (kg_slider)
            lv_obj_add_flag(kg_slider, LV_OBJ_FLAG_HIDDEN);
        if (weight_bar)
            lv_obj_clear_flag(weight_bar, LV_OBJ_FLAG_HIDDEN);
        if (kg_value_label)
        {
            int32_t bar_value = weight_bar ? lv_arc_get_value(weight_bar) : 15;
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", (int)bar_value);
            lv_label_set_text(kg_value_label, buf);
        }
        // Send ADP mode to Arduino
        char buf[32];
        snprintf(buf, sizeof(buf), "MODE:ADP\n");
        uart_write_bytes(UART_NUM_1, buf, strlen(buf));
        ESP_LOGI(TAG, "Switched to ADP mode");
    }
    else
    {
        // CNS mode
        if (kg_slider)
        {
            lv_arc_set_value(kg_slider, 15); // Reset arc to 15 kg
            lv_obj_clear_flag(kg_slider, LV_OBJ_FLAG_HIDDEN);
        }
        if (weight_bar)
            lv_obj_add_flag(weight_bar, LV_OBJ_FLAG_HIDDEN);
        if (kg_value_label)
        {
            int32_t slider_value = kg_slider ? lv_arc_get_value(kg_slider) : 15;
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", (int)slider_value);
            lv_label_set_text(kg_value_label, buf);
        }
        // Send CNS mode to Arduino
        char buf[32];
        snprintf(buf, sizeof(buf), "MODE:CNS\n");
        uart_write_bytes(UART_NUM_1, buf, strlen(buf));
        ESP_LOGI(TAG, "Switched to CNS mode");
    }
    xSemaphoreGiveRecursive(lvgl_mux);
}

void app_main(void)
{
    printf("Starting app_main\n");
    ESP_LOGI(TAG, "Starting app_main");

    // Initialize UART for Arduino communication
    ESP_LOGI(TAG, "Setting up UART");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 20, 19, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 256, 256, 0, NULL, 0));

    display_init();

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
    lv_obj_set_pos(splash_img, (LCD_WIDTH - 400) / 2, (LCD_HEIGHT - 70) / 2);

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

    // Helbur logo (100x17, scaled to ~176x30), at (50, 50)
    ESP_LOGI(TAG, "Adding small logo");
    lv_obj_t *small_logo = lv_img_create(main_screen);
    lv_img_set_src(small_logo, &helbur_small);
    lv_img_set_zoom(small_logo, 450);
    lv_obj_set_pos(small_logo, 130, 50);

    // Toggle switch with "CNS" and "ADP" labels on either side
    lv_obj_t *cns_label = lv_label_create(main_screen);
    lv_label_set_text(cns_label, "CNS");
    lv_obj_set_style_text_color(cns_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(cns_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_set_pos(cns_label, 620, 50);

    lv_obj_t *mode_switch = lv_switch_create(main_screen);
    lv_obj_set_style_bg_color(mode_switch, lv_color_hex(0x2E4E5C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(mode_switch, lv_color_hex(0x2E4E5C), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(mode_switch, lv_color_hex(0xBCD24B), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_size(mode_switch, 80, 30);
    lv_obj_set_style_height(mode_switch, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_height(mode_switch, 30, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(mode_switch, 0, LV_PART_KNOB);
    lv_obj_set_style_size(mode_switch, 30, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_pos(mode_switch, 703, 53);

    lv_obj_t *adp_label = lv_label_create(main_screen);
    lv_label_set_text(adp_label, "ADP");
    lv_obj_set_style_text_color(adp_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(adp_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_set_pos(adp_label, 810, 50);

    // KG Label (left side, above value)
    lv_obj_t *kg_label = lv_label_create(main_screen);
    lv_label_set_text(kg_label, "KG");
    lv_obj_set_style_text_color(kg_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(kg_label, &lv_font_montserrat_72, LV_PART_MAIN);
    lv_obj_set_pos(kg_label, 200, 250);

    // KG Value Label (below "KG")
    lv_obj_t *kg_value_label = lv_label_create(main_screen);
    lv_label_set_text(kg_value_label, "15"); // Initial value
    lv_obj_set_style_text_color(kg_value_label, lv_color_hex(0xBCD24B), LV_PART_MAIN);
    lv_obj_set_style_text_font(kg_value_label, &lv_font_montserrat_72, LV_PART_MAIN);
    lv_obj_set_pos(kg_value_label, 200, 330);

    // CNS Mode: Arc Slider (15 to 50 kg)
    ESP_LOGI(TAG, "Adding KG arc slider");
    lv_obj_t *kg_slider = lv_arc_create(main_screen);
    lv_arc_set_range(kg_slider, 15, 50);
    lv_arc_set_value(kg_slider, 15);
    lv_obj_set_size(kg_slider, 300, 300);
    lv_obj_set_style_arc_width(kg_slider, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(kg_slider, 30, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(kg_slider, lv_color_hex(0x2E4E5C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(kg_slider, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(kg_slider, lv_color_hex(0xBCD24B), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(kg_slider, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(kg_slider, lv_color_hex(0xBCD24B), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(kg_slider, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(kg_slider, 10, LV_PART_KNOB);
    lv_arc_set_bg_angles(kg_slider, 135, 45);
    lv_obj_align(kg_slider, LV_ALIGN_CENTER, 0, 50);

    // Add the event callback here
    lv_obj_add_event_cb(kg_slider, kg_slider_event_cb, LV_EVENT_VALUE_CHANGED, kg_value_label);

    // ADP Mode: Arc Progress Indicator (15 to 50 kg)
    lv_obj_t *weight_bar = lv_arc_create(main_screen);
    lv_arc_set_range(weight_bar, 15, 50); // Updated range to match kg_slider
    lv_arc_set_value(weight_bar, 15);
    lv_obj_set_size(weight_bar, 300, 300); // Retained as per your latest code
    lv_obj_set_style_arc_width(weight_bar, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(weight_bar, 30, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(weight_bar, lv_color_hex(0x2E4E5C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(weight_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(weight_bar, lv_color_hex(0xBCD24B), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(weight_bar, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_remove_style(weight_bar, NULL, LV_PART_KNOB);
    lv_arc_set_bg_angles(weight_bar, 135, 45);
    lv_obj_align(weight_bar, LV_ALIGN_BOTTOM_MID, 0, -100);
    lv_obj_add_flag(weight_bar, LV_OBJ_FLAG_HIDDEN);

    // Rep Counter Label (right side, aligned with "KG")
    ESP_LOGI(TAG, "Adding rep counter");
    lv_obj_t *rep_label = lv_label_create(main_screen);
    lv_label_set_text(rep_label, "REPS");
    lv_obj_set_style_text_color(rep_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(rep_label, &lv_font_montserrat_72, LV_PART_MAIN);
    lv_obj_set_pos(rep_label, 750, 250);

    // Rep Value Label (below "REPS")
    lv_obj_t *rep_value_label = lv_label_create(main_screen);
    lv_label_set_text(rep_value_label, "0");
    lv_obj_set_style_text_color(rep_value_label, lv_color_hex(0x87A2AB), LV_PART_MAIN);
    lv_obj_set_style_text_font(rep_value_label, &lv_font_montserrat_72, LV_PART_MAIN);
    lv_obj_set_pos(rep_value_label, 750, 330);

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
        // Serial data parsing for rep counts and ADP effort
        char rx_buf[64];
        int len = uart_read_bytes(UART_NUM_1, (uint8_t *)rx_buf, sizeof(rx_buf) - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            rx_buf[len] = '\0';
            xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY);
            // Parse rep counts (e.g., "REPS:5")
            int reps;
            if (sscanf(rx_buf, "REPS:%d", &reps) == 1)
            {
                reps = (reps >= 0 && reps <= 99) ? reps : 0;
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", reps);
                lv_label_set_text(rep_value_label, buf);
                ESP_LOGI(TAG, "Rep count updated: %d", reps);
            }
            // Parse ADP effort (e.g., "EFFORT:25")
            if (lv_obj_has_state(mode_switch, LV_STATE_CHECKED))
            {
                int effort;
                if (sscanf(rx_buf, "EFFORT:%d", &effort) == 1)
                {
                    effort = (effort >= 15 && effort <= 50) ? effort : 15; // Updated range
                    lv_arc_set_value(weight_bar, effort);
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%d", effort);
                    lv_label_set_text(kg_value_label, buf);
                    ESP_LOGI(TAG, "Effort updated: %d kg", effort);
                }
            }
            xSemaphoreGiveRecursive(lvgl_mux);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void display_init(void)
{
    ESP_LOGI(TAG, "Calling init_display");
    init_display();
    ESP_LOGI(TAG, "Calling set_backlight_brightness");
    set_backlight_brightness(2);
    ESP_LOGI(TAG, "Display initialized");
}