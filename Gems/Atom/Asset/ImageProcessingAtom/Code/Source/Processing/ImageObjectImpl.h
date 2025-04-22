/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/ImageObject.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Processing/DDSHeader.h>

namespace ImageProcessingAtom
{
    // ImageObject allows the abstraction of different kinds of
    // images generated during conversion. Supports 3D Image.
    class CImageObject
        : public IImageObject
    {
    public:
        AZ_CLASS_ALLOCATOR(CImageObject, AZ::SystemAllocator);

    public:
        // Constructors
        CImageObject(AZ::u32 width, AZ::u32 height, AZ::u32 maxMipCount, EPixelFormat pixelFormat);
        CImageObject(AZ::u32 width, AZ::u32 height, AZ::u32 depth, AZ::u32 maxMipCount, EPixelFormat pixelFormat);

        ~CImageObject();

        //virtual functions from IImageObject
        IImageObject* AllocateImage(EPixelFormat pixelFormat, uint32_t maxMipCount = (std::numeric_limits<uint32_t>::max)()) const override;
        IImageObject* AllocateImage(uint32_t maxMipCount = (std::numeric_limits<uint32_t>::max)()) const override;
        IImageObject* Clone(uint32_t maxMipCount = (std::numeric_limits<uint32_t>::max)()) const override;
        EPixelFormat GetPixelFormat() const override;
        AZ::u32 GetPixelCount(AZ::u32 mip) const override;
        AZ::u32 GetWidth(AZ::u32 mip) const override;
        AZ::u32 GetHeight(AZ::u32 mip) const override;
        AZ::u32 GetDepth(AZ::u32 mip) const override;
        AZ::u32 GetMipCount() const override;

        void GetImagePointer(AZ::u32 mip, AZ::u8*& pMem, AZ::u32& pitch) const override;
        AZ::u32 GetMipBufSize(AZ::u32 mip) const override;
        void SetMipData(AZ::u32 mip, AZ::u8* mipBuf, AZ::u32 bufSize, AZ::u32 pitch) override;

        AZ::u32 GetImageFlags() const override;
        void SetImageFlags(AZ::u32 imageFlags) override;
        void AddImageFlags(AZ::u32 imageFlags) override;
        void RemoveImageFlags(AZ::u32 imageFlags) override;
        bool HasImageFlags(AZ::u32 imageFlags) const override;

        //image data operations and calculations
        void ScaleAndBiasChannels(AZ::u32 firstMip, AZ::u32 maxMipCount, const AZ::Vector4& scale, const AZ::Vector4& bias) override;
        void ClampChannels(AZ::u32 firstMip, AZ::u32 maxMipCount, const AZ::Vector4& min, const AZ::Vector4& max) override;

        void TransferAlphaCoverage(const TextureSettings* textureSetting, const IImageObjectPtr srcImg) override;
        float ComputeAlphaCoverageScaleFactor(AZ::u32 mip, float fDesiredCoverage, float fAlphaRef) const override;
        float ComputeAlphaCoverage(AZ::u32 firstMip, float fAlphaRef) const override;

        bool CompareImage(const IImageObjectPtr otherImage) const override;

        uint32_t GetTextureMemory() const override;

        EAlphaContent GetAlphaContent() const override;

        void NormalizeVectors(AZ::u32 firstMip, AZ::u32 maxMipCount) override;

        void CopyPropertiesFrom(const IImageObjectPtr src) override;

        void Swizzle(const char channels[4]) override;

        void GetColorRange(AZ::Color& minColor, AZ::Color& maxColor) const override;
        void SetColorRange(const AZ::Color& minColor, const AZ::Color& maxColor) override;
        float GetAverageBrightness() const override;
        void SetAverageBrightness(float avgBrightness) override;
        AZ::Color GetAverageColor() const override;
        void SetAverageColor(const AZ::Color& averageColor) override;
        AZ::u32 GetNumPersistentMips() const override;
        void SetNumPersistentMips(AZ::u32 nMips) override;

        void GlossFromNormals(bool hasAuthoredGloss) override;
        void ClearColor(float r, float g, float b, float a) override;
        //end virtual functions from IImageObject

    private:

        enum EColorNormalization
        {
            eColorNormalization_Normalize,
            eColorNormalization_PassThrough,
        };

        enum EAlphaNormalization
        {
            eAlphaNormalization_SetToZero,
            eAlphaNormalization_Normalize,
            eAlphaNormalization_PassThrough,
        };

    private:
        class MipLevel
        {
        public:
            AZ_CLASS_ALLOCATOR(MipLevel, AZ::SystemAllocator);

            AZ::u32 m_width;
            AZ::u32 m_height;
            AZ::u32 m_depth;
            // m_rowCount is the number of rows for each depth slice.
            AZ::u32 m_rowCount;  // for compressed textures m_rowCount is usually less than m_height
            AZ::u32 m_pitch;     // row size in bytes
            AZ::u8* m_pData;

        public:
            MipLevel()
                : m_width(0)
                , m_height(0)
                , m_depth(1)
                , m_rowCount(0)
                , m_pitch(0)
                , m_pData(0)
            {
            }

            ~MipLevel()
            {
                delete[] m_pData;
                m_pData = 0;
            }

            void Alloc()
            {
                AZ_Assert(m_pData == 0, "Mip data must be empty before Allocation!");
                m_pData = new AZ::u8[GetSize()];
            }

            AZ::u32 GetSize() const
            {
                AZ_Assert(m_pitch, "Pitch must be greater than zero!");
                return m_pitch * m_rowCount * m_depth;
            }

            bool operator==(const MipLevel& other)
            {
                if (m_width == other.m_width && m_height == other.m_height && m_depth == other.m_depth
                    && m_rowCount == other.m_rowCount && m_pitch == other.m_pitch)
                {
                    return (memcmp(m_pData, other.m_pData, GetSize()) == 0);
                }
                return false;
            }
        };

    private:
        EPixelFormat           m_pixelFormat;
        AZStd::vector<MipLevel*> m_mips;               // stores *pointers* to avoid reallocations when elements are erase()'d

        AZ::Color         m_colMinARGB;             // ARGB will be added the properties of the DDS file
        AZ::Color         m_colMaxARGB;             // ARGB will be added the properties of the DDS file
        AZ::Color         m_averageColor;
        float        m_averageBrightness;           // will be added to the properties of the DDS file
        AZ::u32       m_imageFlags;                  //
        AZ::u32       m_numPersistentMips;           // number of mipmaps won't be splitted

    public:
        // Reset this image object to specified format and size. Calling this function on a pre-existing
        // 3D Image, will result in a new 2D image.
        void ResetImage(AZ::u32 width, AZ::u32 height, AZ::u32 maxMipCount, EPixelFormat pixelFormat);
        void ResetImage(AZ::u32 width, AZ::u32 height, AZ::u32 depth, AZ::u32 maxMipCount, EPixelFormat pixelFormat);

        //get mip count and the origin (top mip) size
        void GetExtent(AZ::u32& width, AZ::u32& height, AZ::u32& mipCount) const;
        void GetExtent(AZ::u32& width, AZ::u32& height, AZ::u32& depth, AZ::u32& mipCount) const;

        AZ::u32 GetMipDataSize(AZ::u32 mip) const;

        //! calculates the average brightness for a texture
        float CalculateAverageBrightness() const;

        bool HasPowerOfTwoSizes() const;

        void CopyPropertiesFrom(const CImageObject* src);

        // Computes the dynamically used range for the texture and expands it to use the
        // full range [0,2^(2^ExponentBits-1)] for better quality.
        void NormalizeImageRange(EColorNormalization eColorNorm, EAlphaNormalization eAlphaNorm, bool bMaintainBlack = false, int nExponentBits = 0);
        // Brings normalized ranges back to it's original range.
        void ExpandImageRange(EColorNormalization eColorNorm, EAlphaNormalization eAlphaNorm, int nExponentBits = 0);

    private:
        //build image file header from this image object
        bool BuildSurfaceHeader(DDS_HEADER_LEGACY& header) const;
        bool BuildSurfaceExtendedHeader(DDS_HEADER_DXT10& exthead) const;
    };
} // namespace ImageProcessingAtom


