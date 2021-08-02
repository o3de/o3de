/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/AreaLightComponent.h>

namespace AZ
{
    namespace Render
    {

        AreaLightComponent::AreaLightComponent(const AreaLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void AreaLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AreaLightComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<AreaLightComponent>()->RequestBus("AreaLightRequestBus");

                behaviorContext->ConstantProperty("AreaLightComponentTypeId", BehaviorConstant(Uuid(AreaLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
