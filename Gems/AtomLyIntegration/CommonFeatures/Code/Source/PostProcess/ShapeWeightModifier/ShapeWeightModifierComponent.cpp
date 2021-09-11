/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        ShapeWeightModifierComponent::ShapeWeightModifierComponent(const ShapeWeightModifierComponentConfig& config)
            : BaseClass(config)
        {
        }

        void ShapeWeightModifierComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ShapeWeightModifierComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ShapeWeightModifierComponent>()->RequestBus("PostFxWeightRequestBus");

                behaviorContext->ConstantProperty("ShapeWeightModifierComponentTypeId", BehaviorConstant(Uuid(ShapeWeightModifierComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    }
}

