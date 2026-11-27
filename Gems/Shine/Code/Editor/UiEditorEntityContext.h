/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UiEditorEntityContextBus.h"

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>
#include <Cry_Vector2.h>
#include <Shine/UiEntityContext.h>
#include "UiSimpleEntityOwnershipService.h"

namespace AZ
{
    class SerializeContext;
    class Entity;
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
{
public: // member functions

    UiEditorEntityContext(EditorWindow* editorWindow);
    ~UiEditorEntityContext() override;

    bool HandleLoadedEntities(const AZStd::vector<AZ::Entity*>& entities, bool remapIds,
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId>* idRemapTable = nullptr) override;

    // UiEntityContext
    void InitUiContext() override;
    void DestroyUiContext() override;
    bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
    bool SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
    // ~UiEntityContext

    // UiEntityContextRequestBus
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
    void DeleteElements(AzToolsFramework::EntityIdList elements) override;
    bool HasPendingRequests() override;
    // ~UiEditorEntityContextRequestBus

    // EntityContextRequestBus
    void ResetContext() override;
    // ~EntityContextRequestBus

    AZStd::string GetErrorMessage() const { return m_errorMessage; }

protected: // member functions

    void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
    void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
    void OnContextEntitiesAdded(const AzFramework::EntityList& entities) override;

    // Used to validate that the entities are valid entities for this context
    bool ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities) override;

    void SetupUiEntity(AZ::Entity* entity);
    void InitializeEntities(const AzFramework::EntityList& entities);

private: // data

    EditorWindow* m_editorWindow;

    //! List of selected entities prior to entering game.
    AZStd::vector<AZ::EntityId> m_selectedBeforeStartingGame;

    AZ::ComponentTypeList m_requiredEditorComponentTypes;

    AZStd::string m_errorMessage;

    AZ::Data::AssetId m_rootAssetId;
};
