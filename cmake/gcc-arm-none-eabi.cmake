set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)

# 指定编译器
set(CMAKE_C_COMPILER      starm-clang)
set(CMAKE_CXX_COMPILER    starm-clang++)
set(CMAKE_ASM_COMPILER    starm-clang)

# 链接器、objcopy、size 等工具
set(CMAKE_LINKER          starm-lld)
set(CMAKE_OBJCOPY         starm-objcopy)
set(CMAKE_SIZE            starm-size)

# 使用正确的 target triple
set(CMAKE_C_COMPILER_TARGET   thumbv7m-st-none-eabihf)
set(CMAKE_CXX_COMPILER_TARGET thumbv7m-st-none-eabihf)
set(CMAKE_ASM_COMPILER_TARGET thumbv7m-st-none-eabihf)

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard")

# 通用编译标志
set(COMMON_FLAGS "${TARGET_FLAGS}")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wall -Wextra -Wpedantic")
set(COMMON_FLAGS "${COMMON_FLAGS} -fdata-sections -ffunction-sections")
set(COMMON_FLAGS "${COMMON_FLAGS} -fexec-charset=UTF-8 -finput-charset=UTF-8")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wno-invalid-source-encoding -Wno-invalid-utf8")
# 禁用异常处理和 RTTI，不生成 EXIDX
set(COMMON_FLAGS "${COMMON_FLAGS} -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables")

# C 编译标志
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu11")

# C++ 编译标志
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=gnu++11 -fno-rtti -fno-threadsafe-statics")

# ASM 编译标志
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS} -x assembler-with-cpp -MMD -MP")

# 优化标志
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -g0 -fomit-frame-pointer  -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -g0 -fomit-frame-pointer  -fno-strict-aliasing")
# 添加链接时优化
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -flto -fuse-linker-plugin")
# 链接标志
set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32H750XX_FLASH.ld\"")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--relax")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--gc-sections")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")
set(COMMON_FLAGS "${COMMON_FLAGS} -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables")
set(COMMON_FLAGS "${COMMON_FLAGS} -fno-unwind-tables -fno-exception-tables")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--no-eh-frame-hdr")
# set(CMAKE_SYSTEM_NAME               Generic)
# set(CMAKE_SYSTEM_PROCESSOR          arm)

# # set(CMAKE_C_COMPILER_ID GNU)
# # set(CMAKE_CXX_COMPILER_ID GNU)
# set(CMAKE_C_COMPILER_ID Clang)
# set(CMAKE_CXX_COMPILER_ID Clang)
# # Some default GCC settings
# # arm-none-eabi- must be part of path environment
# # set(TOOLCHAIN_PREFIX                arm-none-eabi-)

# # set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc)
# # set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
# # set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++)
# # set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}g++)
# # set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy)
# # set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size)

# set(CMAKE_SYSTEM_NAME      Generic)
# set(CMAKE_SYSTEM_PROCESSOR arm)

# # 指定编译器前缀/路径（如已加入 PATH 可只写文件名）
# set(TOOLCHAIN_PREFIX arm-st-)          # 如果你用的是 ST 的命名习惯
# # set(TOOLCHAIN_PREFIX starm-)         # 你的目录里实际是 starm- 前缀

# # C / C++ / 汇编 编译器
# set(CMAKE_C_COMPILER      starm-clang)      # 或 ${TOOLCHAIN_PREFIX}clang
# set(CMAKE_CXX_COMPILER    starm-clang++)    # 或 ${TOOLCHAIN_PREFIX}clang++
# set(CMAKE_ASM_COMPILER    starm-clang)      # clang 也能当汇编器用

# # 链接器、objcopy、size 等工具
# set(CMAKE_LINKER          starm-lld)        # 也可以用 starm-clang 当链接驱动
# set(CMAKE_OBJCOPY         starm-objcopy)
# set(CMAKE_SIZE            starm-size)

# # 让 CMake 用 clang 的交叉编译模式
# # 使用正确的 target triple（针对 STM32H7 cortex-m7）
# set(CMAKE_C_COMPILER_TARGET   thumbv7m-st-none-eabihf)
# set(CMAKE_CXX_COMPILER_TARGET thumbv7m-st-none-eabihf)
# set(CMAKE_ASM_COMPILER_TARGET thumbv7m-st-none-eabihf)


# set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
# set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
# set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

# set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# # MCU specific flags
# set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ")

# # 添加编译器 sysroot（指向 arm-none-eabi 的库路径）
# set(CMAKE_FIND_ROOT_PATH "/opt/stm32-tools" CACHE STRING "sysroot")
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
# set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections")

# # 编码支持：添加 UTF-8 编码标志
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexec-charset=UTF-8 -finput-charset=UTF-8")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wno-invalid-source-encoding -Wno-invalid-utf8")
# # 禁用 TLS 和异常处理
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti")
# set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
# # set(CMAKE_C_FLAGS_RELEASE "-O3 -g0")
# set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -g0  -fno-asynchronous-unwind-tables -fomit-frame-pointer")
# set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
# # set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -g0  -fno-asynchronous-unwind-tables -fomit-frame-pointer")

# set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

# set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32H750XX_FLASH.ld\"")
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

# set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")
