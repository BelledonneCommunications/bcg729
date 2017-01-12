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
set(UTIL_SRC src/testUtils.c  src/testUtils.h)

add_executable(adaptativeCodebookSearchTest src/adaptativeCodebookSearchTest.c ${UTIL_SRC})
target_link_libraries(adaptativeCodebookSearchTest bcg729)

add_executable(computeAdaptativeCodebookGainTest src/computeAdaptativeCodebookGainTest.c ${UTIL_SRC})
target_link_libraries(computeAdaptativeCodebookGainTest bcg729)

add_executable(computeLPTest src/computeLPTest.c ${UTIL_SRC}) 
target_link_libraries(computeLPTest bcg729)

add_executable(computeWeightedSpeechTest src/computeWeightedSpeechTest.c ${UTIL_SRC})
target_link_libraries(computeWeightedSpeechTest bcg729)

add_executable(decodeAdaptativeCodeVectorTest src/decodeAdaptativeCodeVectorTest.c ${UTIL_SRC})
target_link_libraries(decodeAdaptativeCodeVectorTest bcg729)

add_executable(decodeFixedCodeVectorTest src/decodeFixedCodeVectorTest.c ${UTIL_SRC})
target_link_libraries(decodeFixedCodeVectorTest bcg729)

add_executable(decodeGainsTest src/decodeGainsTest.c ${UTIL_SRC})
target_link_libraries(decodeGainsTest bcg729)

add_executable(decodeLSPTest src/decodeLSPTest.c ${UTIL_SRC})
target_link_libraries(decodeLSPTest bcg729)

add_executable(decoderTest src/decoderTest.c ${UTIL_SRC})
target_link_libraries(decoderTest bcg729)

add_executable(CNGRFC3389decoderTest src/CNGRFC3389decoderTest.c ${UTIL_SRC})
target_link_libraries(CNGRFC3389decoderTest bcg729)

add_executable(CNGdecoderTest src/CNGdecoderTest.c ${UTIL_SRC})
target_link_libraries(CNGdecoderTest bcg729)

add_executable(decoderMultiChannelTest src/decoderMultiChannelTest.c ${UTIL_SRC})
target_link_libraries(decoderMultiChannelTest bcg729)

add_executable(encoderTest src/encoderTest.c ${UTIL_SRC})
target_link_libraries(encoderTest bcg729)

add_executable(encoderMultiChannelTest src/encoderMultiChannelTest.c ${UTIL_SRC})
target_link_libraries(encoderMultiChannelTest bcg729)

add_executable(findOpenLoopPitchDelayTest src/findOpenLoopPitchDelayTest.c ${UTIL_SRC})
target_link_libraries(findOpenLoopPitchDelayTest bcg729)

add_executable(fixedCodebookSearchTest src/fixedCodebookSearchTest.c ${UTIL_SRC})
target_link_libraries(fixedCodebookSearchTest bcg729)

add_executable(g729FixedPointMathTest src/g729FixedPointMathTest.c ${UTIL_SRC})
target_link_libraries(g729FixedPointMathTest bcg729 m)

add_executable(gainQuantizationTest src/gainQuantizationTest.c ${UTIL_SRC})
target_link_libraries(gainQuantizationTest bcg729)

add_executable(interpolateqLSPAndConvert2LPTest src/interpolateqLSPAndConvert2LPTest.c ${UTIL_SRC})
target_link_libraries(interpolateqLSPAndConvert2LPTest bcg729)

add_executable(LP2LSPConversionTest src/LP2LSPConversionTest.c ${UTIL_SRC})
target_link_libraries(LP2LSPConversionTest bcg729)

add_executable(LPSynthesisFilterTest src/LPSynthesisFilterTest.c ${UTIL_SRC})
target_link_libraries(LPSynthesisFilterTest bcg729)

add_executable(LSPQuantizationTest src/LSPQuantizationTest.c ${UTIL_SRC})
target_link_libraries(LSPQuantizationTest bcg729)

add_executable(postFilterTest src/postFilterTest.c ${UTIL_SRC})
target_link_libraries(postFilterTest bcg729)

add_executable(postProcessingTest src/postProcessingTest.c ${UTIL_SRC})
target_link_libraries(postProcessingTest bcg729)

add_executable(preProcessingTest src/preProcessingTest.c ${UTIL_SRC})
target_link_libraries(preProcessingTest bcg729)

add_executable(computeNoiseExcitationTest src/computeNoiseExcitationTest.c ${UTIL_SRC})
target_link_libraries(computeNoiseExcitationTest bcg729)

add_executable(encoderVADTest src/encoderVADTest.c ${UTIL_SRC})
target_link_libraries(encoderVADTest bcg729)

