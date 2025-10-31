/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Components/RecastNavigationPhysXProviderComponent.h>
#include <Misc/RecastNavigationPhysXProviderComponentController.h>
#include <ToolsComponents/EditorComponentAdapter.h>

namespace RecastNavigation
{
    //! Editor version of @RecastNavigationPhysXProviderComponent
    class EditorRecastNavigationPhysXProviderComponent final
        : public AzToolsFramework::Components::EditorComponentAdapter<RecastNavigationPhysXProviderComponentController,
                                                                      RecastNavigationPhysXProviderComponent, RecastNavigationPhysXProviderConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<RecastNavigationPhysXProviderComponentController, RecastNavigationPhysXProviderComponent, RecastNavigationPhysXProviderConfig>;
        AZ_EDITOR_COMPONENT(EditorRecastNavigationPhysXProviderComponent, EditorRecastNavigationPhysXProviderComponentTypeId, BaseClass);
        static void Reflect(AZ::ReflectContext* context);

        EditorRecastNavigationPhysXProviderComponent() = default;
        explicit EditorRecastNavigationPhysXProviderComponent(const RecastNavigationPhysXProviderConfig& config);

        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        AZ::u32 OnConfigurationChanged() override;
    };
} // namespace RecastNavigation
