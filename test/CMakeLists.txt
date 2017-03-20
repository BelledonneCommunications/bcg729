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

set(BCG729_LIBRARY bcg729)

set(UTIL_SRC src/testUtils.c  src/testUtils.h)

add_executable(adaptativeCodebookSearchTest src/adaptativeCodebookSearchTest.c ${UTIL_SRC})
target_link_libraries(adaptativeCodebookSearchTest ${BCG729_LIBRARY})

add_executable(computeAdaptativeCodebookGainTest src/computeAdaptativeCodebookGainTest.c ${UTIL_SRC})
target_link_libraries(computeAdaptativeCodebookGainTest ${BCG729_LIBRARY})

add_executable(computeLPTest src/computeLPTest.c ${UTIL_SRC}) 
target_link_libraries(computeLPTest ${BCG729_LIBRARY})

add_executable(computeWeightedSpeechTest src/computeWeightedSpeechTest.c ${UTIL_SRC})
target_link_libraries(computeWeightedSpeechTest ${BCG729_LIBRARY})

add_executable(decodeAdaptativeCodeVectorTest src/decodeAdaptativeCodeVectorTest.c ${UTIL_SRC})
target_link_libraries(decodeAdaptativeCodeVectorTest ${BCG729_LIBRARY})

add_executable(decodeFixedCodeVectorTest src/decodeFixedCodeVectorTest.c ${UTIL_SRC})
target_link_libraries(decodeFixedCodeVectorTest ${BCG729_LIBRARY})

add_executable(decodeGainsTest src/decodeGainsTest.c ${UTIL_SRC})
target_link_libraries(decodeGainsTest ${BCG729_LIBRARY})

add_executable(decodeLSPTest src/decodeLSPTest.c ${UTIL_SRC})
target_link_libraries(decodeLSPTest ${BCG729_LIBRARY})

add_executable(decoderTest src/decoderTest.c ${UTIL_SRC})
target_link_libraries(decoderTest ${BCG729_LIBRARY})

add_executable(CNGRFC3389decoderTest src/CNGRFC3389decoderTest.c ${UTIL_SRC})
target_link_libraries(CNGRFC3389decoderTest ${BCG729_LIBRARY})

add_executable(CNGdecoderTest src/CNGdecoderTest.c ${UTIL_SRC})
target_link_libraries(CNGdecoderTest ${BCG729_LIBRARY})

add_executable(decoderMultiChannelTest src/decoderMultiChannelTest.c ${UTIL_SRC})
target_link_libraries(decoderMultiChannelTest ${BCG729_LIBRARY})

add_executable(encoderTest src/encoderTest.c ${UTIL_SRC})
target_link_libraries(encoderTest ${BCG729_LIBRARY})

add_executable(encoderMultiChannelTest src/encoderMultiChannelTest.c ${UTIL_SRC})
target_link_libraries(encoderMultiChannelTest ${BCG729_LIBRARY})

add_executable(findOpenLoopPitchDelayTest src/findOpenLoopPitchDelayTest.c ${UTIL_SRC})
target_link_libraries(findOpenLoopPitchDelayTest ${BCG729_LIBRARY})

add_executable(fixedCodebookSearchTest src/fixedCodebookSearchTest.c ${UTIL_SRC})
target_link_libraries(fixedCodebookSearchTest ${BCG729_LIBRARY})

add_executable(g729FixedPointMathTest src/g729FixedPointMathTest.c ${UTIL_SRC})
target_link_libraries(g729FixedPointMathTest ${BCG729_LIBRARY} m)

add_executable(gainQuantizationTest src/gainQuantizationTest.c ${UTIL_SRC})
target_link_libraries(gainQuantizationTest ${BCG729_LIBRARY})

add_executable(interpolateqLSPAndConvert2LPTest src/interpolateqLSPAndConvert2LPTest.c ${UTIL_SRC})
target_link_libraries(interpolateqLSPAndConvert2LPTest ${BCG729_LIBRARY})

add_executable(LP2LSPConversionTest src/LP2LSPConversionTest.c ${UTIL_SRC})
target_link_libraries(LP2LSPConversionTest ${BCG729_LIBRARY})

add_executable(LPSynthesisFilterTest src/LPSynthesisFilterTest.c ${UTIL_SRC})
target_link_libraries(LPSynthesisFilterTest ${BCG729_LIBRARY})

add_executable(LSPQuantizationTest src/LSPQuantizationTest.c ${UTIL_SRC})
target_link_libraries(LSPQuantizationTest ${BCG729_LIBRARY})

add_executable(postFilterTest src/postFilterTest.c ${UTIL_SRC})
target_link_libraries(postFilterTest ${BCG729_LIBRARY})

add_executable(postProcessingTest src/postProcessingTest.c ${UTIL_SRC})
target_link_libraries(postProcessingTest ${BCG729_LIBRARY})

add_executable(preProcessingTest src/preProcessingTest.c ${UTIL_SRC})
target_link_libraries(preProcessingTest ${BCG729_LIBRARY})

add_executable(computeNoiseExcitationTest src/computeNoiseExcitationTest.c ${UTIL_SRC})
target_link_libraries(computeNoiseExcitationTest ${BCG729_LIBRARY})

add_executable(encoderVADTest src/encoderVADTest.c ${UTIL_SRC})
target_link_libraries(encoderVADTest ${BCG729_LIBRARY})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/testCampaign.cmake DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ )
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/testCampaign.cmake ${CMAKE_CURRENT_BINARY_DIR}/testCampaign )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testCampaignAll.cmake ${CMAKE_CURRENT_BINARY_DIR}/testCampaignAll)

