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

set(SOURCE_FILES
	bcg729_dec.c
	bcg729_enc.c
)

include_directories(${MEDIASTREAMER2_INCLUDE_DIRS})

if(ENABLE_STATIC)
	add_library(msbcg729-static STATIC ${BCG729_SOURCE_FILES} ${SOURCE_FILES})
	set_target_properties(msbcg729-static PROPERTIES OUTPUT_NAME msbcg729)
	target_link_libraries(msbcg729-static ${MEDIASTREAMER2_LIBRARIES})
	install(TARGETS msbcg729-static
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/mediastreamer/plugins
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif()
if(ENABLE_SHARED)
	add_library(msbcg729 MODULE ${BCG729_SOURCE_FILES} ${SOURCE_FILES})
	set_target_properties(msbcg729 PROPERTIES SOVERSION 0)
	target_link_libraries(msbcg729 ${MEDIASTREAMER2_LIBRARIES})
	if(MSVC)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/libmsbcg729.pdb
				DESTINATION ${CMAKE_INSTALL_BINDIR}
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()
		set_target_properties(msbcg729 PROPERTIES PREFIX "lib")
	endif()
	install(TARGETS msbcg729
		RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/mediastreamer/plugins
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mediastreamer/plugins
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/mediastreamer/plugins
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif()
