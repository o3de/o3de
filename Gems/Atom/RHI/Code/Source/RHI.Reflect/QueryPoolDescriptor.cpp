/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void QueryPoolDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<QueryPoolDescriptor, ResourcePoolDescriptor>()
                ->Version(1)
                ->Field("m_count", &QueryPoolDescriptor::m_queriesCount)
                ->Field("m_type", &QueryPoolDescriptor::m_type)
                ->Field("m_pipelineStatisticsMask", &QueryPoolDescriptor::m_pipelineStatisticsMask)
                ;
        }
    }
}
