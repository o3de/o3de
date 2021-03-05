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

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
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
}
