/*
 * 流光琴 LightString —— 基于 STM32F103ZET6(战舰V3) 的非接触式光感电子乐器
 * Target: ALIENTEK WarShip STM32F103ZET6 V3 development board
 *
 * 工程组织 (ALIENTEK SYSTEM 框架风格, 与《STM32F1开发指南(HAL库版)》一致):
 *   SYSTEM/  delay.c sys.c usart.c            -- 开发指南第5章
 *   HALLIB/  STM32F1xx HAL 驱动
 *   HARDWARE/ led.c key.c beep.c lcd.c lsens.c adc3.c
 *             vs10xx.c w25qxx.c spi.c remote.c -- 取自配套例程
 *   APP/     light_sense.c scale_quantizer.c synth_vs1053.c
 *            recorder.c ui_lcd.c lightstring.c -- 本设计新增
 */
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "beep.h"
#include "lightstring.h"

int main(void)
{
    HAL_Init();
    Stm32_Clock_Init(RCC_PLL_MUL9);   /* HSE 8MHz x9 = 72MHz */
    delay_init(72);
    uart_init(115200);                /* USART1 调试/事件日志 */
    LED_Init();
    KEY_Init();
    BEEP_Init();

    LightString_Init();               /* 光敏+ADC3 / VS1053 / LCD / 红外 / FLASH */

    while (1)
    {
        LightString_Loop();
    }
}
