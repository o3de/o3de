/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>

DECLARE_EBUS_INSTANTIATION(AzFramework::VisibleGeometryRequests);

namespace AzFramework
{
    void VisibleGeometry::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VisibleGeometry>()
                ->Version(0)
                ->Field("vertices", &VisibleGeometry::m_vertices)
                ->Field("indices", &VisibleGeometry::m_indices)
                ;

            serializeContext->Class<VisibleGeometryContainer>()
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<VisibleGeometry>("VisibleGeometry")
                ;
        }
    }

    void VisibleGeometryRequests::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<VisibleGeometryRequestBus>("VisibleGeometryRequestBus")
                ->Event("GetVisibleGeometry", &VisibleGeometryRequestBus::Events::GetVisibleGeometry)
                ;
        }
    }
} // namespace AzFramework
