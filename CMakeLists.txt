############################################################################
# CMakeLists.txt
# Copyright (C) 2014-2023  Belledonne Communications, Grenoble France
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

cmake_minimum_required(VERSION 3.22)

project(BCG729 VERSION 1.1.1 LANGUAGES C)


set(PACKAGE "${PROJECT_NAME}")
set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "support@belledonne-communications.com")
set(PACKAGE_TARNAME "bcg729")
set(PACKAGE_URL "")
set(VERSION "${PACKAGE_VERSION}")


option(ENABLE_STRICT "Build with strict compile options." YES)
option(ENABLE_UNIT_TESTS "Enable compilation of the tests." NO)

include(GNUInstallDirs)



set(BCG729_CPPFLAGS )
if(NOT BUILD_SHARED_LIBS)
	set(BCG729_STATIC 1)
	list(APPEND BCG729_CPPFLAGS "-DBCG729_STATIC")
endif()
add_definitions(-DHAVE_CONFIG_H)

if(MSVC)
	add_definitions("/W3")
else()
	add_definitions("-Wall")
	if(ENABLE_STRICT)
		if (NOT IOS)
			add_definitions(" -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers ")
		endif()
	endif()
	if(NOT ENABLE_UNIT_TESTS) # test access inner functions so maintain visibility if we want to run tests
		add_definitions("-fvisibility=hidden")
	endif()
	if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		add_definitions("-Wno-error=unused-command-line-argument")
	endif()
endif()

configure_file(${PROJECT_SOURCE_DIR}/config.h.cmake ${PROJECT_BINARY_DIR}/config.h)

add_subdirectory(include)
add_subdirectory(src)
if(ENABLE_UNIT_TESTS AND NOT WIN32)
	# Deactivated on Windows because of symbol export issues (TODO: fix that)
	add_subdirectory(test)
endif()


include(CMakePackageConfigHelpers)
set(CMAKE_MODULES_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake")
configure_package_config_file("${PROJECT_NAME}Config.cmake.in" "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	INSTALL_DESTINATION "${CMAKE_MODULES_INSTALL_DIR}"
	NO_SET_AND_CHECK_MACRO
)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)
install(FILES
	"${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	"${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION ${CMAKE_MODULES_INSTALL_DIR}
)

export(EXPORT ${PROJECT_NAME}Targets
	FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
)
install(EXPORT ${PROJECT_NAME}Targets
	FILE "${PROJECT_NAME}Targets.cmake"
	DESTINATION ${CMAKE_MODULES_INSTALL_DIR}
)


set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(includedir  "\${prefix}/include")
set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
configure_file(libbcg729.pc.in
	"${PROJECT_BINARY_DIR}/libbcg729.pc"
	@ONLY
)
install(FILES
	"${PROJECT_BINARY_DIR}/libbcg729.pc"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)


add_subdirectory(build)
