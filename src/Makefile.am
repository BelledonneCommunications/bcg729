if BUILD_MSBCG729
noinst_LTLIBRARIES = libbcg729.la
else
lib_LTLIBRARIES = libbcg729.la
endif

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
			utils.c

libbcg729_la_LDFLAGS= -no-undefined

AM_CPPFLAGS= -I$(top_srcdir)/include

AM_CFLAGS=$(VISIBILITY_CFLAGS)
