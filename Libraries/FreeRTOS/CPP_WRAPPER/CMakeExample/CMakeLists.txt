cmake_minimum_required(VERSION 3.20)

###################### CONSTANTS ######################################
set (PROJECT_TYPE_EXECUTABLE          "exe")
set (PROJECT_TYPE_STATIC_LIBRARY      "static-lib")
set (MCPU_CORTEX_M0				      "-mcpu=cortex-m0")
set (MCPU_CORTEX_M0PLUS				  "-mcpu=cortex-m0plus")
set (MCPU_CORTEX_M3				      "-mcpu=cortex-m3")
set (MCPU_CORTEX_M4				      "-mcpu=cortex-m4")
set (MCPU_CORTEX_M7				      "-mcpu=cortex-m7")
set (MCPU_CORTEX_M33				  "-mcpu=cortex-m33")
set (MFPU_FPV4_SP_D16                 "-mfpu=fpv4-sp-d16")
set (MFPU_FPV5_D16                    "-mfpu=fpv5-d16")
set (RUNTIME_LIBRARY_REDUCED_C        "--specs=nano.specs")
set (RUNTIME_LIBRARY_STD_C            "")
set (RUNTIME_LIBRARY_SYSCALLS_MINIMAL "--specs=nosys.specs")
set (RUNTIME_LIBRARY_SYSCALLS_NONE    "")
set (MFLOAT_ABI_SOFTWARE              "-mfloat-abi=soft")
set (MFLOAT_ABI_HARDWARE              "-mfloat-abi=hard")
set (MFLOAT_ABI_MIX                   "-mfloat-abi=softfp")
#######################################################################

###################### VARIABLES ######################################
set (PROJECT_NAME             "Micro-XRCE-DDS-Client-MCU")
set (PROJECT_TYPE             "exe")
set (LINKER_SCRIPT            "../STM32H743ZITX_FLASH.ld")
set (MCPU                     ${MCPU_CORTEX_M7})
set (MFPU                     ${MFPU_FPV5_D16})
set (MFLOAT_ABI               ${MFLOAT_ABI_HARDWARE})
set (RUNTIME_LIBRARY          ${RUNTIME_LIBRARY_REDUCED_C})
set (RUNTIME_LIBRARY_SYSCALLS ${RUNTIME_LIBRARY_SYSCALLS_MINIMAL})

set (PROJECT_DEFINES
	# LIST COMPILER DEFINITIONS HERE
	STM32H743xx
	USE_HAL_DRIVER
    )

set(CMAKE_TOOLCHAIN_FILE gcc-arm-none-eabi.cmake)

project(${PROJECT_NAME})
enable_language(ASM)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_DEFINES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDES})

if(!${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING FORCE)
endif()

set (CMAKE_EXECUTABLE_SUFFIX ".elf")
set (CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set (CMAKE_C_FLAGS "${MCPU} -std=gnu11 ${MFPU} ${MFLOAT_ABI} ${RUNTIME_LIBRARY} -mthumb -Wall -ffunction-sections -fdata-sections")
set (CMAKE_CXX_FLAGS "${MCPU} ${MFPU} ${MFLOAT_ABI} ${RUNTIME_LIBRARY} -mthumb -Wall -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} ${RUNTIME_LIBRARY_SYSCALLS} -Wl,-Map=${PROJECT_NAME}.map -Wl,--gc-sections -static -Wl,--start-group -lc -lm -lnosys -Wl,--end-group")
set (CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")
set (CMAKE_C_FLAGS_DEBUG "-Og -g3 -ggdb")
set (CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set (CMAKE_CXX_FLAGS_DEBUG "-Og -g3 -ggdb")
set (CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}> # 编译完顺便打印一下 Flash/RAM 占用
    COMMENT "Building .hex and .bin files, and printing size..."
)
