/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>

#include <Cry_Vector2.h>

#include <LyShine/UiEntityContext.h>
#include "UiEditorEntityContextBus.h"

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class EntityContext;
}

class EditorWindow;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UIEditorEntityContext extends the UiEditorContext to add functionality only needed when
//! a UI canvas is loaded in the UI Editor.
class UiEditorEntityContext
    : public UiEntityContext
    , public AZ::Data::AssetBus::MultiHandler
    , private UiEditorEntityContextRequestBus::Handler
    , private AzToolsFramework::EditorEntityContextPickingRequestBus::Handler
    , private AzFramework::AssetCatalogEventBus::Handler
    , private AzFramework::SliceInstantiationResultBus::MultiHandler
{
public: // member functions

    UiEditorEntityContext(EditorWindow* editorWindow);
    ~UiEditorEntityContext() override;

    bool HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr);

    // UiEntityContext
    void InitUiContext() override;
    void DestroyUiContext() override;
    bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
    bool SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
    // ~UiEntityContext

    // UiEntityContextRequestBus
    AZ::SliceComponent* GetUiRootSlice() override;
    AZ::Entity* CreateUiEntity(const char* name) override;
    void AddUiEntity(AZ::Entity* entity) override;
    void AddUiEntities(const AzFramework::EntityList& entities) override;
    bool CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities) override;
    bool DestroyUiEntity(AZ::EntityId entityId) override;
    // ~UiEntityContextRequestBus

    // EditorEntityContextPickingRequestBus
    bool SupportsViewportEntityIdPicking() override;
    // ~EditorEntityContextPickingRequestBus

    // UiEditorEntityContextRequestBus
    AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance) override;
    AzFramework::SliceInstantiationTicket InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, AZ::Vector2 viewportPosition) override;
    AzFramework::SliceInstantiationTicket InstantiateEditorSliceAtChildIndex(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
                                                                                AZ::Vector2 viewportPosition,
                                                                                int childIndex) override;
    void RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info) override;
    void QueueSliceReplacement(const char* targetPath, 
                                const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
                                const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
                                AZ::Entity* commonParent, AZ::Entity* insertBefore) override;
    void DeleteElements(AzToolsFramework::EntityIdList elements) override;
    bool HasPendingRequests() override;
    bool IsInstantiatingSlices() override;
    void DetachSliceEntities(const AzToolsFramework::EntityIdList& entities) override;
    // ~UiEditorEntityContextRequestBus

    // AzFramework::SliceInstantiationResultBus
    void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
    void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
    // ~AzFramework::SliceInstantiationResultBus
        
    // AssetCatalogEventBus::Handler
    void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
    // ~AssetCatalogEventBus::Handler

    // EntityContextRequestBus
    void ResetContext() override;
    // ~EntityContextRequestBus

    AZStd::string GetErrorMessage() const { return m_errorMessage; }

protected: // types

    struct InstantiatingEditorSliceParams
    {
        InstantiatingEditorSliceParams(const AZ::Vector2& viewportPosition, int childIndex = -1)
        {
            m_viewportPosition = viewportPosition;
            m_childIndex = childIndex;
        }

        AZ::Vector2 m_viewportPosition;
        int m_childIndex;
    };

protected: // member functions

    void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
    void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
    void OnContextEntitiesAdded(const AzFramework::EntityList& entities) override;

    // Used to validate that the entities in an instantiated slice are valid entities for this context
    bool ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities) override;

    void SetupUiEntity(AZ::Entity* entity);
    void InitializeEntities(const AzFramework::EntityList& entities);

protected: // data

    using InstantiatingSlicePair = AZStd::pair<AZ::Data::Asset<AZ::Data::AssetData>, InstantiatingEditorSliceParams>;
    AZStd::vector<InstantiatingSlicePair> m_instantiatingSlices;

private: // types

    //! Tracks a queued slice replacement, which is a deferred operation.
    //! If the asset has not yet been processed (a new asset), we need
    //! to defer before attempting a load.
    struct QueuedSliceReplacement
    {
        ~QueuedSliceReplacement() = default;
        QueuedSliceReplacement() = default;

        QueuedSliceReplacement(const QueuedSliceReplacement&) = delete;
        QueuedSliceReplacement& operator=(const QueuedSliceReplacement&) = delete;

        void Setup(const char* path,
                    const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
                    const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
                    AZ::Entity* commonParent, AZ::Entity* insertBefore)
        {
            m_path = path;
            m_selectedToAssetMap = selectedToAssetMap;
            m_entitiesInSelection.clear();
            m_entitiesInSelection.insert(entitiesInSelection.begin(), entitiesInSelection.end());
            m_commonParent = commonParent;
            m_insertBefore = insertBefore;
        }

        bool IsValid() const;
        void Reset();

        void Finalize(const AZ::SliceComponent::SliceInstanceAddress& instanceAddress, EditorWindow* editorWindow);

        AZStd::string                                       m_path;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId>    m_selectedToAssetMap;
        AZStd::unordered_set<AZ::EntityId>                  m_entitiesInSelection;
        AZ::Entity*                                         m_commonParent;
        AZ::Entity*                                         m_insertBefore;
        AzFramework::SliceInstantiationTicket               m_ticket;
    };

private: // member functions

    void GetTopLevelEntities(const AZStd::unordered_set<AZ::EntityId>& entities, AZStd::unordered_set<AZ::EntityId>& topLevelEntities);

private: // data

    EditorWindow* m_editorWindow;

    //! List of selected entities prior to entering game.
    AZStd::vector<AZ::EntityId> m_selectedBeforeStartingGame;

    QueuedSliceReplacement m_queuedSliceReplacement;

    //! Slice entity restore requests, which can be deferred if asset wasn't loaded at request time.
    struct SliceEntityRestoreRequest
    {
        AZ::Entity* m_entity;
        AZ::SliceComponent::EntityRestoreInfo m_restoreInfo;
        AZ::Data::Asset<AZ::Data::AssetData> m_asset;
    };

    AZStd::vector<SliceEntityRestoreRequest> m_queuedSliceEntityRestores;

    AZ::ComponentTypeList m_requiredEditorComponentTypes;

    AZStd::string m_errorMessage;

    AZ::Data::AssetId m_rootAssetId;
};
