
################################################################################

cmake_minimum_required(VERSION 3.7.1)


set(CMAKE_CXX_STANDARD 17)


################################################################################

project(ZImageViewer)

####################
# CHECK IS 64BIT BUILD

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
else()
	message(FATAL_ERROR "32bit Unsupported")
endif()


####################
# ZImageViewer

set(ZIMAGEVIEWER_SOURCES 
Main_ZImageViewer.h Main_ZImageViewer.cpp Main_Win64.cpp 
ImageViewer.h ImageViewer.cpp
ZImageViewer.rc 
resource_win64.h
res/ZImageViewer.ico
)

set (GUI_FILES
../ZFramework/ZWin.h                ../ZFramework/ZWin.cpp
../ZFramework/ZWinBtn.h             ../ZFramework/ZWinBtn.cpp
../ZFramework/ZWinDebugConsole.h    ../ZFramework/ZWinDebugConsole.cpp
../ZFramework/ZWinFormattedText.h   ../ZFramework/ZWinFormattedText.cpp
../ZFramework/ZWinImage.h           ../ZFramework/ZWinImage.cpp
../ZFramework/ZMainWin.h            ../ZFramework/ZMainWin.cpp
../ZFramework/ZWinScriptedDialog.h  ../ZFramework/ZWinScriptedDialog.cpp
../ZFramework/ZWinSlider.h          ../ZFramework/ZWinSlider.cpp
../ZFramework/ZWinControlPanel.h    ../ZFramework/ZWinControlPanel.cpp
../ZFramework/ZWinWatchPanel.h      ../ZFramework/ZWinWatchPanel.cpp
../ZFramework/ZWinText.h            ../ZFramework/ZWinText.cpp
../ZFramework/ZWinFileDialog.h      ../ZFramework/platforms/windows/ZWinFileDialog_Windows.cpp
../ZFramework/ZWinPaletteDialog.h   ../ZFramework/ZWinPaletteDialog.cpp

../ZFramework/ZBuffer.h             ../ZFramework/ZBuffer.cpp
../ZFramework/ZFloatColorBuffer.h   ../ZFramework/ZFloatColorBuffer.cpp

../ZFramework/ZGraphicSystem.h      ../ZFramework/ZGraphicSystem.cpp
../ZFramework/ZGUIHelpers.h         ../ZFramework/ZGUIHelpers.cpp
../ZFramework/ZGUIStyle.h           ../ZFramework/ZGUIStyle.cpp

../ZFramework/ZRasterizer.h         ../ZFramework/ZRasterizer.cpp
../ZFramework/ZScreenBuffer.h       ../ZFramework/ZScreenBuffer.cpp
../ZFramework/ZFont.h               ../ZFramework/ZFont.cpp
../ZFramework/ZColor.h
)


set(ZFRAMEWORK_FILES

../ZFramework/ZAssert.h
../ZFramework/ZDebug.h
../ZFramework/ZRandom.h
../ZFramework/ZTypes.h
../ZFramework/Resources.h           ../ZFramework/Resources.cpp
../ZFramework/ZAnimator.h           ../ZFramework/ZAnimator.cpp
../ZFramework/ZAnimObjects.h        ../ZFramework/ZAnimObjects.cpp
../ZFramework/ZBitBuffer.cpp
../ZFramework/ZBitBuffer.h
../ZFramework/ZCompression.cpp
../ZFramework/ZCompression.h
../ZFramework/zlibAPI.cpp
../ZFramework/zlibAPI.h
../ZFramework/ZMemBuffer.h          ../ZFramework/ZMemBuffer.cpp
../ZFramework/ZMessageSystem.h      ../ZFramework/ZMessageSystem.cpp
../ZFramework/Z3DMath.h
../ZFramework/ZStringHelpers.h      ../ZFramework/ZStringHelpers.cpp
../ZFramework/ZTickManager.h        ../ZFramework/ZTickManager.cpp
../ZFramework/ZTimer.h              ../ZFramework/ZTimer.cpp
../ZFramework/ZTransformable.h      ../ZFramework/ZTransformable.cpp
../ZFramework/ZXMLNode.h            ../ZFramework/ZXMLNode.cpp
../ZFramework/ZZipAPI.h             ../ZFramework/ZZipAPI.cpp
../ZFramework/platforms/windows/GDIImageTags.h
../ZFramework/ZFramework.natvis
)

set(COMMON_FILES ../../ZLibraries/Common/helpers/FNMatch.h ../../ZLibraries/Common/helpers/FNMatch.cpp ../../ZLibraries/Common/helpers/HTTPCache.h ../../ZLibraries/Common/helpers/HTTPCache.cpp)
list(APPEND COMMON_FILES 
../../ZLibraries/Common/helpers/CommandLineParser.h 
../../ZLibraries/Common/helpers/CommandLineParser.cpp 
../../ZLibraries/Common/helpers/StringHelpers.h 
../../ZLibraries/Common/helpers/StringHelpers.cpp 
../../ZLibraries/Common/helpers/ThreadPool.h
../../ZLibraries/Common/helpers/Registry.h
../../ZLibraries/Common/helpers/Registry.cpp
../../ZLibraries/Common/json/json.hpp
../../ZLibraries/Common/stb/stb_image.h
../../ZLibraries/Common/stb/stb_image_write.h
../../ZLibraries/Common/easyexif/exif.h ../../ZLibraries/Common/easyexif/exif.cpp

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
../../ZLibraries/Common/stb
../ZFramework/platforms/windows
)


####################
# PROJECT DEPENDENCY AND INCLUDES
list(APPEND LINK_LIBS gdiplus.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib )


####################
# source and include sets

set(INCLUDES ${INCLUDES} ${ZIMAGEVIEWER_HEADERS} ${COMMON_HEADERS})
set(SOURCES ${ZIMAGEVIEWER_SOURCES} ${COMMON_FILES} ${GUI_FILES} ${ZFRAMEWORK_FILES})


####################
# GUI groups

source_group(Common FILES ${COMMON_FILES})
source_group(ZFramework FILES ${ZFRAMEWORK_FILES})
source_group(ZFramework_GUI FILES ${GUI_FILES})
source_group(ZImageViewer FILES ${ZIMAGEVIEWER_SOURCES} ${ZIMAGEVIEWER_HEADERS})


####################
# EXTRA FLAGS

if(MSVC)
    # ignore pdb not found
    set(EXTRA_FLAGS "/W3 /MP")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ignore:4099")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(JSON_NOEXCEPTION)
    set(TARGET_PROPS  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:ZImageViewer>")
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

add_executable(ZImageViewer WIN32 ${SOURCES} ${INCLUDES})
target_link_libraries(ZImageViewer ${LINK_LIBS})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/res" $<TARGET_FILE_DIR:${PROJECT_NAME}>/res)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/../ZFramework/default_resources" $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/default_resources)
set_target_properties(ZImageViewer ${TARGET_PROPS})
    