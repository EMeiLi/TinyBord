SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.7)

file(GLOB_RECURSE SOURCES
    "u8g2/csrc/*.*"
    "Config/*.*"
)

add_library(u8g2 STATIC ${SOURCES})

target_include_directories(u8g2 PRIVATE
    ../../stm32cubemx/Drivers/STM32F4xx_HAL_Driver/Inc 
    ../../stm32cubemx/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy 
    ../../stm32cubemx/Drivers/CMSIS/Device/ST/STM32F4xx/Include 
    ../../stm32cubemx/Drivers/CMSIS/Include
    ../../stm32cubemx/Core/Inc 
)

target_include_directories(u8g2 PUBLIC
    u8g2/csrc/
    Config/
)