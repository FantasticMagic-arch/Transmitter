

#ifndef __RELAY_H
#define __RELAY_H

#include "./SYSTEM/sys/sys.h"


/* 继电器控制引脚定义 (挂载于 GPIOD) */
#define RELAY_PORT          GPIOD
#define RELAY_CH1_PIN       GPIO_PIN_0
#define RELAY_CH2_PIN       GPIO_PIN_1
#define RELAY_CH3_PIN       GPIO_PIN_2
#define RELAY_CH4_PIN       GPIO_PIN_3
#define RELAY_CH5_PIN       GPIO_PIN_4
#define RELAY_CH6_PIN       GPIO_PIN_5
#define RELAY_CH7_PIN       GPIO_PIN_6
#define RELAY_CH8_PIN       GPIO_PIN_7

/* 函数声明 */
void relay_init(void);
void relay_switch_spacing(uint8_t spacing_level);
void relay_all_off(void);

#endif

