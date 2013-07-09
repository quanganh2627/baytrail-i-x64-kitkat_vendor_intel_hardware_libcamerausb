ifeq ($(USE_CAMERA_USB), true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        DumpImage.cpp \
	ControlThread.cpp \
	PreviewThread.cpp \
	PictureThread.cpp \
	VideoThread.cpp \
	PipeThread.cpp \
	CameraDriver.cpp \
	DebugFrameRate.cpp \
	Callbacks.cpp \
	CallbacksThread.cpp \
	CameraHAL.cpp \
	ColorConverter.cpp \
        VAConvertor.cpp \
        EXIFFields.cpp \
	JpegCompressor.cpp \
	CameraBuffer.cpp \
	CameraBufferAllocator.cpp \
        GraphicBufferAllocator.cpp \
	JpegEncoder.cpp


LOCAL_C_INCLUDES += \
	external/PRIVATE/drm/include/drm \
	external/PRIVATE/drm/intel \
	external/jpeg \
	vendor/intel/hardware/PRIVATE/libmix/videoencoder \
	$(call include-path-for, libmix_videovpp) \
	vendor/intel/hardware/PRIVATE/libmix/imagedecoder \
	vendor/intel/hardware/libva \
	bionic \
	$(call include-path-for, frameworks-base) \
	$(call include-path-for, frameworks-base)/binder \
	$(call include-path-for, frameworks-base)/camera \
	$(call include-path-for, system-core)/cutils \
	$(call include-path-for, libjpeg) \
	$(call include-path-for, libhardware) \
	$(call include-path-for, skia)/core \
	$(call include-path-for, skia)/images \
	$(TARGET_OUT_HEADERS)/libdrm \
	$(TARGET_OUT_HEADERS)/libmix_videoencoder \
	$(TARGET_OUT_HEADERS)/libmix_videovpp \
	$(TARGET_OUT_HEADERS)/libjpegdec \
	hardware/intel/PRIVATE/libmix/videoencoder \
	vendor/intel/hardware/libva \
	bionic \
	$(call include-path-for, stlport) \

LOCAL_SHARED_LIBRARIES := \
	libcamera_client \
	libutils \
	libcutils \
	libbinder \
	libskia \
	libui \
	libdrm \
	libdrm_intel \
	libintelmetadatabuffer \
	libstlport \
	libva-android \
	libva \
        libmix_videovpp \
        libjpegdec \
        libhardware \
        libexpat \

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE := camera.$(TARGET_PRODUCT)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif #ifeq ($(USE_CAMERA_USB), true)
