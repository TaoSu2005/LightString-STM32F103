#include "synth_vs1053.h"
#include "vs10xx.h"
#include "delay.h"

/*
 * VLSI 官方 vs1053b-rtmidistart 补丁 (压缩插件格式).
 * 来源: VLSI Solution, VS1053b Patches package (rtmidistart.plg).
 * 作用: 启动芯片内置的实时 MIDI / GM1 波表合成器.
 */
static const u16 rtmidi_plugin[28] = {
    0x0007, 0x0001, 0x8050, 0x0006, 0x0014, 0x0030, 0x0715, 0xb080,
    0x3400, 0x0007, 0x9255, 0x3d00, 0x0024, 0x0030, 0x0295, 0x6890,
    0x3400, 0x0030, 0x0495, 0x3d00, 0x0024, 0x2908, 0x4d40, 0x0030,
    0x0200, 0x000a, 0x0001, 0x0050,
};

/* 按 VLSI 压缩插件格式写入 (与开发指南 VS_Load_Patch 同构) */
static void load_plugin(const u16 *plg, u16 len)
{
    u16 i = 0, addr, n, val;
    while (i < len)
    {
        addr = plg[i++];
        n = plg[i++];
        if (n & 0x8000)              /* RLE: 同一值写 n 次 */
        {
            n &= 0x7fff;
            val = plg[i++];
            while (n--) VS_WR_Cmd((u8)addr, val);
        }
        else                          /* 原样复制 n 个值 */
        {
            while (n--) { val = plg[i++]; VS_WR_Cmd((u8)addr, val); }
        }
    }
}

/* 经 SDI 发送一个 MIDI 字节: RT-MIDI 模式要求每字节前补 0x00 */
static void midi_byte(u8 b)
{
    while (VS_DQ == 0);              /* 等待 DREQ 有效 */
    VS_XDCS = 0;
    VS_SPI_ReadWriteByte(0x00);
    VS_SPI_ReadWriteByte(b);
    VS_XDCS = 1;
}

static void midi3(u8 s, u8 d1, u8 d2) { midi_byte(s); midi_byte(d1); midi_byte(d2); }
static void midi2(u8 s, u8 d1)        { midi_byte(s); midi_byte(d1); }

void Synth_Init(void)
{
    VS_Init();                       /* IO + SPI 初始化 */
    VS_HD_Reset();
    VS_Soft_Reset();
    load_plugin(rtmidi_plugin, 28);  /* 进入实时 MIDI 合成模式 */
    delay_ms(10);
    VS_SPK_Set(1);                   /* 打开战舰 V3 板载 HT6872 喇叭功放 */
    Synth_MasterVol(0x10);           /* 较大音量 (衰减小) */
    Synth_Program(0, 0);             /* 默认大钢琴 */
}

void Synth_NoteOn(u8 ch, u8 note, u8 vel) { midi3(0x90 | (ch & 0x0f), note & 0x7f, vel & 0x7f); }
void Synth_NoteOff(u8 ch, u8 note)        { midi3(0x80 | (ch & 0x0f), note & 0x7f, 0x40); }
void Synth_Program(u8 ch, u8 prog)        { midi2(0xC0 | (ch & 0x0f), prog & 0x7f); }
void Synth_AllNotesOff(u8 ch)             { midi3(0xB0 | (ch & 0x0f), 123, 0); }

void Synth_PitchBend(u8 ch, u16 bend14)
{
    midi3(0xE0 | (ch & 0x0f), bend14 & 0x7f, (bend14 >> 7) & 0x7f);
}

/* SCI_VOL: 高低字节分别为左右声道衰减, 每级 0.5dB */
void Synth_MasterVol(u8 vol)
{
    VS_WR_Cmd(SPI_VOL, ((u16)vol << 8) | vol);
}

/* 适合"挥手演奏"的 GM 音色精选 */
static const gm_preset_t PRESETS[] = {
    {0,  "Piano   "},   /* 大钢琴 */
    {10, "MusicBox"},   /* 八音盒 */
    {24, "Guitar  "},   /* 尼龙吉他 */
    {40, "Violin  "},   /* 小提琴 */
    {52, "Choir   "},   /* 人声合唱 */
    {73, "Flute   "},   /* 长笛 */
    {78, "Whistle "},   /* 口哨 (最接近theremin) */
    {80, "SquareLd"},   /* 方波合成主音 */
    {104,"Sitar   "},   /* 西塔琴 */
    {114,"SteelDrm"},   /* 钢鼓 */
};

u8 Synth_PresetCount(void) { return sizeof(PRESETS) / sizeof(PRESETS[0]); }
gm_preset_t Synth_Preset(u8 idx) { return PRESETS[idx % Synth_PresetCount()]; }
