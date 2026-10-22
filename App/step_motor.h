#ifndef __STEP_MOTOR_H
#define __STEP_MOTOR_H

/* 头文件包含----------------------------------------------------- */
#include "sys.h"
#include "main.h"
#include "step_motor_types.h"  // 包含类型定义

/* 全局变量 ------------------------------------------------------ */
/* 宏定义 ------------------------------------------ */
/* 步进电机1参数宏 */
#define LA PCout(0)     /* A相 */
#define LB PCout(1)     /* B相 */
#define LC PCout(2)     /* C相 */
#define LD PCout(3)     /* D相 */

/* 函数声明------------------------------------------------------- */

void Step_Motor_Stop(void);
void Step_Motor_CW(uint32_t nus);
void Step_Motor_CCW(uint32_t nus);

/* 新增函数：温度控制步进电机 */
void Stepper_SetSpeedByTemperature(float temperature);
uint32_t Stepper_GetSpeedFromTemperature(float temperature);
uint8_t Stepper_GetTempRange(float temperature);
uint32_t Stepper_GetCurrentSpeed(void);

/* 外部变量声明 */
extern TempSpeedMap temp_speed_map[TEMP_RANGES];

#endif