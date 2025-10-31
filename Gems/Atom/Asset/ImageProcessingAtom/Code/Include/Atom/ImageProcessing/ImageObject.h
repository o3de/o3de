/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/PixelFormats.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Color.h>

namespace ImageProcessingAtom
{
    class IImageObject;
    class TextureSettings;
    typedef AZStd::shared_ptr<IImageObject> IImageObjectPtr;

    //cubemap layouts
    enum CubemapLayoutType
    {
        CubemapLayoutHorizontal = 0, //6x1 strip. with rotations.
        CubemapLayoutHorizontalCross,   //4x3.
        CubemapLayoutVerticalCross,     //3x4
        CubemapLayoutVertical,     //1x6 strip. new output format. it's better because the memory is continuous for each face
        CubemapLayoutTypeCount,
        CubemapLayoutNone = CubemapLayoutTypeCount
    };

    enum class EAlphaContent
    {
        eAlphaContent_Indeterminate,      // the format may have alpha, but can't be calculated
        eAlphaContent_Absent,             // the format has no alpha
        eAlphaContent_OnlyWhite,          // alpha contains just white
        eAlphaContent_OnlyBlack,          // alpha contains just black
        eAlphaContent_OnlyBlackAndWhite,  // alpha contains just black and white
        eAlphaContent_Greyscale           // alpha contains grey tones
    };

    // Interface for image object. The image object may have mipmaps.
    // The image may be a Volume Texture (3D Image), the 3rd dimension is named Depth.
    // For 2D Images, Depth == 1.
    class IImageObject
    {
    public:
        //static functions
        static IImageObject* CreateImage(AZ::u32 width, AZ::u32 height, AZ::u32 maxMipCount, EPixelFormat pixelFormat);
        static IImageObject* CreateImage(AZ::u32 width, AZ::u32 height, AZ::u32 depth, AZ::u32 maxMipCount, EPixelFormat pixelFormat);

        virtual ~IImageObject() {};
    public:
        //creating new image object outof this image object
        virtual IImageObject* Clone(uint32_t maxMipCount = std::numeric_limits<uint32_t>::max()) const = 0;
        // allocate an empty image object with requested format and same properties with current image
        virtual IImageObject* AllocateImage(EPixelFormat pixelFormat, uint32_t maxMipCount = std::numeric_limits<uint32_t>::max()) const = 0;
        virtual IImageObject* AllocateImage(uint32_t maxMipCount = std::numeric_limits<uint32_t>::max()) const = 0;

        //get pixel format
        virtual EPixelFormat GetPixelFormat() const = 0;

        virtual AZ::u32 GetPixelCount(AZ::u32 mip) const = 0;
        virtual AZ::u32 GetWidth(AZ::u32 mip) const = 0;
        virtual AZ::u32 GetHeight(AZ::u32 mip) const = 0;
        virtual AZ::u32 GetDepth(AZ::u32 /*mip*/) const { return 1; }
        virtual AZ::u32 GetMipCount() const = 0;

        //get pixel data buffer
        virtual void GetImagePointer(AZ::u32 mip, AZ::u8*& pMem, AZ::u32& pitch) const = 0;
        virtual AZ::u32 GetMipBufSize(AZ::u32 mip) const = 0;
        virtual void SetMipData(AZ::u32 mip, AZ::u8* mipBuf, AZ::u32 bufSize, AZ::u32 pitch) = 0;

        //get/set image flags
        virtual AZ::u32 GetImageFlags() const = 0;
        virtual void SetImageFlags(AZ::u32 imageFlags) = 0;
        virtual void AddImageFlags(AZ::u32 imageFlags) = 0;
        virtual void RemoveImageFlags(AZ::u32 imageFlags) = 0;
        virtual bool HasImageFlags(AZ::u32 imageFlags) const = 0;

        // image data operations and calculation
        // Calculates "(pixel.rgba * scale) + bias"
        virtual void ScaleAndBiasChannels(AZ::u32 firstMip, AZ::u32 maxMipCount, const AZ::Vector4& scale, const AZ::Vector4& bias) = 0;
        // Calculates "clamp(pixel.rgba, min, max)"
        virtual void ClampChannels(AZ::u32 firstMip, AZ::u32 maxMipCount, const AZ::Vector4& min, const AZ::Vector4& max) = 0;

        //transfer alpha coverage from source image
        virtual void TransferAlphaCoverage(const TextureSettings* textureSetting, const IImageObjectPtr srcImg) = 0;
        // Routines to measure and manipulate alpha coverage
        virtual float ComputeAlphaCoverageScaleFactor(AZ::u32 mip, float fDesiredCoverage, float fAlphaRef) const = 0;
        virtual float ComputeAlphaCoverage(AZ::u32 mip, float fAlphaRef) const = 0;

        //helper functions
        //compare whether two images are same. return true if they are same.
        virtual bool CompareImage(const IImageObjectPtr otherImage) const = 0;

        //get total image data size in memory of all mipmaps. Not includs header and flags.
        virtual AZ::u32 GetTextureMemory() const = 0;

        //identify content of the alpha channel
        virtual EAlphaContent GetAlphaContent() const = 0;

        //normalize rgb channel for specified mips
        virtual void NormalizeVectors(AZ::u32 firstMip, AZ::u32 maxMipCount) = 0;

        // use when you convert an image to another one
        virtual void CopyPropertiesFrom(const IImageObjectPtr src) = 0;

        //swizzle data for source channels to dest channels
        virtual void Swizzle(const char channels[4]) = 0;

        //get/set properties of the image object
        virtual void GetColorRange(AZ::Color& minColor, AZ::Color& maxColor) const = 0;
        virtual void SetColorRange(const AZ::Color& minColor, const AZ::Color& maxColor) = 0;
        virtual AZ::u32 GetNumPersistentMips() const = 0;
        virtual void SetNumPersistentMips(AZ::u32 nMips) = 0;
        virtual float GetAverageBrightness() const = 0;
        virtual void SetAverageBrightness(float avgBrightness) = 0;
        virtual AZ::Color GetAverageColor() const = 0;
        virtual void SetAverageColor(const AZ::Color& averageColor) = 0;

        // Derive new roughness from normal variance to preserve the bumpiness of normal map mips and to reduce specular aliasing.
        // The derived roughness is combined with the artist authored roughness stored in the alpha channel of the normal map.
        // The algorithm is based on the Frequency Domain Normal Mapping implementation presented by Neubelt and Pettineo at Siggraph 2013.
        virtual void GlossFromNormals(bool hasAuthoredGloss) = 0;

        //clear image with color
        virtual void ClearColor(float r, float g, float b, float a) = 0;
    };
} // namespace ImageProcessingAtom
