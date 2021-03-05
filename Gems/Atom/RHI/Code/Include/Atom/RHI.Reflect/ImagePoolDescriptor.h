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

            /// The set of image bind flags supported by this pool.
            ImageBindFlags m_bindFlags = ImageBindFlags::Color;
        };
    }
}