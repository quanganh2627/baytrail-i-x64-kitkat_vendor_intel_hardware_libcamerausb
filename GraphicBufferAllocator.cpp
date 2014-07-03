/*
 * Copyright (C) 2012 The Android Open Source Project
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
 *
 */
#define LOG_TAG "Camera_GraphicBufferAllocator"

#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "IntelMetadataBuffer.h"
#include "CameraBuffer.h"
#include "LogHelper.h"
#include "CameraCommon.h"
#include "ColorConverter.h"
#include "VAConvertor.h"
#include "GraphicBufferAllocator.h"


namespace android
{
CamGraphicBufferAllocator::CamGraphicBufferAllocator():
        mGrAllocDev(NULL)
        ,mGralloc_module(NULL)
{
     init();
}

CamGraphicBufferAllocator::~CamGraphicBufferAllocator()
{
     deinit();
}

status_t CamGraphicBufferAllocator::init()
{
    int ret = NO_ERROR;
    hw_module_t const* module = NULL;
    LOG1("@%s", __FUNCTION__);

    ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if(ret != NO_ERROR)
    {
        LOGE("hw_get_module error");
        return ret;
    }
    mGralloc_module = (struct gralloc_module_t*)module;
    ret = gralloc_open(module, &mGrAllocDev);
    if(ret != NO_ERROR)
    {
        LOGE("gralloc_open error");
        return ret;
    }

    return NO_ERROR;
}
void CamGraphicBufferAllocator::deinit()
{
    LOG1("@%s", __FUNCTION__);
    gralloc_close(mGrAllocDev);
    mGrAllocDev =NULL;
}

status_t CamGraphicBufferAllocator::allocate(CameraBuffer * gcamBuff,int width, int height, int format)
{
    int res = NO_ERROR;
    int stride = 0;
    int HalFormat = 0;
    int alignedheight = 0;

    buffer_handle_t handle;
    struct mfx_gralloc_drm_handle_t *pGrallocHandle;
    unsigned long boname;
    uint32_t fourcc;
    int bpp;

    LOG1("@%s,gcamBuff=%p", __FUNCTION__,gcamBuff);
    memset((void*)gcamBuff,0,sizeof(CameraBuffer));
    if(mGralloc_module == NULL)
    {
        ALOGE("not init, so do init");
        init();
    }
    gcamBuff->mGralloc_module = mGralloc_module;
    if(format == V4L2_PIX_FMT_NV21)
    {
        //currently, gralloc don't support nv21, so change to yv12 first, then colorconvert to yv12 with cpu
        format = V4L2_PIX_FMT_YVU420;
    }
    HalFormat = V4L2FormatToHalPixel(format);
    if(HalFormat == HAL_PIXEL_FORMAT_NV12_TILED_INTEL) // for video encoder
    {
       gcamBuff->mType = BUFFER_TYPE_VIDEOENCODER;
    }
    else if(HalFormat == HAL_PIXEL_FORMAT_YCbCr_422_H_INTEL)
    {
       gcamBuff->mType = BUFFER_TYPE_JPEGDEC;
    }
    alignedheight = ((height == 120) ||(height == 1080))? ALIGN(height,32): height;//We need to work with VPG team and deliver the correct ufo.
    res = mGrAllocDev->alloc(mGrAllocDev, width, alignedheight,
            HalFormat,
            GRALLOC_USAGE_HW_RENDER,
            &handle, &stride);
    if(res != NO_ERROR)
    {
       ALOGE("alloc failed");
       return res;
    }
    res = mGralloc_module->perform(mGralloc_module,INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_NAME, handle, &boname);
    if(res != NO_ERROR)
    {
       ALOGE("alloc failed");
       return res;
    }
    pGrallocHandle = (struct mfx_gralloc_drm_handle_t *)handle;
    gcamBuff->mStride = pGrallocHandle->pitch;
    if((HalFormat == HAL_PIXEL_FORMAT_YV12) || (HalFormat == HAL_PIXEL_FORMAT_NV12_TILED_INTEL))
    {
         gcamBuff->mGraBuffSize = pGrallocHandle->pitch * height *3/2;
    }
    else if((HalFormat == HAL_PIXEL_FORMAT_YCbCr_422_H_INTEL) ||(HalFormat == HAL_PIXEL_FORMAT_YCbCr_422_I))
    {
        gcamBuff->mGraBuffSize = pGrallocHandle->pitch * height *3;
    }
    gcamBuff->mGrhandle = handle;

    bpp  = V4L2ToLumaBitsPerPixel(format);
    gcamBuff->mDecTargetBuf = new RenderTarget();
    if(gcamBuff->mDecTargetBuf == NULL)
    {
        ALOGE("gcamBuff->mDecTargetBuf == NULL");
        return -1;
    }
    gcamBuff->mDecTargetBuf->type = RenderTarget::KERNEL_DRM;
    gcamBuff->mDecTargetBuf->handle = boname;
    gcamBuff->mDecTargetBuf->width = width;
    gcamBuff->mDecTargetBuf->height = alignedheight;
    gcamBuff->mDecTargetBuf->pixel_format = HalFormat;
    gcamBuff->mDecTargetBuf->rect.x = gcamBuff->mDecTargetBuf->rect.y = 0;
    gcamBuff->mDecTargetBuf->rect.width = gcamBuff->mDecTargetBuf->width;
    gcamBuff->mDecTargetBuf->rect.height = height;
    gcamBuff->mDecTargetBuf->stride = stride * bpp;
    return NO_ERROR;
}
status_t CamGraphicBufferAllocator::free(CameraBuffer * buffer)
{
    LOG1("@%s buffer=%p", __FUNCTION__,buffer);

    if((mGrAllocDev != NULL) && (buffer != NULL))
    {
       if(buffer->mDecTargetBuf !=NULL)
           delete buffer->mDecTargetBuf;
       if(buffer->mGrhandle)
          mGrAllocDev->free(mGrAllocDev, (buffer->mGrhandle));
   }
   else
   {
       ALOGE("mGrAllocDev ==NULL");
       return -1;
   }
    return NO_ERROR;
}
}//namespace




