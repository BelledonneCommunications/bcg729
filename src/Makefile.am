
lib_LTLIBRARIES = libbcg729.la


libbcg729_la_SOURCES=	LP2LSPConversion.c \
			LPSynthesisFilter.c \
			LSPQuantization.c \
			adaptativeCodebookSearch.c \
			codebooks.c \
			computeAdaptativeCodebookGain.c \
			computeLP.c \
			computeWeightedSpeech.c \
			decodeAdaptativeCodeVector.c \
			decodeFixedCodeVector.c \
			decodeGains.c \
			decodeLSP.c \
			decoder.c \
			encoder.c \
			findOpenLoopPitchDelay.c \
			fixedCodebookSearch.c \
			gainQuantization.c \
			interpolateqLSP.c \
			postFilter.c \
			postProcessing.c \
			preProcessing.c \
			qLSP2LP.c \
			utils.c \
			cng.c \
			vad.c \
			dtx.c

libbcg729_la_LDFLAGS= -no-undefined

AM_CPPFLAGS= -I$(top_srcdir)/include

AM_CFLAGS=$(VISIBILITY_CFLAGS)

private_headers= adaptativeCodebookSearch.h \
                basicOperationsMacros.h \
                codebooks.h \
                codecParameters.h \
                computeAdaptativeCodebookGain.h \
                computeLP.h \
                computeWeightedSpeech.h \
		cng.h \
                decodeAdaptativeCodeVector.h \
                decodeFixedCodeVector.h \
                decodeGains.h \
                decodeLSP.h \
		dtx.h \
                findOpenLoopPitchDelay.h \
                fixedCodebookSearch.h \
                fixedPointMacros.h \
                floatingPointMacros.h \
                g729FixedPointMath.h \
                gainQuantization.h \
                interpolateqLSP.h \
                LP2LSPConversion.h \
                LPSynthesisFilter.h \
                LSPQuantization.h \
                postFilter.h \
                postProcessing.h \
                preProcessing.h \
                qLSP2LP.h \
                typedef.h \
                utils.h \
		vad.h

EXTRA_DIST=$(private_headers)
