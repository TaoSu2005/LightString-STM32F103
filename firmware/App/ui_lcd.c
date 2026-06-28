#include "ui_lcd.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>

#define CURVE_X0   10
#define CURVE_Y0   160
#define CURVE_W    220
#define CURVE_H    70

static u8  curve[CURVE_W];     /* 曲线纵坐标缓冲 */
static u16 curve_pos = 0;
static u8  last_note = 0xFF, last_active = 0xFF;

void UI_Init(void)
{
    LCD_Init();
    LCD_Clear(WHITE);
    POINT_COLOR = BLACK;
    /* LCD_ShowString 仅支持 ASCII 点阵; 中文需第45章字库方案, 标题用英文 */
    LCD_ShowString(36, 6, 200, 24, 24, (u8 *)"LightString");
    LCD_ShowString(34, 34, 200, 12, 12, (u8 *)"SYSU Embedded Course Design");
    LCD_DrawLine(0, 50, 239, 50);
    LCD_ShowString(10, 56, 80, 16, 16, (u8 *)"Mode:");
    LCD_ShowString(10, 76, 80, 16, 16, (u8 *)"Inst:");
    LCD_ShowString(130, 76, 60, 16, 16, (u8 *)"Scale:");
    LCD_DrawRectangle(CURVE_X0 - 1, CURVE_Y0 - 1,
                      CURVE_X0 + CURVE_W + 1, CURVE_Y0 + CURVE_H + 1);
    LCD_ShowString(10, 250, 230, 12, 12,
                   (u8 *)"KEY0:Inst KEY1:Scale KEY2:Rec");
    LCD_ShowString(10, 266, 230, 12, 12,
                   (u8 *)"WKUP:Calibrate  IR:Preset/Oct/Vol");
    memset(curve, CURVE_H - 1, sizeof(curve));
}

void UI_SetMode(const char *mode)
{
    POINT_COLOR = RED;
    LCD_Fill(54, 56, 128, 72, WHITE);
    LCD_ShowString(54, 56, 80, 16, 16, (u8 *)mode);
    POINT_COLOR = BLACK;
}

void UI_SetInstrument(const char *name)
{
    POINT_COLOR = BLUE;
    LCD_Fill(54, 76, 128, 92, WHITE);
    LCD_ShowString(54, 76, 80, 16, 16, (u8 *)name);
    POINT_COLOR = BLACK;
}

void UI_SetScale(const char *name)
{
    POINT_COLOR = BLUE;
    LCD_Fill(182, 76, 239, 92, WHITE);
    LCD_ShowString(182, 76, 58, 16, 16, (u8 *)name);
    POINT_COLOR = BLACK;
}

void UI_ShowNote(u8 active, u8 note, u8 vel)
{
    char nm[5];
    u8 jp;
    char line[20];
    if (active == last_active && note == last_note) return;
    last_active = active; last_note = note;

    LCD_Fill(10, 100, 230, 152, WHITE);
    if (!active)
    {
        POINT_COLOR = GRAY;
        LCD_ShowString(80, 110, 120, 24, 24, (u8 *)"----");
        POINT_COLOR = BLACK;
        return;
    }
    Quant_NoteName(note, nm, &jp);
    POINT_COLOR = RED;
    LCD_ShowString(60, 104, 100, 24, 24, (u8 *)nm);           /* 音名 C4 */
    sprintf(line, "do-re-mi: %d", jp);
    POINT_COLOR = BLACK;
    LCD_ShowString(60, 132, 120, 16, 16, (u8 *)line);
    /* 力度条 */
    LCD_Fill(160, 104, 230, 120, WHITE);
    LCD_DrawRectangle(160, 104, 230, 120);
    POINT_COLOR = GREEN;
    LCD_Fill(161, 105, 161 + (u16)vel * 68 / 127, 119, GREEN);
    POINT_COLOR = BLACK;
}

void UI_PushCurve(u16 norm)
{
    u16 x;
    u8 y_new = (u8)(CURVE_H - 1 - (norm * (CURVE_H - 2)) / 1000);

    /* 滚动绘制: 擦旧列画新列, 避免整屏重绘闪烁 */
    x = CURVE_X0 + curve_pos;
    LCD_Fill(x, CURVE_Y0, x, CURVE_Y0 + CURVE_H, WHITE);
    POINT_COLOR = BLUE;
    LCD_DrawPoint(x, CURVE_Y0 + y_new);
    if (y_new < curve[curve_pos])
        LCD_DrawLine(x, CURVE_Y0 + y_new, x, CURVE_Y0 + curve[curve_pos]);
    POINT_COLOR = BLACK;
    curve[curve_pos] = y_new;
    curve_pos = (curve_pos + 1) % CURVE_W;
    /* 游标 */
    POINT_COLOR = RED;
    LCD_DrawPoint(CURVE_X0 + curve_pos, CURVE_Y0 + 1);
    POINT_COLOR = BLACK;
}

void UI_Message(const char *msg)
{
    LCD_Fill(10, 290, 230, 306, WHITE);
    POINT_COLOR = RED;
    LCD_ShowString(10, 290, 220, 16, 16, (u8 *)msg);
    POINT_COLOR = BLACK;
}
