#ifndef __TIMEBASE_H
#define __TIMEBASE_H
#include "sys.h"

/*
 * timebase: 独立毫秒时基 (TIM7 基本定时器, 1kHz 中断)
 * 背景: ALIENTEK 非OS版 delay.c 会接管 SysTick(手动装载/轮询/停表),
 *       导致 HAL 的 uwTick 不再走时, HAL_GetTick() 冻结.
 *       本工程所有调度(采样节拍/录放时间戳/节拍器)改用 millis().
 */

void Timebase_Init(void);   /* 在 delay_init 之后调用 */
u32  millis(void);

#endif
