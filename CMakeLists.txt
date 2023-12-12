cmake_minimum_required(VERSION 3.25)
project(vulkan_pg)
# Global includes
include(GNUInstallDirs)

# ---------
# Options
# ---------
set(_OPT "BC_")
option(${_OPT}USE_VOLK_LOADER "Use VOLK Meta loader" OFF)
option(${_OPT}USE_GLAD_LOADER "Use GLAD loader" OFF)
option(${_OPT}AUTO_LOCATE_VULKAN "Attempting to auto locate vulkan using CMake FindVulkan ..." ON)

# Volk and Glad are mutually exclusive
if(${_OPT}USE_GLAD_LOADER AND ${_OPT}USE_VOLK_LOADER)
    message(FATAL_ERROR "Glad and Volk loader can NOT be enabled at the same time.")
elseif(NOT ${_OPT}USE_GLAD_LOADER AND NOT ${_OPT}USE_VOLK_LOADER)
    # TODO: Make use of VULKAN SDK install and ENV variable
endif()

# ---------
# Includes
# ---------
## Custom
set(BC_VULKAN_NOTES_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${BC_VULKAN_NOTES_MODULE_PATH} ${CMAKE_MODULE_PATH})
include(HandleVulkanDeps)
# ---------
# Settings
# ---------
## C Lang
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET "hidden")
## Common
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(src)