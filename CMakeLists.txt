cmake_minimum_required(VERSION 3.16)
project(fatrat-unpack LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

option(WITH_NLS "Install translations" OFF)
option(WITH_ZIP "Support ZIP archives (requires libzip)" OFF)
option(WITH_EVERYTHING "Enable all optional features" OFF)

if(WITH_EVERYTHING)
	set(WITH_NLS ON)
	set(WITH_ZIP ON)
endif(WITH_EVERYTHING)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Xml LinguistTools)

# FatRat's development headers (installed under <prefix>/include/fatrat)
find_path(FATRAT_INCLUDE_DIR NAMES fatrat/fatrat.h fatrat/config.h)
if(NOT FATRAT_INCLUDE_DIR)
	message(FATAL_ERROR "No FatRat development headers found, set FATRAT_INCLUDE_DIR")
endif(NOT FATRAT_INCLUDE_DIR)
message(STATUS "FatRat headers: ${FATRAT_INCLUDE_DIR}/fatrat")

# This plugin is a GUI plugin and must be installed where FatRat looks for plugins
file(STRINGS "${FATRAT_INCLUDE_DIR}/fatrat/config.h" FATRAT_GUI_DEFINE REGEX "^#define WITH_GUI")
if(NOT FATRAT_GUI_DEFINE)
	message(FATAL_ERROR "FatRat was built without WITH_GUI, the unpack plugin needs the GUI")
endif(NOT FATRAT_GUI_DEFINE)

if(NOT FATRAT_PLUGIN_DIR)
	file(STRINGS "${FATRAT_INCLUDE_DIR}/fatrat/config.h" FATRAT_PLUGIN_DEFINE REGEX "^#define PLUGIN_LOCATION ")
	string(REGEX REPLACE "^#define PLUGIN_LOCATION \"(.*)\"$" "\\1" FATRAT_PLUGIN_DIR "${FATRAT_PLUGIN_DEFINE}")
	if(NOT IS_ABSOLUTE "${FATRAT_PLUGIN_DIR}")
		# Older FatRat builds wrote PLUGIN_LOCATION relative to their install prefix
		get_filename_component(FATRAT_PREFIX "${FATRAT_INCLUDE_DIR}" DIRECTORY)
		set(FATRAT_PLUGIN_DIR "${FATRAT_PREFIX}/${FATRAT_PLUGIN_DIR}")
	endif(NOT IS_ABSOLUTE "${FATRAT_PLUGIN_DIR}")
endif(NOT FATRAT_PLUGIN_DIR)
message(STATUS "Plugin install directory: ${FATRAT_PLUGIN_DIR}")

if(WITH_ZIP)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip>=1.0)
endif(WITH_ZIP)

add_subdirectory(unrar)

configure_file(unpack_config.h.in unpack_config.h)

set(fatrat-unpack_SRCS
	fr_unpack.cpp
	Unpacker.cpp Unpacker.h
	RarUnpacker.cpp RarUnpacker.h
	UnpackDlg.cpp UnpackDlg.h
	FileConcater.cpp FileConcater.h
	PasswordDlg.h
	PipeDlg.h
	FilesDlg.h
	ProgressWidget.h
	ProgressWidget.ui
	PasswordDlg.ui
	UnpackDlg.ui
	FilesDlg.ui
	PipeDlg.ui
	)

if(WITH_ZIP)
	list(APPEND fatrat-unpack_SRCS ZipUnpacker.cpp ZipUnpacker.h)
endif(WITH_ZIP)

add_library(fatrat-unpack MODULE ${fatrat-unpack_SRCS})
# FatRat loads "*.so" from its plugin directory
set_target_properties(fatrat-unpack PROPERTIES PREFIX "" CXX_VISIBILITY_PRESET hidden)
target_include_directories(fatrat-unpack PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	${FATRAT_INCLUDE_DIR} ${FATRAT_INCLUDE_DIR}/fatrat)
target_link_libraries(fatrat-unpack PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Xml unrar)
if(WITH_ZIP)
	target_link_libraries(fatrat-unpack PRIVATE PkgConfig::LIBZIP)
endif(WITH_ZIP)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	target_compile_definitions(fatrat-unpack PRIVATE DEBUG)
	target_compile_options(fatrat-unpack PRIVATE -Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

install(TARGETS fatrat-unpack DESTINATION ${FATRAT_PLUGIN_DIR})

if(WITH_NLS)
	qt_add_translation(lrelease_outputs
		locale/fatrat-unpack_cs_CZ.ts locale/fatrat-unpack_sk_SK.ts locale/fatrat-unpack_tr_TR.ts)
	add_custom_target(fatrat-unpack-translations ALL DEPENDS ${lrelease_outputs})
	install(FILES ${lrelease_outputs} DESTINATION share/fatrat/lang)
endif(WITH_NLS)

install(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/data/pipecmds
	DESTINATION share/fatrat/data/plugins/fatrat-unpack
	)

install(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/data/TRANSLATIONS
	${CMAKE_CURRENT_SOURCE_DIR}/data/3RDPARTIES
	DESTINATION share/doc/fatrat-unpack
	)
