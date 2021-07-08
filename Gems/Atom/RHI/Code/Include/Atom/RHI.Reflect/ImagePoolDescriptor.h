/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class ImagePoolDescriptor
            : public ResourcePoolDescriptor
        {
        public:
            virtual ~ImagePoolDescriptor() = default;
            AZ_RTTI(ImagePoolDescriptor, "{9828B912-7D7D-4443-8794-E10E0EF34269}", ResourcePoolDescriptor);
            static void Reflect(AZ::ReflectContext* context);

            ImagePoolDescriptor() = default;
            ImagePoolDescriptor(ImageBindFlags bindFlags) : m_bindFlags(bindFlags) {};

            /// The set of image bind flags supported by this pool.
            ImageBindFlags m_bindFlags = ImageBindFlags::Color;
        };
    }
}
