#include "mode_manager.h"
#include "ui_handler.h"
#include "sensor_handler.h"
#include "key_handler.h"
#include <stdio.h>
#include <string.h>

/* ?????? - ??????? */
const char* light_names[LIGHT_COUNT] = {
    "Toilet",    /* PA2 - ?? */
    "Living",    /* PA3 - ?? */
    "Bedroom",   /* PA4 - ?? */
    "Kitchen",   /* PA5 - ?? */
    "Study"      /* PA6 - ?? */
};

const uint16_t light_pins[LIGHT_COUNT] = {
    GPIO_PIN_2,  /* PA2 - ?? */
    GPIO_PIN_3,  /* PA3 - ?? */
    GPIO_PIN_4,  /* PA4 - ?? */
    GPIO_PIN_5,  /* PA5 - ?? */
    GPIO_PIN_6   /* PA6 - ?? */
};

uint8_t light_states[LIGHT_COUNT] = {0};
uint8_t current_light_index = 0;

/* Private variables ---------------------------------------------------------*/
static uint8_t password_input_buffer[PASSWORD_LEN] = {0};
static uint8_t password_input_index = 0;
static uint8_t password_error_count = 0;
static uint8_t password_alarm_active = 0;
static uint32_t password_alarm_start_time = 0;

/* System variables ----------------------------------------------------------*/
volatile SystemMode current_mode = MODE_TEMP_HUMIDITY;

/* Private function prototypes -----------------------------------------------*/
static void password_process(uint32_t current_time);
static void password_check_input(void);
static void password_reset_input(void);

/**
  * @brief ????????
  */
void mode_manager_init(void)
{
    password_reset_input();
    password_alarm_active = 0;
    password_error_count = 0;
    
    /* ???????????? */
    for (int i = 0; i < LIGHT_COUNT; i++) {
        set_light_state(i, 0);
    }
}

/**
  * @brief ??????????
  */
void mode_manager_process(uint32_t current_time)
{
    /* ?????? */
    if (password_alarm_active && (current_time - password_alarm_start_time >= 2000)) {
        /* ????,????????? */
        if (!sensor_flame_alarm_is_active()) {
            HAL_GPIO_WritePin(GPIOB, BEEP_Pin, GPIO_PIN_RESET);
        }
        password_alarm_active = 0;
    }
    
    switch (current_mode) {
        case MODE_TEMP_HUMIDITY:
            /* ??????????????? */
            break;
            
        case MODE_PASSWORD:
            password_process(current_time);
            break;
            
        case MODE_LIGHT_CONTROL:
            /* ?????????? */
            break;
    }
}

/**
  * @brief ??????
  */
static void password_process(uint32_t current_time)
{
    static uint32_t key_cancel_press_start = 0;
    static uint8_t key_cancel_long_press_handled = 0;
    
    /* KEY_UP (PA0): ??1 */
    if (HAL_GPIO_ReadPin(GPIOA, KEY_UP_PIN) == GPIO_PIN_SET) {
        static uint32_t key_up_last_time = 0;
        if (current_time - key_up_last_time > 300) {
            key_up_last_time = current_time;
            if (password_input_index < PASSWORD_LEN) {
                password_input_buffer[password_input_index] = 1;
                password_input_index++;
                ui_update_password_display(password_input_buffer, password_input_index);
                printf("KEY_UP Pressed - Input 1\r\n");
            }
        }
    }
    
    /* KEY_0 (PE2): ??0/?? */
    if (HAL_GPIO_ReadPin(GPIOE, KEY_CANCEL_PIN) == GPIO_PIN_RESET) {
        /* ?????? */
        if (key_cancel_press_start == 0) {
            key_cancel_press_start = current_time;
            key_cancel_long_press_handled = 0;
        }
        
        /* ??(1?) */
        if (!key_cancel_long_press_handled && (current_time - key_cancel_press_start > 1000)) {
            password_reset_input();
            ui_update_password_display(password_input_buffer, password_input_index);
            ui_show_message("Input Cleared", 250);
            key_cancel_long_press_handled = 1;
            printf("KEY_0 Long Press - Input Cleared\r\n");
        }
    } else {
        /* ???? */
        if (key_cancel_press_start > 0) {
            /* ??:??0 */
            if (!key_cancel_long_press_handled && password_input_index < PASSWORD_LEN) {
                password_input_buffer[password_input_index] = 0;
                password_input_index++;
                ui_update_password_display(password_input_buffer, password_input_index);
                printf("KEY_0 Short Press - Input 0\r\n");
            }
            key_cancel_press_start = 0;
            key_cancel_long_press_handled = 0;
        }
    }
    
    /* ????(???4??) */
    if (password_input_index >= PASSWORD_LEN) {
        password_check_input();
    }
}

/**
  * @brief ????????
  */
void mode_switch_to_next(void)
{
    uint8_t next_mode = (uint8_t)current_mode;
    next_mode = (next_mode + 1) % 3;
    current_mode = (SystemMode)next_mode;
    
    /* ???????UI */
    switch (current_mode) {
        case MODE_TEMP_HUMIDITY:
            ui_show_temperature_humidity_mode();
            sensor_update_display();  /* ????????? */
            break;
            
        case MODE_PASSWORD:
            password_reset_input();
            ui_show_password_mode();
            break;
            
        case MODE_LIGHT_CONTROL:
            ui_show_light_control_mode();
            break;
    }
    
    printf("Switched to Mode: %d\r\n", current_mode);
}

/**
  * @brief ??????
  */
SystemMode mode_get_current(void)
{
    return current_mode;
}

/**
  * @brief ??????
  */
static void password_check_input(void)
{
    uint8_t value = 0;
    for (int i = 0; i < PASSWORD_LEN; i++) {
        value = (value << 1) | password_input_buffer[i];
    }
    
    if (value == CORRECT_PASSWORD) {
        password_error_count = 0;
        password_alarm_active = 0;
        /* ??PB8??(??:????????) */
        if (!sensor_flame_alarm_is_active()) {
            HAL_GPIO_WritePin(GPIOB, BEEP_Pin, GPIO_PIN_RESET);
        }
        ui_show_message("Access Granted!", 250);
        HAL_Delay(1000);
        current_mode = MODE_TEMP_HUMIDITY;
        ui_show_temperature_humidity_mode();
        sensor_update_display();
        printf("Password Correct - Access Granted\r\n");
    } else {
        password_error_count++;
        char error_msg[30];
        sprintf(error_msg, "Wrong! Try %d/3", 3 - password_error_count);
        ui_show_message(error_msg, 250);
        
        if (password_error_count >= 3) {
            password_alarm_active = 1;
            password_alarm_start_time = HAL_GetTick();
            /* ??PB8??(??:????????) */
            HAL_GPIO_WritePin(GPIOB, BEEP_Pin, GPIO_PIN_SET);
            ui_show_message("ALARM ACTIVATED!", 270);
            HAL_Delay(2000);
            password_error_count = 0;
            current_mode = MODE_TEMP_HUMIDITY;
            ui_show_temperature_humidity_mode();
            sensor_update_display();
            printf("3 Wrong Attempts - Alarm Activated\r\n");
        } else {
            HAL_Delay(1000);
            password_reset_input();
            ui_update_password_display(password_input_buffer, password_input_index);
            ui_clear_message_area();
        }
    }
}

/**
  * @brief ??????
  */
static void password_reset_input(void)
{
    password_input_index = 0;
    memset(password_input_buffer, 0, sizeof(password_input_buffer));
}

/**
  * @brief ??????
  */
void set_light_state(uint8_t index, uint8_t state)
{
    if (index < LIGHT_COUNT) {
        light_states[index] = state;
        
        GPIO_PinState pin_state = state ? GPIO_PIN_RESET : GPIO_PIN_SET;
        HAL_GPIO_WritePin(GPIOA, light_pins[index], pin_state);
        
        ui_update_light_status_display(index);
        
        printf("Light %s (%s) turned %s\r\n", 
               light_names[index], 
               index == 0 ? "PA2" : 
               index == 1 ? "PA3" : 
               index == 2 ? "PA4" : 
               index == 3 ? "PA5" : "PA6",
               state ? "ON" : "OFF");
    }
}

/**
  * @brief ??????
  */
void toggle_current_light(void)
{
    light_states[current_light_index] = !light_states[current_light_index];
    
    GPIO_PinState pin_state = light_states[current_light_index] ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(GPIOA, light_pins[current_light_index], pin_state);
    
    ui_update_light_status_display(current_light_index);
    ui_update_selected_light_display();
    
    printf("Toggled %s light to %s\r\n", 
           light_names[current_light_index],
           light_states[current_light_index] ? "ON" : "OFF");
}

/**
  * @brief ???????
  */
void select_next_light(void)
{
    uint8_t old_index = current_light_index;
    
    if (current_light_index < (LIGHT_COUNT - 1)) {
        current_light_index++;
    } else {
        current_light_index = 0;
    }
    
    ui_update_arrow_indicator();
    ui_update_selected_light_display();
    
    printf("Selected NEXT light: %s (from %s)\r\n", 
           light_names[current_light_index],
           light_names[old_index]);
}

/**
  * @brief ???????
  */
void select_prev_light(void)
{
    uint8_t old_index = current_light_index;
    
    if (current_light_index > 0) {
        current_light_index--;
    } else {
        current_light_index = LIGHT_COUNT - 1;
    }
    
    ui_update_arrow_indicator();
    ui_update_selected_light_display();
    
    printf("Selected PREVIOUS light: %s (from %s)\r\n", 
           light_names[current_light_index],
           light_names[old_index]);
}