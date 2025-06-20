/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/MotionBlur/MotionBlurComponent.h>

namespace AZ
{
    namespace Render
    {
        MotionBlurComponent::MotionBlurComponent(const MotionBlurComponentConfig& config)
            : BaseClass(config)
        {
        }

        void MotionBlurComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MotionBlurComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<MotionBlurComponent>()->RequestBus("MotionBlurRequestBus");

                behaviorContext->ConstantProperty("MotionBlurComponentTypeId", BehaviorConstant(Uuid(MotionBlur::MotionBlurComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }

    } // namespace Render
} // namespace AZ
