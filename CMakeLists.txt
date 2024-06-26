cmake_minimum_required (VERSION 3.19)
#
option(BUILD_64BIT 		"build 64bit app" 		 	ON)
option(WITH_TEST 		"build test" 				OFF)
option(WITH_COVERAGE 	"generate coverage report" 	OFF)
option(WITH_DOC 		"generate doxygen report" 	OFF)
#
#
#
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
# Paths
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/3rdParty)
set(CMAKE_PREFIX_PATH ${CMAKE_THIRD_PARTY_DIR})
# Enable c++ language
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_COMPILER "clang.exe")
set(CMAKE_CXX_COMPILER "clang++.exe")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -g -DSQLITE_OMIT_DEPRECATED")

if (BUILD_64BIT)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()
#
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY") # https://stackoverflow.com/a/53635241/8653778
enable_language(CXX)
#
project (WatcherApp)
#
# find_package (Threads REQUIRED)
# find_package (Protobuf REQUIRED)
# if (NOT SQLite3_FOUND)
include(FetchContent)
include(ExternalProject)
include(Build-sqlite3)
# find_package(SQLite3 REQUIRED)
# endif ()

#
add_subdirectory(app)
#
if(WITH_TEST)
	enable_testing()
	add_subdirectory(utest)
endif()

if(WITH_COVERAGE)
	include(CodeCoverage)
endif()

if(WITH_DOC)
	find_package (Doxygen)
	if(${CMAKE_COMPILER_ID} MATCHES GNU AND $my_project_WITH_PROF)
		set_target_properties(my_prog
			PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -gp -fprofile-coverage -fprofile-args" LINK_FLAGS "-fprofile-arcs")
	endif()

	if(DOXYGEN_FOUND)
		if(DOXYGEN_DOT_FOUND)
			set(DOXYGEN_CONFIGURE_HAVE_DOT "HAVE_DOT=YES")
			get_filename_component(DOXYGEN_DOT_PATH "${DOXYGEN_DOT_EXECUTABLE}" DIRECTORY)
			file(TO_NATIVE_PATH "${DOXYGEN_DOT_PATH}" DOXYGEN_DOT_PATH)
			set(DOXYGEN_CONFIGURE_DOT_PATH "DOT_PATH=${DOXYGEN_DOT_PATH}")
		endif()
		add_custom_target(doc
			configure_file(Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile)
			${DOXYGEN_EXECUTABLE}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			COMMENT "Running Doxygen" VERBATIM
		)
	endif()
endif()


