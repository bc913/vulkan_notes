include(FetchContent)
# --------------------------------
# Declare
# --------------------------------
# GLM
FetchContent_Declare(
        glm 
        GIT_REPOSITORY https://github.com/g-truc/glm.git 
        GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e #0.9.9.8
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

if(${_OPT}USE_GLAD_LOADER)
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
elseif(${_OPT}USE_VOLK_LOADER)
    # Volk meta loader
    set(VOLK_HEADERS_ONLY ON CACHE BOOL "" FORCE)
    set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk.git 
        GIT_TAG b3bc21e584f97400b6884cb2a541a56c6a5ddba3 #sdk-1.3.250
    )
    if(NOT ${_OPT}AUTO_LOCATE_VULKAN)
        # Vulkan_headers
        FetchContent_Declare(
            vulkan_headers 
            GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git 
            GIT_TAG 85c2334e92e215cce34e8e0ed8b2dce4700f4a50 #sdk-1.3.261.1
        )
    endif()
endif()

# --------------------------------
# Make Available
# --------------------------------
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(glfw)

if(${_OPT}USE_GLAD_LOADER)
    FetchContent_MakeAvailable(glad)
elseif(${_OPT}USE_VOLK_LOADER)
    FetchContent_MakeAvailable(volk)
    if(NOT ${_OPT}AUTO_LOCATE_VULKAN)
        FetchContent_MakeAvailable(vulkan_headers)
    endif()
endif()