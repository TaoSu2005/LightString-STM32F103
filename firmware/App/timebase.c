#include "timebase.h"

static TIM_HandleTypeDef TIM7_Handler;
static volatile u32 sys_ms = 0;

/* TIM7: APB1 定时器时钟 72MHz, 7200 分频 -> 10kHz, 计 10 -> 1ms 更新中断 */
void Timebase_Init(void)
{
    __HAL_RCC_TIM7_CLK_ENABLE();
    TIM7_Handler.Instance = TIM7;
    TIM7_Handler.Init.Prescaler = 7200 - 1;
    TIM7_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIM7_Handler.Init.Period = 10 - 1;
    TIM7_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&TIM7_Handler);
    HAL_NVIC_SetPriority(TIM7_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
    HAL_TIM_Base_Start_IT(&TIM7_Handler);
}

/* 直接操作寄存器, 不走 HAL 回调, 避免与工程内其他 PeriodElapsed 回调冲突 */
void TIM7_IRQHandler(void)
{
    if (TIM7->SR & TIM_SR_UIF)
    {
        TIM7->SR = ~TIM_SR_UIF;
        sys_ms++;
    }
}

u32 millis(void) { return sys_ms; }
