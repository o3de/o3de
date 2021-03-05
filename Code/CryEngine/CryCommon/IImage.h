/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_IIMAGE_H
#define CRYINCLUDE_CRYCOMMON_IIMAGE_H
#pragma once

/**
 * Possible errors for IImageFile::mfGet_error.
 */
enum EImFileError
{
    eIFE_OK = 0, eIFE_IOerror, eIFE_OutOfMemory, eIFE_BadFormat, eIFE_ChunkNotFound
};

// here are all of the flags that can be passed into the image load flags
#define FIM_NORMALMAP               0x0001
#define FIM_NOTSUPPORTS_MIPS        0x0004
#define FIM_ALPHA                   0x0008  // request attached alpha image
#define FIM_DECAL                   0x0010
#define FIM_GREYSCALE               0x0020  // hint this texture is greyscale (could be DXT1 with colored artifacts)
#define FIM_STREAM_PREPARE          0x0080
#define FIM_UNUSED_BIT              0x0100  // Free to use
#define FIM_BIG_ENDIANNESS          0x0400  // for textures converted to big endianness format
#define FIM_SPLITTED                0x0800  // for dds textures stored in splitted files
#define FIM_SRGB_READ               0x1000
#define FIM_X360_NOT_PRETILED       0x2000  // for dds textures that cannot be pretiled
#define FIM_UNUSED_BIT_1            0x4000  // Free to use
#define FIM_RENORMALIZED_TEXTURE    0x8000  // for dds textures with EIF_RenormalizedTexture set in the dds header (not currently supported in the engine at runtime)
#define FIM_HAS_ATTACHED_ALPHA      0x10000 // image has an attached alpha image
#define FIM_SUPPRESS_DOWNSCALING    0x20000 // don't allow to drop mips when texture is non-streamable
#define FIM_DX10IO                  0x40000 // for dds textures with extended DX10+ header
#define FIM_NOFALLBACKS             0x80000 // if the texture can't be loaded or is not found, do not replace it with a default 'not found' texture.

class IImageFile
{
public:
    virtual int AddRef() = 0;
    virtual int Release() = 0;

    virtual const string& mfGet_filename () const = 0;

    virtual int mfGet_width () const = 0;
    virtual int mfGet_height () const = 0;
    virtual int mfGet_depth () const = 0;
    virtual int mfGet_NumSides () const = 0;

    virtual EImFileError mfGet_error () const = 0;

    virtual byte* mfGet_image (const int nSide) = 0;
    virtual bool mfIs_image (const int nSide) const = 0;

    virtual ETEX_Format mfGetFormat() const = 0;
    virtual ETEX_TileMode mfGetTileMode() const = 0;
    virtual int  mfGet_numMips () const = 0;
    virtual int mfGet_numPersistentMips () const = 0;
    virtual int mfGet_Flags () const = 0;
    virtual const ColorF& mfGet_minColor () const = 0;
    virtual const ColorF& mfGet_maxColor () const = 0;
    virtual int mfGet_ImageSize() const = 0;

protected:
    virtual ~IImageFile() {}
};

#endif // CRYINCLUDE_CRYCOMMON_IIMAGE_H
