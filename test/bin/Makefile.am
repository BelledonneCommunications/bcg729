check_PROGRAMS=adaptativeCodebookSearchTest computeAdaptativeCodebookGainTest computeLPTest computeWeightedSpeechTest decodeAdaptativeCodeVectorTest decodeFixedCodeVectorTest decodeGainsTest decodeLSPTest \
       decoderTest encoderTest decoderMultiChannelTest encoderMultiChannelTest findOpenLoopPitchDelayTest fixedCodebookSearchTest g729FixedPointMathTest gainQuantizationTest interpolateqLSPAndConvert2LPTest \
       LP2LSPConversionTest LPSynthesisFilterTest LSPQuantizationTest postFilterTest postProcessingTest preProcessingTest computeNoiseExcitationTest CNGdecoderTest CNGRFC3389decoderTest encoderVADTest
util_src= \
	$(top_srcdir)/test/src/testUtils.c \
	$(top_srcdir)/test/src/testUtils.h

adaptativeCodebookSearchTest_SOURCES=$(top_srcdir)/test/src/adaptativeCodebookSearchTest.c $(util_src)
computeAdaptativeCodebookGainTest_SOURCES=$(top_srcdir)/test/src/computeAdaptativeCodebookGainTest.c $(util_src)
computeLPTest_SOURCES=$(top_srcdir)/test/src/computeLPTest.c $(util_src)
computeWeightedSpeechTest_SOURCES=$(top_srcdir)/test/src/computeWeightedSpeechTest.c $(util_src)
decodeAdaptativeCodeVectorTest_SOURCES=$(top_srcdir)/test/src/decodeAdaptativeCodeVectorTest.c $(util_src)
decodeFixedCodeVectorTest_SOURCES=$(top_srcdir)/test/src/decodeFixedCodeVectorTest.c $(util_src)
decodeGainsTest_SOURCES=$(top_srcdir)/test/src/decodeGainsTest.c $(util_src)
decodeLSPTest_SOURCES=$(top_srcdir)/test/src/decodeLSPTest.c $(util_src)
decoderTest_SOURCES=$(top_srcdir)/test/src/decoderTest.c $(util_src)
CNGRFC3389decoderTest_SOURCES=$(top_srcdir)/test/src/CNGRFC3389decoderTest.c $(util_src)
CNGdecoderTest_SOURCES=$(top_srcdir)/test/src/CNGdecoderTest.c $(util_src)
decoderMultiChannelTest_SOURCES=$(top_srcdir)/test/src/decoderMultiChannelTest.c $(util_src)
encoderTest_SOURCES=$(top_srcdir)/test/src/encoderTest.c $(util_src)
encoderMultiChannelTest_SOURCES=$(top_srcdir)/test/src/encoderMultiChannelTest.c $(util_src)
findOpenLoopPitchDelayTest_SOURCES=$(top_srcdir)/test/src/findOpenLoopPitchDelayTest.c $(util_src)
fixedCodebookSearchTest_SOURCES=$(top_srcdir)/test/src/fixedCodebookSearchTest.c $(util_src)
g729FixedPointMathTest_SOURCES=$(top_srcdir)/test/src/g729FixedPointMathTest.c $(util_src)
g729FixedPointMathTest_LDADD=-lm
gainQuantizationTest_SOURCES=$(top_srcdir)/test/src/gainQuantizationTest.c $(util_src)
interpolateqLSPAndConvert2LPTest_SOURCES=$(top_srcdir)/test/src/interpolateqLSPAndConvert2LPTest.c $(util_src)
LP2LSPConversionTest_SOURCES=$(top_srcdir)/test/src/LP2LSPConversionTest.c $(util_src)
LPSynthesisFilterTest_SOURCES=$(top_srcdir)/test/src/LPSynthesisFilterTest.c $(util_src)
LSPQuantizationTest_SOURCES=$(top_srcdir)/test/src/LSPQuantizationTest.c $(util_src)
postFilterTest_SOURCES=$(top_srcdir)/test/src/postFilterTest.c $(util_src)
postProcessingTest_SOURCES=$(top_srcdir)/test/src/postProcessingTest.c $(util_src)
preProcessingTest_SOURCES=$(top_srcdir)/test/src/preProcessingTest.c $(util_src)
computeNoiseExcitationTest_SOURCES=$(top_srcdir)/test/src/computeNoiseExcitationTest.c $(util_src)
encoderVADTest_SOURCES=$(top_srcdir)/test/src/encoderVADTest.c $(util_src)

LDADD=	$(top_builddir)/src/libbcg729.la 
AM_CPPFLAGS=-I$(top_srcdir)/include/ -I$(top_srcdir)/src/
AM_LDFLAGS=-all-static
