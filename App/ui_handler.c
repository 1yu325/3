#include "ui_handler.h"
#include "mode_manager.h"  /* ??????????? */
#include "sensor_handler.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static char last_temp_display[30] = "";
static char last_humi_display[30] = "";
static uint8_t last_light_states[LIGHT_COUNT] = {0xFF};
static uint8_t last_light_index = 0xFF;

/* ??:???????mode_manager.c???,??????? */

/**
  * @brief ???UI???
  */
void ui_handler_init(void)
{
    /* ???LCD */
    TFTLCD_Init();
    BACK_COLOR = WHITE;
    FRONT_COLOR = BLACK;
    
    /* ?????? */
    memset(last_temp_display, 0, sizeof(last_temp_display));
    memset(last_humi_display, 0, sizeof(last_humi_display));
    
    for (int i = 0; i < LIGHT_COUNT; i++) {
        last_light_states[i] = 0xFF;
    }
    last_light_index = 0xFF;
    
    printf("UI Handler Initialized\r\n");
}


/**
  * @brief UI??????
  */
void ui_handler_process(uint32_t current_time)
{
    (void)current_time;
    /* ??????????UI??,?????????????? */
}

/* ========================= ???????? ========================= */

/**
  * @brief ??????????
  */
void ui_show_temperature_humidity_mode(void)
{
    /* ?? */
    LCD_Clear(WHITE);
    
    /* ?? */
    LCD_ShowString(10, 10, tftlcd_data.width, tftlcd_data.height, 24, (u8*)"Smart Home Monitor");

    /* ???? */
    LCD_ShowString(10, 60, 150, 20, 16, (u8*)"Temperature:");
    
    /* ???? */
    LCD_ShowString(10, 100, 150, 20, 16, (u8*)"Humidity:");

    /* ????? */
    LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 14, (u8*)"Humidifier: Auto by Humidity");
    LCD_ShowString(10, 160, tftlcd_data.width, tftlcd_data.height, 14, (u8*)"Humi<40%: Level3(High), 40-50%: Level2(Medium)");
    LCD_ShowString(10, 180, tftlcd_data.width, tftlcd_data.height, 14, (u8*)"50-55%: Level1(Low), >55%: OFF/Standby");

    /* ??????? */
    LCD_ShowString(10, 200, tftlcd_data.width, tftlcd_data.height, 14, (u8*)"Flame Sensor: AO(PA1) DO(PB1)");

    /* ???? */
    LCD_ShowString(10, 220, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"PF1: Switch Mode");
    LCD_ShowString(10, 240, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Current: Temp/Humi");
    
    /* ?????? */
    strcpy(last_temp_display, "");
    strcpy(last_humi_display, "");
    
    printf("UI: Show Temperature/Humidity Mode\r\n");
}

/**
  * @brief ????????
  */
void ui_show_password_mode(void)
{
    /* ?? */
    LCD_Clear(WHITE);
    
    /* ?? */
    LCD_ShowString(10, 30, tftlcd_data.width, tftlcd_data.height, 24, (u8*)"Password Mode");
    
    /* ???? */
    LCD_ShowString(10, 80, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Enter 4-bit password:");
    
    /* ??????(??????) */
    LCD_ShowString(10, 110, 150, 20, 24, (u8*)"____");
    
    /* ???? */
    LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Bit:0/4");
    
    /* ???? */
    LCD_ShowString(10, 170, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_UP(PA0): Input 1");
    LCD_ShowString(10, 190, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_0(PE2): Input 0");
    LCD_ShowString(10, 210, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Hold KEY_0 >1s: Cancel");
    LCD_ShowString(10, 230, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"PF1: Switch Mode");
    
    printf("UI: Show Password Mode\r\n");
}

/**
  * @brief ??????????
  */
void ui_show_light_control_mode(void)
{
    /* ?? */
    LCD_Clear(WHITE);
    
    /* ?? */
    LCD_ShowString(10, 30, tftlcd_data.width, tftlcd_data.height, 24, (u8*)"Light Control Mode");
    
    /* ???????? */
    LCD_ShowString(10, 60, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"All Lights Status:");
    
    /* ???? */
    char light_line[50];
    snprintf(light_line, sizeof(light_line), "%-8s %-8s %-8s %-8s %-8s", 
             light_names[0], light_names[1], light_names[2], 
             light_names[3], light_names[4]);
    LCD_ShowString(10, 80, tftlcd_data.width, tftlcd_data.height, 16, (u8*)light_line);
    
    /* ??????(????) */
    LCD_ShowString(10, 100, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                                                 ");
    
    /* ??????(????) */
    LCD_ShowString(10, 120, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                                                 ");
    
    /* ???????? */
    LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Selected:          ");
    LCD_ShowString(10, 160, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Status:            ");
    
    /* ???? */
    LCD_ShowString(10, 180, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_0(PE2): Select Prev");
    LCD_ShowString(10, 200, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_INC(PE3): Select Next");
    LCD_ShowString(10, 220, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_DEC(PE4): Toggle Light");
    LCD_ShowString(10, 240, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"PF1: Switch Mode");
    
    /* ?????? */
    for (int i = 0; i < LIGHT_COUNT; i++) {
        last_light_states[i] = 0xFF;
    }
    last_light_index = 0xFF;
    
    /* ????????? */
    current_light_index = 0;
    
    /* ?????? */
    ui_update_all_lights_display();
    ui_update_selected_light_display();
    ui_update_arrow_indicator();
    
    printf("UI: Show Light Control Mode\r\n");
}

/* ========================= ?????????? ========================= */

/**
  * @brief ??????
  */
void ui_update_temperature_display(uint8_t temp_int, uint8_t temp_dec)
{
    char temp_str[30];
    
    if (temp_int == 0 && temp_dec == 0) {
        sprintf(temp_str, "Temp: --.- C");
    } else {
        sprintf(temp_str, "Temp: %d.%d C", temp_int, temp_dec);
    }
    
    if (strcmp(temp_str, last_temp_display) != 0) {
        ui_clear_area(10, 80, 150, 20);
        LCD_ShowString(10, 80, 150, 20, 16, (u8*)temp_str);
        strcpy(last_temp_display, temp_str);
    }
}

/**
  * @brief ??????
  */
void ui_update_humidity_display(uint8_t humi_int, uint8_t humi_dec)
{
    char humi_str[30];
    
    if (humi_int == 0 && humi_dec == 0) {
        sprintf(humi_str, "Humi: --.- %%");
    } else {
        sprintf(humi_str, "Humi: %d.%d %%", humi_int, humi_dec);
    }
    
    if (strcmp(humi_str, last_humi_display) != 0) {
        ui_clear_area(10, 120, 150, 20);
        LCD_ShowString(10, 120, 150, 20, 16, (u8*)humi_str);
        strcpy(last_humi_display, humi_str);
    }
}

/**
  * @brief ??????
  */
void ui_show_flame_info(const char* info)
{
    if (mode_get_current() == MODE_TEMP_HUMIDITY) {
        ui_clear_area(10, 280, tftlcd_data.width, 12);
        LCD_ShowString(10, 280, tftlcd_data.width, tftlcd_data.height, 12, (u8*)info);
    }
}

/**
  * @brief ???????
  */
void ui_show_humidifier_info(const char* info)
{
    if (mode_get_current() == MODE_TEMP_HUMIDITY) {
        ui_clear_area(10, 300, tftlcd_data.width, 12);
        LCD_ShowString(10, 300, tftlcd_data.width, tftlcd_data.height, 12, (u8*)info);
    }
}

/* ========================= ???????? ========================= */

/**
  * @brief ??????
  */
void ui_update_password_display(uint8_t* buffer, uint8_t index)
{
    char password_display[5] = "____";
    
    for (int i = 0; i < index; i++) {
        if (buffer[i] == 1) {
            password_display[i] = '1';
        } else {
            password_display[i] = '0';
        }
    }
    
    ui_clear_area(10, 110, 100, 30);
    LCD_ShowString(10, 110, 100, 30, 24, (u8*)password_display);
    
    /* ?????? */
    char bit_str[16];
    sprintf(bit_str, "Bit:%d/4", index);
    ui_clear_area(10, 140, 50, 16);
    LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)bit_str);
}

/**
  * @brief ????
  */
void ui_show_message(const char* msg, uint16_t y)
{
    ui_clear_area(10, y, tftlcd_data.width, 16);
    LCD_ShowString(10, y, tftlcd_data.width, tftlcd_data.height, 16, (u8*)msg);
}

/**
  * @brief ??????
  */
void ui_clear_message_area(void)
{
    ui_clear_area(10, 250, tftlcd_data.width, 32);
}

/* ========================= ?????????? ========================= */

/**
  * @brief ??????????
  */
void ui_update_light_status_display(uint8_t index)
{
    if (index >= LIGHT_COUNT) return;
    
    uint8_t current_state = (HAL_GPIO_ReadPin(GPIOA, light_pins[index]) == GPIO_PIN_RESET) ? 1 : 0;
    
    if (last_light_states[index] == current_state) {
        return;
    }
    
    last_light_states[index] = current_state;
    
    uint16_t x_pos = 10 + index * 72;
    char state_str[8];
    snprintf(state_str, sizeof(state_str), "%-8s", current_state ? "ON" : "OFF");
    
    ui_clear_area(x_pos, 100, 64, 16);
    LCD_ShowString(x_pos, 100, 64, 16, 16, (u8*)state_str);
}

/**
  * @brief ??????????
  */
void ui_update_all_lights_display(void)
{
    for (int i = 0; i < LIGHT_COUNT; i++) {
        ui_update_light_status_display(i);
    }
}

/**
  * @brief ????????
  */
void ui_update_selected_light_display(void)
{
    static char last_status_str[40] = "";
    static char last_control_str[40] = "";

    char status_str[40];
    char control_str[40];
    
    sprintf(status_str, "Selected: %s", light_names[current_light_index]);
    sprintf(control_str, "Status: %s", light_states[current_light_index] ? "ON" : "OFF");
    
    if (strcmp(status_str, last_status_str) != 0) {
        ui_clear_area(10, 140, 200, 16);
        LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)status_str);
        strcpy(last_status_str, status_str);
    }
    
    if (strcmp(control_str, last_control_str) != 0) {
        ui_clear_area(10, 160, 200, 16);
        LCD_ShowString(10, 160, tftlcd_data.width, tftlcd_data.height, 16, (u8*)control_str);
        strcpy(last_control_str, control_str);
    }
}

/**
  * @brief ???????
  */
void ui_update_arrow_indicator(void)
{
    if (last_light_index == current_light_index) {
        return;
    }
    
    last_light_index = current_light_index;
    
    uint16_t x_pos = 10 + current_light_index * 72;
    
    ui_clear_area(10, 120, tftlcd_data.width, 16);
    LCD_ShowString(x_pos, 120, 16, 16, 16, (u8*)"^");
}

/* ========================= ?????? ========================= */

/**
  * @brief ??????
  */
void ui_clear_area(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    LCD_Fill(x, y, x + width, y + height, WHITE);
}

/**
  * @brief ?????
  */
void ui_draw_string(uint16_t x, uint16_t y, const char* str, uint8_t size)
{
    LCD_ShowString(x, y, tftlcd_data.width, tftlcd_data.height, size, (u8*)str);
}