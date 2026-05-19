/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-6
 * @brief       跑马灯 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 阿波罗 H743开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/TIMER/btim.h"
#include "./BSP/RELAY/relay.h"


int main(void)
{
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(160, 5, 2, 4);         /* 设置时钟, 400Mhz */
    delay_init(400);                            /* 延时初始化 */
    led_init();                                 /* 初始化LED */
    
    /* 初始化基本定时器 TIM6 产生 1ms 中断
     * 定时器时钟为 200MHz
     * 预分频器 psc = 200-1, 计数频率为 1MHz (1us)
     * 重装载值 arr = 1000-1, 溢出周期为 1000us = 1ms
     */
    
    btim_tim6_int_init(1000 - 1, 200 - 1);
    
    while (1)
    {
        delay_ms(500);
        LED0_TOGGLE();
        
    }
}
