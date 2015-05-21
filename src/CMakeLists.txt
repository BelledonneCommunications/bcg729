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
)

if (ENABLE_STATIC)
	add_library(bcg729 STATIC ${BCG729_SOURCE_FILES})
else()
	add_library(bcg729 SHARED ${BCG729_SOURCE_FILES})
	set_target_properties(bcg729 PROPERTIES VERSION 0)
	if(MSVC)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/libbcg729.pdb
				DESTINATION bin
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()
		set_target_properties(bcg729 PROPERTIES PREFIX "lib")
	endif()
endif()

install(TARGETS bcg729 EXPORT Bcg729Targets
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

set(ABS_BCG729_SOURCE_FILES )
foreach(elem ${BCG729_SOURCE_FILES})
	list(APPEND ABS_BCG729_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${elem}")
endforeach()
set(BCG729_SOURCE_FILES ${ABS_BCG729_SOURCE_FILES} PARENT_SCOPE)
