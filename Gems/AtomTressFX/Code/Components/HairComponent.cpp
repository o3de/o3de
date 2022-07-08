/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Components/TransformComponent.h>

#include <Components/HairComponent.h>

#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairComponent::HairComponent(const HairComponentConfig& config)
                : BaseClass(config)
            {
            }

            HairComponent::~HairComponent()
            {
            }

            void HairComponent::Reflect(AZ::ReflectContext* context)
            {
                BaseClass::Reflect(context);

                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<HairComponent, BaseClass>();
                }

                if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
                {
                    behaviorContext->Class<HairComponent>()->RequestBus("HairRequestsBus");

                    behaviorContext->ConstantProperty("HairComponentTypeId", BehaviorConstant(Uuid(Hair::HairComponentTypeId)))
                        ->Attribute(AZ::Script::Attributes::Module, "render")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
                }
            }

            void HairComponent::Activate()
            {
                BaseClass::Activate();
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ


