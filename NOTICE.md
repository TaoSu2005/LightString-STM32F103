# Notices

This repository is a mixed-source embedded project. The LightString
application logic and project-specific documentation are original work by Su
Tao. Several low-level support files are included so that the firmware can be
built and inspected without the original course workspace.

## Original LightString code

The following project-specific components are licensed under the repository
MIT license unless stated otherwise:

- `firmware/App/`
- `firmware/Core/Src/main.c`
- `firmware/CMakeLists.txt`
- `firmware/cmake/`
- `firmware/STM32F103XX_FLASH.ld`
- `host_tools/`
- `tools/figures_src/`
- top-level documentation

## STMicroelectronics HAL

Files under `firmware/HALLIB/STM32F1xx_HAL_Driver/` are derived from the STM32F1
HAL/LL driver package. These files keep STMicroelectronics copyright headers.
Most files state the BSD 3-Clause license in their headers.

## Arm CMSIS and STM32 device headers

CMSIS core headers and STM32F103 device support files are included under
`firmware/Core/` and `firmware/USER/` where required by the firmware. These
files retain their upstream notices.

## ALIENTEK board support code

Files under these directories are adapted from ALIENTEK STM32F103 WarShip V3
HAL example projects and board support packages:

- `firmware/SYSTEM/`
- `firmware/HARDWARE/`
- selected Keil project metadata under `firmware/USER/`

They are included for board-level reproducibility on the ALIENTEK WarShip V3
development board. Their original comments and attribution are preserved where
present. This repository does not claim authorship of those BSP files.

## Hardware and firmware safety

The firmware targets STM32F103ZET6 on the ALIENTEK WarShip V3 board. Verify the
pin map, board revision, and ST-Link wiring before flashing. Do not flash
blindly on unrelated STM32F1 boards.
