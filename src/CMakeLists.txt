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

add_library(bcg729 ${BCG729_SOURCE_FILES})
target_compile_definitions(bcg729 PRIVATE "BCG729_EXPORTS")
set_target_properties(bcg729 PROPERTIES VERSION 0)
target_include_directories(bcg729
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/src
		${PROJECT_BINARY_DIR}
)

if(MSVC AND BUILD_SHARED_LIBS)
	install(FILES $<TARGET_PDB_FILE:bcg729>
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		CONFIGURATIONS Debug RelWithDebInfo
	)
	set_target_properties(bcg729 PROPERTIES PREFIX "lib")
endif()


install(TARGETS bcg729 EXPORT ${PROJECT_NAME}Targets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
