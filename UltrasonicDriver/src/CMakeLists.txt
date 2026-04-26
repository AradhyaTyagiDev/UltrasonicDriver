# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
# Component-level file. This tells ESP-IDF which source files belong to the src component.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

list(FILTER app_sources EXCLUDE REGEX ".*/mock/.*")
list(FILTER app_sources EXCLUDE REGEX ".*/drivers/esp32_arduino_isr/.*")

idf_component_register(SRCS ${app_sources})
