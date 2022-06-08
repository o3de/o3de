/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationMeshComponent.h"

#include <DetourDebugDraw.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Preprocessor/CodeGen.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/CameraBus.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about a navigation mesh");
AZ_CVAR(
    float, cl_navmesh_debugRadius, 25.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Limit debug draw to within a specified distance from the active camera");

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    RecastNavigationMeshComponent::RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config)
        : BaseClass(config)
    {
    }

    void RecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RecastNavigationMeshComponent, BaseClass>()
                ->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("RecastNavigationMeshComponentTypeId", BehaviorConstant(AZ::Uuid(RecastNavigationMeshComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            behaviorContext->EBus<RecastNavigationMeshRequestBus>("RecastNavigationMeshRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Recast Navigation")
                ->Event("Update Navigation Mesh", &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted)
                ->Event("Update Navigation Mesh Async", &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);

            behaviorContext->Class<RecastNavigationMeshComponentController>()->RequestBus("RecastNavigationMeshRequestBus");

            behaviorContext->EBus<RecastNavigationMeshNotificationBus>("RecastNavigationMeshNotificationBus")
                ->Handler<RecastNavigationNotificationHandler>();
        }
    }
} // namespace RecastNavigation
