# 流光琴 LightString 固件

基于 ALIENTEK 战舰 STM32F103ZET6 V3 开发板的非接触式光感电子乐器。

## 目录

- `Core/Src/main.c`: 入口，初始化 + 主循环。
- `App/`: 本设计的全部应用层模块（与板级驱动解耦）：
  - `light_sense.c/h`: 光敏采样（100 Hz）、中值+一阶低通滤波、挥手定标、
    归一化与变化率估计。
  - `scale_quantizer.c/h`: 光强→音阶台阶量化，迟滞 + 稳定计数消抖，
    力度映射，五种音阶。
  - `synth_vs1053.c/h`: VS1053b 实时 MIDI 模式（加载 VLSI rtmidistart
    补丁），NoteOn/Off、音色、音量、弯音。
  - `recorder.c/h`: 音符事件录制到 W25Q128（5 字节/事件）与回放。
  - `ui_lcd.c/h`: TFTLCD 界面（状态栏、大字音符、滚动光强曲线）。
  - `lightstring.c/h`: 主状态机（演奏/录制/回放/定标）、按键与红外处理、
    蜂鸣器节拍器。
  - `timebase.c/h`: TIM7 1 ms 独立时基。**必须使用**：ALIENTEK 非 OS 版
    `delay.c` 会接管 SysTick，`HAL_GetTick()` 不走时，全部调度改用
    `millis()`。

## 依赖的 ALIENTEK BSP（取自光盘标准例程，开发指南对应章节）

| 模块 | 文件 | 章节 |
|---|---|---|
| SYSTEM | `sys.c delay.c usart.c` | 第 5 章 |
| LED/按键/蜂鸣器 | `led.c key.c beep.c` | 第 6–8 章 |
| TFTLCD (FSMC) | `lcd.c` | 第 18 章 |
| 光敏 + ADC3 | `lsens.c adc3.c` | 第 22/24 章 |
| 红外遥控 (TIM4_CH4) | `remote.c` | 第 33 章 |
| VS1053 (SPI1) | `vs10xx.c spi.c` | 第 48 章 |
| W25Q128 (SPI2) | `w25qxx.c` | 第 29 章 |

参考来源：ALIENTEK 战舰 STM32F103 V3 HAL 标准例程与开发指南。

## 引脚一览（全部为板载资源，无需外部接线）

| 资源 | 引脚 | 外设 |
|---|---|---|
| 光敏二极管 LS1 | PF8 | ADC3_IN6 |
| VS1053 SCK/MISO/MOSI | PA5/PA6/PA7 | SPI1 |
| VS1053 XCS/XDCS/DREQ/RST | PF7/PF6/PC13/PE6 | GPIO |
| W25Q128 | PB13/PB14/PB15 + PB12 | SPI2 |
| TFTLCD | FSMC Bank1 NE4，A10=RS | FSMC |
| 红外接收头 | PB9 | TIM4_CH4 输入捕获 |
| KEY0/KEY1/KEY2/WK_UP | PE4/PE3/PE2/PA0 | GPIO |
| LED0/LED1、蜂鸣器 | PB5/PE5、PB8 | GPIO |
| 调试串口 | PA9/PA10 | USART1 115200 |

注：VS1053 七根信号线的引脚以板上原理图（开发指南图 48.1.1/表 48.1.1）
为准；上表与战舰 V3 配套例程 `vs10xx.h` 一致，移植前请对照确认。

移植注意：

- `light_sense.c` 直接调用 `Get_Adc3(ADC_CHANNEL_6)` 做单次采样，
  不要用 `Lsens_Get_Val()`（它内部 10 次平均 + 每次 `delay_ms(5)`，
  单次调用阻塞约 50 ms，会拖垮 100 Hz 采样节拍）；平均已由本模块的
  中值窗口完成。
- ALIENTEK `Lsens` 标度为 0=最暗、100=最亮，手遮挡读数变小；
  归一化方向翻转已在 `light_sense.c` 内处理。
- 红外数字键码表按开发指南第 33 章配套遥控器核对
  （0=0x42，1=0x68，2=0x98，3=0xB0，4=0x30，5=0x18，6=0x7A，
  7=0x10，8=0x38，9=0x5A）。

## 当前工程状态

本目录已经整理为可在 macOS 上直接构建的 Arm GNU/CMake 固件工程：

- `CMakeLists.txt`、`cmake/gcc-arm-none-eabi.cmake`、`STM32F103XX_FLASH.ld`
  是构建入口。
- `CORE/`、`USER/`、`SYSTEM/`、`HALLIB/`、`HARDWARE/` 来自正点原子 HAL
  标准例程，并补入光敏、红外、蜂鸣器等本设计需要的 BSP。
- `App/` 是流光琴应用层；`Core/Src/main.c` 是固件入口。
- 构建产物生成在 `build/ArmGNU/LightString.elf/.hex/.bin`。

## 构建

两条等价路线：

1. **Keil MDK**（Windows）：用光盘"实验48 音乐播放器"工程作模板，
   删除 `mp3player` 应用层，将 `App/` 全部加入工程，替换 `main.c`，
   再补入 `lsens.c/adc3.c/remote.c`。
2. **CubeMX + CMake + Arm GNU Toolchain**（macOS/Linux）：
   按引脚表在 CubeMX 配置 SPI1/SPI2/ADC3/TIM4/FSMC/USART1 与 GPIO，
   SYS Debug 选 Serial Wire（防 SWD 失锁），
   生成后把 `App/` 与所列 BSP 移植进 `USER CODE` 框架编译。

```sh
cd firmware
export PATH=/path/to/arm-gnu-toolchain/bin:$PATH
cmake -S . -B build/ArmGNU -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ArmGNU
```

已验证构建结果：

```text
RAM:   27208 B / 64 KB
FLASH: 66224 B / 512 KB
```

## 烧录

确认 ST-Link 与开发板连接后再烧录：

```sh
st-info --probe
```

如果能识别到 `STM32F103xE/F1xx_HD`，再执行：

```sh
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/ArmGNU/LightString.elf verify reset exit"
```

或使用 HEX：

```sh
st-flash --format ihex write build/ArmGNU/LightString.hex
```

## 操作说明

上电后先按 WK_UP 做 3 秒挥手定标（手从高处压到最低再抬起）。
之后悬空升降手掌即可演奏：

- KEY0 切音色，KEY1 切音阶，KEY2 开始/停止录制；
- 遥控器数字键 0–9 直选音色，↑/↓ 升降八度，VOL± 音量，
  PLAY 回放录音，POWER 开关节拍器；
- 串口 115200 输出音符事件日志，可配合 `host_tools/serial_midi_bridge.py`
  把开发板变成电脑的 MIDI 键盘。
