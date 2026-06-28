# LightString STM32F103 | 流光琴

[中文](#中文) | [English](#english)

## 中文

**流光琴 LightString** 是一个运行在 ALIENTEK 战舰 STM32F103ZET6 V3 开发板上的非接触式光感电子乐器。演奏者不需要触摸按键或弦，只需要在板载光敏传感器上方移动手掌，系统就会把光强变化实时转换成音阶内的音符，并通过 VS1053b MIDI 合成器发声。

这个项目的核心想法是：把“悬空手势”变成一个容易上手、不会跑调、可录制回放的嵌入式乐器。它不是简单读取 ADC 后蜂鸣器响一下，而是完整串起了传感、滤波、音阶量化、合成器控制、显示界面、红外遥控、Flash 录音和串口 MIDI 桥接。

### 亮点

- **非接触演奏**：手掌高度改变板载光敏二极管读数，形成连续演奏输入。
- **稳定音准**：中值滤波 + 一阶低通 + 迟滞量化，减少环境光噪声和边界抖动。
- **实时合成**：驱动 VS1053b 实时 MIDI 模式，支持音符开关、音色、音量和弯音控制。
- **完整交互**：TFTLCD 显示状态和光强曲线，按键/红外遥控切换音色、音阶、八度和录音。
- **事件级录音**：音符事件写入 W25Q128 Flash，可以回放旋律。
- **电脑扩展**：USART1 输出音符日志，配合 Python 脚本可桥接为虚拟 MIDI 设备。

### 系统链路

```text
手掌遮光
  -> PF8 / ADC3_IN6 光敏采样
  -> 中值滤波 + 一阶低通
  -> 挥手定标 + 归一化
  -> 音阶量化 + 迟滞消抖
  -> VS1053b MIDI 合成
  -> 扬声器 / 耳机输出
```

### 仓库结构

```text
firmware/
  App/                 流光琴原创应用层模块
  Core/                启动、系统调用和固件入口
  USER/                STM32F103 设备与中断支持文件
  SYSTEM/              ALIENTEK system/delay/usart BSP
  HARDWARE/            ALIENTEK 板级外设驱动
  HALLIB/              STM32F1 HAL/LL 驱动子集
  cmake/               Arm GNU 工具链配置
host_tools/
  serial_midi_bridge.py
tools/figures_src/
  make_figures.py      技术图表生成脚本
docs/images/
  滤波和量化示意图
```

原创核心主要在 `firmware/App/` 和 `firmware/Core/Src/main.c`。`SYSTEM/`、`HARDWARE/`、`HALLIB/` 和部分 `USER/` 文件是为了让固件可独立构建而保留的底层 BSP/HAL 支持代码。

### 硬件目标

| 功能 | 引脚 / 接口 |
| --- | --- |
| 光敏二极管 LS1 | `PF8 / ADC3_IN6` |
| VS1053 SPI | `PA5 / PA6 / PA7` on SPI1 |
| VS1053 控制脚 | `PF7`, `PF6`, `PC13`, `PE6` |
| W25Q128 Flash | `PB13 / PB14 / PB15` + `PB12` on SPI2 |
| TFTLCD | FSMC Bank1 NE4, A10 as RS |
| 红外接收 | `PB9 / TIM4_CH4` 输入捕获 |
| 按键 | `PE4`, `PE3`, `PE2`, `PA0` |
| LED / 蜂鸣器 | `PB5`, `PE5`, `PB8` |
| 调试串口 | `PA9 / PA10`, 115200 baud |

### 构建

需要安装 CMake、Ninja，以及带 newlib 的 Arm GNU embedded toolchain。确保 `arm-none-eabi-gcc` 和 `arm-none-eabi-objcopy` 在 `PATH` 中。

```sh
cd firmware
export PATH=/path/to/arm-gnu-toolchain/bin:$PATH
cmake -S . -B build/ArmGNU -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ArmGNU
```

已验证构建规模：

```text
RAM:   27208 B / 64 KB
FLASH: 66340 B / 512 KB
```

### 烧录

烧录前先确认开发板、ST-Link 和接线：

```sh
st-info --probe
```

OpenOCD：

```sh
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program firmware/build/ArmGNU/LightString.elf verify reset exit"
```

或者使用 `st-flash`：

```sh
st-flash --format ihex write firmware/build/ArmGNU/LightString.hex
```

### 操作

上电后按 `WK_UP`，在 3 秒内用手掌扫过光敏传感器完成定标。随后上下移动手掌即可演奏。

- `KEY0`：切换音色
- `KEY1`：切换音阶
- `KEY2`：开始/停止录音
- 红外数字 `0` 到 `9`：直选音色
- 红外上下键：切换八度
- 红外音量键：调节音量
- 红外播放键：回放录音
- 红外电源键：开关节拍器

### 串口 MIDI 桥

固件会通过 USART1 输出音符事件。可用 Python 工具把这些日志转成电脑上的虚拟 MIDI 输出：

```sh
pip install pyserial mido python-rtmidi
python3 host_tools/serial_midi_bridge.py /dev/cu.usbserial-11130
```

## English

**LightString** is a contact-free optical musical instrument for the ALIENTEK WarShip STM32F103ZET6 V3 development board. Instead of touching keys or strings, the player moves a hand above the onboard light sensor. The firmware turns the changing light intensity into notes in a musical scale and plays them through the onboard VS1053b MIDI synthesizer.

The project is designed as a complete embedded instrument rather than a simple ADC demo. It combines sensing, filtering, scale quantization, MIDI synthesis, TFTLCD feedback, IR/key controls, Flash-based melody recording, and an optional serial-to-MIDI host bridge.

### Highlights

- **Contact-free performance**: hand height changes the onboard light sensor reading.
- **Stable pitch mapping**: median filtering, first-order low-pass filtering, hysteresis, and stable-count debounce reduce noise and boundary jitter.
- **Real-time synthesis**: VS1053b real-time MIDI mode with note, instrument, volume, and pitch controls.
- **Full interaction loop**: TFTLCD status and light curve, onboard keys, and IR remote controls.
- **Melody recording**: note events are stored in W25Q128 Flash and can be played back.
- **Desktop expansion**: USART1 note logs can be bridged to a virtual MIDI device with the Python helper.

### Signal Flow

```text
hand shadow
  -> PF8 / ADC3_IN6 light sampling
  -> median filter + first-order low-pass filter
  -> gesture calibration + normalization
  -> scale quantization + hysteresis debounce
  -> VS1053b MIDI synthesis
  -> speaker / headphone output
```

### Repository Layout

```text
firmware/
  App/                 LightString application modules
  Core/                startup, syscalls, and firmware entry point
  USER/                STM32F103 device and interrupt support files
  SYSTEM/              ALIENTEK system/delay/usart BSP
  HARDWARE/            ALIENTEK board peripheral drivers
  HALLIB/              STM32F1 HAL/LL driver subset
  cmake/               Arm GNU toolchain file
host_tools/
  serial_midi_bridge.py
tools/figures_src/
  make_figures.py      diagram generation script
docs/images/
  filter and quantizer diagrams
```

The project-specific firmware is mainly in `firmware/App/` and
`firmware/Core/Src/main.c`. The lower-level `SYSTEM/`, `HARDWARE/`, `HALLIB/`,
and selected `USER/` files are included so the firmware can be built outside
the original course workspace.

### Hardware Target

| Function | Pin / Interface |
| --- | --- |
| Light sensor LS1 | `PF8 / ADC3_IN6` |
| VS1053 SPI | `PA5 / PA6 / PA7` on SPI1 |
| VS1053 control | `PF7`, `PF6`, `PC13`, `PE6` |
| W25Q128 Flash | `PB13 / PB14 / PB15` + `PB12` on SPI2 |
| TFTLCD | FSMC Bank1 NE4, A10 as RS |
| IR receiver | `PB9 / TIM4_CH4` input capture |
| Keys | `PE4`, `PE3`, `PE2`, `PA0` |
| LEDs and buzzer | `PB5`, `PE5`, `PB8` |
| Debug serial | `PA9 / PA10`, 115200 baud |

### Build

Install CMake, Ninja, and an Arm GNU embedded toolchain with newlib. Then:

```sh
cd firmware
export PATH=/path/to/arm-gnu-toolchain/bin:$PATH
cmake -S . -B build/ArmGNU -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ArmGNU
```

Known build size:

```text
RAM:   27208 B / 64 KB
FLASH: 66340 B / 512 KB
```

### Flash

Verify the board, ST-Link, and wiring before flashing:

```sh
st-info --probe
```

OpenOCD:

```sh
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program firmware/build/ArmGNU/LightString.elf verify reset exit"
```

Or use `st-flash`:

```sh
st-flash --format ihex write firmware/build/ArmGNU/LightString.hex
```

### Controls

After power-on, press `WK_UP` and sweep the hand through the sensing range for
three seconds to calibrate. Then move the hand up and down above the sensor to
play.

- `KEY0`: change instrument
- `KEY1`: change scale
- `KEY2`: start or stop recording
- IR digits `0` to `9`: select instrument
- IR up/down: octave shift
- IR volume keys: volume
- IR play: playback
- IR power: metronome on/off

### Serial MIDI Bridge

The firmware prints note events through USART1. The Python helper can convert
those logs into a virtual MIDI output:

```sh
pip install pyserial mido python-rtmidi
python3 host_tools/serial_midi_bridge.py /dev/cu.usbserial-11130
```

### License and Third-Party Code

The original LightString application code and project-specific tools are licensed under MIT. STMicroelectronics HAL, Arm/CMSIS, and ALIENTEK BSP files retain their original notices and terms. See `LICENSE` and `NOTICE.md`.
