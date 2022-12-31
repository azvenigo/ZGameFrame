
################################################################################

cmake_minimum_required(VERSION 3.7.1)


set(CMAKE_CXX_STANDARD 17)

################################################################################

project(SandboxApps)

####################
# CHECK IS 64BIT BUILD

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
else()
	message(FATAL_ERROR "32bit Unsupported")
endif()


####################
# SandboxApps

set(SANDBOXAPPS_SOURCES 
FloatLinesWin.h FloatLinesWin.cpp LifeWin.h LifeWin.cpp 
Main_Sandbox.h Main_Sandbox.cpp Main_Win64.cpp 3DTestWin.h 3DTestWin.cpp 
ProcessImageWin.h ProcessImageWin.cpp TextTestWin.h TextTestWin.cpp)

set(COMMON_FILES 

../ZFramework/Resources.cpp
../ZFramework/Resources.h
../ZFramework/ZAnimator.cpp
../ZFramework/ZAnimator.h
../ZFramework/ZAnimObjects.cpp
../ZFramework/ZAnimObjects.h
../ZFramework/ZAssert.h
../ZFramework/ZBitBuffer.cpp
../ZFramework/ZBitBuffer.h
../ZFramework/ZBuffer.cpp
../ZFramework/ZBuffer.h
../ZFramework/ZFloatColorBuffer.cpp
../ZFramework/ZFloatColorBuffer.h
../ZFramework/ZCompression.cpp
../ZFramework/ZCompression.h
../ZFramework/ZFont.cpp
../ZFramework/ZFont.h
../ZFramework/ZFormattedTextWin.cpp
../ZFramework/ZFormattedTextWin.h
../ZFramework/ZGraphicSystem.cpp
../ZFramework/ZGraphicSystem.h
../ZFramework/ZImageWin.cpp
../ZFramework/ZImageWin.h
../ZFramework/zlib-1.2.11
../ZFramework/zlibAPI.cpp
../ZFramework/zlibAPI.h
../ZFramework/ZMainWin.cpp
../ZFramework/ZMainWin.h
../ZFramework/ZMemBuffer.cpp
../ZFramework/ZMemBuffer.h
../ZFramework/ZMessageSystem.cpp
../ZFramework/ZMessageSystem.h
../ZFramework/ZRandom.h
../ZFramework/ZRasterizer.cpp
../ZFramework/ZRasterizer.h
../ZFramework/ZScreenBuffer.cpp
../ZFramework/ZScreenBuffer.h
../ZFramework/ZScriptedDialogWin.cpp
../ZFramework/ZScriptedDialogWin.h
../ZFramework/ZSliderWin.cpp
../ZFramework/ZSliderWin.h
../ZFramework/ZStdDebug.h
../ZFramework/ZStdTypes.h
../ZFramework/Z3DMath.h
../ZFramework/ZStringHelpers.cpp
../ZFramework/ZStringHelpers.h
../ZFramework/ZTickManager.cpp
../ZFramework/ZTickManager.h
../ZFramework/ZTimer.cpp
../ZFramework/ZTimer.h
../ZFramework/ZTransformable.cpp
../ZFramework/ZTransformable.h
../ZFramework/ZWin.CPP
../ZFramework/ZWin.H
../ZFramework/ZWinBtn.CPP
../ZFramework/ZWinBtn.H
../ZFramework/ZWinControlPanel.cpp
../ZFramework/ZWinControlPanel.h
../ZFramework/ZWinWatchPanel.cpp
../ZFramework/ZWinWatchPanel.h
../ZFramework/ZXMLNode.cpp
../ZFramework/ZXMLNode.h
../ZFramework/ZZipAPI.cpp
../ZFramework/ZZipAPI.h
../ZFramework/platforms/windows/GDIImageTags.h
)

list(APPEND COMMON_FILES ../../ZLibraries/Common/helpers/FNMatch.h ../../ZLibraries/Common/helpers/FNMatch.cpp ../../ZLibraries/Common/helpers/HTTPCache.h ../../ZLibraries/Common/helpers/HTTPCache.cpp)
list(APPEND COMMON_FILES 
../../ZLibraries/Common/helpers/CommandLineParser.h 
../../ZLibraries/Common/helpers/CommandLineParser.cpp 
../../ZLibraries/Common/helpers/StringHelpers.h 
../../ZLibraries/Common/helpers/StringHelpers.cpp 
../../ZLibraries/Common/helpers/ThreadPool.h
../../ZLibraries/Common/helpers/Registry.h
../../ZLibraries/Common/helpers/Registry.cpp
../../ZLibraries/Common/json/json.hpp
)

list(APPEND COMMON_FILES  ../../ZLibraries/Common/zlib-1.2.11/deflate.c  ../../ZLibraries/Common/zlib-1.2.11/inflate.c 
../../ZLibraries/Common/zlib-1.2.11/adler32.c ../../ZLibraries/Common/zlib-1.2.11/zutil.c ../../ZLibraries/Common/zlib-1.2.11/crc32.c 
../../ZLibraries/Common/zlib-1.2.11/trees.c ../../ZLibraries/Common/zlib-1.2.11/inftrees.c ../../ZLibraries/Common/zlib-1.2.11/inffast.c)


####################
# common source
list(APPEND INCLUDE_DIRS 
../ZFramework 
../../ZLibraries/Common 
../../ZLibraries/Common/zlib-1.2.11 
../../ZLibraries/Common/json 
../ZFramework/platforms/windows
)


####################
# PROJECT DEPENDENCY AND INCLUDES
list(APPEND LINK_LIBS gdiplus.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib )


####################
# source and include sets

set(INCLUDES ${INCLUDES} ${SANDBOXAPPS_HEADERS} ${COMMON_HEADERS})
set(SOURCES ${SANDBOXAPPS_SOURCES} ${COMMON_FILES})


####################
# GUI groups

#source_group(SandboxApps FILES ${SANDBOXAPPS_SOURCES} ${SANDBOXAPPS_HEADERS})
source_group(Common FILES ${COMMON_FILES})


####################
# EXTRA FLAGS

if(MSVC)
    # ignore pdb not found
    set(EXTRA_FLAGS "/W3 /MP")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(JSON_NOEXCEPTION)
    set(TARGET_PROPS  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:SandboxApps>")
else()
    set(EXTRA_FLAGS "-Wall -Wextra -Werror -march=x86-64 -pthread")
    if( SYMBOLS ) 
        set(EXTRA_FLAGS "-g ${EXTRA_FLAGS}")
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_FLAGS} ${EXTRA_CXX_FLAGS}")

####################
# ADDITIONAL LIBRARIES

####################
# PROJECT

link_directories(${LINK_DIRS})
include_directories(${INCLUDE_DIRS})

add_executable(SandboxApps WIN32 ${SOURCES} ${INCLUDES})
target_link_libraries(SandboxApps ${LINK_LIBS})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/res" $<TARGET_FILE_DIR:${PROJECT_NAME}>/res)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/../ZFramework/default_resources" $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/default_resources)
set_target_properties(SandboxApps ${TARGET_PROPS})
    