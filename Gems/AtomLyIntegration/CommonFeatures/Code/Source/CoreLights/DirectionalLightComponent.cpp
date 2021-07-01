/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DirectionalLightComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        DirectionalLightComponent::DirectionalLightComponent(const DirectionalLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void DirectionalLightComponent::Reflect(ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DirectionalLightComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<DirectionalLightComponent>()->RequestBus("DirectionalLightRequestBus");

                behaviorContext->ConstantProperty("DirectionalLightComponentTypeId", BehaviorConstant(Uuid(DirectionalLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }

    } // namespace Render
} // namespace AZ
