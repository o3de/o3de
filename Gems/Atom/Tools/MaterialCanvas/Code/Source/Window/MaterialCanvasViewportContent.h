/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>
#include <AtomToolsFramework/Graph/GraphDocumentNotificationBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <Document/MaterialGraphCompilerNotificationBus.h>

namespace MaterialCanvas
{
    class MaterialCanvasViewportContent final
        : public AtomToolsFramework::EntityPreviewViewportContent
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , public AtomToolsFramework::GraphDocumentNotificationBus::Handler
        , public AzFramework::AssetCatalogEventBus::Handler
        , public AZ::SystemTickBus::Handler
        , public MaterialGraphCompilerNotificationBus::Handler
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
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::GraphDocumentNotificationBus::Handler overrides...
        void OnCompileGraphStarted(const AZ::Uuid& documentId) override;
        void OnCompileGraphProcessing(const AZ::Uuid& documentId) override;
        void OnCompileGraphCompleted(const AZ::Uuid& documentId) override;
        void OnCompileGraphFailed(const AZ::Uuid& documentId) override;

        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        // AzFramework::AssetCatalogEventBus::Handler overrides...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

        // AZ::SystemTickBus::Handler overrides...
        void OnSystemTick() override;

        // MaterialGraphCompilerNotificationBus::Handler overrides...
        void OnMaterialPropertyValuesChanged(
            const AZStd::string& graphPath, const MaterialGraphCompilerNotifications::PropertyValueList& propertyValues) override;

        void ApplyMaterial(const AZ::Uuid& documentId);

        //! Blank the material but keep tracking the document so later assets can restore it; ApplyMaterial(null) stops tracking.
        void ClearMaterial();

        //! Push the last compile's property values onto the live instance; needed after anything that recreates the assignment.
        void ApplyMaterialPropertyValues();

        //! Source path of a generated file with @extension, preferring the preview output set (used by the in-memory path).
        AZStd::string GetGeneratedFilePath(const AZ::Uuid& documentId, AZStd::string_view extension) const;

        //! Builds and assigns the material in process, bypassing FinalStage/MaterialBuilder; false (no change) if inputs aren't ready.
        bool ApplyInMemoryMaterial(const AZ::Uuid& documentId);

        //! Starts a background job compiling @materialTypeAsset's shaders if none is running; never on the main thread.
        void QueueInMemoryShaderCompile(
            const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>& materialTypeAsset, const AZStd::string& materialTypePath);

        //! Milliseconds since the last compile finished, for reporting how long the viewport waited to show a material.
        double MillisecondsSinceCompile() const;

        AZStd::chrono::steady_clock::time_point m_compileCompletedAt = AZStd::chrono::steady_clock::now();

        //! Asset ID of the first generated file ending in @extension, or a null ID (no error) if it can't resolve yet.
        AZ::Data::AssetId GetGeneratedAssetId(const AZ::Uuid& documentId, AZStd::string_view extension) const;

        //! Decide whether an asset catalog update invalidates the applied material and, if so, queue a rebuild for the next system tick.
        void QueueApplyMaterialIfAffected(const AZ::Data::AssetId& assetId);

        //! Absolute graph path for @documentId (or empty); the key for property values, since the compiler reports graph paths.
        AZStd::string GetDocumentPath(const AZ::Uuid& documentId) const;

        AZ::Entity* m_environmentEntity = {};
        AZ::Entity* m_gridEntity = {};
        AZ::Entity* m_objectEntity = {};
        AZ::Entity* m_postFxEntity = {};
        AZ::Entity* m_shadowCatcherEntity = {};
        AZ::Uuid m_lastOpenedDocumentId;

        //! Document whose generated material the viewport is displaying, or waiting on. Null while no material is applied.
        AZ::Uuid m_appliedDocumentId;

        //! The material type on screen, so a shader compile can start before the next one is built (same shaders either way).
        AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> m_appliedMaterialTypeAsset;

        //! Asset IDs from the last ApplyMaterial; the material ID stays null until the AP registers the source (catalog handlers retry).
        AZ::Data::AssetId m_appliedMaterialAssetId;
        AZ::Data::AssetId m_appliedMaterialTypeAssetId;

        //! Shaders compiled here, keyed by the AssetId they replace, racing the AP harmlessly; written by the job, read on main thread.
        AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::ShaderAsset>>> m_compiledShaders;
        mutable AZStd::mutex m_compiledShadersMutex;

        //! Compile generation the shaders above belong to; bumped per graph compile so late jobs are discarded.
        AZStd::atomic_uint m_compileGeneration{ 0 };
        AZStd::atomic_uint m_compiledShadersGeneration{ 0 };

        //! Set while a compile job is running, so a burst of catalog notifications starts one job rather than one per notification.
        AZStd::atomic_bool m_shaderCompileInFlight{ false };

        //! Rebuild next tick without the catalog debounce, for shaders compiled here (the debounce cost ~500 ms).
        AZStd::atomic_bool m_applyMaterialImmediately{ false };

        //! Raised from the asset catalog thread, consumed on the next system tick.
        AZStd::atomic_bool m_applyMaterialQueued{ false };

        //! Main-thread debounce for catalog-driven rebuilds; rebuilding per notification piled up materials and stalled the AP.
        bool m_applyMaterialPending = false;
        AZStd::chrono::steady_clock::time_point m_applyMaterialQuietDeadline;
        AZStd::chrono::steady_clock::time_point m_applyMaterialBurstStart;

        //! Latest property values per graph path, applied as overrides; per graph so background compiles don't clobber the visible one.
        mutable AZStd::mutex m_materialPropertyValuesMutex;
        AZStd::unordered_map<AZStd::string, MaterialGraphCompilerNotifications::PropertyValueList> m_materialPropertyValuesByGraphPath;

        //! Set on the job thread when property values arrive; separate from m_applyMaterialQueued since no rebuild is needed.
        AZStd::atomic_bool m_applyMaterialPropertyValuesQueued{ false };
    };
} // namespace MaterialCanvas
