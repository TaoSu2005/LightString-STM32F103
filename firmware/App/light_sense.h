#ifndef __LIGHT_SENSE_H
#define __LIGHT_SENSE_H
#include "sys.h"

/*
 * light_sense: 光敏传感器采样、滤波与归一化
 * 硬件: 战舰V3 板载光敏二极管 LS1 -> PF8 -> ADC3_IN6
 *       光线越强电压越低, 由 ALIENTEK BSP 的 Lsens_Get_Val() 折算为 0~100
 * 依赖: lsens.c / adc3 (开发指南第24章)
 */

#define LS_FILT_MED_N   5      /* 中值滤波窗口 */
#define LS_SAMPLE_MS    10     /* 采样周期, 100Hz */

void  LightSense_Init(void);
/* 每 LS_SAMPLE_MS 调用一次: 采样+滤波, 返回1表示有新数据 */
u8    LightSense_Tick(void);
/* 滤波后的原始光强 0~100 */
u8    LightSense_Raw(void);
/* 校准归一化输出 0~1000 (0=环境基线/手移开, 1000=完全遮挡) */
u16   LightSense_Norm(void);
/* 归一化值变化速率 (每秒), 用于力度映射 */
s16   LightSense_Rate(void);
/* 挥手定标: 阻塞约3秒, 用户在传感器上方做一次完整的下压-抬起动作,
 * 自动记录个人演奏范围 [min,max]; 返回0成功 */
u8    LightSense_Calibrate(void);
/* 手是否在演奏区内 (高于触发阈值, 带迟滞) */
u8    LightSense_HandPresent(void);

#endif
