project(vulkan_console_app LANGUAGES C)

add_executable(${PROJECT_NAME} "")

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # or ${PROJECT_SOURCE_DIR}/include
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> # or include
)
set(_SOURCE_FILES)
list(APPEND _SOURCE_FILES 
            main.cpp 
            vulkan_renderer.cpp
            defines.h
            log_assert.h
)
target_sources(${PROJECT_NAME} PRIVATE ${_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE glm glfw Vulkan::Headers volk::volk_headers)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    #PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # Windows ONly
)