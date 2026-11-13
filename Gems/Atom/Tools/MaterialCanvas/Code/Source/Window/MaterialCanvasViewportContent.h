/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>
#include <AtomToolsFramework/Graph/GraphDocumentNotificationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace MaterialCanvas
{
    class MaterialCanvasViewportContent final
        : public AtomToolsFramework::EntityPreviewViewportContent
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , public AtomToolsFramework::GraphDocumentNotificationBus::Handler
        , public AzFramework::AssetCatalogEventBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
        , public AZ::SystemTickBus::Handler
    {
    public:
        MaterialCanvasViewportContent(
            const AZ::Crc32& toolId,
            AtomToolsFramework::RenderViewportWidget* widget,
            AZStd::shared_ptr<AzFramework::EntityContext> entityContext);
        ~MaterialCanvasViewportContent();

        AZ::EntityId GetObjectEntityId() const override;
        AZ::EntityId GetEnvironmentEntityId() const override;
        AZ::EntityId GetPostFxEntityId() const override;
        AZ::EntityId GetShadowCatcherEntityId() const;
        AZ::EntityId GetGridEntityId() const;

    private:
        enum class AssetNotification : AZ::u8
        {
            Ready,
            Reloaded,
            Error
        };

        enum class MaterialLoadState : AZ::u8
        {
            None,
            Loading,
            Reloading
        };

        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::GraphDocumentNotificationBus::Handler overrides...
        void OnCompileGraphStarted(const AZ::Uuid& documentId) override;
        void OnCompileGraphGeneratedFilesChanged(
            const AZ::Uuid& documentId, const AZStd::vector<AZStd::string>& modifiedGeneratedFiles) override;
        void OnCompileGraphCompleted(const AZ::Uuid& documentId) override;
        void OnCompileGraphFailed(const AZ::Uuid& documentId) override;

        // AzFramework::AssetCatalogEventBus::Handler overrides...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

        // AZ::Data::AssetBus::MultiHandler overrides...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // AZ::SystemTickBus::Handler overrides...
        void OnSystemTick() override;

        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        struct PreviewState
        {
            AZStd::string m_materialSourcePath;
            AZ::Data::AssetId m_materialAssetId;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
            AZStd::unordered_set<AZ::Data::AssetId> m_trackedAssetReloads;
            AZStd::unordered_set<AZ::Data::AssetId> m_outstandingAssetReloads;
            AZStd::unordered_set<AZ::Data::AssetId> m_supersededAssetReloads;
            bool m_updatePending = false;
            bool m_updateFailed = false;
            bool m_materialProductChanged = false;
            bool m_waitForMaterialProduct = false;
            bool m_sourceGenerationAccepted = true;
            MaterialLoadState m_staleMaterialLoadState = MaterialLoadState::None;
            MaterialLoadState m_materialLoadState = MaterialLoadState::None;
        };

        void UpdateMaterialIdentity(const AZ::Uuid& documentId, PreviewState& state, bool clearIfEmpty);
        void RefreshTrackedAssetReloads(PreviewState& state);
        bool IsMaterialSourcePath(const PreviewState& state, const AZStd::string& assetPath) const;
        void BeginMaterialUpdate(PreviewState& state, const AZStd::vector<AZStd::string>& modifiedGeneratedFiles);
        void ResetMaterialUpdate(PreviewState& state);
        void FailMaterialUpdate(PreviewState& state);
        void RebuildAssetBusConnections();
        void ApplyExpectedMaterialIfReady(PreviewState& state);
        void ApplyMaterial(const AZ::Data::AssetId& assetId);
        void ClearMaterial();

        AZ::Entity* m_environmentEntity = {};
        AZ::Entity* m_gridEntity = {};
        AZ::Entity* m_objectEntity = {};
        AZ::Entity* m_postFxEntity = {};
        AZ::Entity* m_shadowCatcherEntity = {};
        AZ::Uuid m_lastOpenedDocumentId;
        AZStd::unordered_map<AZ::Uuid, PreviewState> m_previewStates;
        AZStd::unordered_set<AZ::Data::AssetId> m_connectedAssetIds;
        bool m_materialAssigned = false;

        AZStd::mutex m_assetNotificationMutex;
        AZStd::vector<AZStd::pair<AZ::Data::Asset<AZ::Data::AssetData>, AssetNotification>> m_assetNotifications;
    };
} // namespace MaterialCanvas
