#ifndef __SCALE_QUANTIZER_H
#define __SCALE_QUANTIZER_H
#include "sys.h"

/*
 * scale_quantizer: 连续光强 -> 离散音符 的"音阶量化器"
 * 核心思想: 把 0~1000 的归一化光强映射到所选音阶的台阶序号,
 * 再换算为 MIDI 音符号; 配合稳定计数消抖, 保证"怎么挥都在调上".
 */

typedef enum {
    SCALE_MAJOR = 0,   /* 自然大调 do re mi fa sol la si */
    SCALE_PENTA,       /* 五声音阶 宫商角徵羽 (黑键音阶, 零错音) */
    SCALE_MINOR,       /* 自然小调 */
    SCALE_BLUES,       /* 布鲁斯 */
    SCALE_CHROMA,      /* 半音阶 (theremin 风格) */
    SCALE_COUNT
} scale_t;

typedef struct {
    u8  active;        /* 当前是否有音符在发声 */
    u8  note;          /* 当前 MIDI 音符号 */
    u8  velocity;      /* 触发力度 1~127 */
    u8  changed;       /* 本次 tick 是否发生 note 事件 */
} quant_out_t;

void        Quant_Init(void);
void        Quant_SetScale(scale_t s);
scale_t     Quant_GetScale(void);
const char *Quant_ScaleName(scale_t s);
void        Quant_SetBaseNote(u8 midi_note);   /* 音域下限, 默认 C4=60 */
u8          Quant_GetBaseNote(void);
void        Quant_OctaveShift(s8 dir);         /* 整体移高/低八度 */

/* 每个采样 tick 调用: 输入归一化光强/手存在/变化率, 输出音符事件 */
quant_out_t Quant_Tick(u16 norm, u8 hand, s16 rate);

/* 把 MIDI 音符号转成名字, 如 60 -> "C4", 同时给出简谱数字 */
void        Quant_NoteName(u8 note, char *buf4, u8 *jianpu);

#endif
