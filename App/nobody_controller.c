#include "drivers.h"
#include "nobody_controller.h"
#include "usart.h"
static uint8_t pre_body_state;
uint8_t count=0;
void nobody_control(void){
    uint8_t body_state;
    body_state = drv_hcsr501_get_level();
		USART1_SendChar('0'+body_state);

		//借用按键检测
		if(body_state == SOMEBODY)
		{
			while(drv_hcsr501_get_level() == SOMEBODY);
			//检测到1次人，会输出多个1，等待这次1的输完
			count++;
			//count=1,人进入
			if(count == 1)
				drv_led_on_all();
			else
			{
				drv_led_off_all();
				count=0;
			}
			//count=2，人离开
			
		}
//		if(pre_body_state == SOMEBODY && body_state == NOBODY){
//				drv_led_off_all();
//		}
//		else
//		{
//			drv_led_on_all();
//		}
//		pre_body_state = body_state;    
	

}

void nobody_controller_init(void){
    pre_body_state = drv_hcsr501_get_level();
}
