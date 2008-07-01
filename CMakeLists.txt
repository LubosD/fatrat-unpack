project(fr_unpack)
add_subdirectory(unrar)

cmake_minimum_required(VERSION 2.4.0)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(Qt4 REQUIRED)

set(CMAKE_MODULE_PATH cmake_modules)

set(fatrat_DIR ${CMAKE_MODULE_PATH})
find_package(fatrat REQUIRED)

set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)

if(FATRAT_FOUND)
	message(STATUS "fatrat found OK")
	include_directories(${FATRAT_INCLUDE_DIRS})
else(FATRAT_FOUND)
	message(FATAL_ERROR "No FatRat development headers found")
endif(FATRAT_FOUND)

include( ${QT_USE_FILE} )
add_definitions(${QT_DEFINITIONS})
add_definitions(-fvisibility=hidden)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories(./ ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDE_DIR})

set(fr_unpack_SRCS
	fr_unpack.cpp
	Unpacker.cpp
	RarUnpacker.cpp
	UnpackDlg.cpp
	)

set(fr_unpack_MOC_HDRS
	RarUnpacker.h
	UnpackDlg.h
	Unpacker.h
	)

set(fr_unpack_UIS
	ProgressWidget.ui
	PasswordDlg.ui
	UnpackDlg.ui
	)

qt4_wrap_cpp(fr_unpack_MOC_SRCS ${fr_unpack_MOC_HDRS})

set(fr_unpack_RCS
	#gfx/opensubtitles.qrc
)

qt4_wrap_ui(fr_unpack_UIS_H ${fr_unpack_UIS})
#qt4_add_resources(fr_unpack_QRC_H gfx/opensubtitles.qrc)

add_library(fr_unpack SHARED ${fr_unpack_SRCS} ${fr_unpack_MOC_SRCS} ${fr_unpack_UIS_H} ${fr_unpack_QRC_H})
#add_dependencies(fr_unpack unrar)

target_link_libraries(fr_unpack ${QT_LIBRARIES} unrar)

install(TARGETS fr_unpack DESTINATION lib/fatrat/plugins)
