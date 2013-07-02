ifeq ($(USE_CAMERA_STUB),false)
ifeq ($(USE_CAMERA_USB), true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
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
	EXIFFields.cpp \
	JpegCompressor.cpp \
	CameraBuffer.cpp \
	CameraBufferAllocator.cpp \
	JpegDecoder.cpp \
	JpegEncoder.cpp

LOCAL_C_INCLUDES += \
	frameworks/base/include \
	frameworks/base/include/binder \
	frameworks/base/include/camera \
	system/core/include/cutils \
	external/jpeg \
	hardware/libhardware/include \
	external/skia/include/core \
	external/skia/include/images \
	$(TARGET_OUT_HEADERS)/libdrm \
	$(TARGET_OUT_HEADERS)/libmix_videoencoder \
	vendor/intel/hardware/libva \
	bionic \
	external/stlport/stlport \

LOCAL_SHARED_LIBRARIES := \
	libcamera_client \
	libutils \
	libcutils \
	libbinder \
	libskia \
	libandroid \
	libui \
	libdrm \
	libdrm_intel \
	libintelmetadatabuffer \
	libstlport \
	libva-android \
	libva \

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE := camera.$(TARGET_DEVICE)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif #ifeq ($(USE_CAMERA_STUB),false)
endif #ifeq ($(USE_CAMERA_USB), true)
