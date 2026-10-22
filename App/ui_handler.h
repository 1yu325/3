#ifndef __UI_HANDLER_H
#define __UI_HANDLER_H

#include "main.h"
#include "tftlcd.h"
#include "mode_manager.h"  /* ??mode_manager.h????????? */

/* Exported functions prototypes ---------------------------------------------*/
void ui_handler_init(void);
void ui_handler_process(uint32_t current_time);

/* ???????? */
void ui_show_temperature_humidity_mode(void);
void ui_show_password_mode(void);
void ui_show_light_control_mode(void);

/* ?????????? */
void ui_update_temperature_display(uint8_t temp_int, uint8_t temp_dec);
void ui_update_humidity_display(uint8_t humi_int, uint8_t humi_dec);
void ui_show_flame_info(const char* info);
void ui_show_humidifier_info(const char* info);

/* ???????? */
void ui_update_password_display(uint8_t* buffer, uint8_t index);
void ui_show_message(const char* msg, uint16_t y);
void ui_clear_message_area(void);

/* ?????????? */
void ui_update_light_status_display(uint8_t index);
void ui_update_selected_light_display(void);
void ui_update_arrow_indicator(void);
void ui_update_all_lights_display(void);

/* ?????? */
void ui_clear_area(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void ui_draw_string(uint16_t x, uint16_t y, const char* str, uint8_t size);

#endif /* __UI_HANDLER_H */
