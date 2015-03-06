############################################################################
# CMakeLists.txt
# Copyright (C) 2014  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################

cmake_minimum_required(VERSION 2.8.12)
project(BCG729 C)


set(PACKAGE "bcg729")
set(PACKAGE_NAME "${PACKAGE}")
set(PACKAGE_VERSION "0.1")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "support@belledonne-communications.com")
set(PACKAGE_TARNAME "bcg729")
set(PACKAGE_URL "")
set(VERSION "${PACKAGE_VERSION}")


option(ENABLE_TESTS "Enable compilation of the tests." No)
option(ENABLE_MSPLUGIN "Enable compilation of the mediastreamer2 plugin." Yes)
set(WITH_MS2 "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Set prefix where the mediastreamer libraries can be found (ex:/usr or /usr/local)")
set(WITH_ORTP "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Set prefix where the oRTP library can be found (ex:/usr or /usr/local)")


if(MSVC)
	include_directories(${CMAKE_INSTALL_PREFIX}/include/MSVC)
endif()


list(APPEND CMAKE_MODULE_PATH ${CMAKE_PREFIX_PATH}/share/cmake/Modules)

include(CheckIncludeFile)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

if(ENABLE_MSPLUGIN)
	find_package(ORTP)
	find_package(MS2)

	if(NOT (ORTP_FOUND AND MS2_FOUND))
		message(WARNING "Could not find the oRTP and/or mediastreamer2 libraries!")
		set(ENABLE_MSPLUGIN OFF CACHE BOOL "Enable compilation of the mediastreamer2 plugin." FORCE)
	endif()
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)


include_directories(
	include
	src
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/src
)

add_definitions(-DHAVE_CONFIG_H)
if(MSVC)
	add_definitions("/W3")
else()
	add_definitions("-Wall" "-Werror")
endif()

add_subdirectory(src)
if(ENABLE_MSPLUGIN)
	add_subdirectory(msbcg729)
endif()
if(ENABLE_TESTS)
	add_subdirectory(test)
endif()
