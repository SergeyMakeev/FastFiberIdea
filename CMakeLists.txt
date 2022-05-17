cmake_minimum_required(VERSION 2.8.12)

# if (MSVC)
#   set(CMAKE_VS_GLOBALS "EnableASAN=true")
# endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PROJ_NAME FastFiberTest)
project(${PROJ_NAME} C)
if(MSVC)
  enable_language(CXX ASM_MASM)
else()
  enable_language(CXX ASM)
endif()

set (CMAKE_CXX_STANDARD 17)

set(CPP_SOURCES 
  FastFiberTest.cpp
)

# OS
if (APPLE)
    set(CPU_ARCH "combined")
    set(ASM_EXT "all_macho_gas.S")
elseif (ANDROID)
    # Android
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        set(CPU_ARCH "arm")
        set(ASM_EXT "aapcs_elf_gas.S")
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
        set(CPU_ARCH "arm64")
        set(ASM_EXT "aapcs_elf_gas.S")
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "i686")
        set(CPU_ARCH "i386")
        set(ASM_EXT "sysv_elf_gas.S")
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
        set(CPU_ARCH "x86_64")
        set(ASM_EXT "sysv_elf_gas.S")
    endif()
elseif (UNIX)
    # PC (x86/x64)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPU_ARCH "x86_64")
    else()
        set(CPU_ARCH "i386")
    endif()
    set(ASM_EXT "sysv_elf_gas.S")  # Linux/Unix
elseif (WIN32)
    # Windows PC
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPU_ARCH "x86_64")
    else()
        set(CPU_ARCH "i386")
    endif()
    set(ASM_EXT "ms_pe_masm.asm")
endif()

set(ASM_SOURCES "asm/save_${CPU_ARCH}_${ASM_EXT}"
                "asm/make_${CPU_ARCH}_${ASM_EXT}"
                "asm/restore_${CPU_ARCH}_${ASM_EXT}")

set (APP_NAME ${PROJ_NAME})
add_executable(${APP_NAME} ${CPP_SOURCES} ${ASM_SOURCES})

# if(MSVC)
#   target_compile_options(${APP_NAME} PRIVATE /W4 /WX)
#   add_definitions(-D_CRT_SECURE_NO_WARNINGS)
# else()
#   target_compile_options(${APP_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
# endif()

