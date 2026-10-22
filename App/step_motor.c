/* 头文件包含 -------------------------------------------------- */
#include "step_motor.h"
#include "step_motor_types.h"  // 包含类型定义
#include "DHT11.h"  // 添加DHT11头文件

/* 变量定义 ---------------------------------------------------- */
/* 私有类型定义------------------------------------------------- */
typedef enum _PIN_BIT
{
    PLA = 0x01,
    PLB = 0x02,
    PLC = 0x04,
    PLD = 0x08,
} Pin_Bit;

/* 私有变量定义------------------------------------- */
static uint8_t steps[8] = {0x01, 0x03, 0x02, 0x6, 0x04, 0x0c, 0x08, 0x09};

/* 温度-速度映射表 - 外部可访问 */
TempSpeedMap temp_speed_map[TEMP_RANGES] = {
    {20.0, 22.0, 500, "Low Temp"},     // 温度20-22°C，慢速
    {22.0, 25.0, 300, "Low-Mid Temp"}, // 温度22-25°C，中慢速
    {25.0, 28.0, 150, "Mid Temp"},     // 温度25-28°C，中速
    {28.0, 31.0, 80,  "Mid-High Temp"},// 温度28-31°C，中快速
    {31.0, 35.0, 50,  "High Temp"}     // 温度31-35°C，快速
};

static uint32_t current_stepper_speed = 300;  // 默认速度

/* 函数定义 ---------------------------------------------------- */

/**
 * @name: Step_Motor_Stop
 * @description: 电机停止
 */
void Step_Motor_Stop(void)
{
    LA = 0;
    LB = 0;
    LC = 0;
    LD = 0;
}

/**
 * @name: Step_Motor_CW
 * @description: 电机正转函数
 * @param {uint32_t} nus 每步步进之间间隔时间(us)
 */
void Step_Motor_CW(uint32_t nus)
{
    volatile uint8_t i,j;
    uint8_t temp = 0;
    
   
        for(i = 0; i < 8; i++)
        {
            temp = steps[i];
            LA = (uint8_t)((temp&PLA) >> 0);
            LB = (uint8_t)((temp&PLB) >> 1);
            LC = (uint8_t)((temp&PLC) >> 2);
            LD = (uint8_t)((temp&PLD) >> 3);
            
            HAL_Delay(nus/1000);  // 转换为ms
        }
    
    
    Step_Motor_Stop();
}

/**
 * @name: Step_Motor_CCW
 * @description: 电机反转函数
 * @param {uint32_t} nus 每步步进之间间隔时间(us)
 */
void Step_Motor_CCW(uint32_t nus)
{
    int i,j;
	
	
	for(i = 8; i > 0; i--)
    {
        LA = (uint8_t)((steps[i-1]&PLA) >> 0);
        LB = (uint8_t)((steps[i-1]&PLB) >> 1);
        LC = (uint8_t)((steps[i-1]&PLC) >> 2);
        LD = (uint8_t)((steps[i-1]&PLD) >> 3);
        
        HAL_Delay(nus/1000);  // 转换为ms
    }
	
    
    Step_Motor_Stop();
}

/**
 * @name: Stepper_GetTempRange
 * @description: 根据温度获取温度范围档位
 * @param {float} temperature 当前温度
 * @return {uint8_t} 温度档位(0-4)
 */
uint8_t Stepper_GetTempRange(float temperature)
{
    for(uint8_t i = 0; i < TEMP_RANGES; i++) {
        if(temperature >= temp_speed_map[i].min_temp && 
           temperature <= temp_speed_map[i].max_temp) {
            return i;
        }
    }
    
    // 如果温度超出范围，返回边界值
    if(temperature < MIN_TEMP) return 0;
    if(temperature > MAX_TEMP) return TEMP_RANGES - 1;
    
    return 0;
}

/**
 * @name: Stepper_GetSpeedFromTemperature
 * @description: 根据温度计算步进电机速度
 * @param {float} temperature 当前温度
 * @return {uint32_t} 步进延迟时间(ms)
 */
uint32_t Stepper_GetSpeedFromTemperature(float temperature)
{
    uint8_t range = Stepper_GetTempRange(temperature);
    
    // 线性插值计算速度
    if(range < TEMP_RANGES - 1) {
        float temp_ratio = (temperature - temp_speed_map[range].min_temp) / 
                          (temp_speed_map[range].max_temp - temp_speed_map[range].min_temp);
        
        uint32_t speed_diff = temp_speed_map[range].step_delay - temp_speed_map[range+1].step_delay;
        return temp_speed_map[range].step_delay - (uint32_t)(speed_diff * temp_ratio);
    } else {
        return temp_speed_map[range].step_delay;
    }
}

/**
 * @name: Stepper_SetSpeedByTemperature
 * @description: 根据温度设置步进电机速度
 * @param {float} temperature 当前温度
 */
void Stepper_SetSpeedByTemperature(float temperature)
{
    uint32_t new_speed = Stepper_GetSpeedFromTemperature(temperature);
    
    if(new_speed != current_stepper_speed) {
        current_stepper_speed = new_speed;
        
        uint8_t range = Stepper_GetTempRange(temperature);
        printf("Temperature: %.1f°C -> Range: %s -> Stepper Speed: %lums/step\r\n",
               temperature, temp_speed_map[range].range_desc, current_stepper_speed);
    }
}

/**
 * @name: Stepper_GetCurrentSpeed
 * @description: 获取当前步进电机速度
 * @return {uint32_t} 当前步进延迟(ms)
 */
uint32_t Stepper_GetCurrentSpeed(void)
{
    return current_stepper_speed;
}