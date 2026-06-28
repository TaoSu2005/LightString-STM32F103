#ifndef __LIGHTSTRING_H
#define __LIGHTSTRING_H
#include "sys.h"

/*
 * lightstring: 应用主状态机
 * 模式: 演奏 PLAY -> (KEY2) 录制 REC -> (KEY2) 停止保存
 *       (IR PLAY键 / KEY2长按) 回放 REPLAY
 *       (WK_UP) 挥手定标 CAL
 */

typedef enum { MODE_PLAY = 0, MODE_REC, MODE_REPLAY, MODE_CAL } ls_mode_t;

void LightString_Init(void);
void LightString_Loop(void);     /* 放入 main 的 while(1) */

#endif
