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

set(BCG729_SOURCE_FILES
	adaptativeCodebookSearch.c
	codebooks.c
	computeAdaptativeCodebookGain.c
	computeLP.c
	computeWeightedSpeech.c
	decodeAdaptativeCodeVector.c
	decodeFixedCodeVector.c
	decodeGains.c
	decodeLSP.c
	decoder.c
	encoder.c
	findOpenLoopPitchDelay.c
	fixedCodebookSearch.c
	gainQuantization.c
	interpolateqLSP.c
	LP2LSPConversion.c
	LPSynthesisFilter.c
	LSPQuantization.c
	postFilter.c
	postProcessing.c
	preProcessing.c
	qLSP2LP.c
	utils.c
	cng.c
	dtx.c
	vad.c
)

if(NOT ENABLE_MSPLUGIN)

	if(ENABLE_STATIC)
		add_library(bcg729-static STATIC ${BCG729_SOURCE_FILES})
		set_target_properties(bcg729-static PROPERTIES OUTPUT_NAME bcg729)
		install(TARGETS bcg729-static EXPORT Bcg729Targets
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
	endif()
	if(ENABLE_SHARED)
		add_library(bcg729 SHARED ${BCG729_SOURCE_FILES})
		set_target_properties(bcg729 PROPERTIES VERSION 0)
		if(MSVC)
			if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
				install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/libbcg729.pdb
					DESTINATION ${CMAKE_INSTALL_BINDIR}
					PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
				)
			endif()
			set_target_properties(bcg729 PROPERTIES PREFIX "lib")
		endif()
		install(TARGETS bcg729 EXPORT Bcg729Targets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
	endif()
endif()

set(ABS_BCG729_SOURCE_FILES )
foreach(elem ${BCG729_SOURCE_FILES})
	list(APPEND ABS_BCG729_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${elem}")
endforeach()
set(BCG729_SOURCE_FILES ${ABS_BCG729_SOURCE_FILES} PARENT_SCOPE)

