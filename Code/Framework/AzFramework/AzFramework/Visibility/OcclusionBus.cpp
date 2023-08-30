/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Visibility/OcclusionBus.h>

DECLARE_EBUS_INSTANTIATION(AzFramework::OcclusionRequests);

namespace AzFramework
{
    void OcclusionRequests::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<OcclusionRequestBus>("OcclusionRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Visibility")
                ->Attribute(AZ::Script::Attributes::Module, "visibility")
                ->Event("ClearOcclusionViewDebugInfo", &OcclusionRequestBus::Events::ClearOcclusionViewDebugInfo)
                ->Event("CreateOcclusionView", &OcclusionRequestBus::Events::CreateOcclusionView)
                ->Event("DestroyOcclusionView", &OcclusionRequestBus::Events::DestroyOcclusionView)
                ->Event("UpdateOcclusionView", &OcclusionRequestBus::Events::UpdateOcclusionView)
                ->Event("IsEntityVisibleInOcclusionView", &OcclusionRequestBus::Events::IsEntityVisibleInOcclusionView)
                ->Event("IsAabbVisibleInOcclusionView", &OcclusionRequestBus::Events::IsAabbVisibleInOcclusionView)
                ;
        }
    }
} // namespace AzFramework
