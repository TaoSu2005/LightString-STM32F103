#include "light_sense.h"
#include "lsens.h"
#include "delay.h"

/* ---------------- 内部状态 ---------------- */
static u8  med_buf[LS_FILT_MED_N];   /* 中值滤波环形缓冲 */
static u8  med_idx = 0;
static u16 ema_x10 = 0;              /* 一阶低通输出, 放大10倍定点 */
static u8  filt_val = 0;             /* 最终滤波输出 0~100 */

/*
 * ALIENTEK lsens.c 的 Lsens_Get_Val: 0=最暗, 100=最亮.
 * 手在传感器上方遮光 -> 读数变小. 因此归一化时做方向翻转:
 *   norm = (cal_hi - filt) / (cal_hi - cal_lo) * 1000
 * cal_lo: 定标观测到的最小读数(手压到最低/最暗)
 * cal_hi: 定标观测到的最大读数(手移开/环境亮度)
 * 得到 norm: 0=手移开(亮), 1000=完全遮挡(暗).
 */
static u8  cal_lo = 20;
static u8  cal_hi = 90;
static u16 norm_val = 0;             /* 归一化 0~1000 */
static s16 rate_per_s = 0;
static u8  hand_in = 0;

/* 迟滞门限: 进入演奏区 > 8%, 退出 < 4% (千分比 80/40) */
#define HAND_ON_TH   80
#define HAND_OFF_TH  40

/*
 * 单次读数: 不用 Lsens_Get_Val —— 它内部做 10 次平均且每次 delay_ms(5),
 * 单调用阻塞约 50ms, 与 100Hz 采样节拍矛盾. 平均交给本模块的中值窗口.
 * Get_Adc3 来自标准例程 adc3.c (开发指南第24章).
 */
extern u16 Get_Adc3(u32 ch);
static u8 ls_read_once(void)
{
    u32 v = Get_Adc3(ADC_CHANNEL_6);     /* PF8 = ADC3_IN6 */
    if (v > 4000) v = 4000;
    return (u8)(100 - v / 40);           /* 与 Lsens_Get_Val 同标度: 0暗~100亮 */
}

void LightSense_Init(void)
{
    u8 i, v;
    Lsens_Init();                    /* PF8 模拟输入 + ADC3 通道6 */
    v = ls_read_once();
    for (i = 0; i < LS_FILT_MED_N; i++) med_buf[i] = v;
    ema_x10 = v * 10;
    filt_val = v;
}

/* 取中值: 窗口很小, 直接插入排序副本 */
static u8 median_of(u8 *buf, u8 n)
{
    u8 tmp[LS_FILT_MED_N], i, j, t;
    for (i = 0; i < n; i++) tmp[i] = buf[i];
    for (i = 1; i < n; i++)
        for (j = i; j > 0 && tmp[j - 1] > tmp[j]; j--)
        {
            t = tmp[j]; tmp[j] = tmp[j - 1]; tmp[j - 1] = t;
        }
    return tmp[n / 2];
}

u8 LightSense_Tick(void)
{
    u8 med;
    u32 span;
    s32 nv, diff;

    /* 1. 采样进中值窗口: 去脉冲毛刺(灯光闪烁/ADC尖刺) */
    med_buf[med_idx] = ls_read_once();
    med_idx = (med_idx + 1) % LS_FILT_MED_N;
    med = median_of(med_buf, LS_FILT_MED_N);

    /* 2. 一阶指数低通 y += a*(x-y), a=0.3 -> 定点 3/10 */
    ema_x10 += (3 * ((s16)med * 10 - (s16)ema_x10)) / 10;
    filt_val = (u8)((ema_x10 + 5) / 10);

    /* 3. 方向翻转 + 归一化: 遮光越多 norm 越大 */
    span = (cal_hi > cal_lo) ? (u32)(cal_hi - cal_lo) : 1;
    nv = ((s32)cal_hi - filt_val) * 1000 / (s32)span;
    if (nv < 0) nv = 0;
    if (nv > 1000) nv = 1000;

    /* 4. 变化率 (千分比/秒): 差分钳位防 s16 溢出 */
    diff = nv - (s32)norm_val;
    if (diff > 320)  diff = 320;
    if (diff < -320) diff = -320;
    rate_per_s = (s16)(diff * (1000 / LS_SAMPLE_MS));
    norm_val = (u16)nv;

    /* 5. 手存在判定, 带迟滞防止边界抖动 */
    if (!hand_in && norm_val > HAND_ON_TH)  hand_in = 1;
    if (hand_in  && norm_val < HAND_OFF_TH) hand_in = 0;
    return 1;
}

u8  LightSense_Raw(void)  { return filt_val; }
u16 LightSense_Norm(void) { return norm_val; }
s16 LightSense_Rate(void) { return rate_per_s; }
u8  LightSense_HandPresent(void) { return hand_in; }

/*
 * 挥手定标: 3秒内连续采样, 取观测极值作为个人演奏范围.
 * 用户操作: 先把手移开(记录环境亮度上限), 再将手压到最低再抬起(记录下限).
 * 留 5% 余量防止边界饱和. 范围过窄(环境太暗)返回1失败.
 */
u8 LightSense_Calibrate(void)
{
    u16 i;
    u8 v, lo = 255, hi = 0;
    for (i = 0; i < 300; i++)        /* 300 * 10ms = 3s */
    {
        v = ls_read_once();
        if (v < lo) lo = v;
        if (v > hi) hi = v;
        delay_ms(10);
    }
    if (hi - lo < 10) return 1;      /* 动态范围不足, 校准失败 */
    cal_lo = lo + (hi - lo) / 20;
    cal_hi = hi - (hi - lo) / 20;
    ema_x10 = ls_read_once() * 10;
    return 0;
}
