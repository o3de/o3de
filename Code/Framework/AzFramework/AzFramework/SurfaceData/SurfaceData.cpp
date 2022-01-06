/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/SurfaceData/SurfaceData.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework::SurfaceData
{
    void SurfaceTagWeight::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SurfaceTagWeight>()
                ->Field("m_surfaceType", &SurfaceTagWeight::m_surfaceType)
                ->Field("m_weight", &SurfaceTagWeight::m_weight)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceTagWeight>()
                ->Attribute(AZ::Script::Attributes::Category, "SurfaceData")
                ->Constructor()
                ->Property("surfaceType", BehaviorValueProperty(&SurfaceTagWeight::m_surfaceType))
                ->Property("weight", BehaviorValueProperty(&SurfaceTagWeight::m_weight))
                ;
        }
    }

    void SurfacePoint::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SurfacePoint>()
                ->Field("m_position", &SurfacePoint::m_position)
                ->Field("m_normal", &SurfacePoint::m_normal)
                ->Field("m_surfaceTags", &SurfacePoint::m_surfaceTags)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfacePoint>("AzFramework::SurfaceData::SurfacePoint")
                ->Attribute(AZ::Script::Attributes::Category, "SurfaceData")
                ->Constructor()
                ->Property("position", BehaviorValueProperty(&SurfacePoint::m_position))
                ->Property("normal", BehaviorValueProperty(&SurfacePoint::m_normal))
                ->Property("surfaceTags", BehaviorValueProperty(&SurfacePoint::m_surfaceTags))
                ;
        }
    }

} // namespace AzFramework::SurfaceData
