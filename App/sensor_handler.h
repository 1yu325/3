#ifndef __SENSOR_HANDLER_H
#define __SENSOR_HANDLER_H

#include "main.h"
#include "DHT11.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t humi_int;
    uint8_t humi_dec;
} DHT11_Data_t;

/* ??????? */
#define FLAME_SENSOR_DO_PIN      GPIO_PIN_1
#define FLAME_SENSOR_DO_PORT     GPIOB
#define FLAME_SENSOR_ADC_CHANNEL ADC_CHANNEL_1
#define FLAME_THRESHOLD          2500
#define FLAME_CHECK_INTERVAL     500
#define FLAME_ALARM_DURATION     10000
#define FLAME_ALARM_PIN          GPIO_PIN_8

/* ?????? */
#define HUMIDITY_HIGH_THRESHOLD  60   /* ?????(60%) - ?????????? */
#define HUMIDITY_OFF_THRESHOLD   55   /* ????(55-60%????????) */

/* ????(4???) */
#define HUMIDITY_LEVEL_0         55   /* ??0: 55-60% (??????) */
#define HUMIDITY_LEVEL_1         50   /* ??1: 50-55% (????) */
#define HUMIDITY_LEVEL_2         40   /* ??2: 40-50% (????) */
#define HUMIDITY_LEVEL_3         30   /* ??3: <40% (????) */

/* ??????(????ms) */
#define STEPPER_SPEED_OFF        0     /* ?? */
#define STEPPER_SPEED_LOW        300   /* ?? */
#define STEPPER_SPEED_MEDIUM     150   /* ?? */
#define STEPPER_SPEED_HIGH       80    /* ?? */

/* ????? */
#define HUMIDIFIER_STATE_OFF     0      /* ????? */
#define HUMIDIFIER_STATE_ON      1      /* ????? */

/* Exported variables --------------------------------------------------------*/
extern DHT11_Data_t dht11_data;
extern volatile uint8_t system_ready;

/* Exported functions prototypes ---------------------------------------------*/
void sensor_handler_init(void);
void sensor_handler_process(uint32_t current_time);
void sensor_update_display(void);

/* DHT11???? */
uint8_t sensor_dht11_read(void);
void sensor_dht11_init(void);
void sensor_update_temp_humi_display(void);

/* ????????? */
void sensor_flame_init(void);
void sensor_flame_check(uint32_t current_time);
uint16_t sensor_flame_read_adc(void);
uint8_t sensor_flame_is_detected(void);
uint8_t sensor_flame_alarm_is_active(void);
void sensor_update_flame_display(void);

/* ???????? */
void sensor_humidity_control_init(void);
void sensor_humidity_control_process(uint32_t current_time);
uint8_t sensor_calculate_humidity_level(int16_t humidity_int, int16_t humidity_dec);
uint32_t sensor_get_motor_delay_by_level(uint8_t level);
void sensor_set_humidifier_state(uint8_t state);
void sensor_start_humidifier_opening(void);
void sensor_start_humidifier_closing(void);
void sensor_run_stepper_motor(void);
void sensor_update_humidifier_display(void);

#endif /* __SENSOR_HANDLER_H */
