

#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"
#include "stm32h7xx_hal.h"

/* 极距通道 1 (AB1) 引脚定义 */
#define TX_CH1_PORT         GPIOC
#define TX_CH1_RPWM_PIN     GPIO_PIN_6
#define TX_CH1_LPWM_PIN     GPIO_PIN_7

/* 极距通道 2 (AB2) 引脚定义 */
#define TX_CH2_PORT         GPIOB
#define TX_CH2_RPWM_PIN     GPIO_PIN_0
#define TX_CH2_LPWM_PIN     GPIO_PIN_1

/* 极距通道 3 (AB3) 引脚定义 */
#define TX_CH3_PORT         GPIOC
#define TX_CH3_RPWM_PIN     GPIO_PIN_4
#define TX_CH3_LPWM_PIN     GPIO_PIN_5

/* 函数声明 */
void btim_tim6_int_init(uint16_t arr, uint16_t psc);

#endif

