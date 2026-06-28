#include "scale_quantizer.h"

/* 各音阶在一个八度内的半音偏移表 */
static const u8 SC_MAJOR[]  = {0, 2, 4, 5, 7, 9, 11};
static const u8 SC_PENTA[]  = {0, 2, 4, 7, 9};
static const u8 SC_MINOR[]  = {0, 2, 3, 5, 7, 8, 10};
static const u8 SC_BLUES[]  = {0, 3, 5, 6, 7, 10};
static const u8 SC_CHROMA[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

typedef struct { const u8 *ofs; u8 n; const char *name; } scale_def_t;
static const scale_def_t SCALES[SCALE_COUNT] = {
    {SC_MAJOR,  7,  "Major "},
    {SC_PENTA,  5,  "Penta "},
    {SC_MINOR,  7,  "Minor "},
    {SC_BLUES,  6,  "Blues "},
    {SC_CHROMA, 12, "Chroma"},
};

#define SPAN_OCTAVES   2        /* 演奏音域: 2个八度 */
#define STABLE_N       3        /* 台阶需连续3个tick(30ms)稳定才触发 */
#define HYST_PERMIL    18       /* 台阶边界迟滞带(千分比) */

static scale_t cur_scale = SCALE_PENTA;   /* 默认五声音阶: 零错音 */
static u8 base_note = 60;                 /* C4 */

static u8  cur_active = 0;
static u8  cur_note = 0;
static s16 cur_step = -1;       /* 当前台阶号 */
static s16 cand_step = -1;      /* 候选台阶号 */
static u8  cand_cnt = 0;

void    Quant_Init(void) { cur_active = 0; cur_step = -1; cand_step = -1; cand_cnt = 0; }
void    Quant_SetScale(scale_t s) { if (s < SCALE_COUNT) { cur_scale = s; Quant_Init(); } }
scale_t Quant_GetScale(void) { return cur_scale; }
const char *Quant_ScaleName(scale_t s) { return SCALES[s].name; }
void    Quant_SetBaseNote(u8 n) { base_note = n; }
u8      Quant_GetBaseNote(void) { return base_note; }

void Quant_OctaveShift(s8 dir)
{
    s16 n = (s16)base_note + 12 * dir;
    if (n >= 24 && n <= 96) base_note = (u8)n;
}

/* 台阶号 -> MIDI 音符号 */
static u8 step_to_note(s16 step)
{
    const scale_def_t *sd = &SCALES[cur_scale];
    return base_note + 12 * (step / sd->n) + sd->ofs[step % sd->n];
}

/*
 * 带迟滞的台阶判定:
 * 总台阶数 N = 音阶音数 * 八度数; 名义台阶宽度 w = 1000/N.
 * 仅当 norm 离开当前台阶中心超过 w/2 + HYST 才换台阶,
 * 防止手停在两个音边界处来回颤动.
 */
quant_out_t Quant_Tick(u16 norm, u8 hand, s16 rate)
{
    quant_out_t out = {0, 0, 0, 0};
    const scale_def_t *sd = &SCALES[cur_scale];
    u8  total = sd->n * SPAN_OCTAVES;
    u16 w = 1000 / total;
    s16 step;

    out.active = cur_active;
    out.note = cur_note;

    if (!hand)                       /* 手移开 -> 止音 */
    {
        if (cur_active) { out.changed = 1; out.active = 0; }
        cur_active = 0; cur_step = -1; cand_step = -1; cand_cnt = 0;
        return out;
    }

    step = norm / w;
    if (step >= total) step = total - 1;

    if (cur_active && step != cur_step)
    {
        /* 迟滞: 必须越过当前台阶边界外的死区才算换音 */
        s32 center = (s32)cur_step * w + w / 2;
        s32 d = (s32)norm - center;
        if (d < 0) d = -d;
        if (d <= (s32)w / 2 + HYST_PERMIL) step = cur_step;
    }

    if (step == cur_step && cur_active) { cand_step = -1; cand_cnt = 0; return out; }

    /* 稳定计数消抖: 新台阶需连续 STABLE_N 个 tick 一致 */
    if (step == cand_step) cand_cnt++;
    else { cand_step = step; cand_cnt = 1; }

    if (cand_cnt >= STABLE_N)
    {
        u8 vel;
        s16 r = (rate < 0) ? -rate : rate;
        /* 力度映射: 挥得越快力度越大, 80~127 */
        vel = 80 + (r > 4700 ? 47 : r / 100);

        cur_step = cand_step;
        cur_note = step_to_note(cur_step);
        cur_active = 1;
        cand_step = -1; cand_cnt = 0;

        out.active = 1; out.note = cur_note; out.velocity = vel; out.changed = 1;
    }
    return out;
}

void Quant_NoteName(u8 note, char *buf4, u8 *jianpu)
{
    static const char *NAMES[12] =
        {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static const u8 JP[12] = {1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6, 7};
    u8 pc = note % 12;
    u8 oct = note / 12 - 1;
    u8 i = 0;
    const char *p = NAMES[pc];
    while (*p) buf4[i++] = *p++;
    buf4[i++] = '0' + oct;
    buf4[i] = 0;
    if (jianpu) *jianpu = JP[pc];
}
