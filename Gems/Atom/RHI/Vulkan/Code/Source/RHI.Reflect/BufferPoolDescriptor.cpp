/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/BufferPoolDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Vulkan
    {
        void BufferPoolDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<BufferPoolDescriptor, RHI::BufferPoolDescriptor>()
                    ->Version(2)
                    ->Field("m_bufferPoolPageSizeInBytes", &BufferPoolDescriptor::m_bufferPoolPageSizeInBytes)
                    ->Field("m_additionalMemoryPropertyFlags", &BufferPoolDescriptor::m_additionalMemoryPropertyFlags);
            }
        }

        BufferPoolDescriptor::BufferPoolDescriptor()
        {
            m_bufferPoolPageSizeInBytes = RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_bufferPoolPageSizeInBytes;
        }
    }
}
