project(fr_czshare)

cmake_minimum_required(VERSION 2.4.0)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(Qt4 REQUIRED)

set(CMAKE_MODULE_PATH cmake_modules)

set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)

set(fatrat_DIR ${CMAKE_MODULE_PATH})
find_package(fatrat REQUIRED)

if(FATRAT_FOUND)
	message(STATUS "fatrat found OK")
	include_directories(${FATRAT_INCLUDE_DIRS})
else(FATRAT_FOUND)
	message(FATAL_ERROR "No FatRat development headers found")
endif(FATRAT_FOUND)

include( ${QT_USE_FILE} )
add_definitions(${QT_DEFINITIONS})

if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories(./ ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDE_DIR})

set(fr_opensubtitles_SRCS
	fr_opensubtitles.cpp
	XmlRpc.cpp
	SubtitlesDlg.cpp
	SettingsSubtitles.cpp
	)

set(fr_opensubtitles_MOC_HDRS
	SubtitlesDlg.h
	)

set(fr_opensubtitles_UIS
	SubtitlesDlg.ui
	SettingsSubtitles.ui
	)

qt4_wrap_cpp(fr_opensubtitles_MOC_SRCS ${fr_opensubtitles_MOC_HDRS})

set(fr_opensubtitles_RCS
	gfx/opensubtitles.qrc
)

qt4_wrap_ui(fr_opensubtitles_UIS_H ${fr_opensubtitles_UIS})
qt4_add_resources(fr_opensubtitles_QRC_H gfx/opensubtitles.qrc)

add_library(fr_opensubtitles SHARED ${fr_opensubtitles_SRCS} ${fr_opensubtitles_MOC_SRCS} ${fr_opensubtitles_UIS_H} ${fr_opensubtitles_QRC_H})

target_link_libraries(fr_opensubtitles ${QT_LIBRARIES} -lz)

install(TARGETS fr_opensubtitles DESTINATION lib/fatrat/plugins)
