# LightString STM32F103

LightString is a contact-free optical musical instrument for the ALIENTEK
WarShip STM32F103ZET6 V3 development board. A hand moving above the onboard
light sensor is sampled by ADC3, filtered, quantized into musical scale steps,
and played in real time through the onboard VS1053b MIDI synthesizer.

The project combines STM32F1 peripherals, board-level BSP integration, and a
small application layer for musical interaction:

- 100 Hz light sensing on `PF8 / ADC3_IN6`
- median and first-order low-pass filtering
- scale quantization with hysteresis and stable-count debounce
- VS1053b real-time MIDI note on/off, instrument, volume, and pitch control
- TFTLCD status UI and scrolling light curve
- IR remote and key controls
- W25Q128 event-level melody recording and playback
- USART1 note logs with an optional serial-to-MIDI host bridge

## Repository Layout

```text
firmware/
  App/                 LightString application modules
  Core/                startup, syscalls, and firmware entry point
  USER/                STM32F103 device and interrupt support files
  SYSTEM/              ALIENTEK system, delay, and USART BSP code
  HARDWARE/            ALIENTEK board peripheral drivers
  HALLIB/              STM32F1 HAL/LL driver subset
  cmake/               Arm GNU toolchain file
host_tools/
  serial_midi_bridge.py
tools/figures_src/
  make_figures.py      source for the technical diagrams in docs/images
docs/images/
  filter and quantizer diagrams
```

The core project-specific firmware lives in `firmware/App/` plus
`firmware/Core/Src/main.c`. The lower-level `SYSTEM/`, `HARDWARE/`, `HALLIB/`,
and selected `USER/` files are included to make the board firmware buildable.

## Hardware Target

| Function | Pin / Interface |
| --- | --- |
| Light sensor LS1 | `PF8 / ADC3_IN6` |
| VS1053 SPI | `PA5 / PA6 / PA7` on SPI1 |
| VS1053 control | `PF7`, `PF6`, `PC13`, `PE6` |
| W25Q128 Flash | `PB13 / PB14 / PB15` plus `PB12` on SPI2 |
| TFTLCD | FSMC Bank1 NE4, A10 as RS |
| IR receiver | `PB9 / TIM4_CH4` input capture |
| Keys | `PE4`, `PE3`, `PE2`, `PA0` |
| LEDs and buzzer | `PB5`, `PE5`, `PB8` |
| Debug serial | `PA9 / PA10`, 115200 baud |

## Build

Install CMake, Ninja, and an Arm GNU embedded toolchain that provides
`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, and newlib.

On macOS with Homebrew tools plus an official Arm GNU Toolchain in `PATH`:

```sh
cd firmware
export PATH=/path/to/arm-gnu-toolchain/bin:$PATH
cmake -S . -B build/ArmGNU -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ArmGNU
```

A known good build produces approximately:

```text
RAM:   27208 B / 64 KB
FLASH: 66224 B / 512 KB
```

## Flash

Only flash after verifying the board, ST-Link, and target wiring:

```sh
st-info --probe
```

Using OpenOCD:

```sh
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program firmware/build/ArmGNU/LightString.elf verify reset exit"
```

Using `st-flash` and the generated HEX:

```sh
st-flash --format ihex write firmware/build/ArmGNU/LightString.hex
```

## Controls

After power-on, press `WK_UP` and move the hand through the sensing range for a
three-second calibration. Then raise or lower the hand over the light sensor to
play notes.

- `KEY0`: change instrument
- `KEY1`: change scale
- `KEY2`: start or stop recording
- IR digits `0` to `9`: select instrument
- IR up/down: octave shift
- IR volume keys: volume
- IR play: playback
- IR power: metronome on/off

## Serial MIDI Bridge

The firmware prints note events through USART1. The optional Python helper can
turn those logs into a virtual MIDI output:

```sh
pip install pyserial mido python-rtmidi
python3 host_tools/serial_midi_bridge.py /dev/cu.usbserial-11130
```

## Licensing

The original LightString application code and project-specific tools are under
the MIT license. STMicroelectronics HAL, Arm/CMSIS, and ALIENTEK board support
files keep their own notices. See `LICENSE` and `NOTICE.md`.
