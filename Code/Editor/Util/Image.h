/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Generic image class


#include <ITexture.h>
#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGE_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGE_H
#pragma once
#include "MemoryBlock.h"


#include "Util/XmlArchive.h"

enum class ImageRotationDegrees
{
    Rotate0,
    Rotate90,
    Rotate180,
    Rotate270
};

/*!
 *  Templated image class.
 */

template <class T>
class TImage
{
public:
    TImage()
        : m_data(0)
        , m_width(0)
        , m_height(0)
        , m_bHasAlphaChannel(false)
        , m_bIsLimitedHDR(false)
        , m_bIsCubemap(false)
        , m_bIsSRGB(true)
        , m_nNumberOfMipmaps(1)
        , m_format(eTF_Unknown)
        , m_strDccFilename("")
    {
    }
    virtual ~TImage() {}

    T&  ValueAt(int x, int y) { return m_data[x + y * m_width]; }
    const T& ValueAt(int x, int y) const { return m_data[x + y * m_width]; }

    const T& ValueAtSafe(int x, int y) const
    {
        static T zero = 0;
        if (0 <= x && x < m_width && 0 <= y && y < m_height)
        {
            return m_data[x + y * m_width];
        }
        return zero;
    }

    T*  GetData() const { return m_data; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    bool                    HasAlphaChannel() const { return m_bHasAlphaChannel; }
    bool                    IsLimitedHDR() const { return m_bIsLimitedHDR; }
    bool                    IsCubemap() const { return m_bIsCubemap; }
    unsigned int  GetNumberOfMipMaps() const { return m_nNumberOfMipmaps; }

    // Returns:
    //  size in bytes
    int GetSize() const { return m_width * m_height * sizeof(T); }

    bool IsValid() const { return m_data != 0; }

    void Attach(T* data, int width, int height)
    {
        assert(data);
        m_memory = new CMemoryBlock();
        m_memory->Attach(data, width * height * sizeof(T));
        m_data = data;
        m_width = width;
        m_height = height;
        m_strDccFilename = "";
    }
    void Attach(const TImage<T>& img)
    {
        assert(img.IsValid());
        m_memory = img.m_memory;
        m_data = (T*)m_memory->GetBuffer();
        m_width = img.m_width;
        m_height = img.m_height;
        m_strDccFilename = img.m_strDccFilename;
    }
    void Detach()
    {
        m_memory = 0;
        m_data = 0;
        m_width = 0;
        m_height = 0;
        m_strDccFilename = "";
    }

    bool Allocate(int width, int height)
    {
        if (width < 1)
        {
            width = 1;
        }
        if (height < 1)
        {
            height = 1;
        }

        if (m_data && (m_width == width && m_height == height))
        {
            return true;
        }

        // New memory block.
        m_memory = new CMemoryBlock();
        m_memory->Allocate(width * height * sizeof(T)); // +width for crash safety.
        m_data = (T*)m_memory->GetBuffer();
        m_width = width;
        m_height = height;
        if (!m_data)
        {
            return false;
        }
        return true;
    }

    void Release()
    {
        m_memory = 0;
        m_data = 0;
        m_width = 0;
        m_height = 0;
        m_strDccFilename = "";
    }

    // Copy operator.
    void Copy(const TImage<T>& img)
    {
        if (!img.IsValid())
        {
            return;
        }
        if (m_width != img.GetWidth() || m_height != img.GetHeight())
        {
            Allocate(img.GetWidth(), img.GetHeight());
        }
        *m_memory = *img.m_memory;
        m_data = (T*)m_memory->GetBuffer();
        m_strDccFilename = img.m_strDccFilename;
    }

    //////////////////////////////////////////////////////////////////////////
    void Clear()
    {
        Fill(0);
    }

    //////////////////////////////////////////////////////////////////////////
    void Fill(unsigned char c)
    {
        if (IsValid())
        {
            memset(GetData(), c, GetSize());
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void GetSubImage(int x1, int y1, int width, int height, TImage<T>& img) const
    {
        img.Allocate(width, height);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                img.ValueAt(x, y) = ValueAtSafe(x1 + x, y1 + y);
            }
        }
    }
    void SetSubImage(int x1, int y1, const TImage<T>& subImage, float heightOffset, float fClamp)
    {
        int width = subImage.GetWidth();
        int height = subImage.GetHeight();
        FitSubRect(x1, y1, width, height);

        if (width <= 0 || height <= 0)
        {
            return;
        }

        if (fClamp < 0.0f)
        {
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    ValueAt(x1 + x, y1 + y) = subImage.ValueAt(x, y) + heightOffset;
                }
            }
        }
        else
        {
            T TClamp = fClamp;
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    ValueAt(x1 + x, y1 + y) = clamp_tpl(f32(subImage.ValueAt(x, y) + heightOffset), 0.0f, f32(TClamp));
                }
            }
        }
    }

    void SetSubImage(int x1, int y1, const TImage<T>& subImage)
    {
        int width = subImage.GetWidth();
        int height = subImage.GetHeight();
        FitSubRect(x1, y1, width, height);
        if (width <= 0 || height <= 0)
        {
            return;
        }

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                ValueAt(x1 + x, y1 + y) = subImage.ValueAt(x, y);
            }
        }
    }

    void FitSubRect(int& x1, int& y1, int& width, int& height)
    {
        if (x1 < 0)
        {
            width = width + x1;
            x1 = 0;
        }
        if (y1 < 0)
        {
            height = height + y1;
            y1 = 0;
        }
        if (x1 + width > m_width)
        {
            width = m_width - x1;
        }
        if (y1 + height > m_height)
        {
            height = m_height - y1;
        }
    }

    //! Compress image to memory block.
    void Compress(CMemoryBlock& mem) const
    {
        assert(IsValid());
        m_memory->Compress(mem);
    }

    //! Uncompress image from memory block.
    bool Uncompress(const CMemoryBlock& mem)
    {
        assert(IsValid());
        // New memory block.
        _smart_ptr<CMemoryBlock> temp = new CMemoryBlock();
        mem.Uncompress(*temp);
        bool bValid = (GetSize() == m_memory->GetSize())
            || ((GetSize() + m_width * sizeof(T)) == m_memory->GetSize());
        if (bValid)
        {
            m_memory = temp;
            m_data = (T*)m_memory->GetBuffer();
        }
        return bValid;
        //assert( GetSize() == m_memory.GetSize() );
    }

    void SetHasAlphaChannel(bool bHasAlphaChannel) { m_bHasAlphaChannel = bHasAlphaChannel; }
    void SetIsLimitedHDR(bool bIsLimitedHDR) { m_bIsLimitedHDR = bIsLimitedHDR; }
    void SetIsCubemap(bool bIsCubemap) { m_bIsCubemap = bIsCubemap; }
    void SetNumberOfMipMaps(unsigned int nNumberOfMips) { m_nNumberOfMipmaps = nNumberOfMips; }

    void Serialize(CXmlArchive& ar);

    void SetFormatDescription(const QString& str) { m_formatDescription = str; };
    const QString& GetFormatDescription() const { return m_formatDescription; };

    void SetFormat(ETEX_Format format) { m_format = format; }
    ETEX_Format GetFormat() const { return m_format; }

    void SetSRGB(bool bEnable) { m_bIsSRGB = bEnable; }
    bool GetSRGB() const { return m_bIsSRGB; }

    void SetDccFilename(const QString& str) { m_strDccFilename = str; };
    const QString& GetDccFilename() const { return m_strDccFilename; }

    // RotateOrt() - orthonormal image rotation
    void RotateOrt(const TImage<T>& img, ImageRotationDegrees degrees)
    {
        if (!img.IsValid())
        {
            return;
        }

        int width;
        int height;

        if (degrees == ImageRotationDegrees::Rotate90 || degrees == ImageRotationDegrees::Rotate270)
        {
            width = img.GetHeight();
            height = img.GetWidth();
        }
        else
        {
            width = img.GetWidth();
            height = img.GetHeight();
        }

        if (m_width != width || m_height != height)
        {
            Allocate(width, height);
        }

        for (int y = 0; y < m_height; y++)
        {
            for (int x = 0; x < m_width; x++)
            {
                if (degrees == ImageRotationDegrees::Rotate90)
                {
                    ValueAt(x, y) = img.ValueAt(m_height - y - 1, x);
                }
                else if (degrees == ImageRotationDegrees::Rotate180)
                {
                    ValueAt(x, y) = img.ValueAt(m_width - x - 1, m_height - y - 1);
                }
                else if (degrees == ImageRotationDegrees::Rotate270)
                {
                    ValueAt(x, y) = img.ValueAt(y, m_width - x - 1);
                }
                else
                {
                    ValueAt(x, y) = img.ValueAt(x, y);
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void ScaleToFit(const TImage<T>& img)
    {
        uint32 x, y, u, v;
        T* destRow, *dest, *src, *sourceRow;

        if (!img.IsValid())
        {
            return;
        }

        const uint32 srcW = img.GetWidth();
        const uint32 srcH = img.GetHeight();

        const uint32 trgW = GetWidth();
        const uint32 trgH = GetHeight();

        const uint32 xratio = trgW > 0 ? (srcW << 16) / trgW : 1;
        const uint32 yratio = trgH > 0 ? (srcH << 16) / trgH : 1;

        src = img.GetData();
        destRow = GetData();

        v = 0;
        for (y = 0; y < trgH; y++)
        {
            u = 0;
            sourceRow = src + (v >> 16) * srcW;
            dest = destRow;
            for (x = 0; x < trgW; x++)
            {
                *dest++ = sourceRow[u >> 16];
                u += xratio;
            }
            v += yratio;
            destRow += trgW;
        }
    }


private:
    // Restrict use of copy constructor.
    TImage(const TImage<T>& img);
    TImage<T>& operator=(const TImage<T>& img);


    //! Memory holding image data.
    _smart_ptr<CMemoryBlock> m_memory;

    T*  m_data;
    int m_width;
    int m_height;
    bool m_bHasAlphaChannel;
    bool m_bIsLimitedHDR;
    bool m_bIsCubemap;
    bool m_bIsSRGB;
    unsigned int m_nNumberOfMipmaps;
    QString m_formatDescription;
    QString m_strDccFilename;
    ETEX_Format m_format;
};

template <class T>
void    TImage<T>::Serialize(CXmlArchive& ar)
{
    if (ar.bLoading)
    {
        // Loading
        ar.root->getAttr("ImageWidth", m_width);
        ar.root->getAttr("ImageHeight", m_height);
        ar.root->getAttr("Mipmaps", m_nNumberOfMipmaps);
        ar.root->getAttr("IsCubemap", m_bIsCubemap);
        bool bIsSRGB;
        if (ar.root->getAttr("IsSRGB", bIsSRGB))
        {
            m_bIsSRGB = bIsSRGB;
        }
        ar.root->getAttr("dccFilename", m_strDccFilename);
        int format;
        if (ar.root->getAttr("format", format))
        {
            m_format = (ETEX_Format)format;
        }
        else
        {
            m_format = eTF_Unknown;
        }
        Allocate(m_width, m_height);
        void* pData = 0;
        int nDataSize = 0;
        bool bHaveBlock = ar.pNamedData->GetDataBlock(ar.root->getTag(), pData, nDataSize);
        if (bHaveBlock && nDataSize == GetSize())
        {
            m_data = (T*)pData;
        }
    }
    else
    {
        // Saving.
        ar.root->setAttr("ImageWidth", m_width);
        ar.root->setAttr("ImageHeight", m_height);
        ar.root->setAttr("Mipmaps", m_nNumberOfMipmaps);
        ar.root->setAttr("IsCubemap", m_bIsCubemap);
        ar.root->setAttr("IsSRGB", m_bIsSRGB);
        ar.root->setAttr("format", (int)m_format);
        ar.root->setAttr("dccFilename", m_strDccFilename);

        ar.pNamedData->AddDataBlock(ar.root->getTag(), (void*)m_data, GetSize());
    }
};

//////////////////////////////////////////////////////////////////////////
// Define types of most commonly used images.
//////////////////////////////////////////////////////////////////////////
typedef TImage<float> CFloatImage;
typedef TImage<unsigned char> CByteImage;
typedef TImage<unsigned short> CWordImage;

//////////////////////////////////////////////////////////////////////////
class CImageEx
    : public TImage < unsigned int >
{
public:
    CImageEx()
        : TImage() { m_bGetHistogramEqualization = false; };

    EDITOR_CORE_API bool ConvertToFloatImage(CFloatImage& dstImage);

    EDITOR_CORE_API void SwapRedAndBlue();
    EDITOR_CORE_API void ReverseUpDown();
    EDITOR_CORE_API void FillAlpha(unsigned char value = 0xff);

    // request histogram equalization for HDRs
    void SetHistogramEqualization(bool bHistogramEqualization) { m_bGetHistogramEqualization = bHistogramEqualization; }
    bool GetHistogramEqualization() { return m_bGetHistogramEqualization; }

private:
    bool m_bGetHistogramEqualization;
};

#endif // CRYINCLUDE_EDITOR_UTIL_IMAGE_H
