################################################################################
cmake_minimum_required(VERSION 3.15)
################################################################################

####################
# ZImageViewer
project(ZImageViewer)

set(ZIMAGEVIEWER_SOURCES 
	Main_ZImageViewer.h 		Main_ZImageViewer.cpp
	ImageViewer.h 					ImageViewer.cpp
	ImageContest.h 					ImageContest.cpp
	ImageMeta.h     				ImageMeta.cpp
	ConfirmDeleteDialog.h 	ConfirmDeleteDialog.cpp
	WinTopWinners.h 				WinTopWinners.cpp
	ZImageViewer.rc 
	resource_win64.h
	raw_resources/ZImageViewer.ico
)

set (GUI_FILES
../ZFramework/ZWin.h                ../ZFramework/ZWin.cpp
../ZFramework/ZWinBtn.h             ../ZFramework/ZWinBtn.cpp
../ZFramework/ZWinDebugConsole.h    ../ZFramework/ZWinDebugConsole.cpp
../ZFramework/ZWinFolderSelector.h  ../ZFramework/ZWinFolderSelector.cpp
../ZFramework/ZWinFormattedText.h   ../ZFramework/ZWinFormattedText.cpp
../ZFramework/ZWinImage.h           ../ZFramework/ZWinImage.cpp
../ZFramework/ZMainWin.h            ../ZFramework/ZMainWin.cpp
../ZFramework/ZWinPanel.h           ../ZFramework/ZWinPanel.cpp
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
../ZFramework/ZGUIElements.h        ../ZFramework/ZGUIElements.cpp

../ZFramework/ZThumbCache.h         ../ZFramework/ZThumbCache.cpp
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
../ZFramework/ZInput.h              ../ZFramework/ZInput.cpp
../ZFramework/platforms/windows/GDIImageTags.h
../ZFramework/platforms/windows/Main_Win64.cpp
../ZFramework/ZFramework.natvis
)




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
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ignore:4099")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(JSON_NOEXCEPTION ENABLE_LIB_CURL=1)
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

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/../external/lunasvg/build/$<CONFIG>/lunasvg.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/../external/lunasvg/build/_deps/plutovg-build/$<CONFIG>/plutovg.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/res" $<TARGET_FILE_DIR:${PROJECT_NAME}>/res)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/install" $<TARGET_FILE_DIR:${PROJECT_NAME}>/install)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/../ZFramework/default_resources" $<TARGET_FILE_DIR:${PROJECT_NAME}>/res)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}../../../ZLibraries/Common/curl/bin/libcurl.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}../../../ZLibraries/Common/libssh2/libssh2.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}../../../ZLibraries/Common/openssl/libssl-1_1-x64.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}../../../ZLibraries/Common/openssl/libcrypto-1_1-x64.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)

set_target_properties(ZImageViewer ${TARGET_PROPS})
