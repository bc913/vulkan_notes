if(${_OPT}AUTO_LOCATE_VULKAN)
    # Find package first
    find_package(Vulkan GLOBAL COMPONENTS glslc)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan SDK NOT found.")
    else()
        message(STATUS "Vulkan SDK ${Vulkan_VERSION} found.")
        message(STATUS "Vulkan glslc found: ${Vulkan_glslc_FOUND}")
    endif()
else()
    message(FATAL_ERROR "Vulkan dependency should be handled to proceed!!!")
endif()

# https://copyprogramming.com/howto/cmake-invocation-of-glslc-with-respect-to-includes-dependencies
# https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
macro(APPEND_GLSL_TO_TARGET target glsl_files_dir)
    if(EXISTS "${glsl_files_dir}")
        
        set(SPIRV_BINARY_FILES_RELATIVE_DIR "assets/shaders")
        set(SPIRV_BINARY_FILES_PROJECT_TARGET_DIR ${PROJECT_BINARY_DIR}/${SPIRV_BINARY_FILES_RELATIVE_DIR})
        set(SPIRV_BINARY_FILES)


        # Get the glsl files
        file(GLOB_RECURSE glsl_source_files "${glsl_files_dir}/*.glsl")
        # Generate spv files for each
        foreach(glsl_file ${glsl_source_files})
            cmake_path(GET glsl_file FILENAME filename) # Removes the parent path
            #message("filename: ${filename}")
            cmake_path(GET filename STEM LAST_ONLY filename_final) # Removes the last extension (.glsl)
            #message("filename_final: ${filename_final}")
            set(spv_file_name ${filename_final})
            set(spv_file_path ${SPIRV_BINARY_FILES_PROJECT_TARGET_DIR}/${spv_file_name}.spv)
            cmake_path(GET filename_final EXTENSION LAST_ONLY extension) # Get the extension again (.vert)
            string(REPLACE "." "" glsl_type ${extension}) # Remove "."
            #message("glsl_type: ${glsl_type}")

            add_custom_command(
                COMMENT "Building shader: ${spv_file_name} ..."
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIRV_BINARY_FILES_PROJECT_TARGET_DIR}
                COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=${glsl_type} ${glsl_file} -o ${spv_file_path}
                DEPENDS ${glsl_file} 
                OUTPUT ${spv_file_path}
                VERBATIM
            )
            list(APPEND SPIRV_BINARY_FILES ${spv_file_path})
        endforeach()

        # Define target for this shaders
        add_custom_target( Shaders
            DEPENDS ${SPIRV_BINARY_FILES}
	    )
        # Make the target dependent to shaders
        add_dependencies(${target} Shaders)
        # Copy shader files to target location
        add_custom_command(
            TARGET ${target} 
            POST_BUILD
            COMMENT "Copying shader binaries to ${target} ..."
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/${SPIRV_BINARY_FILES_RELATIVE_DIR}
	        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${SPIRV_BINARY_FILES_PROJECT_TARGET_DIR}
                $<TARGET_FILE_DIR:${target}>/${SPIRV_BINARY_FILES_RELATIVE_DIR}
            VERBATIM
        )
    
    else()
        message(FATAL_ERROR "Given path: ${glsl_files_dir} does not exist")

    endif()
endmacro()