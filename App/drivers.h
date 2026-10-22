#ifndef __DRIVERS_H__
#define __DRIVERS_H__
#include "drv_led.h"
#include "drv_hcsr501.h"
#include "usart.h"

void drivers_init(void);
void drivers_dispatch(void);

#endif
