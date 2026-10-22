/**
 * ************************************************************************
 * 
 * @file DHT11.h
 * @author zxr
 * @brief 
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2024  
 * ************************************************************************
 */
#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

#define DHT11_PORT			GPIOF
#define DHT11_PIN			GPIO_PIN_0

#define DHT11_PULL_1		HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET)
#define DHT11_PULL_0		HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET)

#define DHT11_ReadPin		HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)

/**
 * ************************************************************************
 * @brief 存储传感器数据的结构体
 * 
 * 
 * ************************************************************************
 */
typedef struct
{
	uint8_t humi_int;			// 湿度的整数部分
	uint8_t humi_dec;	 		// 湿度的小数部分
	uint8_t temp_int;	 		// 温度的整数部分
	uint8_t temp_dec;	 		// 温度的小数部分
	uint8_t check_sum;	 		// 校验和

} DHT11_Data_TypeDef;


uint8_t DHT11_ReadData(DHT11_Data_TypeDef* DHT11_Data);

#endif

