/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Origin.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/MultisampleState.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/Utils/TypeHash.h>
#include <Atom/RHI.Reflect/ImageEnums.h>

namespace AZ::RHI
{    
    ImageBindFlags GetImageBindFlags(ScopeAttachmentUsage usage, ScopeAttachmentAccess access);        

    //! Images are comprised of sub-resources corresponding to the number of mip-mip levels
    //! and array slices. Image data is stored as pixels in opaque swizzled formats. Images
    //! represent texture data to the shader.
    struct ImageDescriptor
    {
        AZ_TYPE_INFO(ImageDescriptor, "{D1FDAC7B-E9CF-4B2D-B1FB-646D3EE3159C}");
        static void Reflect(AZ::ReflectContext* context);

        static const uint16_t NumCubeMapSlices = 6;
            
        static ImageDescriptor Create1D(
            ImageBindFlags bindFlags,
            uint32_t width,
            Format format);

        static ImageDescriptor Create1DArray(
            ImageBindFlags bindFlags,
            uint32_t width,
            uint16_t arraySize,
            Format format);

        static ImageDescriptor Create2D(
            ImageBindFlags bindFlags,
            uint32_t width,
            uint32_t height,
            Format format);

        static ImageDescriptor Create2DArray(
            ImageBindFlags bindFlags,
            uint32_t width,
            uint32_t height,
            uint16_t arraySize,
            Format format);

        static ImageDescriptor CreateCubemap(
            ImageBindFlags bindFlags,
            uint32_t width,
            Format format);

        static ImageDescriptor CreateCubemapArray(
            ImageBindFlags bindFlags,
            uint32_t width,
            uint16_t arraySize,
            Format format);

        static ImageDescriptor Create3D(
            ImageBindFlags bindFlags,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            Format format);

        ImageDescriptor() = default;

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        /// union of all bind points for this image
        ImageBindFlags m_bindFlags = ImageBindFlags::ShaderRead;

        /// Number of dimensions.
        ImageDimension m_dimension = ImageDimension::Image2D;

        /// Size of the image in pixels.
        Size m_size;

        /// Number of array elements (must be 1 for 3D images).
        uint16_t m_arraySize = 1;

        /// Number of mip levels.
        uint16_t m_mipLevels = 1;

        /// Pixel format.
        Format m_format = Format::Unknown;

        /// The mask of queue classes supporting shared access of this resource.
        HardwareQueueClassMask m_sharedQueueMask = HardwareQueueClassMask::All;

        /// Multisample information for this image.
        MultisampleState m_multisampleState;

        /// Whether to treat this image as a cubemap.
        // The property is required by Provo RHI.  
        // [GFX TODO][ATOM-1518] Care matching with ImageViewDescriptor::m_isCubemap.
        uint32_t m_isCubemap = 0;
    };

    //! Returns whether mip 'A' is more detailed than mip 'B'.
    inline bool IsMipMoreDetailedThan(uint32_t mipA, uint32_t mipB)
    {
        return mipA < mipB;
    }

    //! Returns whether mip 'A' is less detailed than mip 'B'.
    inline bool IsMipLessDetailedThan(uint32_t mipA, uint32_t mipB)
    {
        return mipA > mipB;
    }

    //! Increases the mip detail by increaseBy levels.
    inline uint32_t IncreaseMipDetailBy(uint32_t mipLevel, uint32_t increaseBy)
    {
        AZ_Assert(mipLevel >= increaseBy, "Exceeded mip detail.");
        return mipLevel - increaseBy;
    }

    //! Decreases the mip detail by decreaseBy levels.
    inline uint32_t DecreaseMipDetailBy(uint32_t mipLevel, uint32_t decreaseBy)
    {
        return mipLevel + decreaseBy;
    }
}
