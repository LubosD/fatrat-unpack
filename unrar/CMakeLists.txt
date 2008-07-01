project(unrar)

cmake_minimum_required(VERSION 2.4.0)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 OLD)
endif(COMMAND cmake_policy)

ADD_DEFINITIONS(-DLPARAM="unsigned long")
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
ADD_DEFINITIONS(-DRARDLL -DSILENT)
ADD_DEFINITIONS(-fvisibility=hidden)
ADD_DEFINITIONS(-fPIC)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
	ADD_DEFINITIONS(-Wall)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

set(unrar_SRCS
	filestr.cpp scantree.cpp dll.cpp
	rar.cpp strlist.cpp strfn.cpp pathfn.cpp int64.cpp savepos.cpp global.cpp file.cpp filefn.cpp filcreat.cpp
	archive.cpp arcread.cpp unicode.cpp system.cpp isnt.cpp crypt.cpp crc.cpp rawread.cpp encname.cpp recvol.cpp
	resource.cpp match.cpp timefn.cpp rdwrfn.cpp consio.cpp options.cpp ulinks.cpp errhnd.cpp rarvm.cpp rs.cpp
	rijndael.cpp getbits.cpp sha1.cpp extinfo.cpp extract.cpp volume.cpp list.cpp find.cpp unpack.cpp cmddata.cpp)

add_library(unrar STATIC ${unrar_SRCS})
