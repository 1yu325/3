#ifndef __My_esp8266_H__
#define __My_esp8266_H__

#include "main.h"

typedef uint16_t u16  ;


extern void esp8266_test(void);
extern void u2_printf(char* fmt);
extern char esp8266_send_cmd(char *cmd,char *ack,u16 waittime); 
extern void esp8266_start_trans(void);

#endif




