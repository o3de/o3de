/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientGlobalIllumination/GradientGlobalIlluminationComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::Render
{
    GradientGlobalIlluminationComponent::GradientGlobalIlluminationComponent(const GradientGlobalIlluminationComponentConfig& config)
        : BaseClass(config)
    {
    }

    void GradientGlobalIlluminationComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientGlobalIlluminationComponent, BaseClass>()->Version(1);
        }

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<GradientGlobalIlluminationComponent>()->RequestBus("GradientGlobalIlluminationComponentRequestBus");

            behaviorContext->ConstantProperty("GradientGlobalIlluminationComponentTypeId", BehaviorConstant(Uuid(GradientGlobalIlluminationComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
        }
    }
} // namespace AZ::Render
