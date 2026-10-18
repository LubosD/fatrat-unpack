find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

# The flow (UnpackTask, Unpacker and its subclasses) depends only on UnpackUi,
# not on FatRat, so these link the plugin's own sources directly rather than
# dlopen()'ing fatrat-unpack.so or linking any FatRat library.
set(unpack_flow_SRCS
	${CMAKE_SOURCE_DIR}/Archivators.cpp
	${CMAKE_SOURCE_DIR}/Unpacker.cpp
	${CMAKE_SOURCE_DIR}/RarUnpacker.cpp
	${CMAKE_SOURCE_DIR}/FileConcater.cpp
	${CMAKE_SOURCE_DIR}/UnpackTask.cpp
	)
if(WITH_LIBARCHIVE)
	list(APPEND unpack_flow_SRCS ${CMAKE_SOURCE_DIR}/ArchiveUnpacker.cpp)
endif(WITH_LIBARCHIVE)

function(fatrat_unpack_test name)
	add_executable(${name} ${name}.cpp ${unpack_flow_SRCS})
	target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
	target_compile_definitions(${name} PRIVATE FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
	target_link_libraries(${name} PRIVATE Qt6::Core Qt6::Test unrar)
	if(WITH_LIBARCHIVE)
		target_link_libraries(${name} PRIVATE PkgConfig::LIBARCHIVE)
	endif(WITH_LIBARCHIVE)
	add_test(NAME ${name} COMMAND ${name})
endfunction()

fatrat_unpack_test(tst_archivators)
fatrat_unpack_test(tst_unpacktask)
