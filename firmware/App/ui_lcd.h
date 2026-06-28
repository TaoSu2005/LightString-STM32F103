#ifndef __UI_LCD_H
#define __UI_LCD_H
#include "sys.h"
#include "scale_quantizer.h"

/*
 * ui_lcd: TFTLCD 用户界面 (FSMC, 240x320 竖屏)
 * 布局:  0~ 79  标题区 + 模式/音色/音阶状态栏
 *       80~159  大字音符显示区 (音名 + 简谱 + 力度条)
 *      160~239  光强历史曲线区 (滚动波形)
 *      240~319  操作提示区
 * 依赖: ALIENTEK lcd.c (开发指南第18章)
 */

void UI_Init(void);
void UI_SetMode(const char *mode);              /* PLAY/REC/REPLAY/CAL */
void UI_SetInstrument(const char *name);
void UI_SetScale(const char *name);
void UI_ShowNote(u8 active, u8 note, u8 vel);   /* 大字音符 */
void UI_PushCurve(u16 norm);                    /* 推入一个光强采样点 */
void UI_Message(const char *msg);               /* 底部提示行 */

#endif
