project(fatrat-unpack)
add_subdirectory(unrar)

cmake_minimum_required(VERSION 2.4.0)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(Qt4 REQUIRED)

set(CMAKE_MODULE_PATH cmake_modules)

if(WITH_EVERYTHING)
	set(WITH_NLS ON)
endif(WITH_EVERYTHING)

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

if(WITH_ZIP)
	set(libzip_DIR ${CMAKE_MODULE_PATH})
	find_package(libzip REQUIRED)
	
	if(LIBZIP_FOUND)
		message(STATUS "libzip found OK: ${LIBZIP_LIBRARY}")
		include_directories(${LIBZIP_INCLUDE_DIRS})
	else(LIBZIP_FOUND)
		message(FATAL_ERROR "No libzip")
	endif(LIBZIP_FOUND)
endif(WITH_ZIP)

include( ${QT_USE_FILE} )
add_definitions(${QT_DEFINITIONS})
add_definitions(-fvisibility=hidden)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories(./ ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDE_DIR})
CONFIGURE_FILE(config.h.in config.h)

set(fatrat-unpack_SRCS
	fr_unpack.cpp
	Unpacker.cpp
	RarUnpacker.cpp
	UnpackDlg.cpp
	)

set(fatrat-unpack_MOC_HDRS
	RarUnpacker.h
	UnpackDlg.h
	Unpacker.h
	)

set(fatrat-unpack_UIS
	ProgressWidget.ui
	PasswordDlg.ui
	UnpackDlg.ui
	FilesDlg.ui
	)

if(WITH_ZIP)
	set(fatrat-unpack_SRCS
		${fatrat-unpack_SRCS}
		ZipUnpacker.cpp
	)
endif(WITH_ZIP)

qt4_wrap_cpp(fatrat-unpack_MOC_SRCS ${fatrat-unpack_MOC_HDRS})

set(fatrat-unpack_RCS
	#gfx/opensubtitles.qrc
)

qt4_wrap_ui(fatrat-unpack_UIS_H ${fatrat-unpack_UIS})
qt4_add_translation(lrelease_outputs locale/fatrat-unpack_cs_CZ.ts locale/fatrat-unpack_sk_SK.ts locale/fatrat-unpack_tr_TR.ts)

add_library(fatrat-unpack SHARED ${fatrat-unpack_SRCS} ${fatrat-unpack_MOC_SRCS} ${fatrat-unpack_UIS_H} ${fatrat-unpack_QRC_H} ${lrelease_outputs})
#add_dependencies(fatrat-unpack unrar)

target_link_libraries(fatrat-unpack ${QT_LIBRARIES} ${LIBZIP_LIBRARY} unrar)

if(WITH_NLS)
	install(FILES ${lrelease_outputs} DESTINATION share/fatrat/lang)
endif(WITH_NLS)

install(TARGETS fatrat-unpack DESTINATION lib/fatrat/plugins)

install(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/data/LICENSE.txt
	${CMAKE_CURRENT_SOURCE_DIR}/data/TRANSLATIONS.txt
	${CMAKE_CURRENT_SOURCE_DIR}/data/3RDPARTIES.txt
	DESTINATION share/fatrat/data/plugins/fatrat-unpack
	)

