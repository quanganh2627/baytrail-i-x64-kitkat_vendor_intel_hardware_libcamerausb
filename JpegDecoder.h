/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef _JPEGDECODER_H
#define _JPEGDECODER_H

#include <va/va.h>
#ifdef __ANDROID__
# include <va/va_android.h>
#else
# include <X11/Xlib.h>
# include <va/va_x11.h>
#endif

namespace android {

// Hardware JPEG decoder using libva acceleration
class JpegDecoder {
public:
    JpegDecoder(int w, int h);
    virtual ~JpegDecoder();

    bool decodeJpeg(const unsigned char *buf, int len);

    bool valid() { return mValid; }

    // Data is returned in a mapped YUY2 buffer
    void *data() { return mOutBuf; }
    size_t dataSize() { return OutSize; }

private:
    bool init();
    bool parse();
    bool decode();

    // Parser utilities
    unsigned char get8();
    int get16();
    void skip(int n);

    int dumpYV16(VAImage va_image, void *pImage_Src, int actW, int actH, void *PDst);
    int dumpYV12(VAImage va_image, void *pImage_Src, int actW, int actH, void *PDst);


    int mWidth, mHeight;
    bool mValid;

    void *mOutBuf;
    int OutSize;

    // Parser scratch data
    bool mParseDead;
    const unsigned char *mBuf, *mEnd;
    int mMaxHSamp, mMaxVSamp;
    bool mHaveHuff;

    // libva buffer objects
    VAPictureParameterBufferJPEGBaseline mPicParm;
    VAIQMatrixBufferJPEGBaseline mIQMat;
    VAHuffmanTableBufferJPEGBaseline mHuff;
    VASliceParameterBufferJPEGBaseline mSliceParm;

    static const VAHuffmanTableBufferJPEGBaseline default_huff;

    // libva state handles
    VADisplay mDpy;
    VAConfigID mCfg;
    VASurfaceID mSurf;
    VAContextID mCtx;
    VAImage mImg;
};

}; //namespace android
#endif // _JPEGDECODER_H
