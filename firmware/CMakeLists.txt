cmake_minimum_required(VERSION 3.22)

project(LightString LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
    Core/Src/main.c
    Core/Src/syscalls.c
    Core/startup_stm32f103xe.s

    USER/system_stm32f1xx.c
    USER/stm32f1xx_it.c
    USER/stm32f1xx_hal_msp.c

    SYSTEM/sys/sys.c
    SYSTEM/delay/delay.c
    SYSTEM/usart/usart.c

    HARDWARE/LED/led.c
    HARDWARE/KEY/key.c
    HARDWARE/BEEP/beep.c
    HARDWARE/LCD/lcd.c
    HARDWARE/SPI/spi.c
    HARDWARE/W25QXX/w25qxx.c
    HARDWARE/VS10XX/vs10xx.c
    HARDWARE/ADC3/adc3.c
    HARDWARE/LSENS/lsens.c
    HARDWARE/REMOTE/remote.c

    App/light_sense.c
    App/scale_quantizer.c
    App/synth_vs1053.c
    App/recorder.c
    App/ui_lcd.c
    App/timebase.c
    App/lightstring.c

    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_sram.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
    HALLIB/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_fsmc.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F103xE
)

target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Src
    CORE
    USER
    SYSTEM/sys
    SYSTEM/delay
    SYSTEM/usart
    HARDWARE/LED
    HARDWARE/KEY
    HARDWARE/BEEP
    HARDWARE/LCD
    HARDWARE/SPI
    HARDWARE/W25QXX
    HARDWARE/VS10XX
    HARDWARE/SDIO
    HARDWARE/ADC3
    HARDWARE/LSENS
    HARDWARE/REMOTE
    App
    HALLIB/STM32F1xx_HAL_Driver/Inc
    HALLIB/STM32F1xx_HAL_Driver/Inc/Legacy
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-address-of-packed-member
)

target_link_libraries(${PROJECT_NAME} PRIVATE m)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generating HEX and BIN images"
)
