#ifndef __SYNTH_VS1053_H
#define __SYNTH_VS1053_H
#include "sys.h"

/*
 * synth_vs1053: 基于 VS1053b 实时 MIDI (RT-MIDI) 模式的合成器
 * 硬件: 战舰V3 板载 VS1053, SPI1 (PA5/PA6/PA7),
 *       XCS=PF7, XDCS=PF6, DREQ=PC13, RST=PE6, 板载喇叭/耳机输出
 * 原理: 复位后通过 SCI 写入 VLSI 官方 rtmidistart 补丁,
 *       芯片进入 GM 波表合成模式; 之后经 SDI 发送标准 MIDI 字节流
 *       (每个 MIDI 字节前补 0x00), 即可零负载播放 128 种乐器音色.
 * 依赖: ALIENTEK vs10xx.c (开发指南第48章) 提供底层 SPI 读写.
 */

void Synth_Init(void);                       /* 复位+加载RT-MIDI补丁 */
void Synth_NoteOn(u8 ch, u8 note, u8 vel);
void Synth_NoteOff(u8 ch, u8 note);
void Synth_AllNotesOff(u8 ch);
void Synth_Program(u8 ch, u8 prog);          /* GM 音色 0~127 */
void Synth_MasterVol(u8 vol);                /* 0~254, 越小越响(VS1053衰减) */
void Synth_PitchBend(u8 ch, u16 bend14);     /* 14bit, 0x2000=不弯音 */

/* 预置音色表 */
typedef struct { u8 prog; const char *name; } gm_preset_t;
u8          Synth_PresetCount(void);
gm_preset_t Synth_Preset(u8 idx);

#endif
