/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Debug/RenderDebugComponent.h>

namespace AZ::Render
{
    RenderDebugComponent::RenderDebugComponent(const RenderDebugComponentConfig& config)
        : BaseClass(config)
    {
    }

    void RenderDebugComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderDebugComponent, BaseClass>();
        }

        if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<RenderDebugComponent>()->RequestBus("RenderDebugRequestBus");

            behaviorContext->ConstantProperty("RenderDebugComponentTypeId", BehaviorConstant(Uuid(RenderDebug::RenderDebugComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
        }
    }

}
