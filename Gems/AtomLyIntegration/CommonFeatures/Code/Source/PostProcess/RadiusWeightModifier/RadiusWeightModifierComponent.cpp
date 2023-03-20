/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        RadiusWeightModifierComponent::RadiusWeightModifierComponent(const RadiusWeightModifierComponentConfig& config)
            : BaseClass(config)
        {
        }

        void RadiusWeightModifierComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<RadiusWeightModifierComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<RadiusWeightModifierComponent>()->RequestBus("PostFxWeightRequestBus");

                behaviorContext->ConstantProperty("RadiusWeightModifierComponentTypeId", BehaviorConstant(Uuid(RadiusWeightModifierComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    }
}

