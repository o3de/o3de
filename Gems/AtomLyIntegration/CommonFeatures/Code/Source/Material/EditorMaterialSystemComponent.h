/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentNotificationBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <Material/MaterialBrowserInteractions.h>
#include <QPixmap>


namespace AZ
{
    namespace Render
    {
        //! System component that manages launching and maintaining connections with the material editor.
        class EditorMaterialSystemComponent final
            : public AZ::Component
            , public AZ::EntitySystemBus::Handler
            , public EditorMaterialSystemComponentNotificationBus::Handler
            , public EditorMaterialSystemComponentRequestBus::Handler
            , public MaterialComponentNotificationBus::Router
            , public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            , public AzToolsFramework::EditorEvents::Bus::Handler
            , public AzToolsFramework::ToolsApplicationNotificationBus::Handler
            , public AZ::SystemTickBus::Handler
            , public AzFramework::AssetCatalogEventBus::Handler
            , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(EditorMaterialSystemComponent, "{96652157-DA0B-420F-B49C-0207C585144C}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

        private:
            //! EditorMaterialSystemComponentRequestBus::Handler overrides...
            void OpenMaterialEditor(const AZStd::string& sourcePath) override;
            void OpenMaterialCanvas(const AZStd::string& sourcePath) override;
            void OpenMaterialInspector(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& entityIdsToEdit,
                const AZ::Render::MaterialAssignmentId& materialAssignmentId) override;
            void RenderMaterialPreview(const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) override;
            QPixmap GetRenderedMaterialPreview(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId) const override;

            // AZ::EntitySystemBus::Handler overrides...
            void OnEntityDeactivated(const AZ::EntityId& entityId) override;

            //! AZ::SystemTickBus::Handler interface overrides...
            void OnSystemTick() override;

            // AzFramework::AssetCatalogEventBus::Handler overrides...
            void OnCatalogLoaded(const char* catalogFile) override;

            //! EditorMaterialSystemComponentNotificationBus::Handler overrides...
            void OnRenderMaterialPreviewRendered(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap) override;

            //! MaterialComponentNotificationBus::Router overrides...
            void OnMaterialSlotLayoutChanged() override;

            //! AssetBrowserInteractionNotificationBus::Handler overrides...
            AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;

            // AztoolsFramework::EditorEvents::Bus::Handler overrides...
            void NotifyRegisterViews() override;

            // AzToolsFramework::ToolsApplicationNotificationBus::Handler overrides...
            void AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList&) override;

            // AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler...
            void OnActionRegistrationHook() override;
            void OnMenuBindingHook() override;

            void PurgePreviews();

            AZStd::unique_ptr<MaterialBrowserInteractions> m_materialBrowserInteractions;
            AZStd::unordered_set<AZStd::pair<AZ::EntityId, AZ::Render::MaterialAssignmentId>> m_materialPreviewRequests;
            AZStd::unordered_map<AZ::EntityId, AZStd::unordered_map<AZ::Render::MaterialAssignmentId, QPixmap>> m_materialPreviews;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_materialPreviewModelAsset;
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_materialPreviewLightingPresetAsset;
            static constexpr const size_t MaterialPreviewLimit = 100;
            static constexpr const int MaterialPreviewResolution = 128;
        };
    } // namespace Render
} // namespace AZ
