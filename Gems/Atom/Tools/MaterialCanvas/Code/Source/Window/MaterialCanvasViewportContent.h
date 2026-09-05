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
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/atomic.h>
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

        //! Blank the displayed material without forgetting which document the viewport is tracking, so that assets landing later can still
        //! bring the preview back. ApplyMaterial with a null document ID stops the tracking as well, which is only correct when the
        //! viewport genuinely has nothing to show.
        void ClearMaterial();

        //! Push the property values from the last compile onto the live material instance. Called after anything that recreates the
        //! material assignment, because that discards the overrides along with the instance.
        void ApplyMaterialPropertyValues();

        //! Resolve the asset ID of the first file generated for @documentId whose path ends with @extension. Returns a null ID, without
        //! reporting an error, when the asset system cannot resolve the file yet.
        //! Path of a generated file with @extension, preferring the preview output set. The asset id counterpart resolves through
        //! the catalog; this returns the source path, which is what the in-memory path needs.
        AZStd::string GetGeneratedFilePath(const AZ::Uuid& documentId, AZStd::string_view extension) const;

        //! Builds the material for @documentId in process and assigns it as a pre-created instance, bypassing the Asset Processor's
        //! FinalStage and MaterialBuilder jobs. Returns false, having changed nothing, whenever the inputs are not ready, so the
        //! caller can fall back to the asset system path that waits for them.
        bool ApplyInMemoryMaterial(const AZ::Uuid& documentId);

        //! Milliseconds since the last compile finished, for reporting how long the viewport waited to show a material.
        double MillisecondsSinceCompile() const;

        AZStd::chrono::steady_clock::time_point m_compileCompletedAt = AZStd::chrono::steady_clock::now();

        AZ::Data::AssetId GetGeneratedAssetId(const AZ::Uuid& documentId, AZStd::string_view extension) const;

        //! Decide whether an asset catalog update invalidates the applied material and, if so, queue a rebuild for the next system tick.
        void QueueApplyMaterialIfAffected(const AZ::Data::AssetId& assetId);

        //! Absolute path of the graph backing @documentId, or an empty string when the document cannot be reached. This is the key the
        //! property values are stored under, because MaterialGraphCompilerNotificationBus identifies a compile by its graph path and the
        //! compiler has no document ID to report.
        AZStd::string GetDocumentPath(const AZ::Uuid& documentId) const;

        AZ::Entity* m_environmentEntity = {};
        AZ::Entity* m_gridEntity = {};
        AZ::Entity* m_objectEntity = {};
        AZ::Entity* m_postFxEntity = {};
        AZ::Entity* m_shadowCatcherEntity = {};
        AZ::Uuid m_lastOpenedDocumentId;

        //! Document whose generated material the viewport is displaying, or waiting on. Null while no material is applied.
        AZ::Uuid m_appliedDocumentId;

        //! Asset IDs resolved by the most recent ApplyMaterial call. The material ID stays null until the Asset Processor has registered
        //! the generated source file, which is the condition the catalog handlers retry on. The material type ID is tracked separately
        //! because its products are built after the material itself resolves, and a change to any of them has to rebuild the preview.
        AZ::Data::AssetId m_appliedMaterialAssetId;
        AZ::Data::AssetId m_appliedMaterialTypeAssetId;

        //! Raised from the asset catalog thread, consumed on the next system tick.
        AZStd::atomic_bool m_applyMaterialQueued{ false };

        //! Debounce state for catalog driven rebuilds, touched only on the main thread.
        //!
        //! Rebuilding is expensive and self defeating if done per notification. ApplyMaterial resolves asset IDs through the Asset
        //! Processor and hands the material component an assignment with m_materialInstanceMustBeUnique, so each rebuild costs two
        //! round trips and a freshly allocated Material with its own SRGs and constant buffers, which the render pipeline can still be
        //! referencing for several frames. The Asset Processor emits a burst of catalog notifications while it drains its queue, and
        //! every product of the generated material type shares that material type's GUID, so the burst all matches. Rebuilding on each
        //! one allocated without bound and kept the freshly loaded material asset open, which in turn blocked the Asset Processor from
        //! replacing its own product ("Unable to remove file ... to copy source file" in the AP log) and stalled the very jobs the
        //! compile was waiting on.
        bool m_applyMaterialPending = false;
        AZStd::chrono::steady_clock::time_point m_applyMaterialQuietDeadline;
        AZStd::chrono::steady_clock::time_point m_applyMaterialBurstStart;

        //! Material property values from the most recent compile of each graph, keyed by that graph's absolute path and applied as
        //! overrides on the material instance so that editing a material input node's value shows up without rebuilding the material type
        //! and its shaders. Written from the graph compilation job thread and read on the main thread, hence the mutex.
        //!
        //! Keyed per graph rather than held as a single set because compiles are per document while the viewport shows one document at a
        //! time. A document compiling in the background used to overwrite the visible document's values, so whichever compile finished
        //! last won regardless of what was on screen. Keeping them apart also means a background compile's results are already waiting
        //! when that document is brought to the front, instead of needing another compile to reappear.
        mutable AZStd::mutex m_materialPropertyValuesMutex;
        AZStd::unordered_map<AZStd::string, MaterialGraphCompilerNotifications::PropertyValueList> m_materialPropertyValuesByGraphPath;

        //! Raised from the graph compilation job thread when new property values arrive, consumed on the next system tick.
        //!
        //! Deliberately separate from m_applyMaterialQueued. Values reaching the viewport this way never change the generated assets,
        //! which is the entire point of moving them out of the material type, so the live instance only needs the overrides pushed onto
        //! it. Rebuilding the material is what makes the catalog driven path expensive, and none of that expense buys anything here.
        AZStd::atomic_bool m_applyMaterialPropertyValuesQueued{ false };
    };
} // namespace MaterialCanvas
