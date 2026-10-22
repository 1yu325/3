/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "tftlcd.h"
#include <string.h>
#include "OLED_Function.h"
#include "DHT11.h"
#include "step_motor.h"
#include "My_esp8266.h"
/**
  * @brief  重定向printf函数到串口1
  * @param  ch: 要发送的字符
  * @param  f: 文件指针（未使用）
  * @retval 发送的字符
  */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */



// 密码相关定义
#define PASSWORD_LEN 4                 // 密码长度：4位二进制
#define CORRECT_PASSWORD 0x0A          // 正确密码：二进制1010（十六进制0x0A）

// 按键定义 - 根据CubeMX配置
#define KEY_NEXT_PIN      GPIO_PIN_3    // PE3 (KEY_INC) - 选择下一个灯
#define KEY_PREV_PIN      GPIO_PIN_2    // PE2 (KEY_0) - 选择上一个灯
#define KEY_TOGGLE_PIN    GPIO_PIN_4    // PE4 (KEY_DEC) - 开关灯
#define KEY_UP_PIN        GPIO_PIN_0    // PA0 (KEY_UP) - 输入1
#define KEY_CANCEL_PIN    GPIO_PIN_2// PE2 (KEY_0) - 输入0/取消
#define KEY_MODE_PIN      GPIO_PIN_1    // PF1 - 模式切换
#define LIGHT_COUNT 4                  // 灯光数量

// 火焰传感器定义
#define FLAME_SENSOR_PIN      GPIO_PIN_1     // PA1 - 火焰传感器引脚（高电平表示有火焰）
#define FLAME_SENSOR_PORT     GPIOA
#define FLAME_ALARM_PIN       GPIO_PIN_8     // 报警引脚：PB8
#define FLAME_CHECK_INTERVAL  100            // 火焰检查间隔：100ms

// 加湿器控制阈值
#define HUMIDITY_HIGH_THRESHOLD  60   // 高湿度阈值(60%) - 关闭加湿器
#define HUMIDITY_OFF_THRESHOLD   55   // 关闭阈值(55-60%之间停止)

// 湿度控制等级(4个等级)
#define HUMIDITY_LEVEL_0         55   // 等级0: 55-60% (保持当前状态)
#define HUMIDITY_LEVEL_1         50   // 等级1: 50-55% (低速加湿)
#define HUMIDITY_LEVEL_2         40   // 等级2: 40-50% (中速加湿)
#define HUMIDITY_LEVEL_3         30   // 等级3: <40% (高速加湿)

// 步进电机速度等级(延迟时间，单位ms)
#define MOTOR_SPEED_LEVEL_0      0     // 停止
#define MOTOR_SPEED_LEVEL_1      300   // 低速 (50-55%)
#define MOTOR_SPEED_LEVEL_2      150   // 中速 (40-50%)
#define MOTOR_SPEED_LEVEL_3      80    // 高速 (<40%)

// 步进电机速度定义(ms延迟)
#define STEPPER_SPEED_OFF        0     // 停止
#define STEPPER_SPEED_LOW        300   // 低速
#define STEPPER_SPEED_MEDIUM     150   // 中速
#define STEPPER_SPEED_HIGH       80    // 高速

// 加湿器状态
#define HUMIDIFIER_STATE_OFF     0      // 加湿器关闭
#define HUMIDIFIER_STATE_ON      1      // 加湿器开启

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define RXBUFFERSIZE  256     //最大接收字节数
char RxBuffer[RXBUFFERSIZE];   //接收数据
uint8_t aRxBuffer;			//接收中断缓冲
uint8_t Uart2_Rx_Cnt = 0;		//接收缓冲计数
uint8_t my_test_v = 0 ;
char my_order[15]={0};
char receive_flag=0;



// 系统模式枚举
typedef enum {
    MODE_TEMP_HUMIDITY = 0,  // 温湿度显示模式
    MODE_PASSWORD = 1,       // 密码输入模式
    MODE_LIGHT_CONTROL = 2   // 灯光控制模式
} SystemMode;

static SystemMode current_mode = MODE_TEMP_HUMIDITY; // 当前模式

// 密码相关变量
static uint8_t input_buffer[PASSWORD_LEN] = {0};  // 输入缓冲区
static uint8_t input_index = 0;                   // 输入位索引
static uint8_t error_count = 0;                   // 错误次数

// 报警相关变量
static uint8_t alarm_active = 0;                  // 报警激活标志
static uint32_t alarm_start_time = 0;             // 报警开始时间
const uint32_t ALARM_DURATION = 2000;             // 报警持续时间：2秒

// DHT11相关变量
static DHT11_Data_TypeDef dht11_data = {0};       // DHT11数据
static uint32_t last_dht11_time = 0;              // 上次DHT11读取时间
const uint32_t DHT11_UPDATE_INTERVAL = 2000;      // DHT11更新间隔：2秒

// 灯光控制相关变量
// 灯光名称
const char* light_names[LIGHT_COUNT] = {
    "Toilet",    // PA2 - 厕所
    "Bedroom",   // PA4 - 卧室
    "Kitchen",   // PA5 - 厨房
    "Study"      // PA6 - 书房
};

// 灯光引脚
const uint16_t light_pins[LIGHT_COUNT] = {
    GPIO_PIN_7,  // PA7 - 厕所
    GPIO_PIN_4,  // PA4 - 卧室
    GPIO_PIN_5,  // PA5 - 厨房
    GPIO_PIN_6   // PA6 - 书房
};

// 灯光状态，0:关，1:开
static uint8_t light_states[LIGHT_COUNT] = {0};
// 当前选中的灯光索引（用于灯光控制模式）
static uint8_t current_light_index = 0;  // 默认选中第一个灯

// LCD显示缓存（用于检测显示是否需要更新）
static uint8_t last_light_states[LIGHT_COUNT] = {0xFF};  // 初始化为无效值
static uint8_t last_light_index = 0xFF;
static char last_temp_display[30] = "";
static char last_humi_display[30] = "";

// 火焰传感器相关变量
static uint8_t flame_detected = 0;                 // 火焰检测标志 0:无，1:有
static uint32_t flame_last_check_time = 0;         // 上次火焰检查时间
static char last_flame_display[60] = "";          // 上次火焰信息显示

// 加湿器控制相关变量
static uint8_t humidifier_state = HUMIDIFIER_STATE_OFF;     // 加湿器状态
static uint8_t last_humidifier_state = HUMIDIFIER_STATE_OFF;// 上次加湿器状态
static uint8_t humidity_level = 0;                         // 当前湿度等级(0-3)
static uint8_t last_humidity_level = 0xFF;                 // 上次湿度等级
static uint8_t motor_direction = 0;                        // 电机方向：0=正转(开启)，1=反转(关闭)
static uint32_t humidifier_last_update = 0;                // 上次湿度控制更新时间
static char last_humidifier_display[80] = "";              // 上次加湿器显示

// 步进电机控制变量
static uint8_t motor_running = 0;                      // 电机运行标志:0=停止，1=运行
static uint32_t motor_last_step_time = 0;              // 上次步进时间
static uint32_t motor_current_delay = STEPPER_SPEED_OFF; // 当前步进延迟(ms)
static uint32_t motor_run_interval = 0;                // 电机运行间隔
static uint8_t motor_is_opening = 0;                   // 正在开启加湿器标志
static uint8_t motor_is_closing = 0;                   // 正在关闭加湿器标志
static uint32_t motor_target_steps = 0;                // 目标步数
static uint32_t motor_current_steps = 0;               // 当前已走步数

// 按键防抖相关变量
static uint32_t pf1_last_time = 0;
static uint32_t key_prev_last_time = 0;
static uint32_t key_next_last_time = 0;
static uint32_t key_toggle_last_time = 0;
static uint32_t key_up_last_time = 0;

// KEY2长按检测相关变量
static uint32_t key_cancel_press_start = 0;
static uint8_t key_cancel_long_press_handled = 0;

// 系统就绪标志
static uint8_t system_ready = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// 显示函数
void show_temperature_humidity_mode(void);
void show_password_mode(void);
void show_light_control_mode(void);
void update_password_display(void);
void update_lcd_temperature_display(uint16_t x, uint16_t y);
void update_lcd_humidity_display(uint16_t x, uint16_t y);
void update_light_control_display(void);
void update_selected_light_display(void);
void update_light_status_display(uint8_t index);
void update_arrow_indicator(void);
uint8_t read_dht11_data(void);
void toggle_current_light(void);
void set_light_state(uint8_t index, uint8_t state);
void select_next_light(void);
void select_prev_light(void);
void switch_mode(void);
uint8_t is_key_pressed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t active_level);

// 火焰传感器功能函数
void check_flame_sensor(void);
void update_flame_info(void);
void trigger_flame_alarm(void);
void stop_flame_alarm(void);

// 湿度控制功能函数
void init_humidity_control(void);
void update_humidity_control(void);
void control_humidifier_by_humidity(void);
uint8_t calculate_humidity_level(int16_t humidity_int, int16_t humidity_dec);
void set_humidifier_state(uint8_t state);
void start_humidifier_opening(void);
void start_humidifier_closing(void);
void run_stepper_motor_by_humidity(void);
void update_humidifier_display(void);
void display_humidity_control_info(uint16_t x, uint16_t y);
uint32_t get_motor_delay_by_level(uint8_t level);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  检查火焰传感器状态
  * @note   读取PA1引脚状态，检测到火焰时触发报警
  */
void check_flame_sensor(void)
{
    uint32_t current_time = HAL_GetTick();
    
    // 定时检查，避免频繁读取
    if (current_time - flame_last_check_time < FLAME_CHECK_INTERVAL) {
        return;
    }
    flame_last_check_time = current_time;
    
    // 读取火焰传感器状态 - 高电平表示有火焰
    uint8_t flame_state = HAL_GPIO_ReadPin(FLAME_SENSOR_PORT, FLAME_SENSOR_PIN);
    
    // 保存之前的检测状态，用于LCD更新检测
    uint8_t previous_detection = flame_detected;
    
    // 逻辑判断：高电平表示检测到火焰（参考您提供的简单main文件）
    if (flame_state == GPIO_PIN_SET) { // 高电平 = 检测到火焰
        if (!flame_detected) {
            printf("检测到火焰! (PA1=高电平)\r\n");
            trigger_flame_alarm();  // 触发报警
        }
        flame_detected = 1;
    } else {
        if (flame_detected) {
            printf("火焰消失 (PA1=低电平)\r\n");
        }
        flame_detected = 0;
    }
    
    // 更新LCD显示（仅当模式为温湿度显示模式且有状态变化时）
    if (current_mode == MODE_TEMP_HUMIDITY && previous_detection != flame_detected) {
        update_flame_info();
    }
}

/**
  * @brief  触发火焰报警
  * @note   激活PB8报警引脚
  */
void trigger_flame_alarm(void)
{
    // 设置PB8为高电平（蜂鸣器/LED）
    HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_SET);
    
    printf("火焰报警激活! (PB8=高电平)\r\n");
    
    // 更新LCD显示
    if (current_mode == MODE_TEMP_HUMIDITY) {
        update_flame_info();
    }
}

/**
  * @brief  停止火焰报警
  * @note   关闭PB8报警引脚
  */
void stop_flame_alarm(void)
{
    // 设置PB8为低电平
    HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_RESET);
    
    printf("火焰报警关闭 (PB8=低电平)\r\n");
    
    // 更新LCD显示
    if (current_mode == MODE_TEMP_HUMIDITY) {
        update_flame_info();
    }
}

/**
  * @brief  更新火焰传感器信息显示（到LCD）
  * @note   在LCD底部显示火焰检测状态和报警信息
  */
void update_flame_info(void)
{
    char info_str[80];
    
    if (flame_detected) {
        snprintf(info_str, sizeof(info_str), "火焰警报! 传感器: PA1=高电平  报警: PB8=高电平");
    } else {
        snprintf(info_str, sizeof(info_str), "无火焰  传感器: PA1=低电平  报警: PB8=低电平");
    }
    
    // 如果显示内容与上次不同，则更新显示（避免频繁刷新）
    if (strcmp(info_str, last_flame_display) != 0) {
        // 先清空显示区域
        LCD_ShowString(10, 280, tftlcd_data.width, tftlcd_data.height, 12, (u8*)"                                                                        ");
        // 显示新内容
        LCD_ShowString(10, 280, tftlcd_data.width, tftlcd_data.height, 12, (u8*)info_str);
        strcpy(last_flame_display, info_str);
        printf("火焰信息更新: %s\r\n", info_str);
    }
}

// 湿度控制相关函数

/**
  * @brief  初始化湿度控制系统
  * @note   配置步进电机引脚，初始化相关变量
  */
void init_humidity_control(void)
{
    // 步进电机引脚已在CubeMX中配置
    // PC0-PC3 连接步进电机驱动
    
    // 初始化加湿器状态
    humidifier_state = HUMIDIFIER_STATE_OFF;
    last_humidifier_state = HUMIDIFIER_STATE_OFF;
    humidity_level = 0;
    last_humidity_level = 0xFF;
    motor_direction = 0;
    
    // 停止步进电机
    Step_Motor_Stop();
    motor_running = 0;
    motor_is_opening = 0;
    motor_is_closing = 0;
    motor_current_delay = STEPPER_SPEED_OFF;
    
    printf("Humidity Control System Initialized\r\n");
    printf("Humidity Control Range:\r\n");
    printf("  - >%d%%: Humidifier OFF\r\n", HUMIDITY_OFF_THRESHOLD);
    printf("  - %d-%d%%: Level 0 (Keep current)\r\n", HUMIDITY_LEVEL_0, HUMIDITY_OFF_THRESHOLD);
    printf("  - %d-%d%%: Level 1 (Low speed, Delay: %dms)\r\n", 
           HUMIDITY_LEVEL_1, HUMIDITY_LEVEL_0, STEPPER_SPEED_LOW);
    printf("  - %d-%d%%: Level 2 (Medium speed, Delay: %dms)\r\n", 
           HUMIDITY_LEVEL_2, HUMIDITY_LEVEL_1, STEPPER_SPEED_MEDIUM);
    printf("  - <%d%%: Level 3 (High speed, Delay: %dms)\r\n", 
           HUMIDITY_LEVEL_3, STEPPER_SPEED_HIGH);
}

/**
  * @brief  计算湿度等级
  * @param  humidity_int: 湿度整数部分
  * @param  humidity_dec: 湿度小数部分
  * @retval 湿度等级 (0-3)
  */
uint8_t calculate_humidity_level(int16_t humidity_int, int16_t humidity_dec)
{
    float humidity = (float)humidity_int + (float)humidity_dec / 10.0;
    
    if (humidity >= HUMIDITY_OFF_THRESHOLD) {
        return 0;  // 55-60%之间，停止加湿
    } else if (humidity >= HUMIDITY_LEVEL_1) {
        return 1;  // 50-55%，等级1
    } else if (humidity >= HUMIDITY_LEVEL_2) {
        return 2;  // 40-50%，等级2
    } else {
        return 3;  // <40%，等级3
    }
}

/**
  * @brief  根据湿度等级获取电机延迟时间
  * @param  level: 湿度等级 (0-3)
  * @retval 电机步进延迟时间(ms)
  */
uint32_t get_motor_delay_by_level(uint8_t level)
{
    switch (level) {
        case 1:
            return STEPPER_SPEED_LOW;    // 低速
        case 2:
            return STEPPER_SPEED_MEDIUM; // 中速
        case 3:
            return STEPPER_SPEED_HIGH;   // 高速
        case 0:
        default:
            return STEPPER_SPEED_OFF;    // 停止
    }
}

/**
  * @brief  设置加湿器状态
  * @param  state: 状态 (HUMIDIFIER_STATE_ON/OFF)
  */
void set_humidifier_state(uint8_t state)
{
    if (state != humidifier_state) {
        humidifier_state = state;
        
        if (state == HUMIDIFIER_STATE_ON) {
            printf("Humidifier: Turning ON (Level: %d)\r\n", humidity_level);
            start_humidifier_opening();
        } else {
            printf("Humidifier: Turning OFF\r\n");
            start_humidifier_closing();
        }
        
        last_humidifier_state = humidifier_state;
    }
}

/**
  * @brief  开始开启加湿器
  * @note   控制步进电机正转，模拟开启加湿器
  */
void start_humidifier_opening(void)
{
    if (!motor_is_opening && !motor_is_closing) {
        motor_is_opening = 1;
        motor_is_closing = 0;
        motor_direction = 0;  // 正转
        motor_running = 1;
        motor_last_step_time = HAL_GetTick();
        motor_current_steps = 0;
        motor_target_steps = 100;  // 目标步数，可根据实际调整
        
        // 根据湿度等级设置延迟
        motor_current_delay = get_motor_delay_by_level(humidity_level);
        
       printf("Starting humidifier opening: Level=%d, Delay=%dms, Steps=%d\r\n",
               humidity_level, motor_current_delay, motor_target_steps);
    }
}

/**
  * @brief  开始关闭加湿器
  * @note   控制步进电机反转，模拟关闭加湿器
  */
void start_humidifier_closing(void)
{
    if (!motor_is_opening && !motor_is_closing) {
        motor_is_opening = 0;
        motor_is_closing = 1;
        motor_direction = 1;  // 反转
        motor_running = 1;
        motor_last_step_time = HAL_GetTick();
        motor_current_steps = 0;
        motor_target_steps = 100;  // 目标步数，可根据实际调整
        
        // 关闭时使用中速
        motor_current_delay = STEPPER_SPEED_MEDIUM;
        
        printf("Starting humidifier closing: Delay=%dms, Steps=%d\r\n",
               motor_current_delay, motor_target_steps);
    }
}

/**
  * @brief  根据湿度控制步进电机运行
  * @note   定时控制电机步进，直到达到目标步数
  */
void run_stepper_motor_by_humidity(void)
{
    if (!motor_running || motor_current_delay == STEPPER_SPEED_OFF) {
        return;
    }
    
    uint32_t current_time = HAL_GetTick();
    
    // 检查是否达到步进时间
    if (current_time - motor_last_step_time >= motor_current_delay) {
        motor_last_step_time = current_time;
        
        // 根据方向步进
        if (motor_direction == 0) {
            // 正转（开启加湿器）
            Step_Motor_CW(1);
        } else {
            // 反转（关闭加湿器）
            Step_Motor_CCW(1);
        }
        
        motor_current_steps++;
        motor_run_interval++;
        
        // 检查是否达到目标步数
        if (motor_current_steps >= motor_target_steps) {
            // 完成动作
            if (motor_is_opening) {
                motor_is_opening = 0;
                printf("Humidifier opened successfully. Steps: %d\r\n", motor_current_steps);
            } else if (motor_is_closing) {
                motor_is_closing = 0;
                printf("Humidifier closed successfully. Steps: %d\r\n", motor_current_steps);
            }
            
            motor_running = 0;
            Step_Motor_Stop();
        }
        
        // 每10步暂停一下，防止过热
        if (motor_run_interval >= 10) {
            motor_run_interval = 0;
            Step_Motor_Stop();
            HAL_Delay(10);
        }
    }
}

/**
  * @brief  根据湿度控制加湿器
  * @note   定时读取湿度，根据湿度等级控制加湿器开关
  */
void control_humidifier_by_humidity(void)
{
    uint32_t current_time = HAL_GetTick();
    
    // 每2秒检查一次湿度
    if (current_time - humidifier_last_update < 2000) {
        return;
    }
    
    humidifier_last_update = current_time;
    
    // 检查DHT11数据是否有效
    if (dht11_data.humi_int == 0 && dht11_data.humi_dec == 0) {
        // 数据无效，不处理
        return;
    }
    
    // 计算当前湿度
    float current_humidity = (float)dht11_data.humi_int + (float)dht11_data.humi_dec / 10.0;
    
    // 计算新的湿度等级
    uint8_t new_level = calculate_humidity_level(dht11_data.humi_int, dht11_data.humi_dec);
    
    // 如果湿度等级发生变化
    if (new_level != last_humidity_level) {
        printf("Humidity Level Changed: %d -> %d (Humidity: %.1f%%)\r\n",
               last_humidity_level, new_level, current_humidity);
        
        last_humidity_level = new_level;
        humidity_level = new_level;
        
        // 根据湿度等级控制加湿器
        if (current_humidity < HUMIDITY_HIGH_THRESHOLD) {
            // 湿度低于60%，考虑加湿
            if (new_level > 0) {  // 等级1-3
                set_humidifier_state(HUMIDIFIER_STATE_ON);
            } else {
                // 等级0，停止加湿
                set_humidifier_state(HUMIDIFIER_STATE_OFF);
            }
        } else {
            // 湿度高于60%，停止加湿
            set_humidifier_state(HUMIDIFIER_STATE_OFF);
        }
    }
    
    // 如果电机正在运行，根据湿度等级调整速度
    if (motor_running && motor_is_opening) {
        uint32_t new_delay = get_motor_delay_by_level(humidity_level);
        if (new_delay != motor_current_delay) {
            motor_current_delay = new_delay;
           printf("Motor speed adjusted: %dms\r\n", motor_current_delay);
        }
    }
    
    // 更新LCD显示
    if (current_mode == MODE_TEMP_HUMIDITY) {
        update_humidifier_display();
    }
}

/**
  * @brief  更新加湿器显示信息（到LCD）
  * @note   在LCD底部显示加湿器状态和湿度信息
  */
void update_humidifier_display(void)
{
    char info_str[80];
    const char* state_str;
    const char* level_str;
    
    // 状态字符串
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
    
    // 等级字符串
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
    
    // 格式化显示字符串
    if (dht11_data.humi_int == 0 && dht11_data.humi_dec == 0) {
        snprintf(info_str, sizeof(info_str), "Humidifier: %s  (Humi: --.-%%)", state_str);
    } else {
        snprintf(info_str, sizeof(info_str), "Humidifier: %s  Level: %s  (Humi: %d.%d%%)", 
                 state_str, level_str, dht11_data.humi_int, dht11_data.humi_dec);
    }
    
    // 如果显示内容与上次不同，则更新显示
    if (strcmp(info_str, last_humidifier_display) != 0) {
        // 先清空显示区域
        LCD_ShowString(10, 300, tftlcd_data.width, tftlcd_data.height, 12, (u8*)"                                                                        ");
        // 显示新内容
        LCD_ShowString(10, 300, tftlcd_data.width, tftlcd_data.height, 12, (u8*)info_str);
        strcpy(last_humidifier_display, info_str);
        
         printf("Humidifier Display Updated: %s\r\n", info_str);
    }
}

// 显示模式相关函数

/**
  * @brief  显示温湿度模式界面
  * @note   清屏并显示温湿度监测界面
  */
void show_temperature_humidity_mode(void)
{
    LCD_Clear(WHITE);

    // 标题
    LCD_ShowString(10, 10, tftlcd_data.width, tftlcd_data.height, 24, (u8*)"Smart Home Monitor");

    // 温度标签
   LCD_ShowString(10, 60, 150, 20, 16, (u8*)"Temperature:");
    
    LCD_ShowString(10, 100, 150, 20, 16, (u8*)"Humidity:");

    // 操作提示
		LCD_ShowString(10, 220, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"PF1: Switch Mode");
    LCD_ShowString(10, 240, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Current: Temp/Humi");
    
    // 清空显示缓存 强制后续的更新函数重新显示所有内容（因为缓存为空，与新内容肯定不同）
    strcpy(last_temp_display, "");
    strcpy(last_humi_display, "");
    strcpy(last_flame_display, "");
    strcpy(last_humidifier_display, "");

    // 更新温湿度显示
    update_lcd_temperature_display(10, 80);
    update_lcd_humidity_display(10, 120);

    // 更新火焰传感器信息
    update_flame_info();

    // 更新加湿器信息
    update_humidifier_display();
}

/**
  * @brief  显示密码模式界面
  * @note   清屏并显示密码输入界面
  */
void show_password_mode(void)
{
    // 清屏
    LCD_Clear(WHITE);
    
    // 标题
		LCD_ShowString(10, 30, tftlcd_data.width, tftlcd_data.height, 24, (u8*)"Password Mode");
   
    // 提示文字
    LCD_ShowString(10, 80, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Enter 4-bit password:");
    
    // 密码显示区（初始为下划线）
    LCD_ShowString(10, 110, 150, 20, 24, (u8*)"____");
    
    // 位计数器
    LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Bit:0/4");
    
    LCD_ShowString(10, 170, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_UP(PA0): Input 1");
    LCD_ShowString(10, 190, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_0(PE2): Input 0");
    LCD_ShowString(10, 210, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Hold KEY_0 >1s: Cancel");
    LCD_ShowString(10, 230, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"PF1: Switch Mode");
}

/**
  * @brief  显示灯光控制模式界面
  * @note   清屏并显示灯光控制界面
  */
void show_light_control_mode(void)
{
    // 清屏
    LCD_Clear(WHITE);
    
    // 标题
    LCD_ShowString(10, 30, tftlcd_data.width, tftlcd_data.height, 24, (u8*)"Light Control Mode");
    
    // 所有灯光状态标题
    LCD_ShowString(10, 60, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"All Lights Status:");
    
    // 灯光名称行
    char light_line[50];
    snprintf(light_line, sizeof(light_line), "%-8s %-8s %-8s %-8s ", 
             light_names[0], light_names[1], light_names[2], 
             light_names[3]);
    LCD_ShowString(10, 80, tftlcd_data.width, tftlcd_data.height, 16, (u8*)light_line);
    
    // 灯光状态行（预留位置）
    LCD_ShowString(10, 100, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                                                 ");
    
    // 箭头指示行（预留位置）
    LCD_ShowString(10, 120, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                                                 ");
    
    // 选中灯光信息
    LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Selected:          ");
    LCD_ShowString(10, 160, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Status:            ");
    
    // 按键说明 - 根据CubeMX配置
    LCD_ShowString(10, 180, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_0(PE2): Select Prev");
    LCD_ShowString(10, 200, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_INC(PE3): Select Next");
    LCD_ShowString(10, 220, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"KEY_DEC(PE4): Toggle Light");
    LCD_ShowString(10, 240, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"PF1: Switch Mode");
    
    // 注意：初始化显示缓存为无效值，确保第一次更新会显示
    for (int i = 0; i < LIGHT_COUNT; i++) {
        last_light_states[i] = 0xFF;  // 设置为无效值，确保更新
    }
    last_light_index = 0xFF;  // 设置为无效值
    
    // 更新所有灯光状态显示
    for (int i = 0; i < LIGHT_COUNT; i++) {
        update_light_status_display(i);
    }
    
    // 默认选中第一个灯（厕所）
    current_light_index = 0;
    
    // 更新选中灯光显示
    update_selected_light_display();
    update_arrow_indicator();
    
   printf("Light Control Mode Initialized - Default: Toilet selected\r\n");
}

/**
  * @brief  更新选中灯光显示
  * @note   显示当前选中灯光的名称和状态
  */
void update_selected_light_display(void)
{
    static char last_status_str[40] = "";
    static char last_control_str[40] = "";

    char status_str[40];
    char control_str[40];
    
    // 格式化显示字符串
   sprintf(status_str, "Selected: %s", light_names[current_light_index]);
    sprintf(control_str, "Status: %s", light_states[current_light_index] ? "ON" : "OFF");
    
    // 如果内容发生变化，则更新显示
    if (strcmp(status_str, last_status_str) != 0) {
        // 先清空区域
        LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                ");
        // 显示新内容
        LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)status_str);
        strcpy(last_status_str, status_str);
    }
    
    if (strcmp(control_str, last_control_str) != 0) {
        // 先清空区域
        LCD_ShowString(10, 160, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                ");
        // 显示新内容
        LCD_ShowString(10, 160, tftlcd_data.width, tftlcd_data.height, 16, (u8*)control_str);
        strcpy(last_control_str, control_str);
    }
}

/**
  * @brief  更新灯光状态显示
  * @param  index: 灯光索引 (0-4)
  */
void update_light_status_display(uint8_t index)
{
    if (index >= LIGHT_COUNT) return;
    
    // 读取GPIO状态
    uint8_t current_state = (HAL_GPIO_ReadPin(GPIOA, light_pins[index]) == GPIO_PIN_RESET) ? 1 : 0;
    
    // 如果状态没有变化，不更新显示
    if (last_light_states[index] == current_state) {
        return;
    }
    
    // 更新缓存
    last_light_states[index] = current_state;
    
    // 计算显示位置
    // 每个灯占用8个字符宽度，大约72像素
    uint16_t x_pos = 10 + index * 72;  // 72像素间隔
    
    // 格式化状态字符串
    char state_str[8];
    snprintf(state_str, sizeof(state_str), "%-8s", current_state ? "ON" : "OFF");
    
    // 显示状态
    LCD_ShowString(x_pos, 100, 64, 16, 16, (u8*)state_str);
    
     printf("Light %s status updated to %s\r\n", light_names[index], state_str);
}
/**
  * @brief  更新箭头指示器
  * @note   在选中的灯光下方显示箭头
  */
void update_arrow_indicator(void)
{
    // 如果选中的灯没有变化，不更新
    if (last_light_index == current_light_index) {
        return;
    }
    
    // 更新缓存
    last_light_index = current_light_index;
    
    // 计算显示位置
    // 每个灯占用8个字符宽度，大约72像素
    uint16_t x_pos = 10 + current_light_index * 72;  // 72像素间隔
    
    // 清空箭头行
    LCD_ShowString(10, 120, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"                                                 ");
    
    // 显示箭头
    LCD_ShowString(x_pos, 120, 16, 16, 16, (u8*)"^");
    
   printf("Arrow moved to position %d\r\n", current_light_index);
}

/**
  * @brief  更新密码显示
  * @note   根据输入更新密码显示区
  */
void update_password_display(void)
{
    char password_display[5] = "____"; // 4位密码加结束符
    
    // 将输入缓冲区的值转换为字符
    for (int i = 0; i < input_index; i++) {
        if (input_buffer[i] == 1) {
            password_display[i] = '1';
        } else {
            password_display[i] = '0';
        }
    }
    
    // 先清空区域
    LCD_ShowString(10, 110, 100, 30, 24, (u8*)"    ");
    // 显示密码
    LCD_ShowString(10, 110, 100, 30, 24, (u8*)password_display);
    
    // 更新位计数器
    static char last_bit_str[16] = "";
    char bit_str[16];
    sprintf(bit_str, "Bit:%d/4", input_index);
    
    if (strcmp(bit_str, last_bit_str) != 0) {
        LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"       ");
        LCD_ShowString(10, 140, tftlcd_data.width, tftlcd_data.height, 16, (u8*)bit_str);
        strcpy(last_bit_str, bit_str);
    }
}

/**
  * @brief  更新LCD温度显示
  * @param  x: 显示X坐标
  * @param  y: 显示Y坐标
  */
void update_lcd_temperature_display(uint16_t x, uint16_t y)
{
    char temp_str[30];
    
    if (dht11_data.temp_int == 0 && dht11_data.temp_dec == 0) {
        // 数据无效，显示"--"
        sprintf(temp_str, "Temp: --.- C");
    } else {
        // 数据有效，显示温度（保留一位小数）
        sprintf(temp_str, "Temp: %d.%d ℃", 
                dht11_data.temp_int, dht11_data.temp_dec);
    }
    
    // 如果显示内容与上次不同，则更新显示
    if (strcmp(temp_str, last_temp_display) != 0) {
        // 先清空区域
        LCD_ShowString(x, y, 150, 20, 16, (u8*)"                ");
        // 显示新内容
        LCD_ShowString(x, y, 150, 20, 16, (u8*)temp_str);
        strcpy(last_temp_display, temp_str);
    }
}

/**
  * @brief  更新LCD湿度显示
  * @param  x: 显示X坐标
  * @param  y: 显示Y坐标
  */
void update_lcd_humidity_display(uint16_t x, uint16_t y)
{
    char humi_str[30];
    
    if (dht11_data.humi_int == 0 && dht11_data.humi_dec == 0) {
        // 数据无效，显示"--"
        sprintf(humi_str, "Humi: --.- %%");
    } else {
        // 数据有效，显示湿度（保留一位小数）
        sprintf(humi_str, "Humi: %d.%d %%", 
                dht11_data.humi_int, dht11_data.humi_dec);
    }
    
    // 如果显示内容与上次不同，则更新显示
    if (strcmp(humi_str, last_humi_display) != 0) {
        // 先清空区域
        LCD_ShowString(x, y, 150, 20, 16, (u8*)"                ");
        // 显示新内容
        LCD_ShowString(x, y, 150, 20, 16, (u8*)humi_str);
        strcpy(last_humi_display, humi_str);
    }
}

/**
  * @brief  读取DHT11数据
  * @retval 1:成功 0:失败
  */
uint8_t read_dht11_data(void)
{
    uint8_t result = DHT11_ReadData(&dht11_data);
    
    if (result) {
        // 读取成功，输出调试信息
      printf("DHT11 Read Success: Temp=%d.%dC, Humi=%d.%d%%\r\n",
               dht11_data.temp_int, dht11_data.temp_dec,
               dht11_data.humi_int, dht11_data.humi_dec);
    } else {
        // 读取失败
        printf("DHT11 Read Failed!\r\n");
    }
    
    return result;
}

/**
  * @brief  设置灯光状态
  * @param  index: 灯光索引 (0-4)
  * @param  state: 0:关，1:开
  */
void set_light_state(uint8_t index, uint8_t state)
{
    if (index < LIGHT_COUNT) {
        light_states[index] = state;
        
        // 控制GPIO:state=1时引脚输出低电平GPIO_PIN_RESET，state=0时输出高电平GPIO_PIN_SET
        GPIO_PinState pin_state = state ? GPIO_PIN_RESET : GPIO_PIN_SET;
        HAL_GPIO_WritePin(GPIOA, light_pins[index], pin_state);
        
        // 更新显示
        update_light_status_display(index);
        
       printf("Light %s (%s) turned %s\r\n", 
               light_names[index], 
               index == 0 ? "PA7" :   // 对应厕所
               index == 1 ? "PA4" :   // 对应卧室
               index == 2 ? "PA5" :   // 对应厨房
                            "PA6",    // 对应书房
               state ? "ON" : "OFF");
    }
}

/**
  * @brief  切换当前选中灯光的状态
  */
void toggle_current_light(void)
{
    // 切换状态
    light_states[current_light_index] = !light_states[current_light_index];
    
    // 控制GPIO(低电平有效，所以开启时输出低电平，关闭时输出高电平)
    GPIO_PinState pin_state = light_states[current_light_index] ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(GPIOA, light_pins[current_light_index], pin_state);
    
    // 更新显示
    update_light_status_display(current_light_index);
    update_selected_light_display();
    
     printf("Toggled %s light to %s\r\n", 
           light_names[current_light_index],
           light_states[current_light_index] ? "ON" : "OFF");
}


/**
  * @brief  选择下一个灯光（循环）
  */
void select_next_light(void)
{
    uint8_t old_index = current_light_index;
    
    if (current_light_index < (LIGHT_COUNT - 1)) {
        current_light_index++;
    } else {
        current_light_index = 0;  // 循环到第一个
    }
    
    // 更新显示
    update_arrow_indicator();//移动箭头指示器
    update_selected_light_display();//更新选中灯信息
    
    printf("Selected NEXT light: %s (from %s)\r\n", 
           light_names[current_light_index],
           light_names[old_index]);
}

/**
  * @brief  选择上一个灯光（循环）
  */
void select_prev_light(void)
{
    uint8_t old_index = current_light_index;
    
    if (current_light_index > 0) {
        current_light_index--;
    } else {
        current_light_index = LIGHT_COUNT - 1;  // 循环到最后一个
    }
    
    // 更新显示
    update_arrow_indicator();
    update_selected_light_display();
    
    printf("Selected PREVIOUS light: %s (from %s)\r\n", 
           light_names[current_light_index],
           light_names[old_index]);
}

/**
  * @brief  切换模式（通过PF1按键）
  */
void switch_mode(void)
{
    // 循环切换模式
    uint8_t next_mode = (uint8_t)current_mode;
    next_mode = (next_mode + 1) % 3;
    current_mode = (SystemMode)next_mode;
    
    // 根据新模式显示界面
    switch (current_mode) {
        case MODE_TEMP_HUMIDITY:
            show_temperature_humidity_mode();
            // 立即更新温湿度显示
            update_lcd_temperature_display(10, 80);
            update_lcd_humidity_display(10, 120);
            // 更新火焰传感器信息
            update_flame_info();
            // 更新加湿器信息
            update_humidifier_display();
            printf("Switched to Temperature/Humidity Mode\r\n");
            break;
            
        case MODE_PASSWORD:
            // 重置密码输入
            input_index = 0;
            memset(input_buffer, 0, sizeof(input_buffer));
            show_password_mode();
            printf("Switched to Password Mode\r\n");
            break;
            
        case MODE_LIGHT_CONTROL:
            show_light_control_mode();
            printf("Switched to Light Control Mode\r\n");
            break;
    }
}

/**
  * @brief  检测按键是否按下（带防抖）
  * @param  GPIOx: GPIO端口
  * @param  GPIO_Pin: GPIO引脚
  * @param  active_level: 有效电平（0:低电平有效，1:高电平有效）
  * @retval 1:按下，0:未按下
  */
uint8_t is_key_pressed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t active_level)
{
    if (active_level == 0) {
        // 低电平有效
        return (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET);
    } else {
        // 高电平有效
        return (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC3_Init();
  MX_FSMC_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_TIM5_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET );
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&aRxBuffer, 1);
	esp8266_start_trans();
	
	
  // 启动ADC校准
  HAL_ADCEx_Calibration_Start(&hadc3);
  
  // 初始化：将所有灯光设置为关闭状态
  for (int i = 0; i < LIGHT_COUNT; i++) {
    set_light_state(i, 0);
  }
  
  // 初始化：默认选中第一个灯（厕所）
  current_light_index = 0;
  
  // 初始化湿度控制系统
  init_humidity_control();
    
  HAL_Delay(1000);
    
  // 设置LCD颜色
  BACK_COLOR = WHITE;
  FRONT_COLOR = BLACK;

  // 初始化LCD
  TFTLCD_Init();
  
  // 设置LCD颜色
  BACK_COLOR = WHITE;
  FRONT_COLOR = BLACK;
  
  // 初始显示：温湿度模式（模式0）
  show_temperature_humidity_mode();

  // 记录DHT11首次读取时间
  last_dht11_time = HAL_GetTick();
  
  // 首次读取DHT11数据
  if (read_dht11_data()) {
      // 读取成功，更新显示
      update_lcd_temperature_display(10, 80);
      update_lcd_humidity_display(10, 120);
      // 根据湿度控制加湿器
      control_humidifier_by_humidity();
  }

  // 初始化火焰报警引脚为低电平
  HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_RESET);

   printf("System Initialized\r\n");
  printf("PF1: Switch Mode, Current Mode: Temp/Humidity\r\n");
  printf("Flame Sensor: AO=PA1(ADC), DO=PB1\r\n");
  printf("Flame Alarm Pin: PB8 (shared with password alarm)\r\n");
  printf("Humidity Control: Auto control by humidity\r\n");
  printf("Light Control: Default selection = Toilet (PA2)\r\n");
  printf("Key Mapping:\r\n");
  printf("  - PF1: Mode switch\r\n");
  printf("  - PA0(KEY_UP): Password mode input 1\r\n");
  printf("  - PE2(KEY_0): Light control select prev, Password mode input 0\r\n");
  printf("  - PE3(KEY_INC): Light control select next\r\n");
  printf("  - PE4(KEY_DEC): Light control toggle switch\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 系统就绪标志（启动后2秒）
    if (HAL_GetTick() > 2000) {
        system_ready = 1;
    }
    
    uint32_t current_time_ms = HAL_GetTick();
		
		
		
		
		//WIFI----------------------------
		if(receive_flag)
	{
		receive_flag=0;
		if(strstr((const char*)my_order,(const char*)"<on>")) //输入on，LED0亮
		{
			Uart2_Rx_Cnt=0;
			memset(RxBuffer,0x00,sizeof(RxBuffer)); //清空数组	
			memset(RxBuffer,0x00,sizeof(my_order)); //清空数组	
			my_test_v=1;
		}
		else if(strstr((const char*)my_order,(const char*)"<off>"))
		{
					Uart2_Rx_Cnt=0;
			memset(RxBuffer,0x00,sizeof(RxBuffer)); //清空数组	
			memset(RxBuffer,0x00,sizeof(my_order)); //清空数组	
		  my_test_v=0;
		}
		else
			{
			Uart2_Rx_Cnt=0;
			memset(RxBuffer,0x00,sizeof(RxBuffer)); //清空数组	
			memset(RxBuffer,0x00,sizeof(my_order)); //清空数组	
		
		}
			
 	}
		if(my_test_v==1)	
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET );
		else
	  	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET );
		//----------------------------------
		
		
		
		
		
		
    // ==============================
    // === 1. 火焰传感器检测 ===
    // ==============================
    check_flame_sensor();

    // ==============================
    // === 2. DHT11数据读取 ===
    // ==============================
    if (current_time_ms - last_dht11_time >= DHT11_UPDATE_INTERVAL)
    {
        last_dht11_time = current_time_ms;
        
        if (read_dht11_data()) {
            // 读取成功，更新LCD显示（仅温湿度模式）
            if (current_mode == MODE_TEMP_HUMIDITY) {
                update_lcd_temperature_display(10, 80);
                update_lcd_humidity_display(10, 120);
            }
        }
    }

    // ==============================
    // === 3. 湿度控制 ===
    // ==============================
    control_humidifier_by_humidity();
    
    // ==============================
    // === 4. 步进电机控制 ===
    // ==============================
    run_stepper_motor_by_humidity();

    // ==============================
    // === 5. PF1按键检测（模式切换） ===
    // ==============================
    // PF1: 模式切换（低电平有效）
    if (HAL_GPIO_ReadPin(GPIOF, KEY_MODE_PIN) == GPIO_PIN_RESET) {
        if (current_time_ms - pf1_last_time > 500) { // 500ms防抖
            pf1_last_time = current_time_ms;
            switch_mode();
            printf("PF1 Pressed - Mode Changed\r\n");
        }
    }

    // ==============================
    // === 6. 模式相关处理 ===
    // ==============================
    switch (current_mode) {
        case MODE_TEMP_HUMIDITY:
            // 温湿度模式无特殊按键处理
            break;
            
        case MODE_PASSWORD:
            // 密码模式处理
            
            // KEY_UP (PA0): 输入1 - 高电平有效
            if (HAL_GPIO_ReadPin(GPIOA, KEY_UP_PIN) == GPIO_PIN_SET) {
                if (current_time_ms - key_up_last_time > 300) { // 300ms防抖
                    key_up_last_time = current_time_ms;
                    if (input_index < PASSWORD_LEN) {
                        input_buffer[input_index] = 1;
                        input_index++;
                        update_password_display();
                        printf("KEY_UP Pressed - Input 1\r\n");
                    }
                }
            }
            
            // KEY_0 (PE2): 输入0/取消 - 低电平有效
            if (HAL_GPIO_ReadPin(GPIOE, KEY_CANCEL_PIN) == GPIO_PIN_RESET) {
                // 记录按下开始时间
                if (key_cancel_press_start == 0) {
                    key_cancel_press_start = current_time_ms;
                    key_cancel_long_press_handled = 0;
                }
                
                // 长按检测（1秒）
                if (!key_cancel_long_press_handled && (current_time_ms - key_cancel_press_start > 1000)) {
                    // 长按：清除输入
                    input_index = 0;
                    memset(input_buffer, 0, sizeof(input_buffer));
                    update_password_display();
                    LCD_ShowString(10, 250, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Input Cleared");
                    HAL_Delay(500);
                    LCD_ShowString(10, 250, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"              ");
                    key_cancel_long_press_handled = 1;
                    printf("KEY_0 Long Press - Input Cleared\r\n");
                }
            } else {
                // 按键释放
                if (key_cancel_press_start > 0) {
                    // 短按：输入0
                    if (!key_cancel_long_press_handled && input_index < PASSWORD_LEN) {
                        input_buffer[input_index] = 0;
                        input_index++;
                        update_password_display();
                       printf("KEY_0 Short Press - Input 0\r\n");
                    }
                    key_cancel_press_start = 0;
                    key_cancel_long_press_handled = 0;
                }
            }
            
            // 密码验证（当输入满4位时）
            if (input_index >= PASSWORD_LEN) {
                uint8_t value = 0;
                for (int i = 0; i < PASSWORD_LEN; i++) {
                    value = (value << 1) | input_buffer[i];
                }
                
                if (value == CORRECT_PASSWORD) {
                    // 密码正确
                    error_count = 0;
                    alarm_active = 0;
                    // 关闭PB8报警（注意检查火焰报警状态）
                    if (!flame_detected) {
                        HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_RESET);
                    }
                    LCD_ShowString(10, 250, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"Access Granted!");
                    HAL_Delay(1000);
                    current_mode = MODE_TEMP_HUMIDITY;
                    show_temperature_humidity_mode();
                    update_lcd_temperature_display(10, 80);
                    update_lcd_humidity_display(10, 120);
                    update_flame_info();
                    update_humidifier_display();
                    printf("Password Correct - Access Granted\r\n");
                } else {
                    // 密码错误
                    error_count++;
                    char error_msg[30];
                    sprintf(error_msg,  "Wrong! Try %d/3", 3 - error_count);
                    LCD_ShowString(10, 250, tftlcd_data.width, tftlcd_data.height, 16, (u8*)error_msg);
                    
                    if (error_count >= 3) {
                        // 错误3次，触发报警
                        alarm_active = 1;
                        alarm_start_time = current_time_ms;
                        // 开启PB8报警
                        HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_SET);
                        LCD_ShowString(10, 270, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"ALARM ACTIVATED!");
                        HAL_Delay(2000);
                        error_count = 0;
                        current_mode = MODE_TEMP_HUMIDITY;
                        show_temperature_humidity_mode();
                        update_lcd_temperature_display(10, 80);
                        update_lcd_humidity_display(10, 120);
                        update_flame_info(); 
                        update_humidifier_display();
                       printf("3 Wrong Attempts - Alarm Activated\r\n");
                    } else {
                        HAL_Delay(1000);
                        input_index = 0;
                        memset(input_buffer, 0, sizeof(input_buffer));
                        update_password_display();
                        LCD_ShowString(10, 250, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"              ");
                        LCD_ShowString(10, 270, tftlcd_data.width, tftlcd_data.height, 16, (u8*)"              ");
                    }
                }
            }
            break;
            
        case MODE_LIGHT_CONTROL:
            // 灯光控制模式处理
            
            // KEY_0 (PE2): 选择上一个灯光（前一个）- 低电平有效
            if (HAL_GPIO_ReadPin(GPIOE, KEY_PREV_PIN) == GPIO_PIN_RESET) {
                if (current_time_ms - key_prev_last_time > 300) { // 300ms防抖
                    key_prev_last_time = current_time_ms;
                    select_prev_light();
                    printf("KEY_0(PE2) Pressed - Select Previous Light\r\n");
                }
            }
            
            // KEY_INC (PE3): 选择下一个灯光（后一个）- 低电平有效
            if (HAL_GPIO_ReadPin(GPIOE, KEY_NEXT_PIN) == GPIO_PIN_RESET) {
                if (current_time_ms - key_next_last_time > 300) { // 300ms防抖
                    key_next_last_time = current_time_ms;
                    select_next_light();
                    printf("KEY_INC(PE3) Pressed - Select Next Light\r\n");
                }
            }
            
            // KEY_DEC (PE4): 切换当前灯光开关 - 低电平有效
            if (HAL_GPIO_ReadPin(GPIOE, KEY_TOGGLE_PIN) == GPIO_PIN_RESET) {
                if (current_time_ms - key_toggle_last_time > 300) { // 300ms防抖
                    key_toggle_last_time = current_time_ms;
                    toggle_current_light();
                     printf("KEY_DEC(PE4) Pressed - Toggle Light\r\n");
                }
            }
            break;
    }

    // ==============================
    // === 7. 报警处理（2秒后自动关闭，但火焰报警需要手动检查） ===
    // ==============================
    if (alarm_active && (current_time_ms - alarm_start_time >= ALARM_DURATION)) {
        // 关闭PB8报警，注意检查火焰报警状态
        if (!flame_detected) {
            HAL_GPIO_WritePin(GPIOB, FLAME_ALARM_PIN, GPIO_PIN_RESET);
        }
        alarm_active = 0;
    }
    
    // 火焰传感器报警处理（火焰消失时自动关闭报警）
    if (flame_detected) {
        // 持续读取火焰传感器状态
        uint8_t flame_state = HAL_GPIO_ReadPin(FLAME_SENSOR_PORT, FLAME_SENSOR_PIN);
        if (flame_state == GPIO_PIN_RESET) { // 低电平 = 火焰消失
            flame_detected = 0;
            stop_flame_alarm();  // 火焰消失，停止报警
            update_flame_info(); // 更新显示
        }
    }

    // 主循环延时，降低CPU使用率
    HAL_Delay(10);
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
 
	if(Uart2_Rx_Cnt >= 255)  //溢出判断
	{
		Uart2_Rx_Cnt = 0;
		memset(RxBuffer,0x00,sizeof(RxBuffer));	        
	}
	else
	{
		static uint8_t Uart2_count=0;
		
		RxBuffer[Uart2_Rx_Cnt] = aRxBuffer;   //接收数据转存
		if(receive_flag==0)
		{
		if(RxBuffer[Uart2_Rx_Cnt-Uart2_count]=='<')
		{	
			Uart2_count++;
		if((RxBuffer[Uart2_Rx_Cnt]=='>')||Uart2_count>=14)
		{
			uint8_t My_i=0;
		for(int i=Uart2_Rx_Cnt-Uart2_count+1;i<Uart2_Rx_Cnt+1;i++)
			my_order[My_i++]=RxBuffer[i];
			receive_flag=1;
			Uart2_count=0;
		}
	}
	}
		Uart2_Rx_Cnt++;
	}
	
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&aRxBuffer, 1);   //再开启接收中断
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* 用户可以添加自己的错误处理实现 */
 __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* 用户可以添加自己的实现来报告文件名和行号 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
