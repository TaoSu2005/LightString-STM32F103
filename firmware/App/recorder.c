#include "recorder.h"
#include "w25qxx.h"
#include "synth_vs1053.h"
#include "timebase.h"

static rec_event_t ev_buf[REC_MAX_EV];   /* 录制缓冲: 5B*4000=20KB, F103ZE 64KB SRAM 可容纳 */
static u16 ev_cnt = 0;
static u8  recording = 0;
static u32 last_ms = 0;
static rec_header_t hdr;

/* 回放状态 */
static u8  playing = 0;
static u16 play_idx = 0;
static u32 play_next_ms = 0;

void Rec_Init(void)
{
    W25QXX_Init();
    recording = 0; playing = 0;
}

void Rec_Start(u8 program, u8 scale, u8 base_note)
{
    ev_cnt = 0;
    recording = 1;
    last_ms = millis();
    hdr.magic = REC_MAGIC;
    hdr.program = program;
    hdr.scale = scale;
    hdr.base_note = base_note;
}

void Rec_Event(u8 type, u8 note, u8 vel)
{
    u32 now;
    u32 dt;
    if (!recording || ev_cnt >= REC_MAX_EV) return;
    now = millis();
    dt = now - last_ms;
    if (dt > 65535) dt = 65535;
    last_ms = now;
    ev_buf[ev_cnt].dt_ms = (u16)dt;
    ev_buf[ev_cnt].type = type;
    ev_buf[ev_cnt].note = note;
    ev_buf[ev_cnt].vel = vel;
    ev_cnt++;
}

u16 Rec_Stop(void)
{
    if (!recording) return 0;
    recording = 0;
    hdr.count = ev_cnt;
    /* W25QXX_Write 自带读-改-擦-写, 直接顺序写头+事件 */
    W25QXX_Write((u8 *)&hdr, REC_BASE_ADDR, sizeof(hdr));
    W25QXX_Write((u8 *)ev_buf, REC_BASE_ADDR + sizeof(hdr),
                 ev_cnt * sizeof(rec_event_t));
    return ev_cnt;
}

u8 Rec_Recording(void) { return recording; }

u8 Rec_PlayStart(void)
{
    rec_header_t h;
    W25QXX_Read((u8 *)&h, REC_BASE_ADDR, sizeof(h));
    if (h.magic != REC_MAGIC || h.count == 0 || h.count > REC_MAX_EV) return 1;
    W25QXX_Read((u8 *)ev_buf, REC_BASE_ADDR + sizeof(h),
                h.count * sizeof(rec_event_t));
    ev_cnt = h.count;
    Synth_Program(0, h.program);
    play_idx = 0;
    playing = 1;
    play_next_ms = millis() + ev_buf[0].dt_ms;
    return 0;
}

u8 Rec_PlayTick(void)
{
    if (!playing) return 1;
    while (playing && (s32)(millis() - play_next_ms) >= 0)
    {
        rec_event_t *e = &ev_buf[play_idx];
        if (e->type == EV_NOTE_ON)  Synth_NoteOn(0, e->note, e->vel);
        if (e->type == EV_NOTE_OFF) Synth_NoteOff(0, e->note);
        play_idx++;
        if (play_idx >= ev_cnt) { Rec_PlayStop(); return 1; }
        play_next_ms += ev_buf[play_idx].dt_ms;
    }
    return 0;
}

void Rec_PlayStop(void)
{
    playing = 0;
    Synth_AllNotesOff(0);
}
