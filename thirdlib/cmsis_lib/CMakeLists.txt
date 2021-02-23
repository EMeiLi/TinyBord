SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.7)

file(GLOB_RECURSE SOURCES
    "CMSIS_LIB/CMSIS/RTOS2/Source/os_systick.c"
    "CMSIS_LIB/CMSIS/RTOS2/RTX/Config/*.*"
    "CMSIS_LIB/CMSIS/RTOS2/RTX/Source/*.c"
    "CMSIS_LIB/CMSIS/RTOS2/RTX/Source/*.h"
    "CMSIS_LIB/CMSIS/RTOS2/RTX/Source/GCC/irq_cm4f.S"
)

add_library(rtx5 STATIC ${SOURCES})

target_include_directories(rtx5 PRIVATE
    ../../stm32cubemx/Drivers/STM32F4xx_HAL_Driver/Inc 
    ../../stm32cubemx/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy 
    ../../stm32cubemx/Drivers/CMSIS/Device/ST/STM32F4xx/Include 
    ../../stm32cubemx/Drivers/CMSIS/Include
    ../../stm32cubemx/Core/Inc 
    CMSIS_LIB/CMSIS/RTOS2/RTX/Include/
    CMSIS_LIB/CMSIS/RTOS2/RTX/Config
    Config/
)

target_include_directories(rtx5 PUBLIC
    CMSIS_LIB/CMSIS/RTOS2/Include/
)