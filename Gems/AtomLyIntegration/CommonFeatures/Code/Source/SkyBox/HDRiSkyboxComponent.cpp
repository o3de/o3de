/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyBox/HDRiSkyboxComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {

        HDRiSkyboxComponent::HDRiSkyboxComponent(const HDRiSkyboxComponentConfig& config)
            : BaseClass(config)
        {
        }

        void HDRiSkyboxComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<HDRiSkyboxComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<HDRiSkyboxComponent>()->RequestBus("HDRiSkyboxRequestBus");

                behaviorContext->ConstantProperty("HDRiSkyboxComponentTypeId", BehaviorConstant(Uuid(HDRiSkyboxComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
