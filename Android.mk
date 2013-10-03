LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libbcg729

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES =       src/LP2LSPConversion.c \
                        src/LPSynthesisFilter.c \
                        src/LSPQuantization.c \
                        src/adaptativeCodebookSearch.c \
                        src/codebooks.c \
                        src/computeAdaptativeCodebookGain.c \
                        src/computeLP.c \
                        src/computeWeightedSpeech.c \
                        src/decodeAdaptativeCodeVector.c \
                        src/decodeFixedCodeVector.c \
                        src/decodeGains.c \
                        src/decodeLSP.c \
                        src/decoder.c \
                        src/encoder.c \
                        src/findOpenLoopPitchDelay.c \
                        src/fixedCodebookSearch.c \
                        src/gainQuantization.c \
                        src/interpolateqLSP.c \
                        src/postFilter.c \
                        src/postProcessing.c \
                        src/preProcessing.c \
                        src/qLSP2LP.c \
                        src/utils.c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)


