/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationPhysXProviderComponent.h"

#include <AzCore/Debug/Budget.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    RecastNavigationPhysXProviderComponent::RecastNavigationPhysXProviderComponent(const RecastNavigationPhysXProviderConfig& config)
        : BaseClass(config)
    {
    }

    void RecastNavigationPhysXProviderComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RecastNavigationPhysXProviderComponent, BaseClass>()
                ->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("RecastNavigationPhysXProviderComponentTypeId",
                BehaviorConstant(AZ::Uuid(RecastNavigationPhysXProviderComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            behaviorContext->Class<RecastNavigationPhysXProviderComponent>()->RequestBus("RecastNavigationProviderRequestBus");
        }
    }
} // namespace RecastNavigation
