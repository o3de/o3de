/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    void NonUniformScaleRequests::Reflect(ReflectContext* context)
    {
        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->EBus<NonUniformScaleRequestBus>("NonUniformScaleRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(Script::Attributes::Category, "Entity")
                ->Attribute(Script::Attributes::Module, "entity")
                ->Event("GetScale", &NonUniformScaleRequestBus::Events::GetScale)
                ->Event("SetScale", &NonUniformScaleRequestBus::Events::SetScale)
                ;
        }
    }
} // namespace AZ
