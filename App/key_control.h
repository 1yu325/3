#ifndef KEY_CONTROL_H
#define KEY_CONTROL_H

#include "main.h"

uint8_t Key_Scan(uint8_t mode);



//???????
#define KEY_UP 	HAL_GPIO_ReadPin(KEY_UP_GPIO_Port,KEY_UP_Pin)
#define KEY0 	HAL_GPIO_ReadPin(KEY0_GPIO_Port,KEY0_Pin)
#define KEY1 	HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin)
#define KEY2 	HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin)

//???????  
#define KEY_UP_PRESS 	1
#define KEY0_PRESS		2
#define KEY1_PRESS		3
#define KEY2_PRESS		4


#endif


