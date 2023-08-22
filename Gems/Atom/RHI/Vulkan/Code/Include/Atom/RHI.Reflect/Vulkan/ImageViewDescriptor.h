/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Defines the mapping of the RGBA channels of an image view.
        //! Each channel can be map to their identity value (e.g red channel maps to the red channel),
        //! to another channel (e.g red channel maps to blue channel) or to a
        //! constant value (e.g red channel maps to zero).
        struct ImageComponentMapping
        {
            //! Mapping for a specific channel
            enum class Swizzle : uint32_t
            {
                Identity,   // No swizzling
                Zero,       // Constant value of zero
                One,        // Constant value of one
                R,          // Red channel swizzling
                G,          // Green channel swizzling
                B,          // Blue channel swizzling
                A           // Alpha channel swizzling
            };

            ImageComponentMapping() = default;
            ImageComponentMapping(Swizzle r, Swizzle g, Swizzle b, Swizzle a)
                : m_red(r)
                , m_green(g)
                , m_blue(b)
                , m_alpha(a)
            {}

            //! Red channel mapping
            Swizzle m_red = Swizzle::Identity;
            //! green channel mapping
            Swizzle m_green = Swizzle::Identity;
            //! Blue channel mapping
            Swizzle m_blue = Swizzle::Identity;
            //! Alpha channel mapping
            Swizzle m_alpha = Swizzle::Identity;
        };

        //! Vulkan descriptor for Image Views with specific Vulkan properties.
        class ImageViewDescriptor
            : public RHI::ImageViewDescriptor
        {
        public:
            AZ_CLASS_ALLOCATOR(ImageViewDescriptor, SystemAllocator)
            AZ_RTTI(ImageViewDescriptor, "{1D710152-2306-4F06-BB1D-93F5371EE1C8}", RHI::ImageViewDescriptor);

            ImageViewDescriptor() = default;

            //! Mapping of the image channels (swizzling).
            ImageComponentMapping m_componentMapping;
        };
    }
}
