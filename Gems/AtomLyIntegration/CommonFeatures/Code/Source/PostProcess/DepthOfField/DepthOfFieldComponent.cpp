/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
