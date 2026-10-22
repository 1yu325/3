#include "sensor_handler.h"
#include "ui_handler.h"
#include "step_motor.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static uint32_t last_dht11_time = 0;
static const uint32_t DHT11_UPDATE_INTERVAL = 2000;

/* ??????? */
static uint8_t flame_detected = 0;
static uint8_t flame_alarm_active = 0;
static uint32_t flame_last_check_time = 0;
static uint32_t flame_alarm_start_time = 0;
static uint16_t flame_adc_value = 0;
static char last_flame_display[60] = "";

/* ?????? */
static uint8_t humidifier_state = HUMIDIFIER_STATE_OFF;
static uint8_t last_humidifier_state = HUMIDIFIER_STATE_OFF;  /* ?????? */
static uint8_t humidity_level = 0;
static uint8_t last_humidity_level = 0xFF;
static uint8_t motor_direction = 0;
static uint32_t humidifier_last_update = 0;
static char last_humidifier_display[80] = "";

/* ???????? */
static uint8_t motor_running = 0;
static uint32_t motor_last_step_time = 0;
static uint32_t motor_current_delay = STEPPER_SPEED_OFF;
static uint32_t motor_run_interval = 0;
static uint8_t motor_is_opening = 0;
static uint8_t motor_is_closing = 0;
static uint32_t motor_target_steps = 0;
static uint32_t motor_current_steps = 0;

/* ?????? - ??????? */
DHT11_Data_t dht11_data = {0};
volatile uint8_t system_ready = 0;  /* ??????? */

/* ?????? */
extern ADC_HandleTypeDef hadc3;
static void flame_trigger_alarm(void);  /* ?????? */
static void flame_stop_alarm(void);     /* ?????? */
/* ... sensor_handler.c ????????? ... */
/**
  * @brief ?????????
  */
void sensor_handler_init(void)
{
    /* ???DHT11 */
    sensor_dht11_init();
    
    /* ???????? */
    sensor_flame_init();
    
    /* ??????? */
    sensor_humidity_control_init();
    
    printf("Sensor Handler Initialized\r\n");
}

/**
  * @brief ?????????
  */
void sensor_handler_process(uint32_t current_time)
{
    if (!system_ready) return;
    
    /* 1. ??DHT11?? */
    if (current_time - last_dht11_time >= DHT11_UPDATE_INTERVAL) {
        last_dht11_time = current_time;
        sensor_dht11_read();
    }
    
    /* 2. ??????? */
    sensor_flame_check(current_time);
    
    /* 3. ?????? */
    sensor_humidity_control_process(current_time);
    
    /* 4. ?????? */
    sensor_run_stepper_motor();
}

/**
  * @brief ???????
  */
void sensor_update_display(void)
{
    sensor_update_temp_humi_display();
    sensor_update_flame_display();
    sensor_update_humidifier_display();
}

/* ========================= DHT11???? ========================= */

/**
  * @brief ???DHT11
  */
void sensor_dht11_init(void)
{
    last_dht11_time = HAL_GetTick();
    memset(&dht11_data, 0, sizeof(dht11_data));
}

/**
  * @brief ??DHT11??
  */
uint8_t sensor_dht11_read(void)
{
    DHT11_Data_TypeDef data;
    uint8_t result = DHT11_ReadData(&data);
    
    if (result) {
        dht11_data.temp_int = data.temp_int;
        dht11_data.temp_dec = data.temp_dec;
        dht11_data.humi_int = data.humi_int;
        dht11_data.humi_dec = data.humi_dec;
        
        printf("DHT11: Temp=%d.%dC, Humi=%d.%d%%\r\n",
               data.temp_int, data.temp_dec,
               data.humi_int, data.humi_dec);
    }
    
    return result;
}

/**
  * @brief ???????
  */
void sensor_update_temp_humi_display(void)
{
    ui_update_temperature_display(dht11_data.temp_int, dht11_data.temp_dec);
    ui_update_humidity_display(dht11_data.humi_int, dht11_data.humi_dec);
}

/* ========================= ????????? ========================= */

/**
  * @brief ????????
  */
void sensor_flame_init(void)
{
    flame_detected = 0;
    flame_alarm_active = 0;
    flame_last_check_time = HAL_GetTick();
    
    /* ??????PB8 */
    HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_RESET);
    
    /* ????ADC????? */
    sensor_flame_read_adc();
    HAL_Delay(10);
    sensor_flame_read_adc();
    
    printf("Flame Sensor Initialized\r\n");
}

/**
  * @brief ???????
  */
void sensor_flame_check(uint32_t current_time)
{
    if (current_time - flame_last_check_time < FLAME_CHECK_INTERVAL) {
        return;
    }
    
    flame_last_check_time = current_time;
    
    /* ??DO???? */
    uint8_t do_state = HAL_GPIO_ReadPin(FLAME_SENSOR_DO_PORT, FLAME_SENSOR_DO_PIN);
    
    /* ??ADC? */
    flame_adc_value = sensor_flame_read_adc();
    
    /* ???? */
    uint8_t previous_detection = flame_detected;
    
    if (do_state == GPIO_PIN_RESET) {  /* DO????????????? */
        if (!flame_detected) {
            printf("FLAME DETECTED! (DO=LOW)\r\n");
            flame_trigger_alarm();
        }
        flame_detected = 1;
    } else {
        if (flame_detected) {
            printf("Flame Disappeared (DO=HIGH)\r\n");
        }
        flame_detected = 0;
        
        /* ?????? */
        if (flame_alarm_active && (current_time - flame_alarm_start_time >= FLAME_ALARM_DURATION)) {
            flame_stop_alarm();
        }
    }
    
    /* ????(?2?????) */
    static uint32_t last_debug_time = 0;
    if (current_time - last_debug_time > 2000) {
        last_debug_time = current_time;
        printf("Flame: ADC=%d, DO=%s, Detected=%d, Alarm=%d\r\n",
               flame_adc_value,
               do_state == GPIO_PIN_RESET ? "LOW" : "HIGH",
               flame_detected,
               flame_alarm_active);
    }
    
    /* ????(?????????) */
    if (previous_detection != flame_detected) {
        sensor_update_flame_display();
    }
}

/**
  * @brief ???????ADC?
  */
uint16_t sensor_flame_read_adc(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    sConfig.Channel = FLAME_SENSOR_ADC_CHANNEL;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
    
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) {
        return 0;
    }
    
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 100);
    flame_adc_value = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    
    return flame_adc_value;
}

/**
  * @brief ???????
  */
uint8_t sensor_flame_is_detected(void)
{
    return flame_detected;
}

/**
  * @brief ????????
  */
uint8_t sensor_flame_alarm_is_active(void)
{
    return flame_alarm_active;
}

/**
  * @brief ????????
  */
void sensor_update_flame_display(void)
{
    char info_str[80];
    
    if (flame_detected) {
        if (flame_alarm_active) {
            uint32_t remaining_time = FLAME_ALARM_DURATION - (HAL_GetTick() - flame_alarm_start_time);
            uint32_t seconds = remaining_time / 1000;
            snprintf(info_str, sizeof(info_str), 
                     "Flame! Level:%d  Alarm: ON (%lus)", 
                     flame_adc_value, seconds);
        } else {
            snprintf(info_str, sizeof(info_str), 
                     "Flame! Level:%d  Alarm: OFF", 
                     flame_adc_value);
        }
    } else {
        snprintf(info_str, sizeof(info_str), 
                 "No Flame  Level:%d  Alarm: OFF", 
                 flame_adc_value);
    }
    
    /* ?????????????LCD */
    if (strcmp(info_str, last_flame_display) != 0) {
        ui_show_flame_info(info_str);
        strcpy(last_flame_display, info_str);
        printf("Flame Info Updated: %s\r\n", info_str);
    }
}

/**
  * @brief ??????
  */
static void flame_trigger_alarm(void)
{
    if (!flame_alarm_active) {
        flame_alarm_active = 1;
        flame_alarm_start_time = HAL_GetTick();
        
        HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_SET);
        
        printf("FLAME ALARM ACTIVATED!\r\n");
        
        sensor_update_flame_display();
    }
}

/**
  * @brief ??????
  */
static void flame_stop_alarm(void)
{
    if (flame_alarm_active) {
        flame_alarm_active = 0;
        
        HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_RESET);
        
        printf("Flame Alarm Deactivated\r\n");
        
        sensor_update_flame_display();
    }
}

/* ========================= ???????? ========================= */

/**
  * @brief ???????
  */
void sensor_humidity_control_init(void)
{
    humidifier_state = HUMIDIFIER_STATE_OFF;
    last_humidifier_state = HUMIDIFIER_STATE_OFF;
    humidity_level = 0;
    last_humidity_level = 0xFF;
    motor_direction = 0;
    
    /* ?????? */
    Step_Motor_Stop();
    motor_running = 0;
    motor_is_opening = 0;
    motor_is_closing = 0;
    motor_current_delay = STEPPER_SPEED_OFF;
    
    printf("Humidity Control Initialized\r\n");
}

/**
  * @brief ??????
  */
void sensor_humidity_control_process(uint32_t current_time)
{
    if (current_time - humidifier_last_update < 2000) {
        return;
    }
    
    humidifier_last_update = current_time;
    
    /* ??DHT11?????? */
    if (dht11_data.humi_int == 0 && dht11_data.humi_dec == 0) {
        return;
    }
    
    /* ?????? */
    float current_humidity = (float)dht11_data.humi_int + (float)dht11_data.humi_dec / 10.0;
    
    /* ?????? */
    uint8_t new_level = sensor_calculate_humidity_level(dht11_data.humi_int, dht11_data.humi_dec);
    
    /* ?????? */
    if (new_level != last_humidity_level) {
        printf("Humidity Level: %d -> %d (%.1f%%)\r\n",
               last_humidity_level, new_level, current_humidity);
        
        last_humidity_level = new_level;
        humidity_level = new_level;
        
        /* ????????? */
        if (current_humidity < HUMIDITY_HIGH_THRESHOLD) {
            if (new_level > 0) {  /* ??1-3 */
                sensor_set_humidifier_state(HUMIDIFIER_STATE_ON);
            } else {
                sensor_set_humidifier_state(HUMIDIFIER_STATE_OFF);
            }
        } else {
            sensor_set_humidifier_state(HUMIDIFIER_STATE_OFF);
        }
    }
    
    /* ????????,???????????? */
    if (motor_running && motor_is_opening) {
        uint32_t new_delay = sensor_get_motor_delay_by_level(humidity_level);
        if (new_delay != motor_current_delay) {
            motor_current_delay = new_delay;
            printf("Motor speed adjusted: %dms\r\n", motor_current_delay);
        }
    }
    
    /* ???? */
    sensor_update_humidifier_display();
}

/**
  * @brief ??????
  */
uint8_t sensor_calculate_humidity_level(int16_t humidity_int, int16_t humidity_dec)
{
    float humidity = (float)humidity_int + (float)humidity_dec / 10.0;
    
    if (humidity >= HUMIDITY_OFF_THRESHOLD) {
        return 0;  /* 55-60%,???? */
    } else if (humidity >= HUMIDITY_LEVEL_1) {
        return 1;  /* 50-55%,??1 */
    } else if (humidity >= HUMIDITY_LEVEL_2) {
        return 2;  /* 40-50%,??2 */
    } else {
        return 3;  /* <40%,??3 */
    }
}

/**
  * @brief ??????????
  */
uint32_t sensor_get_motor_delay_by_level(uint8_t level)
{
    switch (level) {
        case 1:
            return STEPPER_SPEED_LOW;    /* ?? */
        case 2:
            return STEPPER_SPEED_MEDIUM; /* ?? */
        case 3:
            return STEPPER_SPEED_HIGH;   /* ?? */
        case 0:
        default:
            return STEPPER_SPEED_OFF;    /* ?? */
    }
}

/**
  * @brief ???????
  */
void sensor_set_humidifier_state(uint8_t state)
{
    if (state != humidifier_state) {
        humidifier_state = state;
        
        if (state == HUMIDIFIER_STATE_ON) {
            printf("Humidifier: Turning ON (Level: %d)\r\n", humidity_level);
            sensor_start_humidifier_opening();
        } else {
            printf("Humidifier: Turning OFF\r\n");
            sensor_start_humidifier_closing();
        }
        
        last_humidifier_state = humidifier_state;
    }
}

/**
  * @brief ???????
  */
void sensor_start_humidifier_opening(void)
{
    if (!motor_is_opening && !motor_is_closing) {
        motor_is_opening = 1;
        motor_is_closing = 0;
        motor_direction = 0;  /* ?? */
        motor_running = 1;
        motor_last_step_time = HAL_GetTick();
        motor_current_steps = 0;
        motor_target_steps = 100;  /* ???? */
        
        motor_current_delay = sensor_get_motor_delay_by_level(humidity_level);
        
        printf("Starting humidifier opening: Level=%d, Delay=%dms, Steps=%d\r\n",
               humidity_level, motor_current_delay, motor_target_steps);
    }
}

/**
  * @brief ???????
  */
void sensor_start_humidifier_closing(void)
{
    if (!motor_is_opening && !motor_is_closing) {
        motor_is_opening = 0;
        motor_is_closing = 1;
        motor_direction = 1;  /* ?? */
        motor_running = 1;
        motor_last_step_time = HAL_GetTick();
        motor_current_steps = 0;
        motor_target_steps = 100;  /* ???? */
        
        motor_current_delay = STEPPER_SPEED_MEDIUM;
        
        printf("Starting humidifier closing: Delay=%dms, Steps=%d\r\n",
               motor_current_delay, motor_target_steps);
    }
}

/**
  * @brief ??????
  */
void sensor_run_stepper_motor(void)
{
    if (!motor_running || motor_current_delay == STEPPER_SPEED_OFF) {
        return;
    }
    
    uint32_t current_time = HAL_GetTick();
    
    if (current_time - motor_last_step_time >= motor_current_delay) {
        motor_last_step_time = current_time;
        
        if (motor_direction == 0) {
            Step_Motor_CW(1);
        } else {
            Step_Motor_CCW(1);
        }
        
        motor_current_steps++;
        motor_run_interval++;
        
        if (motor_current_steps >= motor_target_steps) {
            if (motor_is_opening) {
                motor_is_opening = 0;
                printf("Humidifier opened. Steps: %d\r\n", motor_current_steps);
            } else if (motor_is_closing) {
                motor_is_closing = 0;
                printf("Humidifier closed. Steps: %d\r\n", motor_current_steps);
            }
            
            motor_running = 0;
            Step_Motor_Stop();
        }
        
        if (motor_run_interval >= 10) {
            motor_run_interval = 0;
            Step_Motor_Stop();
            HAL_Delay(10);
        }
    }
}

/**
  * @brief ???????
  */
void sensor_update_humidifier_display(void)
{
    char info_str[80];
    const char* state_str;
    const char* level_str;
    
    /* ????? */
    if (humidifier_state == HUMIDIFIER_STATE_ON) {
        if (motor_is_opening) {
            state_str = "OPENING";
        } else if (motor_is_closing) {
            state_str = "CLOSING";
        } else {
            state_str = "ON";
        }
    } else {
        state_str = "OFF";
    }
    
    /* ????? */
    switch (humidity_level) {
        case 0:
            level_str = "Standby";
            break;
        case 1:
            level_str = "Level 1 (Low)";
            break;
        case 2:
            level_str = "Level 2 (Medium)";
            break;
        case 3:
            level_str = "Level 3 (High)";
            break;
        default:
            level_str = "Unknown";
            break;
    }
    
    /* ??????? */
    if (dht11_data.humi_int == 0 && dht11_data.humi_dec == 0) {
        snprintf(info_str, sizeof(info_str), "Humidifier: %s  (Humi: --.-%%)", state_str);
    } else {
        snprintf(info_str, sizeof(info_str), "Humidifier: %s  Level: %s  (Humi: %d.%d%%)", 
                 state_str, level_str, dht11_data.humi_int, dht11_data.humi_dec);
    }
    
    /* ?????????????LCD */
    if (strcmp(info_str, last_humidifier_display) != 0) {
        ui_show_humidifier_info(info_str);
        strcpy(last_humidifier_display, info_str);
        printf("Humidifier Display: %s\r\n", info_str);
    }
}