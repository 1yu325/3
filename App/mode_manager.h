#ifndef __MODE_MANAGER_H
#define __MODE_MANAGER_H

#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    MODE_TEMP_HUMIDITY = 0,
    MODE_PASSWORD = 1,
    MODE_LIGHT_CONTROL = 2
} SystemMode;

/* Exported constants --------------------------------------------------------*/
#define PASSWORD_LEN 4
#define CORRECT_PASSWORD 0x0A  // 1010
#define LIGHT_COUNT 5

/* ?????? - ???? */
extern const char* light_names[LIGHT_COUNT];
extern const uint16_t light_pins[LIGHT_COUNT];
extern uint8_t light_states[LIGHT_COUNT];
extern uint8_t current_light_index;

/* Exported variables --------------------------------------------------------*/
extern volatile SystemMode current_mode;

/* Exported functions prototypes ---------------------------------------------*/
void mode_manager_init(void);
void mode_manager_process(uint32_t current_time);
void mode_switch_to_next(void);
SystemMode mode_get_current(void);

/* ?????? */
void set_light_state(uint8_t index, uint8_t state);
void toggle_current_light(void);
void select_next_light(void);
void select_prev_light(void);

#endif /* __MODE_MANAGER_H */
