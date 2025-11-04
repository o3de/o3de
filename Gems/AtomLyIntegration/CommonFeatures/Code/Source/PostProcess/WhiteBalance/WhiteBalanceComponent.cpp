/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/WhiteBalance/WhiteBalanceComponent.h>

namespace AZ
{
    namespace Render
    {
        WhiteBalanceComponent::WhiteBalanceComponent(const WhiteBalanceComponentConfig& config)
            : BaseClass(config)
        {
        }

        void WhiteBalanceComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WhiteBalanceComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<WhiteBalanceComponent>()->RequestBus("WhiteBalanceRequestBus");

                behaviorContext->ConstantProperty("WhiteBalanceComponentTypeId", BehaviorConstant(Uuid(WhiteBalance::WhiteBalanceComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }

    } // namespace Render
} // namespace AZ
