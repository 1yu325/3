#include "drivers.h"


void drivers_init(void){ 
    drv_led_init();
    drv_hcsr501_init();
	USART1_Init();
}


void drivers_dispatch(void){ 

}
