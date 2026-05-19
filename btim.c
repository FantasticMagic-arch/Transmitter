
#include "./BSP/TIMER/btim.h"

TIM_HandleTypeDef g_tim6_handle;

/* 7位伪随机序列 (PRBS) */
const uint8_t PRBS_SEQ[7] = {1, 1, 1, 0, 0, 1, 0}; 
#define SEQ_LENGTH 7
#define DEAD_TIME_MS 2  /* 强电死区时间 2ms */

/* 三通道独立时间轴与序列索引 */
volatile uint16_t t1 = 0, t2 = 0, t3 = 0;
volatile uint8_t i1 = 0, i2 = 0, i3 = 0;

/* 三通道 GPIO 与定时器联合初始化 */
void btim_tim6_int_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* 1. 开启时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* 2. 配置 CH1 (PC6, PC7) 和 CH3 (PC4, PC5) - 均在 GPIOC */
    gpio_init_struct.Pin = TX_CH1_RPWM_PIN | TX_CH1_LPWM_PIN | 
                           TX_CH3_RPWM_PIN | TX_CH3_LPWM_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio_init_struct);

    /* 3. 配置 CH2 (PB0, PB1) - 在 GPIOB */
    gpio_init_struct.Pin = TX_CH2_RPWM_PIN | TX_CH2_LPWM_PIN;
    HAL_GPIO_Init(TX_CH2_PORT, &gpio_init_struct);

    /* 初始状态强制拉低所有引脚防直通 */
    HAL_GPIO_WritePin(TX_CH1_PORT, TX_CH1_RPWM_PIN | TX_CH1_LPWM_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TX_CH2_PORT, TX_CH2_RPWM_PIN | TX_CH2_LPWM_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TX_CH3_PORT, TX_CH3_RPWM_PIN | TX_CH3_LPWM_PIN, GPIO_PIN_RESET);

    /* 4. 定时器 TIM6 基础配置 */
    g_tim6_handle.Instance = TIM6;
    g_tim6_handle.Init.Prescaler = psc;
    g_tim6_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tim6_handle.Init.Period = arr;
    HAL_TIM_Base_Init(&g_tim6_handle);
    
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 3);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    HAL_TIM_Base_Start_IT(&g_tim6_handle);
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tim6_handle);
}

/* 1ms 中断服务函数：并发执行三通道 PRBS 输出 (智能死区升级版) */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        uint8_t curr_bit, prev_bit;

        /* ================= 极距 1 (AB1): 码元 100ms ================= */
        t1++;
        curr_bit = PRBS_SEQ[i1];
        prev_bit = (i1 == 0) ? PRBS_SEQ[SEQ_LENGTH - 1] : PRBS_SEQ[i1 - 1]; // 获取上一个码元状态

        /* 仅当极性发生翻转时，才在周期的前 2ms 插入死区 */
        if ((curr_bit != prev_bit) && (t1 <= DEAD_TIME_MS)) {
            HAL_GPIO_WritePin(TX_CH1_PORT, TX_CH1_RPWM_PIN | TX_CH1_LPWM_PIN, GPIO_PIN_RESET);
        } else {
            if (curr_bit == 1) {
                HAL_GPIO_WritePin(TX_CH1_PORT, TX_CH1_RPWM_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(TX_CH1_PORT, TX_CH1_LPWM_PIN, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(TX_CH1_PORT, TX_CH1_RPWM_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(TX_CH1_PORT, TX_CH1_LPWM_PIN, GPIO_PIN_SET);
            }
        }
        if (t1 >= 100) { t1 = 0; i1++; if (i1 >= SEQ_LENGTH) i1 = 0; }

        /* ================= 极距 2 (AB2): 码元 130ms ================= */
        t2++;
        curr_bit = PRBS_SEQ[i2];
        prev_bit = (i2 == 0) ? PRBS_SEQ[SEQ_LENGTH - 1] : PRBS_SEQ[i2 - 1];

        if ((curr_bit != prev_bit) && (t2 <= DEAD_TIME_MS)) {
            HAL_GPIO_WritePin(TX_CH2_PORT, TX_CH2_RPWM_PIN | TX_CH2_LPWM_PIN, GPIO_PIN_RESET);
        } else {
            if (curr_bit == 1) {
                HAL_GPIO_WritePin(TX_CH2_PORT, TX_CH2_RPWM_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(TX_CH2_PORT, TX_CH2_LPWM_PIN, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(TX_CH2_PORT, TX_CH2_RPWM_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(TX_CH2_PORT, TX_CH2_LPWM_PIN, GPIO_PIN_SET);
            }
        }
        if (t2 >= 130) { t2 = 0; i2++; if (i2 >= SEQ_LENGTH) i2 = 0; }

        /* ================= 极距 3 (AB3): 码元 170ms ================= */
        t3++;
        curr_bit = PRBS_SEQ[i3];
        prev_bit = (i3 == 0) ? PRBS_SEQ[SEQ_LENGTH - 1] : PRBS_SEQ[i3 - 1];

        if ((curr_bit != prev_bit) && (t3 <= DEAD_TIME_MS)) {
            HAL_GPIO_WritePin(TX_CH3_PORT, TX_CH3_RPWM_PIN | TX_CH3_LPWM_PIN, GPIO_PIN_RESET);
        } else {
            if (curr_bit == 1) {
                HAL_GPIO_WritePin(TX_CH3_PORT, TX_CH3_RPWM_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(TX_CH3_PORT, TX_CH3_LPWM_PIN, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(TX_CH3_PORT, TX_CH3_RPWM_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(TX_CH3_PORT, TX_CH3_LPWM_PIN, GPIO_PIN_SET);
            }
        }
        if (t3 >= 170) { t3 = 0; i3++; if (i3 >= SEQ_LENGTH) i3 = 0; }
    }
}
