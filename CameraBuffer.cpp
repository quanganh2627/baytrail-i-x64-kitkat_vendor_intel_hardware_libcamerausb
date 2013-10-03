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
 */
#include <cutils/atomic.h>
#include "CameraBuffer.h"
#include "CameraBufferAllocator.h"
#include "LogHelper.h"
namespace android
{

CameraBuffer::CameraBuffer() :
        mCamMem(0), mID(-1),
        mDriverPrivate(0),
        mOwner(0),
        mProcessorCount(0),
        mType(BUFFER_TYPE_INTERMEDIATE),
        mFormat(0),
        mSize(-1),
        mWidth(-1),
        mHeight(-1),
        mData(0),
        mAlloc(0),
        mAllocPrivate(0)
{
}

CameraBuffer::~CameraBuffer()
{
    releaseMemory();
}

int CameraBuffer::getID() const
{
    return mID;
}

void* CameraBuffer::getData()
{
    if (mData == 0)
        mData = mAlloc->map(this);
    return mData;
}

void CameraBuffer::releaseMemory()
{
    if (mAlloc != 0) {
        mAlloc->releaseMemory(this);
    }
}

camera_memory_t* CameraBuffer::getCameraMem()
{
    mAlloc->toMetaDataStream(this);
    return mCamMem;
}

void CameraBuffer::decrementProccessor()
{
    android_atomic_dec(&mProcessorCount);
    // if all decrements done and count is zero
    // return to driver
    int32_t rc = android_atomic_acquire_load(&mProcessorCount);
    if (rc == 0)
        returnToOwner();
}

void CameraBuffer::incrementProcessor()
{
    android_atomic_inc(&mProcessorCount);
}

void CameraBuffer::setOwner(IBufferOwner* o)
{
    if (mOwner == 0)
        mOwner = o;
    else {
        ALOGE("taking ownership from previous owner is not allowed.");
    }
}

void CameraBuffer::returnToOwner()
{
    if (mOwner != 0)
        mOwner->returnBuffer(this);
}
bool CameraBuffer::hasData(void* data) const
{
     if(mAlloc!=0)
         return mAlloc->bufferOwnsThisData(this, data);
     else// buffer has to have an allocator to own data.
         return false;
}

} //namespace
