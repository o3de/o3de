/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <SpecularReflections/SpecularReflectionsComponent.h>

namespace AZ
{
    namespace Render
    {
        SpecularReflectionsComponent::SpecularReflectionsComponent(const SpecularReflectionsComponentConfig& config)
            : BaseClass(config)
        {
        }

        void SpecularReflectionsComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpecularReflectionsComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("SpecularReflectionsComponentTypeId", BehaviorConstant(Uuid(SpecularReflectionsComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }

    } // namespace Render
} // namespace AZ
