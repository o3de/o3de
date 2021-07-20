/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponent.h>

namespace AZ
{
    namespace Render
    {

        DiffuseGlobalIlluminationComponent::DiffuseGlobalIlluminationComponent(const DiffuseGlobalIlluminationComponentConfig& config)
            : BaseClass(config)
        {
        }

        void DiffuseGlobalIlluminationComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DiffuseGlobalIlluminationComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("DiffuseGlobalIlluminationComponentTypeId", BehaviorConstant(Uuid(DiffuseGlobalIlluminationComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }

    } // namespace Render
} // namespace AZ
