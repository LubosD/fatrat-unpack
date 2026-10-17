# UnRAR 7.13 from https://www.rarlab.com/rar_add.htm, built as the static
# "RARDLL" library (the makefile's "lib" target). Other .cpp files are
# #included by these and must not be compiled on their own.
set(unrar_SRCS
	rar.cpp strlist.cpp strfn.cpp pathfn.cpp smallfn.cpp global.cpp file.cpp filefn.cpp filcreat.cpp
	archive.cpp arcread.cpp unicode.cpp system.cpp crypt.cpp crc.cpp rawread.cpp encname.cpp
	resource.cpp match.cpp timefn.cpp rdwrfn.cpp consio.cpp options.cpp errhnd.cpp rarvm.cpp secpassword.cpp
	rijndael.cpp getbits.cpp sha1.cpp sha256.cpp blake2s.cpp hash.cpp extinfo.cpp extract.cpp volume.cpp
	list.cpp find.cpp unpack.cpp headers.cpp threadpool.cpp rs16.cpp cmddata.cpp ui.cpp largepage.cpp
	filestr.cpp scantree.cpp dll.cpp qopen.cpp)

find_package(Threads REQUIRED)

add_library(unrar STATIC ${unrar_SRCS})
set_target_properties(unrar PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden AUTOMOC OFF AUTOUIC OFF)
target_compile_definitions(unrar PRIVATE _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE RAR_SMP RARDLL)
target_link_libraries(unrar PUBLIC Threads::Threads)
# Third-party code, don't drown the build in its warnings
target_compile_options(unrar PRIVATE -w)
