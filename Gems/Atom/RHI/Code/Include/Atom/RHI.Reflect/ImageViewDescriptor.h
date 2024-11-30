/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageEnums.h>
#include <Atom/RHI.Reflect/Format.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! Image views map to a range of mips / array slices in an image.
    struct ImageViewDescriptor
    {
        AZ_TYPE_INFO(ImageViewDescriptor, "{7dc08a6e-5a1d-4730-b1fa-3a6e11bb7178}");
        AZ_CLASS_ALLOCATOR(ImageViewDescriptor, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        //! Creates a view with a custom format and mip chain range.
        static ImageViewDescriptor Create(
            Format overrideFormat,
            uint16_t mipSliceMin,
            uint16_t mipSliceMax);

        //! Creates a view with a custom format, mip slice range, and array slice range.
        static ImageViewDescriptor Create(
            Format overrideFormat,
            uint16_t mipSliceMin,
            uint16_t mipSliceMax,
            uint16_t arraySliceMin,
            uint16_t arraySliceMax);

        //! Creates a default view that maps to the full subresource range, but is
        //! set to interpret the array slices as cubemap faces.
        static ImageViewDescriptor CreateCubemap();

        //! Creates a cubemap view with a specific format and mip slice range.
        static ImageViewDescriptor CreateCubemap(
            Format overrideFormat,
            uint16_t mipSliceMin,
            uint16_t mipSliceMax);

        //! Creates a cubemap view with a specific format, mip slice range, and array slice range.
        static ImageViewDescriptor CreateCubemap(
            Format overrideFormat,
            uint16_t mipSliceMin,
            uint16_t mipSliceMax,
            uint16_t cubeSliceMin,
            uint16_t cubeSliceMax);

        //! Creates a cubemap face view with a specific format and mip slice range.
        static ImageViewDescriptor CreateCubemapFace(
            Format overrideFormat,
            uint16_t mipSliceMin,
            uint16_t mipSliceMax,
            uint16_t faceSlice);

        //! Create a view for 3d texture
        static ImageViewDescriptor Create3D(
            Format overrideFormat,
            uint16_t mipSliceMin,
            uint16_t mipSliceMax,
            uint16_t depthSliceMin,
            uint16_t depthSliceMax);

        static const uint16_t HighestSliceIndex = static_cast<uint16_t>(-1);

        ImageViewDescriptor() = default;
        explicit ImageViewDescriptor(Format overrideFormat);
        bool operator==(const ImageViewDescriptor& other) const;
        bool operator!=(const ImageViewDescriptor& other) const;
            
        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        // Returns true if other is the same sub resource
        bool IsSameSubResource(const ImageViewDescriptor& other) const;

        //! Return true if any subresource overlaps with another ImageViewDescriptor
        bool OverlapsSubResource(const ImageViewDescriptor& other) const;
            
        /// Minimum mip slice offset.
        uint16_t m_mipSliceMin = 0;

        /// Maximum mip slice offset. Must be greater than or equal to the min mip slice offset.
        uint16_t m_mipSliceMax = HighestSliceIndex;

        /// Minimum array slice offset.
        uint16_t m_arraySliceMin = 0;

        /// Maximum array slice offset. Must be greater or equal to the min array slice offset.
        uint16_t m_arraySliceMax = HighestSliceIndex;

        /// Minimum depth slice offset.
        uint16_t m_depthSliceMin = 0;

        /// Maximum depth slice offset. Must be greater or equal to the min depth slice offset.
        uint16_t m_depthSliceMax = HighestSliceIndex;

        /// Typed format of the view, which may override a format in
        /// the image. A value of Unknown will default to the image format.
        Format m_overrideFormat = Format::Unknown;

        /// The bind flags used by this view. Should be compatible with the bind flags of the underlying image.
        ImageBindFlags m_overrideBindFlags = ImageBindFlags::None;

        /// Whether to treat this image as a cubemap in the shader
        uint32_t m_isCubemap = 0;

        /// Aspects of the image accessed by the view.
        ImageAspectFlags m_aspectFlags = ImageAspectFlags::All;

        /// Whether to treat this image as texture array.
        /// This is needed because a texture array can have 1 layer only.
        uint32_t m_isArray = 0;            
    };
}
