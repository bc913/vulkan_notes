project(vulkan_console_app LANGUAGES C)

# https://github.com/Dav1dde/glad/tree/glad2/example/c/vulkan_tri_glfw

# Glad Setup
# For Glad 2.0, use glad generator to create file and manually add it.
# Remove the following commands for glad v2.0
FetchContent_Declare(
        glad 
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG d08b1aa01f8fe57498f04d47b5fa8c48725be877 #v2.0.4
        SOURCE_SUBDIR cmake # use for glad 2.0 and above
        #GIT_TAG 1ecd45775d96f35170458e6b148eb0708967e402 #v0.1.36
)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw 
    GIT_REPOSITORY https://github.com/glfw/glfw.git 
    GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e #v3.3.8
)
# Since glad is the loader, no need to bring Vulkan::Headers
glad_add_library(glad_vulkan_13 REPRODUCIBLE LOADER API vulkan=1.3)
add_executable(${PROJECT_NAME} "")
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE glm glad_vulkan_13 glfw)
