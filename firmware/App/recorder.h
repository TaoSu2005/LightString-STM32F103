#ifndef __RECORDER_H
#define __RECORDER_H
#include "sys.h"

/*
 * recorder: 旋律录制与回放
 * 思路: 不录音频波形, 只录"音符事件"(时间差+事件类型+音符+力度),
 *       每条事件仅 5 字节, 64KB 一个扇区就能录一万多个音符.
 * 存储: 战舰V3 板载 W25Q128 SPI FLASH (SPI2), 从扇区 REC_BASE_ADDR 起.
 * 依赖: ALIENTEK w25qxx.c
 */

#define REC_MAGIC      0x4C53      /* "LS" */
#define REC_BASE_ADDR  0x00700000  /* 避开字库等已用区域, 7MB 处起 */
#define REC_MAX_EV     4000

typedef enum { EV_NOTE_ON = 1, EV_NOTE_OFF = 2 } rec_evtype_t;

#pragma pack(1)
typedef struct {
    u16 dt_ms;     /* 与上一事件的时间差, 最大 65.5s */
    u8  type;
    u8  note;
    u8  vel;
} rec_event_t;

typedef struct {
    u16 magic;
    u16 count;
    u8  program;   /* 录制时音色 */
    u8  scale;
    u8  base_note;
    u8  rsv;
} rec_header_t;
#pragma pack()

void Rec_Init(void);
void Rec_Start(u8 program, u8 scale, u8 base_note);
void Rec_Event(u8 type, u8 note, u8 vel);    /* 录制中调用 */
u16  Rec_Stop(void);                         /* 写入FLASH, 返回事件数 */
u8   Rec_Recording(void);

/* 回放: 每个主循环 tick 调用, 返回0=回放中, 1=结束/无录音 */
u8   Rec_PlayStart(void);
u8   Rec_PlayTick(void);
void Rec_PlayStop(void);

#endif
