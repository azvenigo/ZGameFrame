
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
../../ZLibraries/Common/helpers/FileLogger.h        ../../ZLibraries/Common/helpers/FileLogger.cpp
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
)

list(APPEND LINK_DIRS ../../ZLibraries/Common/curl/lib)
list(APPEND LINK_LIBS gdiplus.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libcurl.lib lunasvg DXGI.lib)


####################
# PROJECTS

set(LUNASVG_BUILD_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(external/lunasvg EXCLUDE_FROM_ALL)
add_subdirectory(SandboxApps)
add_subdirectory(ZImageViewer)


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
