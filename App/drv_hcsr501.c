#include "stm32f10x.h"
#include "drv_hcsr501.h"

uint8_t drv_hcsr501_get_level(void){
    uint8_t level;
    level = (GPIOC->IDR>>6) & 0x01;
    return level;
}

void drv_hcsr501_init(void){
    //使能GPIOC
    RCC->APB2ENR |= 1<<4;
    // 将GPIOC PC6设置为输入上拉
    GPIOC->CRL &= ~(0xF<<6*4);
    GPIOC->CRL |= 0x8<<6*4; //输入上拉
    GPIOC->ODR |= 1<<6; 
}
