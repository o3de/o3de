/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponent.h>

namespace AZ
{
    namespace Render
    {

        DepthOfFieldComponent::DepthOfFieldComponent(const DepthOfFieldComponentConfig& config)
            : BaseClass(config)
        {
        }

        void DepthOfFieldComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DepthOfFieldComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<DepthOfFieldComponent>()->RequestBus("DepthOfFieldRequestBus");

                behaviorContext->ConstantProperty("DepthOfFieldComponentTypeId", BehaviorConstant(Uuid(DepthOfField::DepthOfFieldComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }

    } // namespace Render
} // namespace AZ
