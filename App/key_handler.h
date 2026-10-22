#ifndef __KEY_HANDLER_H
#define __KEY_HANDLER_H

#include "main.h"

/* ???? - ??CubeMX?? */
#define KEY_MODE_PIN      WEN_SHI_Pin    /* PF1 - ????? */
#define KEY_NEXT_PIN      KEY_INC_Pin    /* PE3 (KEY_INC) - ????????? */
#define KEY_PREV_PIN      KEY_0_Pin      /* PE2 (KEY_0) - ?????????/????0 */
#define KEY_TOGGLE_PIN    KEY_DEC_Pin    /* PE4 (KEY_DEC) - ???????? */
#define KEY_UP_PIN        KEY_1_Pin      /* PA0 (KEY_UP) - ????1 */
#define KEY_CANCEL_PIN    KEY_0_Pin      /* PE2 (KEY_0) - ????0/?? */

/* ??????(ms) */
#define KEY_DEBOUNCE_TIME 50
#define KEY_LONG_PRESS_TIME 1000

/* Exported functions prototypes ---------------------------------------------*/
void key_handler_init(void);
void key_handler_process(uint32_t current_time);

/* ?????? */
void key_light_select_prev(void);
void key_light_select_next(void);
void key_light_toggle_current(void);
void key_set_light_state(uint8_t index, uint8_t state);

/* ?????? */
uint8_t key_is_pressed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t active_level);

#endif /* __KEY_HANDLER_H */
