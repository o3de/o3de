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
