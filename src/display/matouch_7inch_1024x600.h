
// PWM Configuration
#define PWM_FREQ               200 
#define PWM_RESOLUTION         LEDC_TIMER_8_BIT 
#define LEDC_CHANNEL           LEDC_CHANNEL_0  
#define LEDC_TIMER             LEDC_TIMER_0 
#define LEDC_PIN_NUM_BK_LIGHT  GPIO_NUM_10
#define LEDC_OUTPUT_INVERT     1

// I2C
#define I2C_SCL          GPIO_NUM_18
#define I2C_SDA          GPIO_NUM_17
#define I2C_RST          GPIO_NUM_38
#define I2C_CLK_SPEED_HZ 100000
#define I2C_NUM          I2C_NUM_0

// LCD
#define LCD_PIXEL_CLOCK_HZ     (16 * 1000 * 1000)

#define PIN_NUM_HSYNC          GPIO_NUM_39
#define PIN_NUM_VSYNC          GPIO_NUM_41
#define PIN_NUM_DE             GPIO_NUM_40
#define PIN_NUM_PCLK           GPIO_NUM_42
#define PIN_NUM_DATA0          GPIO_NUM_8   // B0
#define PIN_NUM_DATA1          GPIO_NUM_3   // B1
#define PIN_NUM_DATA2          GPIO_NUM_46  // B2
#define PIN_NUM_DATA3          GPIO_NUM_9   // B3
#define PIN_NUM_DATA4          GPIO_NUM_1   // B4
#define PIN_NUM_DATA5          GPIO_NUM_5   // G0
#define PIN_NUM_DATA6          GPIO_NUM_6   // G1
#define PIN_NUM_DATA7          GPIO_NUM_7   // G2
#define PIN_NUM_DATA8          GPIO_NUM_15  // G3
#define PIN_NUM_DATA9          GPIO_NUM_16  // G4
#define PIN_NUM_DATA10         GPIO_NUM_4   // G5
#define PIN_NUM_DATA11         GPIO_NUM_45  // R0
#define PIN_NUM_DATA12         GPIO_NUM_48  // R1
#define PIN_NUM_DATA13         GPIO_NUM_47  // R2
#define PIN_NUM_DATA14         GPIO_NUM_21  // R3
#define PIN_NUM_DATA15         GPIO_NUM_14  // R4
#define PIN_NUM_DISP_EN        GPIO_NUM_NC

#define HSYNC_BACK_PORCH  16
#define HSYNC_FRONT_PORCH 80
#define HSYNC_PULSE_WIDTH 4
#define VSYNC_BACK_PORCH  4
#define VSYNC_FRONT_PORCH 22
#define VSYNC_PULSE_WIDTH 4

#define LCD_H_RES         1024
#define LCD_V_RES         600

// LVGL
#define LVGL_TASK_DELAY_MS   10
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY   2