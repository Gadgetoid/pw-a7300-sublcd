cmake_minimum_required(VERSION 3.20)
project(pw_a7300_sublcd LANGUAGES C)

if(NOT JORNADA_TOOLCHAIN)
    message(FATAL_ERROR "Set JORNADA_TOOLCHAIN to the jornada-72x-tools directory")
endif()

find_program(NODE_EXECUTABLE node REQUIRED)
list(APPEND CMAKE_MODULE_PATH "${JORNADA_TOOLCHAIN}/cmake")
include(JornadaCE)

set(CARD_DIRECTORY "${CMAKE_BINARY_DIR}/card")
set(DESKTOP_DIRECTORY "${CARD_DIRECTORY}/Apps/SubLCD")
set(DICTIONARY_DIRECTORY "${CARD_DIRECTORY}/アプリ")

set(IMPORT_DIRECTORY "${CMAKE_BINARY_DIR}/imports")
execute_process(
    COMMAND "${NODE_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/toolchain/prepare-imports.mjs"
        "${CMAKE_CURRENT_LIST_DIR}/toolchain/ce-imports" "${IMPORT_DIRECTORY}"
    COMMAND_ERROR_IS_FATAL ANY)
set(coredll_stubs "${IMPORT_DIRECTORY}/coredll.S")
add_custom_command(OUTPUT "${coredll_stubs}"
    COMMAND "${Python3_EXECUTABLE}" "${JORNADA_TOOLCHAIN}/tools/mkimplib.py"
        "${IMPORT_DIRECTORY}/coredll.txt" "coredll.dll" "${coredll_stubs}"
    DEPENDS "${IMPORT_DIRECTORY}/coredll.txt"
    VERBATIM)
add_library(import_coredll STATIC "${coredll_stubs}")

add_library(sublcd STATIC
    sublcd/sublcd.c
    sublcd/draw.c
    sublcd/font.c
    sublcd/button.c
    sublcd/backlight.c
    sublcd/buzzer.c
    sublcd/touch.c
    sublcd/window.c)
target_include_directories(sublcd PUBLIC sublcd)
target_compile_options(sublcd PUBLIC -mcpu=arm926ej-s -Wall -Werror -Wno-unused-parameter)
target_compile_definitions(sublcd PUBLIC UNICODE _UNICODE UNDER_CE WINCE _WINDOWS JORNADA_ALL_DECLARATIONS)
target_link_libraries(sublcd PUBLIC jornada::headers)

function(sublcd_app source_directory directory name)
    set(target ${source_directory}_${directory})
    set(linker_script "${CMAKE_CURRENT_LIST_DIR}/toolchain/exe.ld")
    set(desktop_executable "${DESKTOP_DIRECTORY}/${name}.exe")
    set(dictionary_app "${DICTIONARY_DIRECTORY}/SubLCD ${name}")
    add_executable(${target} ${source_directory}/${directory}/main.c)
    set_target_properties(${target} PROPERTIES SUFFIX ".elf")
    target_link_options(${target} PRIVATE -T "${linker_script}" --entry=WinMain)
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${linker_script}")
    target_link_libraries(${target} PRIVATE sublcd jornada::runtime import_coredll)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DESKTOP_DIRECTORY}" "${dictionary_app}"
        COMMAND "${Python3_EXECUTABLE}" "${JORNADA_TOOLCHAIN}/tools/mkpe.py"
            "$<TARGET_FILE:${target}>" "${desktop_executable}" --ce-version=5.1
        COMMAND "${CMAKE_COMMAND}" -E copy "${desktop_executable}" "${dictionary_app}/AppMain.exe"
        COMMAND "${CMAKE_COMMAND}" -E touch "${dictionary_app}/index.din"
        VERBATIM)
endfunction()

sublcd_app(examples hello Hello)
sublcd_app(examples drawing Drawing)
sublcd_app(examples backlight Backlight)
sublcd_app(examples keyboard Keyboard)
sublcd_app(examples snake Snake)
sublcd_app(examples greys Greys)

option(SUBLCD_EXPERIMENTS "Build experiments" OFF)
if(SUBLCD_EXPERIMENTS)
    sublcd_app(experiments compat-mode "Compat Mode")
    sublcd_app(experiments touch-log "Touch Log")
endif()
