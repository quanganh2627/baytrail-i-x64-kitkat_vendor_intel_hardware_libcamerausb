/*
* Copyright (c) 2007-2013 Intel Corporation. All Rights Reserved:
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sub license, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice (including the
* next paragraph) shall be included in all copies or substantial portions
* of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
* IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
* ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
*\file VAConvert.h
* TBF
*/
#ifndef GRAPHICBUFFERALLOCATOR_H_
#define GRAPHICBUFFERALLOCATOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <utils/KeyedVector.h>
#include <ui/FramebufferNativeWindow.h>
#include <ui/GraphicBuffer.h>

#include <system/graphics.h>
#include <unistd.h>
#include <hardware/gralloc.h>

#include <intel_bufmgr.h>
#include <i915_drm.h>
#include <drm_fourcc.h>
#include "CameraCommon.h"



namespace android {
class CameraBuffer;

class CamGraphicBufferAllocator{
// public methods
public:
    CamGraphicBufferAllocator();
    ~CamGraphicBufferAllocator();

   status_t allocate(CameraBuffer * gcamBuff,int width, int height, int format);
   status_t free(CameraBuffer * buffer);

// private methods
private:
    status_t init();
    void deinit();
    alloc_device_t *mGrAllocDev;
    struct gralloc_module_t *mGralloc_module;

}; // class GraphicBufferAllocator

struct mfx_gralloc_drm_handle_t {
    native_handle_t base;
    int magic;

    int width;
    int height;
    int format;
    int usage;

    int name;
    int pid;  // creator
    mutable int other;    // registered owner (pid)
    mutable union { int data1; mutable drm_intel_bo *bo; };  // drm buffer object
    union { int data2; uint32_t fb; }; // framebuffer id
    int pitch;    // buffer pitch (in bytes)
    int allocWidth;  // Allocated buffer width in pixels
    int allocHeight;   // Allocated buffer height in lines
};

}; // namespace android

#endif //GRAPHICBUFFERALLOCATOR_H_
