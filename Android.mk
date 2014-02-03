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
	frameworks/base/include/binder \
	frameworks/base/include/camera \
	system/core/include/cutils \
	hardware/libhardware/include \
	external/jpeg \
	external/PRIVATE/drm/include/drm \
	external/PRIVATE/drm/intel \
	external/skia/include/core \
	external/skia/include/images \
	external/stlport/stlport \
	vendor/intel/hardware/PRIVATE/libmix/videoencoder \
	vendor/intel/hardware/PRIVATE/libmix/videovpp \
	vendor/intel/hardware/PRIVATE/libmix/imagedecoder \
	vendor/intel/hardware/libva \
	bionic \

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
