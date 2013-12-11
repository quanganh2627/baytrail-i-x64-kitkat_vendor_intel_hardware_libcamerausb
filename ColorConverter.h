/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_LIBCAMERA_COLOR_CONVERTER_H
#define ANDROID_LIBCAMERA_COLOR_CONVERTER_H

#include <utils/Errors.h>

namespace android {

// New v4l2 packed format used in haswell
#define color_fourcc(a, b, c, d) ((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))
#define V4L2_PIX_FMT_NV12_PACKED color_fourcc('N', 'V', '1', 'P') // 12  Y/CbCr 4:2:0  Packed

status_t colorConvert(int srcFormat, int dstFormat, int width, int height, void *src, void *dst);
status_t colorConvertwithStride(int srcFormat, int dstFormat, int stride,int width, int alignHeight, int height, void *src, void *dst);

const char *cameraParametersFormat(int v4l2Format);
int V4L2Format(const char *cameraParamsFormat);
void YU16ToYV12(int width, int height, void *src, void *dst);
void YU16ToNV12(int width, int height, void *src, void *dst);
void YU16ToYUYV(int width, int height, void *src, void *dst);
void YV12ToNV12(int width, int height, void *src, void *dst);
void NV12ToP411(int width, int height, void *src, void *dst);
int V4L2ToLumaBitsPerPixel(int format);
int V4L2FormatToHalPixel(int format);
int HalPixelToV4L2Format(int format);


}; // namespace android

#endif // ANDROID_LIBCAMERA_COLOR_CONVERTER_H
