/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AzToolsFrameworkModule.h>

 // Component includes
#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>
#include <AzToolsFramework/Archive/ArchiveComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBundle/AssetBundleComponent.h>
#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <AzToolsFramework/Component/EditorLevelComponentAPIComponent.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntitySystemComponent.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityFixupComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntitySystemComponent.h>
#include <AzToolsFramework/FocusMode/FocusModeSystemComponent.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsSystemComponent.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditorComponent.h>
#include <AzToolsFramework/Render/EditorIntersectorComponent.h>
#include <AzToolsFramework/Slice/SliceDependencyBrowserComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>
#include <AzToolsFramework/Slice/SliceRequestComponent.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/ToolsComponents/AzToolsFrameworkConfigurationSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiSystemComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerNullComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemComponent.h>
#include <AzToolsFramework/Entity/EntityUtilityComponent.h>
#include <AzToolsFramework/Script/LuaEditorSystemComponent.h>
#include <AzToolsFramework/Script/LuaSymbolsReporterSystemComponent.h>
#include <AzToolsFramework/Viewport/SharedViewBookmarkComponent.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkComponent.h>
#include <AzToolsFramework/Viewport/ViewBookmarkSystemComponent.h>
#include <Metadata/MetadataManager.h>
#include <Prefab/ProceduralPrefabSystemComponent.h>
#include <AzToolsFramework/Metadata/UuidUtils.h>

AZ_DEFINE_BUDGET(AzToolsFramework);

namespace AzToolsFramework
{
    AzToolsFrameworkModule::AzToolsFrameworkModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ActionManagerSystemComponent::CreateDescriptor(),
            Components::TransformComponent::CreateDescriptor(),
            Components::EditorNonUniformScaleComponent::CreateDescriptor(),
            Components::SelectionComponent::CreateDescriptor(),
            Components::GenericComponentWrapper::CreateDescriptor(),
            Components::PropertyManagerComponent::CreateDescriptor(),
            Components::ScriptEditorComponent::CreateDescriptor(),
            Components::EditorSelectionAccentSystemComponent::CreateDescriptor(),
            EditorEntityContextComponent::CreateDescriptor(),
            EditorEntityFixupComponent::CreateDescriptor(),
            EntityUtilityComponent::CreateDescriptor(),
            ContainerEntitySystemComponent::CreateDescriptor(),
            ReadOnlyEntitySystemComponent::CreateDescriptor(),
            FocusModeSystemComponent::CreateDescriptor(),
            SliceMetadataEntityContextComponent::CreateDescriptor(),
            SliceRequestComponent::CreateDescriptor(),
            Prefab::PrefabSystemComponent::CreateDescriptor(),
            Prefab::EditorPrefabComponent::CreateDescriptor(),
            Prefab::ProceduralPrefabSystemComponent::CreateDescriptor(),
            AzToolsFramework::ViewBookmarkSystemComponent::CreateDescriptor(),
            AzToolsFramework::LocalViewBookmarkComponent::CreateDescriptor(),
            Components::EditorEntityActionComponent::CreateDescriptor(),
            Components::EditorEntityIconComponent::CreateDescriptor(),
            Components::EditorInspectorComponent::CreateDescriptor(),
            Components::EditorLockComponent::CreateDescriptor(),
            Components::EditorPendingCompositionComponent::CreateDescriptor(),
            Components::EditorVisibilityComponent::CreateDescriptor(),
            Components::EditorDisabledCompositionComponent::CreateDescriptor(),
            Components::EditorOnlyEntityComponent::CreateDescriptor(),
            Components::EditorEntitySortComponent::CreateDescriptor(),
            Components::EditorEntityModelComponent::CreateDescriptor(),
            AssetSystem::AssetSystemComponent::CreateDescriptor(),
            PerforceComponent::CreateDescriptor(),
            AzToolsFramework::ArchiveComponent::CreateDescriptor(),
            AzToolsFramework::AssetBundleComponent::CreateDescriptor(),
            AzToolsFramework::SliceDependencyBrowserComponent::CreateDescriptor(),
            AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor(),
            AzToolsFramework::Thumbnailer::ThumbnailerNullComponent::CreateDescriptor(),
            AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor(),
            AzToolsFramework::EditorInteractionSystemComponent::CreateDescriptor(),
            AzToolsFramework::Components::EditorComponentAPIComponent::CreateDescriptor(),
            AzToolsFramework::Components::EditorLevelComponentAPIComponent::CreateDescriptor(),
            AzToolsFramework::Components::PropertyTreeEditorComponent::CreateDescriptor(),
            AzToolsFramework::Components::EditorEntitySearchComponent::CreateDescriptor(),
            AzToolsFramework::Components::EditorIntersectorComponent::CreateDescriptor(),
            AzToolsFramework::AzToolsFrameworkConfigurationSystemComponent::CreateDescriptor(),
            AzToolsFramework::Components::EditorEntityUiSystemComponent::CreateDescriptor(),
            AzToolsFramework::Script::LuaSymbolsReporterSystemComponent::CreateDescriptor(),
            AzToolsFramework::Script::LuaEditorSystemComponent::CreateDescriptor(),
            AzToolsFramework::GlobalPaintBrushSettingsSystemComponent::CreateDescriptor(),
            AzToolsFramework::MetadataManager::CreateDescriptor(),
            AzToolsFramework::UuidUtilComponent::CreateDescriptor(),
        });
    }
}
