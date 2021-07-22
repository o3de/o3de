/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Vulkan
    {
        void ImagePoolDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImagePoolDescriptor, RHI::ImagePoolDescriptor>()
                    ->Version(1)
                    ->Field("m_imagePageSizeInBytes", &ImagePoolDescriptor::m_imagePageSizeInBytes);
            }
        }

        ImagePoolDescriptor::ImagePoolDescriptor()
        {
            m_imagePageSizeInBytes = RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_imagePoolPageSizeInBytes;
        }
    }
}
