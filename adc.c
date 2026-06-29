
#include "./BSP/ADC/adc.h"

/* 强制 32 字节内存对齐，防止 D-Cache 刷新时误伤其他变量 */
#if defined ( __ICCARM__ )
#pragma data_alignment=32
uint16_t adc_dma_buf[DMA_BUFFER_SIZE];
#elif defined ( __CC_ARM ) || defined ( __GNUC__ )
__attribute__((aligned(32))) uint16_t adc_dma_buf[DMA_BUFFER_SIZE];
#endif

volatile uint8_t i_data_ready = 0;
volatile uint8_t current_half = 0;

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim2;

/* ADC1 硬件通道与 DMA 联动初始化函数 */
void I_Sense_ADC_Init(void)
{
    ADC_MultiModeTypeDef multimedia = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /* 1. 使能时钟与配置输入引脚 */
    __HAL_RCC_GPIOA_CLK_ENABLE();   /* 使能 GPIOA 时钟 */
    __HAL_RCC_ADC12_CLK_ENABLE();   /* 使能 ADC12 内核时钟 */
    __HAL_RCC_DMA1_CLK_ENABLE();    /* 使能 DMA1 时钟 */

    /* PA0(通道16)、PA1(通道17)、PA5(通道19) 配置为模拟输入 */
    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5;
    gpio_init_struct.Mode = GPIO_MODE_ANALOG;
    gpio_init_struct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    /* 2. 配置 DMA1 Stream0 将 ADC1 的 Regular 转换结果搬运至内存 */
    hdma_adc1.Instance = DMA1_Stream0;
    hdma_adc1.Init.Request = DMA_REQUEST_ADC1;               /* 请求源设为 ADC1 */
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;         /* 外设到内存 */
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;             /* 外设地址固定 */
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;                 /* 内存地址自增 */
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; /* 外设16位 */
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;   /* 内存16位 */
    hdma_adc1.Init.Mode = DMA_CIRCULAR;                      /* 循环扫描搬运模式 */
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;             /* 高优先级 */
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    /* 关联 DMA 句柄 */
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* 3. 初始化 ADC1 核心引擎参数 */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;    /* 时钟4分频 */
    hadc1.Init.Resolution = ADC_RESOLUTION_16B;              /* H7最高16位分辨率 */
    hadc1.Init.ScanConvMode = ENABLE;                        /* 开启顺序扫描 */
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;              /* 序列转换完成产生EOC */
    hadc1.Init.ContinuousConvMode = DISABLE;                 /* 关闭连续模式，严格靠定时器触发 */
    hadc1.Init.NbrOfConversion = 3;                          /* 转换序列长度为 3 */
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T2_TRGO;  /* 绑定 TIM2 触发源 */
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING; /* 上升沿触发 */
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* 循环DMA管理 */
    HAL_ADC_Init(&hadc1);

    /* 4. 配置独立多模式管理器（独立运作模式） */
    multimedia.Mode = ADC_MODE_INDEPENDENT;
    HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimedia);

    /* 5. 按照顺序配置 3 个物理通道的采样 Rank */
    /* 顺序 1: 采集通道 16 (PA0) */
    sConfig.Channel = ADC_CHANNEL_16;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_32CYCLES_5;       /* 保证高输入阻抗下采透 */
    sConfig.SingleDiff = ADC_SINGLE_ENDED;                   /* 单端输入模式 */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 顺序 2: 采集通道 17 (PA1) */
    sConfig.Channel = ADC_CHANNEL_17;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 顺序 3: 采集通道 19 (PA5) */
    sConfig.Channel = ADC_CHANNEL_19;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 6. 配置 DMA 中断，以便在 1000 次采满时切换 Ping-Pong 缓冲区 */
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 3, 0);           /* 优先级设置为 3 */
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}







//  利用定时器以500Hz(周期2ms)产生TRGO信号，触发ADC  //


/* TIM2 定时器配置函数：精准输出 500Hz 触发节拍 */
void ADC_Trigger_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();    /* 开启 TIM2 时钟，主频通常为 200MHz */

    /* 200MHz / (200 * 2000) = 500Hz (周期 2.0 毫秒) */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 200 - 1;                          /* 预分频 200 */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 2000 - 1;                            /* 重装载计数 2000 */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);

    /* 核心关键点：将定时器的更新事件（Update Event）映射为 TRGO 触发物理输出 */
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
}

/* DMA 搬运完成中断服务函数入口 */
void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

/* DMA 传输过半回调：代表前 3000 个数据（Ping区）已经填满 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        current_half = 0; 
        i_data_ready = 1;
    }
}

/* DMA 传输完全完成回调：代表后 3000 个数据（Pong区）已经填满，随后DMA会自动绕回头部 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        current_half = 1; 
        i_data_ready = 1;
    }
}

