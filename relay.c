#include "./BSP/RELAY/relay.h"
#include "./SYSTEM/delay/delay.h"

/* 多极距继电器矩阵初始化 */
void relay_init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef gpio_init_struct;
    
    /* 集中初始化 PD0 ~ PD7 */
    gpio_init_struct.Pin = RELAY_CH1_PIN | RELAY_CH2_PIN | RELAY_CH3_PIN | RELAY_CH4_PIN | 
                           RELAY_CH5_PIN | RELAY_CH6_PIN | RELAY_CH7_PIN | RELAY_CH8_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;   
    gpio_init_struct.Pull = GPIO_PULLUP;           /* 默认上拉，适配大多数低电平触发继电器 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RELAY_PORT, &gpio_init_struct);
    
    /* 初始状态强制关闭所有继电器通道，防止电极短路 */
    relay_all_off();
}

/* 关闭所有电极通道 */
void relay_all_off(void)
{
    /* 假设继电器为低电平触发，输出高电平为关闭 (视具体模块调整) */
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_CH1_PIN | RELAY_CH2_PIN | RELAY_CH3_PIN | 
                                  RELAY_CH4_PIN | RELAY_CH5_PIN | RELAY_CH6_PIN | 
                                  RELAY_CH7_PIN | RELAY_CH8_PIN, GPIO_PIN_SET);
}

/* 核心接口：极距切换逻辑 (Spacing Level) 
 * 切换前必须先全断开(Break-before-make)，防止相邻电极短路 
 */
void relay_switch_spacing(uint8_t spacing_level)
{
    relay_all_off();
    delay_ms(50); /* 给予继电器机械触点充足的弹开脱离时间，消除电弧 */
    
    switch(spacing_level)
    {
        case 1: /* 极距 1：闭合 CH1 (如 10cm 间距) */
            HAL_GPIO_WritePin(RELAY_PORT, RELAY_CH1_PIN, GPIO_PIN_RESET);
            break;
        case 2: /* 极距 2：闭合 CH2 (如 20cm 间距) */
            HAL_GPIO_WritePin(RELAY_PORT, RELAY_CH2_PIN, GPIO_PIN_RESET);
            break;
        case 3: /* 极距 3：闭合 CH3 (如 30cm 间距) */
            HAL_GPIO_WritePin(RELAY_PORT, RELAY_CH3_PIN, GPIO_PIN_RESET);
            break;
        case 4: /* 极距 4：闭合 CH4 (如 40cm 间距) */
            HAL_GPIO_WritePin(RELAY_PORT, RELAY_CH4_PIN, GPIO_PIN_RESET);
            break;
        default:
            break;
    }
    delay_ms(50); /* 给予机械触点充足的吸合稳定时间，防止采集到弹跳噪声 */
}

