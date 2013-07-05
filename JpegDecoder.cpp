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

#ifdef __ANDROID__
# define LOG_TAG "JpegDecoder"
# include <utils/Log.h>
# define LOGERR(...) ALOGE(__VA_ARGS__)
#else
# include <X11/Xlib.h>
# include <stdio.h>
# define LOGERR(...) fprintf(stderr, "ERROR: " __VA_ARGS__)
#endif

#include <string.h>
#include <vector>
#include <algorithm>

#include "JpegDecoder.h"

using namespace std;

#define VACALL(exp)                                                     \
    do {                                                                \
        VAStatus err;                                                   \
        if ((err = exp) == VA_STATUS_SUCCESS)                           \
            break;                                                      \
        LOGERR("libva err %d (\"%s\"): %s\n", err, vaErrorStr(err), #exp); \
        return false;                                                   \
    } while (0)

namespace android {

// MJPEG frames are allowed to omit the DHT marker and assume the
// default huffman tables from the standard.  This table was created
// by inserting a valid Huffman table into a captured MJPEG frame
// using Laurent Pinchart & Luc Saillard's mjpeg2jpeg.py script,
// decoding it using the parser below, and pickling the resulting data
// by hand.  No attempt was made to hand-verify this vs. the
// appropriate standards documents.
const VAHuffmanTableBufferJPEGBaseline JpegDecoder::default_huff = {
    { 1, 1 },
    { { { 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
          0x08, 0x09, 0x0a, 0x0b, },
        { 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
          0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d, },
        { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
          0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22,
          0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42,
          0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62,
          0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a,
          0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36,
          0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
          0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
          0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
          0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
          0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92,
          0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2,
          0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2,
          0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
          0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
          0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1,
          0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
          0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
          0xfa, },
        { 0x00, 0x00, },
        },
      { { 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
          0x08, 0x09, 0x0a, 0x0b, },
        { 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
          0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, },
        { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
          0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13,
          0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1,
          0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72,
          0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17,
          0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35,
          0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46,
          0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57,
          0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
          0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
          0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
          0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
          0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
          0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
          0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
          0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
          0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
          0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
          0xfa, },
        { 0x00, 0x00, },
      },
    },
};

unsigned char JpegDecoder::get8()
{
    if (mBuf < mEnd)
        return *mBuf++;
    if (!mParseDead)
        LOGERR("JPEG parse overrun\n");
    mParseDead = true;
    return -1;
}

int JpegDecoder::get16()
{
    return (get8() << 8) | get8();
}

void JpegDecoder::skip(int n)
{
    mBuf += n;
    if (mBuf > mEnd) {
        if (!mParseDead)
            LOGERR("JPEG parse overrun\n");
        mParseDead = true;
    }
}

JpegDecoder::JpegDecoder(int w, int h)
    : mWidth(w), mHeight(h), mOutBuf(0), mDpy(0), mCfg(0), mSurf(0), mCtx(0), mOutputFormat(OUTPUT_FORMAT_YV12)
{
    mValid = init();
}

JpegDecoder::~JpegDecoder()
{
    if (mOutBuf)
        free(mOutBuf);
    if (mImg.image_id)
        vaDestroyImage(mDpy, mImg.image_id);
    if (mCtx)
        vaDestroyContext(mDpy, mCtx);
    if (mSurf)
        vaDestroySurfaces(mDpy, &mSurf, 1);
    if (mCfg)
        vaDestroyConfig(mDpy, mCfg);
    if (mDpy)
        vaTerminate(mDpy);
}

bool JpegDecoder::init()
{
#ifdef __ANDROID__
    // The native display argument is ignored on Android, except to
    // test vs. null.
    mDpy = vaGetDisplay(this);
#else
    // Hack; leaks the resulting Display.  But fine for testing under X...
    mDpy = vaGetDisplay(XOpenDisplay(NULL));
#endif

    memset(&mImg, 0, sizeof(mImg));
    memset(&mPicParm, 0, sizeof(mPicParm));
    memset(&mIQMat, 0, sizeof(mIQMat));
    memset(&mHuff, 0, sizeof(mHuff));
    memset(&mSliceParm, 0, sizeof(mSliceParm));

    int vmajor, vminor;
    VACALL(vaInitialize(mDpy, &vmajor, &vminor));

    // Check for a VLD entrypoint
    vector<VAEntrypoint> eps(vaMaxNumEntrypoints(mDpy));
    int nep;
    VACALL(vaQueryConfigEntrypoints(mDpy, VAProfileJPEGBaseline, &eps[0], &nep));
    bool found = false;
    for (int i=0; i<nep; i++)
        if (eps[i] == VAEntrypointVLD)
            found = true;
    if (!found) {
        LOGERR("VAProfileJPEGBaseline entrypoint not found\n");
        return false;
    }

    // Check for YUV420 format support.
    VAConfigAttrib attr;
    attr.type = VAConfigAttribRTFormat;
    VACALL(vaGetConfigAttributes(mDpy, VAProfileJPEGBaseline, VAEntrypointVLD, &attr, 1));
    if (!(attr.value & VA_RT_FORMAT_YUV422)) {
        LOGERR("YUV422 format not supported in libva attributes\n");
        return false;
    }

    VACALL(vaCreateConfig(mDpy, VAProfileJPEGBaseline, VAEntrypointVLD, &attr, 1, &mCfg));

    VASurfaceAttrib forcc;
    forcc.type = VASurfaceAttribPixelFormat;
    forcc.flags = VA_SURFACE_ATTRIB_SETTABLE;
    forcc.value.type = VAGenericValueTypeInteger;
    forcc.value.value.i = VA_FOURCC_422H;
    VACALL(vaCreateSurfaces(mDpy, VA_RT_FORMAT_YUV422, mWidth, mHeight, &mSurf, 1, &forcc, 1));

    VACALL(vaCreateContext(mDpy, mCfg, mWidth, mHeight, VA_PROGRESSIVE, &mSurf, 1, &mCtx));

    // Create an "image" to extract the data.  We are supposed to be
    // able to use vaDeriveImage() to produce a handle to the internal
    // image buffer, but that fails with the current intel-driver
    // staging tree.  On IVB, the internal storage is "IMC1",
    // (separate U and V planes, but with a stride equal to the Y
    // buffer -- essentially they're packed into the "left hand side"
    // of a second full-size image) that isn't handled by the code in
    // i965_DeriveImage.  See:
    // https://bugs.freedesktop.org/show_bug.cgi?id=62304
    VAImageFormat *ifmt = 0;
    int n = 0;
    vector<VAImageFormat> fmts(vaMaxNumImageFormats(mDpy));
    VACALL(vaQueryImageFormats(mDpy, &fmts[0], &n));
    for (int i=0; i<n; i++) {
        if (fmts[i].fourcc == VA_FOURCC_NV12)
            ifmt = &fmts[i];
    }
    if (!ifmt) {
        LOGERR("can't find NV12 image format\n");
        return false;
    }
    mOutBuf = malloc(mWidth * mHeight * 2); // temp code

    return true;
}

bool JpegDecoder::decodeJpeg(const unsigned char *buf, int len)
{
    mParseDead = false;
    mBuf = buf;
    mEnd = buf + len;
    mMaxHSamp = mMaxVSamp = 0;
    mHaveHuff = false;

    for (int i=0; i<4; i++)
        mIQMat.load_quantiser_table[i] = 0;
    for (int i=0; i<2; i++)
        mHuff.load_huffman_table[i] = 0;
    mSliceParm.slice_data_offset = 0;
    mSliceParm.slice_horizontal_position = 0;
    mSliceParm.slice_vertical_position = 0;

    if (!parse())
        return false;

    mSliceParm.slice_data_size = mEnd - mBuf;
    mSliceParm.slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

    mSliceParm.num_mcus = ((mWidth  + 8*mMaxHSamp - 1) / (8*mMaxHSamp) *
                           (mHeight + 8*mMaxVSamp - 1) / (8*mMaxVSamp));

    return decode();
}

bool JpegDecoder::parse()
{
#define ERR(s) { LOGERR(s); mParseDead = true; return false; }
    while (!mParseDead) {
        if (*mBuf != 0xff)
            ERR("Missing marker start code\n");
        while (*mBuf == 0xff && !mParseDead)
            get8();

        int id = get8();
        if (id == 0xd8)
            continue; // Start of JFIF, no length
        int len = get16();

        if (id == 0xc0 /* SOF: Start of Frame */) {
            skip(1); // FIXME: what is this byte?
            mPicParm.picture_height = get16();
            mPicParm.picture_width = get16();
            if (mPicParm.picture_height != mHeight || mPicParm.picture_width != mWidth)
                ERR("invalid frame size")
            mPicParm.num_components = get8();
            for (int i=0; i<mPicParm.num_components; i++) {
                mPicParm.components[i].component_id = get8();
                int hvsamples = get8();
                int hsamp = hvsamples >> 4;
                int vsamp = hvsamples & 0x0f;
                mMaxHSamp = max(mMaxHSamp, hsamp);
                mMaxVSamp = max(mMaxVSamp, vsamp);
                mPicParm.components[i].h_sampling_factor = hsamp;
                mPicParm.components[i].v_sampling_factor = vsamp;
                mPicParm.components[i].quantiser_table_selector = get8();
            }
        } else if (id == 0xdb /* DQT: Define Quantization Table */) {
            int idx = get8();
            if (idx >= 4)
                ERR("quantiser_table index too high\n");
            mIQMat.load_quantiser_table[idx] = 1;
            memcpy(mIQMat.quantiser_table[idx], mBuf, 64);
            skip(64);
        } else if (id == 0xc4 /* Huffman Table */) {
            mHaveHuff = true;
            // These loop over the segment length, rejigger "mEnd" to detect errors:
            const unsigned char* end0 = mEnd;
            mEnd = mBuf + len - 2;
            while (mBuf < mEnd && !mParseDead) {
                int packedidx = get8();
                bool is_dc = !(packedidx & 0xf0);
                int idx = packedidx & 0x0f;
                if (idx >= 2)
                    ERR("huffman table index too high\n");
                const unsigned char *codes_src = mBuf;

                int tblsz = 0;
                for (int i=0; i<16; i++)
                    tblsz += get8();
                if ((is_dc && tblsz > 12) || tblsz > 162)
                    ERR("huffman table too big\n");
                const unsigned char *val_src = mBuf;
                skip(tblsz);

                unsigned char *codes_dst, *val_dst;
                if (is_dc) {
                    codes_dst = mHuff.huffman_table[idx].num_dc_codes;
                    val_dst = mHuff.huffman_table[idx].dc_values;
                } else {
                    codes_dst = mHuff.huffman_table[idx].num_ac_codes;
                    val_dst = mHuff.huffman_table[idx].ac_values;
                }
                mHuff.load_huffman_table[idx] = 1;
                memcpy(codes_dst, codes_src, 16);
                memcpy(val_dst, val_src, tblsz);
            }
            mEnd = end0;
        } else if (id == 0xdd /* DRI: Define Restart Interval */) {
            mSliceParm.restart_interval = get16();
        } else if (id == 0xda /* SOS: Start of Scan */) {
            int nc = get8();
            mSliceParm.num_components = nc;
            for (int i=0; i<nc; i++) {
                mSliceParm.components[i].component_selector = get8();
                int dcac = get8();
                int dc = dcac >> 4;
                int ac = dcac & 0x0f;
                mSliceParm.components[i].dc_table_selector = dc;
                mSliceParm.components[i].ac_table_selector = ac;
            }
            skip(3);
            break; // This is the last JFIF marker by definition
        } else {
            // 0xEn == APPn, probably EXIF data.  Ignore those.
            if (!(id >= 0xe0 && id <= 0xef))
                LOGERR("Unrecognized JFIF marker 0x%2.2x, skipping...\n", id);
            skip(len - 2);
        }
    }
    return !mParseDead;
#undef ERR
}

bool JpegDecoder::decode()
{
    const VAHuffmanTableBufferJPEGBaseline *huff = mHaveHuff ? &mHuff : &default_huff;

    // Create the buffers to be rendered
    VABufferID bufs[5];
    int nbuf = 0;
    VACALL(vaCreateBuffer(mDpy, mCtx, VAPictureParameterBufferType,
                          sizeof(mPicParm), 1, &mPicParm, &bufs[nbuf++]));
    VACALL(vaCreateBuffer(mDpy, mCtx, VAIQMatrixBufferType,
                          sizeof(mIQMat), 1, &mIQMat, &bufs[nbuf++]));
    VACALL(vaCreateBuffer(mDpy, mCtx, VAHuffmanTableBufferType,
                          sizeof(*huff), 1, (void*)huff, &bufs[nbuf++]));
    VACALL(vaCreateBuffer(mDpy, mCtx, VASliceParameterBufferType,
                          sizeof(mSliceParm), 1, &mSliceParm, &bufs[nbuf++]));
    VACALL(vaCreateBuffer(mDpy, mCtx, VASliceDataBufferType,
                          mEnd-mBuf, 1, (void*)mBuf, &bufs[nbuf++]));

    // Do the deed
    VACALL(vaBeginPicture(mDpy, mCtx, mSurf));
    VACALL(vaRenderPicture(mDpy, mCtx, bufs, nbuf));
    VACALL(vaEndPicture(mDpy, mCtx));
    VACALL(vaSyncSurface(mDpy, mSurf));

    VACALL(vaDeriveImage(mDpy, mSurf, &mImg));
    void * surface_p = NULL;
    VACALL(vaMapBuffer(mDpy, mImg.buf, &surface_p));

    if (mOutputFormat == OUTPUT_FORMAT_YV12)
        OutSize = dumpYV12(mImg, surface_p, mWidth, mHeight, mOutBuf);
    else if (mOutputFormat == OUTPUT_FORMAT_YU16)
        OutSize = dumpYU16(mImg, surface_p, mWidth, mHeight, mOutBuf);
    else if (mOutputFormat == OUTPUT_FORMAT_YUYV)
        OutSize = dumpYUYV(mImg, surface_p, mWidth, mHeight, mOutBuf);

    VACALL(vaUnmapBuffer(mDpy, mImg.buf));
    VACALL(vaDestroyImage(mDpy, mImg.image_id));

    return true;
}

int JpegDecoder::dumpYV12(VAImage va_image, void *pImage_Src, int actW, int actH, void *PDst)
{
    int num_bytes, nWidth, nHeight, nAWidth, nAHeight;
    int y_bytes, u_bytes, v_bytes;
    unsigned char *pSrc_Y, *pSrc_UV, *pDst_Y, *pDst_U, *pDst_V, *pSrcTmp, *pSrc_U, *pSrc_V;
    int i, j;
    int realHeight = (va_image.height > mHeight) ? mHeight : va_image.height;

    pSrc_Y = (unsigned char *)pImage_Src;
    pSrc_U = pSrc_Y + va_image.offsets[1];
    pSrc_V = pSrc_U + va_image.offsets[2];

    // Y
    nWidth =  va_image.width;
    nHeight = realHeight;
    y_bytes = num_bytes = nWidth * nHeight;
    pDst_Y = (unsigned char *)PDst;
    for (i = 0; i < nHeight; i++)
        memcpy(pDst_Y + i * nWidth, pSrc_Y + i * va_image.pitches[0], nWidth);

    pSrc_V = pSrc_U + va_image.height * va_image.pitches[1];
    //V
    pDst_V = pDst_Y + num_bytes;
    nWidth =  va_image.width / 2;
    nHeight = realHeight / 2;
    v_bytes = num_bytes = nWidth * nHeight;

    for (i = 0; i < nHeight; i++)
        memcpy(pDst_V + i * nWidth, pSrc_V + 2 * i * va_image.pitches[2], nWidth);

    //U
    pDst_U = pDst_V + num_bytes;
    nWidth =  va_image.width / 2;
    nHeight = realHeight / 2;
    u_bytes = num_bytes = nWidth * nHeight;

    for (i = 0; i < nHeight; i++)
        memcpy(pDst_U + i * nWidth, pSrc_U + 2 * i * va_image.pitches[1], nWidth);

    num_bytes = y_bytes + u_bytes + v_bytes;
    return num_bytes;
}

// the output is YU16
// the input is YUV422H MJPEG
int JpegDecoder::dumpYU16(VAImage va_image, void *pImage_Src, int actW, int actH, void *PDst)
{
    int num_bytes, nWidth, nHeight, nAWidth, nAHeight;
    int y_bytes, u_bytes, v_bytes;
    unsigned char *pSrc_Y, *pSrc_UV, *pDst_Y, *pDst_U, *pDst_V, *pSrcTmp, *pSrc_U, *pSrc_V;
    int i, j;
    int realHeight = (va_image.height > mHeight) ? mHeight : va_image.height;

    pSrc_Y = (unsigned char *)pImage_Src;
    pSrc_U = pSrc_Y + va_image.offsets[1];
    pSrc_V = pSrc_U + va_image.offsets[2];

    // Y
    nWidth =  va_image.width;
    nHeight = realHeight;
    y_bytes = num_bytes = nWidth * nHeight;
    pDst_Y = (unsigned char *)PDst;
    for (i = 0; i < nHeight; i++)
        memcpy(pDst_Y + i * nWidth, pSrc_Y + i * va_image.pitches[0], nWidth);

    //U
    pDst_U = pDst_Y + num_bytes;
    nWidth =  va_image.width / 2;
    nHeight = realHeight;
    u_bytes = num_bytes = nWidth * nHeight;
    for (i = 0; i < nHeight; i++)
        memcpy(pDst_U + i * nWidth, pSrc_U + i * va_image.pitches[1], nWidth);

    pSrc_V = pSrc_U + va_image.height * va_image.pitches[1];
    //V
    pDst_V = pDst_U + num_bytes;
    nWidth =  va_image.width / 2;
    nHeight = realHeight;
    v_bytes = num_bytes = nWidth * nHeight;
    for (i = 0; i < nHeight; i++)
        memcpy(pDst_V + i * nWidth, pSrc_V + i * va_image.pitches[2], nWidth);

    num_bytes = y_bytes + u_bytes + v_bytes;
    return num_bytes;
}

//YUYV:the same as YUY2. "Y0U0Y1V0 Y2U2Y3V2..."
int JpegDecoder::dumpYUYV(VAImage va_image, void *pImage_Src, int actW, int actH, void *PDst)
{
    int nWidth, nHeight;
    unsigned char *pSrc_Y, *pSrc_U, *pSrc_V, *p, *y, *u, *v;
    int i, j;
    int realHeight = (va_image.height > mHeight) ? mHeight : va_image.height;

    pSrc_Y = (unsigned char *)pImage_Src;
    pSrc_U = pSrc_Y + va_image.offsets[1];
    pSrc_V = pSrc_U + va_image.height * va_image.pitches[1];

    nWidth =  va_image.width / 2;
    nHeight = realHeight;
    p = (unsigned char *)PDst;
    for (i = 0; i < nHeight; i++) {
        y = pSrc_Y + i * va_image.pitches[0];
        u = pSrc_U + i * va_image.pitches[1];
        v = pSrc_V + i * va_image.pitches[2];
        for (j = 0; j < nWidth; j++) {
            *p++ = *y++;
            *p++ = *u++;
            *p++ = *y++;
            *p++ = *v++;
        }
    }

    return (va_image.width * realHeight * 2);
}

void JpegDecoder::configOutputFormat(OutputFormat fmt) {

    if (fmt < OUTPUT_FORMAT_YV12 || fmt < OUTPUT_FORMAT_YUYV)
        return;
    mOutputFormat = fmt;
}

}; // namespace android
