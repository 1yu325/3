#include "key_handler.h"
#include "mode_manager.h"
#include "ui_handler.h"
#include <stdio.h>
#include <string.h>  /* ??string.h??? */

/* Private variables ---------------------------------------------------------*/
static uint32_t pf1_last_time = 0;
static uint32_t key_prev_last_time = 0;
static uint32_t key_next_last_time = 0;
static uint32_t key_toggle_last_time = 0;
static uint32_t key_up_last_time = 0;

/* ?????? */
typedef struct {
    uint32_t press_start_time;
    uint8_t long_press_handled;
    uint8_t last_state;
    uint32_t last_change_time;
} KeyState_t;

static KeyState_t key_cancel_state = {0};
static KeyState_t key_mode_state = {0};
static void key_handle_light_control(uint32_t current_time); 
/**
  * @brief ????????
  */
void key_handler_init(void)
{
    /* ??????? */
    memset(&key_cancel_state, 0, sizeof(KeyState_t));
    memset(&key_mode_state, 0, sizeof(KeyState_t));
    
    printf("Key Handler Initialized\r\n");
}

/**
  * @brief ????????
  */
void key_handler_process(uint32_t current_time)
{
    /* ??PF1????? */
    if (HAL_GPIO_ReadPin(GPIOF, KEY_MODE_PIN) == GPIO_PIN_RESET) {
        /* ?????? */
        if (key_mode_state.press_start_time == 0) {
            key_mode_state.press_start_time = current_time;
            key_mode_state.long_press_handled = 0;
        }
        
        /* ????(???) */
        if (!key_mode_state.long_press_handled && 
            (current_time - key_mode_state.press_start_time > KEY_DEBOUNCE_TIME)) {
            mode_switch_to_next();
            key_mode_state.long_press_handled = 1;
        }
    } else {
        /* ???? */
        key_mode_state.press_start_time = 0;
        key_mode_state.long_press_handled = 0;
    }
    
    /* ???????????? */
    switch (mode_get_current()) {
        case MODE_TEMP_HUMIDITY:
            /* ??????????????? */
            break;
            
        case MODE_PASSWORD:
            /* ???????mode_manager.c? */
            break;
            
        case MODE_LIGHT_CONTROL:
            key_handle_light_control(current_time);
            break;
    }
}

/**
  * @brief ????????????
  */
void key_handle_light_control(uint32_t current_time)
{
    /* KEY_0 (PE2): ??????? */
    if (HAL_GPIO_ReadPin(GPIOE, KEY_PREV_PIN) == GPIO_PIN_RESET) {
        if (current_time - key_prev_last_time > 300) {
            key_prev_last_time = current_time;
            key_light_select_prev();
        }
    }
    
    /* KEY_INC (PE3): ??????? */
    if (HAL_GPIO_ReadPin(GPIOE, KEY_NEXT_PIN) == GPIO_PIN_RESET) {
        if (current_time - key_next_last_time > 300) {
            key_next_last_time = current_time;
            key_light_select_next();
        }
    }
    
    /* KEY_DEC (PE4): ?????? */
    if (HAL_GPIO_ReadPin(GPIOE, KEY_TOGGLE_PIN) == GPIO_PIN_RESET) {  /* ?????? */
        if (current_time - key_toggle_last_time > 300) {
            key_toggle_last_time = current_time;
            key_light_toggle_current();
        }
    }
}

/* ========================= ?????? ========================= */

/**
  * @brief ???????
  */
void key_light_select_prev(void)
{
    uint8_t old_index = current_light_index;
    
    if (current_light_index > 0) {
        current_light_index--;
    } else {
        current_light_index = LIGHT_COUNT - 1;
    }
    
    ui_update_arrow_indicator();
    ui_update_selected_light_display();
    
    printf("Selected PREV light: %s (from %s)\r\n", 
           light_names[current_light_index],
           light_names[old_index]);
}

/**
  * @brief ???????
  */
void key_light_select_next(void)
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
  * @brief ????????
  */
void key_light_toggle_current(void)
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
  * @brief ??????
  */
void key_set_light_state(uint8_t index, uint8_t state)
{
    if (index < LIGHT_COUNT) {
        light_states[index] = state;
        
        GPIO_PinState pin_state = state ? GPIO_PIN_RESET : GPIO_PIN_SET;
        HAL_GPIO_WritePin(GPIOA, light_pins[index], pin_state);
        
        ui_update_light_status_display(index);
        
        printf("Light %s set to %s\r\n", 
               light_names[index],
               state ? "ON" : "OFF");
    }
}

/* ========================= ?????? ========================= */

/**
  * @brief ????????(???)
  */
uint8_t key_is_pressed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t active_level)
{
    static uint32_t last_check_time = 0;
    static uint8_t last_state = 0xFF;
    uint32_t current_time = HAL_GetTick();
    
    /* ?????? */
    if (current_time - last_check_time < 20) {
        return 0;
    }
    
    last_check_time = current_time;
    
    /* ?????? */
    uint8_t current_state;
    if (active_level == 0) {
        current_state = (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET);
    } else {
        current_state = (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET);
    }
    
    /* ?????? */
    if (current_state != last_state) {
        last_state = current_state;
        HAL_Delay(KEY_DEBOUNCE_TIME); /* ???? */
        return current_state;
    }
    
    return 0;
}