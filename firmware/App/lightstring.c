#include "lightstring.h"
#include "light_sense.h"
#include "scale_quantizer.h"
#include "synth_vs1053.h"
#include "recorder.h"
#include "ui_lcd.h"
#include "timebase.h"
#include "delay.h"
#include "key.h"
#include "led.h"
#include "beep.h"
#include "remote.h"
#include "usart.h"
#include <stdio.h>

static ls_mode_t mode = MODE_PLAY;
static u8  preset_idx = 0;
static u8  vol = 0x20;
static u8  metro_on = 0;
static u16 metro_bpm = 90;
static u32 metro_next = 0;
static u32 t_sample = 0, t_ui = 0;
static u16 ui_norm = 0;
static u8  prev_note = 0xFF;     /* 当前发声音符, 0xFF=无 */

/* 红外遥控键值 (战舰V3 配套遥控器, NEC 协议, 开发指南第33章 remote.c 例程) */
#define IR_POWER  0xA2
#define IR_UP     0x62      /* 升八度 */
#define IR_DOWN   0xA8      /* 降八度 */
#define IR_VOLADD 0x90      /* 音量+ */
#define IR_VOLDEC 0xE0      /* 音量- */
#define IR_PLAY   0x02      /* 回放/停止 */
/* 数字键 0~9 的键码, 下标即数字 */
static const u8 IR_NUMS[10] = {0x42, 0x68, 0x98, 0xB0, 0x30,
                               0x18, 0x7A, 0x10, 0x38, 0x5A};

/*
 * 统一止音: 关掉当前音符并复位量化器状态.
 * record=1 时把 NoteOff 一并写入录音事件流, 保证 On/Off 严格配对.
 * 所有模式切换路径(切音阶/开始回放/定标/停录)都必须先经过这里.
 */
static void stop_current_note(u8 record)
{
    if (prev_note != 0xFF)
    {
        Synth_NoteOff(0, prev_note);
        if (record) Rec_Event(EV_NOTE_OFF, prev_note, 0);
        prev_note = 0xFF;
    }
    Quant_Init();
    LED0 = 1;
    UI_ShowNote(0, 0, 0);
}

static void apply_preset(u8 idx)
{
    gm_preset_t p = Synth_Preset(idx);
    Synth_Program(0, p.prog);
    UI_SetInstrument(p.name);
    printf("[LS] program=%d %s\r\n", p.prog, p.name);
}

static void enter_mode(ls_mode_t m)
{
    mode = m;
    switch (m)
    {
    case MODE_PLAY:   UI_SetMode("PLAY  ");  break;
    case MODE_REC:    UI_SetMode("REC ! ");  break;
    case MODE_REPLAY: UI_SetMode("REPLAY");  break;
    case MODE_CAL:    UI_SetMode("CAL...");  break;
    }
}

static void do_calibrate(void)
{
    enter_mode(MODE_CAL);
    stop_current_note(0);
    UI_Message("Wave hand down & up, 3s...");
    if (LightSense_Calibrate() == 0) UI_Message("Calibrated OK");
    else                             UI_Message("Cal FAIL: too dark");
    enter_mode(MODE_PLAY);
}

static void handle_keys(void)
{
    u8 k = KEY_Scan(0);
    if (k == KEY0_PRES)              /* 切音色 */
    {
        preset_idx = (preset_idx + 1) % Synth_PresetCount();
        apply_preset(preset_idx);
    }
    else if (k == KEY1_PRES)         /* 切音阶: 先配对止音再切 */
    {
        scale_t s = (scale_t)((Quant_GetScale() + 1) % SCALE_COUNT);
        stop_current_note(mode == MODE_REC);
        Quant_SetScale(s);
        UI_SetScale(Quant_ScaleName(s));
    }
    else if (k == KEY2_PRES)         /* 录制开始/停止 */
    {
        if (mode == MODE_PLAY)
        {
            Rec_Start(Synth_Preset(preset_idx).prog,
                      Quant_GetScale(), Quant_GetBaseNote());
            enter_mode(MODE_REC);
            UI_Message("Recording...");
        }
        else if (mode == MODE_REC)
        {
            u16 n;
            char m[24];
            stop_current_note(1);    /* 末音 NoteOff 入流后再存盘 */
            n = Rec_Stop();
            sprintf(m, "Saved %d events", n);
            enter_mode(MODE_PLAY);
            UI_Message(m);
        }
    }
    else if (k == WKUP_PRES)
    {
        if (mode == MODE_PLAY) do_calibrate();   /* 录制/回放中禁止定标 */
        else UI_Message("Stop REC/REPLAY first");
    }
}

static void handle_remote(void)
{
    u8 i, rmt = Remote_Scan();
    if (rmt == 0) return;
    if (rmt == IR_UP)        { Quant_OctaveShift(+1); UI_Message("Octave +"); }
    else if (rmt == IR_DOWN) { Quant_OctaveShift(-1); UI_Message("Octave -"); }
    else if (rmt == IR_VOLADD && vol > 0x04) { vol -= 4; Synth_MasterVol(vol); }
    else if (rmt == IR_VOLDEC && vol < 0xF8) { vol += 4; Synth_MasterVol(vol); }
    else if (rmt == IR_PLAY)
    {
        if (mode == MODE_REPLAY) { Rec_PlayStop(); enter_mode(MODE_PLAY); }
        else if (mode == MODE_PLAY)
        {
            stop_current_note(0);    /* 防止演奏中的长音叠在回放上 */
            if (Rec_PlayStart() == 0) { enter_mode(MODE_REPLAY); UI_Message("Replaying..."); }
            else UI_Message("No recording");
        }
    }
    else if (rmt == IR_POWER) { metro_on = !metro_on; UI_Message(metro_on ? "Metronome ON" : "Metronome OFF"); }
    else
        for (i = 0; i < 10; i++)
            if (rmt == IR_NUMS[i]) { preset_idx = i; apply_preset(i); break; }
}

/* 节拍器: 有源蜂鸣器只能开/关, 用 20ms 短促"嗒"声打拍, LED1 同步闪 */
static void metronome_tick(void)
{
    u32 now = millis();
    static u8 click = 0;
    if (!metro_on) { BEEP = 0; return; }
    if ((s32)(now - metro_next) >= 0)
    {
        metro_next = now + 60000UL / metro_bpm;
        BEEP = 1; LED1 = 0; click = 1;
    }
    else if (click && (s32)(now - (metro_next - 60000UL / metro_bpm + 20)) >= 0)
    {
        BEEP = 0; LED1 = 1; click = 0;
    }
}

void LightString_Init(void)
{
    Timebase_Init();                 /* TIM7 毫秒时基 (SysTick 已被 delay.c 接管) */
    LightSense_Init();
    Quant_Init();
    Synth_Init();
    Rec_Init();
    Remote_Init();
    UI_Init();
    enter_mode(MODE_PLAY);
    apply_preset(0);
    UI_SetScale(Quant_ScaleName(Quant_GetScale()));
    UI_Message("WKUP: calibrate first!");
    printf("[LS] LightString ready\r\n");
}

void LightString_Loop(void)
{
    u32 now = millis();

    /* 100Hz: 采样-量化-发声 主链路 */
    if (now - t_sample >= LS_SAMPLE_MS)
    {
        t_sample = now;
        LightSense_Tick();
        ui_norm = LightSense_Norm();

        if (mode == MODE_PLAY || mode == MODE_REC)
        {
            quant_out_t q = Quant_Tick(ui_norm,
                                       LightSense_HandPresent(),
                                       LightSense_Rate());
            if (q.changed)
            {
                if (prev_note != 0xFF)
                {
                    Synth_NoteOff(0, prev_note);
                    if (mode == MODE_REC) Rec_Event(EV_NOTE_OFF, prev_note, 0);
                }
                if (q.active)
                {
                    Synth_NoteOn(0, q.note, q.velocity);
                    if (mode == MODE_REC) Rec_Event(EV_NOTE_ON, q.note, q.velocity);
                    LED0 = 0;
                    prev_note = q.note;
                    printf("[LS] on  n=%d v=%d L=%d\r\n", q.note, q.velocity, ui_norm);
                }
                else { LED0 = 1; prev_note = 0xFF; printf("[LS] off\r\n"); }
                UI_ShowNote(q.active, q.note, q.velocity);
            }
        }
    }

    if (mode == MODE_REPLAY)
        if (Rec_PlayTick()) { enter_mode(MODE_PLAY); UI_Message("Replay done"); }

    /* 20Hz: 曲线刷新 (降低 LCD 带宽占用) */
    if (now - t_ui >= 50)
    {
        t_ui = now;
        UI_PushCurve(ui_norm);
    }

    handle_keys();
    handle_remote();
    metronome_tick();
}
