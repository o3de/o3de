/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void BufferPoolDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<BufferPoolDescriptor, ResourcePoolDescriptor>()
                ->Version(2)
                ->Field("m_heapMemoryLevel", &BufferPoolDescriptor::m_heapMemoryLevel)
                ->Field("m_hostMemoryAccess", &BufferPoolDescriptor::m_hostMemoryAccess)
                ->Field("m_bindFlags", &BufferPoolDescriptor::m_bindFlags)
                ->Field("m_largestPooledAllocationSizeInBytes", &BufferPoolDescriptor::m_largestPooledAllocationSizeInBytes)
                ;
        }
    }
}
