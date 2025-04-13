
################################################################################

cmake_minimum_required(VERSION 3.15)

################################################################################

project(ZGameFrame)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_SUPPRESS_REGENERATION true)

####################
# VISUAL C MT MD CONFIGURATION

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp")
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set(CMAKE_CXX_FLAGS_MDDEBUG   "${CMAKE_CXX_FLAGS_DEBUG}")
	set(CMAKE_CXX_FLAGS_MDRELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	STRING(REPLACE "/MD"  "/MT"  CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")


	set(CMAKE_EXE_LINKER_FLAGS_MDRELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
	set(CMAKE_EXE_LINKER_FLAGS_MDDEBUG      "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

else()

	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

endif()




set(COMMON_FILES 
../../ZLibraries/Common/helpers/FileHelpers.h				../../ZLibraries/Common/helpers/FileHelpers.cpp
../../ZLibraries/Common/helpers/FNMatch.h           ../../ZLibraries/Common/helpers/FNMatch.cpp 
../../ZLibraries/Common/helpers/HTTPCache.h         ../../ZLibraries/Common/helpers/HTTPCache.cpp
../../ZLibraries/Common/helpers/CommandLineParser.h ../../ZLibraries/Common/helpers/CommandLineParser.cpp
../../ZLibraries/Common/helpers/CommandLineEditor.h ../../ZLibraries/Common/helpers/CommandLineEditor.cpp
../../ZLibraries/Common/helpers/LoggingHelpers.h 		../../ZLibraries/Common/helpers/LoggingHelpers.cpp
../../ZLibraries/Common/helpers/StringHelpers.h     ../../ZLibraries/Common/helpers/StringHelpers.cpp 
../../ZLibraries/Common/helpers/ThreadPool.h
../../ZLibraries/Common/helpers/Registry.h          ../../ZLibraries/Common/helpers/Registry.cpp
../../ZLibraries/Common/helpers/Logger.h            ../../ZLibraries/Common/helpers/Logger.cpp
../../ZLibraries/Common/json/json.hpp
../../ZLibraries/Common/stb/stb_image.h
../../ZLibraries/Common/stb/stb_image_write.h
../../ZLibraries/Common/easyexif/exif.h             ../../ZLibraries/Common/easyexif/exif.cpp
../../ZLibraries/Common/helpers/ZZFileApi.h         ../../ZLibraries/Common/helpers/ZZFileApi.cpp
)

add_library(mio INTERFACE External/mio)
add_library(mio::mio ALIAS mio)


#if(MSVC)
list(APPEND LINK_LIBS Version.lib)
add_definitions(/wd4251)
#endif()


list(APPEND COMMON_FILES  ../external/zlib-1.2.11/deflate.c  ../external/zlib-1.2.11/inflate.c 
../external/zlib-1.2.11/adler32.c ../external/zlib-1.2.11/zutil.c ../external/zlib-1.2.11/crc32.c 
../external/zlib-1.2.11/trees.c ../external/zlib-1.2.11/inftrees.c ../external/zlib-1.2.11/inffast.c)

####################
# common source
list(APPEND INCLUDE_DIRS 
../ZFramework 
../../ZLibraries/Common 
../../ZLibraries/Common/zlib-1.2.11 
../../ZLibraries/Common/json 
../../ZLibraries/Common/stb
../../ZLibraries/Common/curl/include
../ZFramework/platforms/windows
../external
../external/lunasvg/include
)

list(APPEND LINK_DIRS ../../ZLibraries/Common/curl/lib ../external/lunasvg/build/debug)
list(APPEND LINK_LIBS gdiplus.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libcurl.lib lunasvg.lib DXGI.lib d3d11.lib)




function(add_shader_compilation_target TARGET_NAME)
    # Create output directory with configuration
    set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/res/shaders)
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
    
    # Define shared shader source directory
    set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/ZFramework/default_resources/shaders)
    
    # Define shader files and their types
    set(SHADER_COMPILE_COMMANDS)
    set(SHADER_OUTPUT_FILES)
    
    # Vertex shader compilation
    set(VS_OUTPUT ${SHADER_OUTPUT_DIR}/v_ScreenSpaceShader.cso)
    add_custom_command(
        OUTPUT ${VS_OUTPUT}
        COMMAND fxc.exe /T vs_5_0 /E "VSMain" /Fo ${VS_OUTPUT} ${SHADER_SOURCE_DIR}/v_ScreenSpaceShader.hlsl
        MAIN_DEPENDENCY ${SHADER_SOURCE_DIR}/v_ScreenSpaceShader.hlsl
        COMMENT "Compiling vertex shader for ${TARGET_NAME}"
    )
    list(APPEND SHADER_OUTPUT_FILES ${VS_OUTPUT})
    
    # Pixel shader compilation
    set(PS_OUTPUT ${SHADER_OUTPUT_DIR}/p_ScreenSpaceShader.cso)
    add_custom_command(
        OUTPUT ${PS_OUTPUT}
        COMMAND fxc.exe /T ps_5_0 /E "PSMain" /Fo ${PS_OUTPUT} ${SHADER_SOURCE_DIR}/p_ScreenSpaceShader.hlsl
        MAIN_DEPENDENCY ${SHADER_SOURCE_DIR}/p_ScreenSpaceShader.hlsl
        COMMENT "Compiling pixel shader for ${TARGET_NAME}"
    )
    list(APPEND SHADER_OUTPUT_FILES ${PS_OUTPUT})
    
    # Compute shaders compilation
    set(CS1_OUTPUT ${SHADER_OUTPUT_DIR}/c_GaussianBlur_Horizontal.cso)
    add_custom_command(
        OUTPUT ${CS1_OUTPUT}
        COMMAND fxc.exe /T cs_5_0 /E "CSMain" /Fo ${CS1_OUTPUT} ${SHADER_SOURCE_DIR}/c_GaussianBlur_Horizontal.hlsl
        MAIN_DEPENDENCY ${SHADER_SOURCE_DIR}/c_GaussianBlur_Horizontal.hlsl
        COMMENT "Compiling horizontal blur compute shader for ${TARGET_NAME}"
    )
    list(APPEND SHADER_OUTPUT_FILES ${CS1_OUTPUT})
    
    set(CS2_OUTPUT ${SHADER_OUTPUT_DIR}/c_GaussianBlur_Vertical.cso)
    add_custom_command(
        OUTPUT ${CS2_OUTPUT}
        COMMAND fxc.exe /T cs_5_0 /E "CSMain" /Fo ${CS2_OUTPUT} ${SHADER_SOURCE_DIR}/c_GaussianBlur_Vertical.hlsl
        MAIN_DEPENDENCY ${SHADER_SOURCE_DIR}/c_GaussianBlur_Vertical.hlsl
        COMMENT "Compiling vertical blur compute shader for ${TARGET_NAME}"
    )
    list(APPEND SHADER_OUTPUT_FILES ${CS2_OUTPUT})
    
    # Create the custom target that depends on all shader outputs
    add_custom_target(${TARGET_NAME}_ShaderCompilation 
        DEPENDS ${SHADER_OUTPUT_FILES}
    )
    
    # Add dependency to the main target
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_ShaderCompilation)
endfunction()


####################
# PROJECTS

add_subdirectory(SandboxApps)
add_subdirectory(ZImageViewer)
#add_subdirectory(External/lunasvg)


set (SHADER_IN ${CMAKE_CURRENT_LIST_DIR}/ZFramework/default_resources/shaders)
set (SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/res/shaders)

#add_custom_command(
#    OUTPUT ${SHADER_OUT}/v_ScreenSpaceShader.cso
#    COMMAND fxc.exe /T vs_5_0 /E "VSMain" /Fo ${SHADER_OUT}/v_ScreenSpaceShader.cso ${SHADER_IN}/v_ScreenSpaceShader.hlsl
#    MAIN_DEPENDENCY ${SHADER_IN}/v_ScreenSpaceShader.hlsl
#    COMMENT "Compiling vertex shader"
#)
#
#add_custom_command(
#    OUTPUT ${SHADER_OUT}/p_ScreenSpaceShader.cso
#    COMMAND fxc.exe /T ps_5_0 /E "PSMain" /Fo ${SHADER_OUT}/p_ScreenSpaceShader.cso ${CMAKE_CURRENT_LIST_DIR}/ZFramework/default_resources/shaders/p_ScreenSpaceShader.hlsl
#    MAIN_DEPENDENCY ${SHADER_IN}/p_ScreenSpaceShader.hlsl
#    COMMENT "Compiling pixel shader"
#)
#
#add_custom_command(
#    OUTPUT ${SHADER_OUT}/c_GaussianBlur_Horizontal.cso
#    COMMAND fxc.exe /T cs_5_0 /E "CSMain" /Fo ${SHADER_OUT}/c_GaussianBlur_Horizontal.cso ${CMAKE_CURRENT_LIST_DIR}/ZFramework/default_resources/shaders/c_GaussianBlur_Horizontal.hlsl
#    MAIN_DEPENDENCY ${SHADER_IN}/c_GaussianBlur_Horizontal.hlsl
#    COMMENT "Compiling compute shader"
#)
#
#add_custom_command(
#    OUTPUT ${SHADER_OUT}/c_GaussianBlur_Vertical.cso
#    COMMAND fxc.exe /T cs_5_0 /E "CSMain" /Fo ${SHADER_OUT}/c_GaussianBlur_Vertical.cso ${SHADER_IN}/c_GaussianBlur_Vertical.hlsl
#    MAIN_DEPENDENCY ${SHADER_IN}/c_GaussianBlur_Vertical.hlsl
#    COMMENT "Compiling compute shader"
#)
#
#
#
#add_custom_target(ShaderCompilation 
#    DEPENDS 
#    ${SHADER_OUT}/v_ScreenSpaceShader.cso
#    ${SHADER_OUT}/p_ScreenSpaceShader.cso
#    ${SHADER_OUT}/c_GaussianBlur_Horizontal.cso
#    ${SHADER_OUT}/c_GaussianBlur_Vertical.cso
#)


set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ZImageViewer)
################################################################################
