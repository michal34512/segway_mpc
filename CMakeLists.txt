cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)
set(PROJ_PATH                       ${CMAKE_CURRENT_SOURCE_DIR})
set(APP_PATH                        ${CMAKE_CURRENT_SOURCE_DIR}/app)
set(CUBEMX_PATH                     ${CMAKE_CURRENT_SOURCE_DIR}/cubemx)
set(CMAKE_TOOLCHAIN_FILE            ${CUBEMX_PATH}/cmake/gcc-arm-none-eabi.cmake)

project(${STM_PROJECT_NAME})
enable_language(C ASM)
message(STATUS "Project name: ${CMAKE_PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

# CPU Parameters (uzupełniane w toolchainie lub generowane z CubeMX)
set(CPU_PARAMETERS ${CPU_PARAMETERS})

set(compiler_define ${compiler_define}
    "USE_HAL_DRIVER"
    "STM32H725xx"
)

add_executable(${CMAKE_PROJECT_NAME})

# Include cubemx cmake files (with workaround)
set(CMAKE_SOURCE_DIR_temp ${CMAKE_SOURCE_DIR})
set(CMAKE_SOURCE_DIR ${CUBEMX_PATH})
include(${CUBEMX_PATH}/cmake/stm32cubemx/CMakeLists.txt)

# Zapisujemy pełne ścieżki do kompilatorów poprawnie wykryte przez CMake na starcie
set(SAVED_C_COMPILER ${CMAKE_C_COMPILER})
set(SAVED_CXX_COMPILER ${CMAKE_CXX_COMPILER})
set(SAVED_ASM_COMPILER ${CMAKE_ASM_COMPILER})

include(${CUBEMX_PATH}/cmake/gcc-arm-none-eabi.cmake)

# Przywracamy pełne ścieżki
set(CMAKE_C_COMPILER ${SAVED_C_COMPILER})
set(CMAKE_CXX_COMPILER ${SAVED_CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${SAVED_ASM_COMPILER})
set(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR_temp})

# Definicje globalne dla STM32
add_compile_definitions(__STM32__ MEASURE_TIMINGS K_MAX_STACK=300)

# Bezwzględne wymuszenie flag procesora
string(REPLACE ";" " " CPU_PARAMETERS_STR "${CPU_PARAMETERS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_PARAMETERS_STR} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_PARAMETERS_STR} -ffunction-sections -fdata-sections")

add_compile_options(${CPU_PARAMETERS})
add_compile_options(-mthumb -fno-pic -fno-pie -ffunction-sections -fdata-sections)

# To usunie dziwne offsety z wskaźników
add_compile_options(-fno-jump-tables)

# SYSVIEW hack
if(TARGET FreeRTOS)
    target_include_directories(FreeRTOS PRIVATE
        ${PROJ_PATH}/3rdParty/Segger/Config
        ${PROJ_PATH}/3rdParty/Segger/RTT
        ${PROJ_PATH}/3rdParty/Segger/SYSVIEW
    )
endif()

# User includes
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Zostawiamy puste lub dodajemy specyficzne katalogi projektu
)

# 3rdParty includes
target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE
    ${APP_PATH}
    ${APP_PATH}/drivers
    ${APP_PATH}/tasks
    ${APP_PATH}/common

    # SEGGER
    ${PROJ_PATH}/3rdParty/Segger/Config
    ${PROJ_PATH}/3rdParty/Segger/RTT
    ${PROJ_PATH}/3rdParty/Segger/SYSVIEW
)

# Sources
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Tasks
    ${APP_PATH}/main/app_main.c
    ${APP_PATH}/tasks/cli_task/cli_task.c
    ${APP_PATH}/tasks/cli_task/cli_commands.c
    ${APP_PATH}/tasks/mpc_task/mpc_task.c

    # Components
    ${APP_PATH}/logger/logger.c

    # Drivers
    ${APP_PATH}/drivers/sysview/sysview_macros.c
    ${APP_PATH}/drivers/i2c_manager/i2c_manager.c
    ${APP_PATH}/drivers/step_motor/step_manager.c
    ${APP_PATH}/drivers/queue_manager/queue_manager.c
    ${APP_PATH}/drivers/cli/cli_rtt.c
    ${APP_PATH}/drivers/imu_manager/imu_manager.c
    
    # MPU6050
    ${APP_PATH}/drivers/mpu6050/I2C.c
    ${APP_PATH}/drivers/mpu6050/mpu6050.c
    ${APP_PATH}/drivers/mpu6050/inv_mpu_dmp_motion_driver.c
    ${APP_PATH}/drivers/mpu6050/inv_mpu.c

    # SEGGER
    ${PROJ_PATH}/3rdParty/Segger/RTT/SEGGER_RTT.c
    ${PROJ_PATH}/3rdParty/Segger/RTT/SEGGER_RTT_printf.c
    ${PROJ_PATH}/3rdParty/Segger/SYSVIEW/SEGGER_SYSVIEW_Config_FreeRTOS.c
    ${PROJ_PATH}/3rdParty/Segger/SYSVIEW/SEGGER_SYSVIEW_FreeRTOS.c
    ${PROJ_PATH}/3rdParty/Segger/SYSVIEW/SEGGER_SYSVIEW.c
)

# Pass definitions
if(DEFINED LOGGER_USE_RTT)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE LOGGER_USE_RTT=${LOGGER_USE_RTT})
endif()

if(DEFINED SEGGER_SYSTEMVIEW)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE SEGGER_SYSTEMVIEW=${SEGGER_SYSTEMVIEW})
endif()

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wpedantic
    -Wno-unused-parameter
)

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -u _printf_float                # STDIO float formatting support
    -Wl,--start-group
    -Wl,--gc-sections               # Usuwanie nieużywanych sekcji kodu i danych
    -lc
    -lm
    -lstdc++
    -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage
)