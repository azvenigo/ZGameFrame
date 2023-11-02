
################################################################################

cmake_minimum_required(VERSION 3.7.1)

################################################################################

project(ZGameFrame)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SUPPRESS_REGENERATION true)

####################
# VISUAL C MT MD CONFIGURATION

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

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
../../ZLibraries/Common/helpers/FNMatch.h           ../../ZLibraries/Common/helpers/FNMatch.cpp 
../../ZLibraries/Common/helpers/HTTPCache.h         ../../ZLibraries/Common/helpers/HTTPCache.cpp
../../ZLibraries/Common/helpers/CommandLineParser.h ../../ZLibraries/Common/helpers/CommandLineParser.cpp
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
../external/lunasvg/include
../external
)

list(APPEND LINK_DIRS ../external/lunasvg/build/$(Configuration) ../../ZLibraries/Common/curl/lib)
list(APPEND LINK_LIBS gdiplus.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lunasvg.lib libcurl.lib)


####################
# PROJECTS

add_subdirectory(SandboxApps)
add_subdirectory(ZImageViewer)

################################################################################
