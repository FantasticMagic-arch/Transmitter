

#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"

#define I_SAMPLE_COUNT  1000
/* 新逻辑：3个通道 * 1000个点 * 2(双缓冲一半) = 6000 个长度的单一大数组 */
#define DMA_BUFFER_SIZE (I_SAMPLE_COUNT * 3 * 2) 

/* 外部变量声明 */
extern uint16_t adc_dma_buf[DMA_BUFFER_SIZE];
extern volatile uint8_t i_data_ready;     // 1: 代表有一半数据采满
extern volatile uint8_t current_half;     // 0: 前一半(Ping)就绪, 1: 后一半(Pong)就绪

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim2;

void I_Sense_ADC_Init(void);
void ADC_Trigger_TIM2_Init(void);

#endif

