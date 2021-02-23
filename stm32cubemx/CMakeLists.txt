SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.7)

file(GLOB_RECURSE SOURCES "Drivers/STM32F4xx_HAL_Driver/*.*" "Drivers/Device/*.*")

add_library(stm32cubemx STATIC ${SOURCES})

target_include_directories(stm32cubemx PUBLIC
    Core/Inc 
    Drivers/STM32F4xx_HAL_Driver/Inc 
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy 
    Drivers/CMSIS/Device/ST/STM32F4xx/Include 
    Drivers/CMSIS/Include
)