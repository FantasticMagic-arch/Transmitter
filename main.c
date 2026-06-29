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
#include "./BSP/ADC/adc.h"

/* 显式声明外部定义的 TIM6 句柄，消除跨文件编译未定义错误 */
extern TIM_HandleTypeDef g_tim6_handle;

int main(void)
{
    sys_cache_enable();                         /* 打开 L1-Cache */
    HAL_Init();                                 /* 初始化 HAL 库 */
    sys_stm32_clock_init(160, 5, 2, 4);         /* 设置系统时钟 400Mhz */
    delay_init(400);                            /* 延时初始化 */
    usart_init(115200);                         /* 串口初始化 */
    led_init();                                 /* 初始化 LED */
    
    /* 1. 初始化 PD3 为推挽输出，专门用于向接收机发送绝对相位发令枪信号 */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = GPIO_PIN_3;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET); /* 初始牢牢锁死在低电平 */

    /* 2. 预初始化多通道发波定时器 TIM6（初始化后强行挂起，不准发波） */
    btim_tim6_int_init(1000 - 1, 200 - 1);
    HAL_TIM_Base_Stop_IT(&g_tim6_handle);

    /* 3. 初始化电流感测双采配置与 500Hz 频率触发器 TIM2 */
    I_Sense_ADC_Init();
    ADC_Trigger_TIM2_Init();

    printf("===========================================\r\n");
    printf("     Multi-Spacing CDMA TX System Ready     \r\n");
    printf("===========================================\r\n");
    
    delay_ms(1500); /* 挺顿1.5秒，给接收机板子上电和串口助手留足绝对的死等准备时间 */

    /* ========================================================================= */
    /*              终极时空同步神作：瞬间拉开双侧大闸                             */
    /* ========================================================================= */
    printf("System Ignition Active! Firing Sync Pulse...\r\n");
    
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);   /* 第一步：瞬间拉高 PD3，击穿接收机中断闸门 */
    delay_ms(10);                                         /* 维持 10ms 脉宽 */
    
    /* 以下四行物理寄存器启动指令必须紧挨，中间严禁穿插任何干扰代码，确保微秒对齐 */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET); /* 接收机在这 1 微秒瞬间检测到下降沿，在对岸开辟 V(t) 数据帧起点 */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buf, DMA_BUFFER_SIZE); /* 启动 DMA，开启连续双缓冲搬运 */
    HAL_TIM_Base_Start(&htim2);                           /* 采样发令枪 TIM2 开始以每 2ms 一次的节拍开火 */
    HAL_TIM_Base_Start_IT(&g_tim6_handle);                        /* 大功率控制主控 TIM6 彻底解除封印，伪随机多频波轰入地下！ */
    /* ========================================================================= */
    
    
    uint8_t has_printed = 0;
    
    while (1)
    {
        if(i_data_ready == 1 && has_printed == 0)
        {
            i_data_ready = 0;
            has_printed = 1; // 标记已打印，防止重复触发
            
            /* 1. 立刻紧急刹车，关闭发射和采集，冻结案发现场 */
            HAL_TIM_Base_Stop_IT(&g_tim6_handle); // 停发波形
            HAL_TIM_Base_Stop(&htim2);            // 停 ADC 触发
            HAL_ADC_Stop_DMA(&hadc1);             // 停 DMA
            
            /* 2. 强行刷新 Cache */
            SCB_InvalidateDCache_by_Addr((uint32_t *)adc_dma_buf, sizeof(adc_dma_buf));
            
            uint32_t start_idx = (current_half == 0) ? 0 : (I_SAMPLE_COUNT * 3);
            
            /* 3. 疯狂打印这 2 秒内（1000个采样点）的所有电流数据 */
            printf("--- Current Data Export Start ---\r\n");
            for(int k = 0; k < I_SAMPLE_COUNT; k++)
            {
                uint16_t raw_ch1 = adc_dma_buf[start_idx + k*3];
                uint16_t raw_ch2 = adc_dma_buf[start_idx + k*3 + 1];
                uint16_t raw_ch3 = adc_dma_buf[start_idx + k*3 + 2];
                
                float c1 = (((raw_ch1 / 65535.0f) * 3.3f) - 1.65f) / 50.0f / 40.2f * 1000.0f;
                float c2 = (((raw_ch2 / 65535.0f) * 3.3f) - 1.65f) / 50.0f / 40.2f * 1000.0f;
                float c3 = (((raw_ch3 / 65535.0f) * 3.3f) - 1.65f) / 50.0f / 40.2f * 1000.0f;
                
                // 以逗号分隔的 CSV 格式打印，方便 MATLAB 直接读取
                printf("%.3f,%.3f,%.3f\r\n", c1, c2, c3);
            }
            printf("--- Current Data Export End ---\r\n");
            
            LED0_TOGGLE(); // 亮灯提示数据导出完成
        }
        
        
//        if(i_data_ready == 1)
//        {
//            i_data_ready = 0;
//            
//            /* ========================================================= */
//            /* 底层指令：强行作废 CPU 的高速缓存，命令它去内存里拿最新数据 */
//            /* ========================================================= */
//            SCB_InvalidateDCache_by_Addr((uint32_t *)adc_dma_buf, sizeof(adc_dma_buf));
//            
//            /* 计算当前该读取的数据块的起始偏移量 */
//            uint32_t start_idx = (current_half == 0) ? 0 : (I_SAMPLE_COUNT * 3);
//            
//            /* 交替存储逻辑解密：
//               adc_dma_buf[start_idx + 0] 是 通道1 (PA0)
//               adc_dma_buf[start_idx + 1] 是 通道2 (PA1)
//               adc_dma_buf[start_idx + 2] 是 通道3 (PA5)
//               adc_dma_buf[start_idx + 3] 是 下一个 通道1...
//            */
//            uint16_t raw_ch1 = adc_dma_buf[start_idx];
//            uint16_t raw_ch2 = adc_dma_buf[start_idx + 1];
//            uint16_t raw_ch3 = adc_dma_buf[start_idx + 2];
//            
//            /* 电流换算公式 */
//            float current_ch1 = (((raw_ch1 / 65535.0f) * 3.3f) - 1.65f) / 50.0f / 40.2f * 1000.0f;
//            float current_ch2 = (((raw_ch2 / 65535.0f) * 3.3f) - 1.65f) / 50.0f / 40.2f * 1000.0f;
//            float current_ch3 = (((raw_ch3 / 65535.0f) * 3.3f) - 1.65f) / 50.0f / 40.2f * 1000.0f;
//            
//            if(current_half == 0)
//                printf("Buf [Ping] Full. CH1: %.3f mA | CH2: %.3f mA | CH3: %.3f mA\r\n", current_ch1, current_ch2, current_ch3);
//            else
//                printf("Buf [Pong] Full. CH1: %.3f mA | CH2: %.3f mA | CH3: %.3f mA\r\n", current_ch1, current_ch2, current_ch3);
//            LED0_TOGGLE(); 
//        }
    }
}
