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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

cmake_minimum_required(VERSION 3.0)
project(bcg729 VERSION 1.0.3 LANGUAGES C)


set(PACKAGE "${PROJECT_NAME}")
set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "support@belledonne-communications.com")
set(PACKAGE_TARNAME "bcg729")
set(PACKAGE_URL "")
set(VERSION "${PACKAGE_VERSION}")


option(ENABLE_SHARED "Build shared library." YES)
option(ENABLE_STATIC "Build static library." YES)
option(ENABLE_TESTS "Enable compilation of the tests." NO)

include(GNUInstallDirs)


include_directories(
	include
	src
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/src
)
set(MSVC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/MSVC")
if(MSVC)
	include_directories(${MSVC_INCLUDE_DIR})
endif()

set(BCG729_CPPFLAGS )
if(ENABLE_STATIC)
	set(BCG729_STATIC 1)
	list(APPEND BCG729_CPPFLAGS "-DBCG729_STATIC")
endif()

add_definitions(-DHAVE_CONFIG_H)
if(MSVC)
	add_definitions("/W3")
else()
	add_definitions("-Wall")
	if (NOT IOS)
		add_definitions("-Werror")
	endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(include)
add_subdirectory(src)
if(ENABLE_TESTS)
	add_subdirectory(test)
endif()


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/Bcg729ConfigVersion.cmake"
	VERSION ${PACKAGE_VERSION}
	COMPATIBILITY AnyNewerVersion
)
export(EXPORT Bcg729Targets
	FILE "${CMAKE_CURRENT_BINARY_DIR}/Bcg729Targets.cmake"
)
configure_file(Bcg729Config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/Bcg729Config.cmake"
	@ONLY
)

set(CONFIG_PACKAGE_LOCATION "${CMAKE_INSTALL_DATADIR}/Bcg729/cmake")
install(EXPORT Bcg729Targets
	FILE Bcg729Targets.cmake
	DESTINATION ${CONFIG_PACKAGE_LOCATION}
)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/Bcg729Config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/Bcg729ConfigVersion.cmake"
	DESTINATION ${CONFIG_PACKAGE_LOCATION}
)

# CPack settings
set(CPACK_PACKAGE_NAME "bcg729")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES
        "^${CMAKE_BINARY_DIR}"
        "/\\\\..+"
)

include(CPack)
