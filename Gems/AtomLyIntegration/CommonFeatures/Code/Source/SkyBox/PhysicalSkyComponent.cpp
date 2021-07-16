/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyBox/PhysicalSkyComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {

        PhysicalSkyComponent::PhysicalSkyComponent(const PhysicalSkyComponentConfig& config)
            : BaseClass(config)
        {
        }

        void PhysicalSkyComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PhysicalSkyComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<PhysicalSkyComponent>()
                    ->RequestBus("PhysicalSkyRequestBus")
                    ->RequestBus("SkyBoxFogRequestBus");

                behaviorContext->ConstantProperty("PhysicalSkyComponentTypeId", BehaviorConstant(Uuid(PhysicalSkyComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
