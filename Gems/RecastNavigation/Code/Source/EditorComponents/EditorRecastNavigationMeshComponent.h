/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <Misc/RecastNavigationConstants.h>
#include <Misc/RecastNavigationMeshComponentController.h>
#include <Misc/RecastNavigationMeshConfig.h>
#include <ToolsComponents/EditorComponentAdapter.h>
#include <ToolsComponents/EditorComponentBase.h>

namespace RecastNavigation
{
    //! Editor version of @RecastNavigationMeshComponent.
    class EditorRecastNavigationMeshComponent final
        : public AzToolsFramework::Components::EditorComponentAdapter<RecastNavigationMeshComponentController,
        RecastNavigationMeshComponent, RecastNavigationMeshConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<RecastNavigationMeshComponentController, RecastNavigationMeshComponent, RecastNavigationMeshConfig>;
        AZ_EDITOR_COMPONENT(EditorRecastNavigationMeshComponent, EditorRecastNavigationMeshComponentTypeId, BaseClass);
        static void Reflect(AZ::ReflectContext* context);

        EditorRecastNavigationMeshComponent() = default;
        explicit EditorRecastNavigationMeshComponent(const RecastNavigationMeshConfig& config);

        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //! Enables or disables in-Editor preview of navigation mesh without entering game mode.
        //! @param enable if true, the preview will be enabled
        void SetEditorPreview(bool enable);

        void OnEditorUpdateTick();

        AZ::u32 OnConfigurationChanged() override;

    private:
        AZ::ScheduledEvent m_inEditorUpdateTick{ [this]() {OnEditorUpdateTick(); }, AZ::Name("EditorRecastNavigationMeshTick") };
    };
} // namespace RecastNavigation
