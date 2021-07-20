/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include "EditorCommon.h"
#include "UiEditorEntityContextBus.h"

class UiSliceManager
    : public UiEditorEntityContextNotificationBus::Handler
{
public:     // member functions

    UiSliceManager(AzFramework::EntityContextId entityContextId);
    ~UiSliceManager() override;

    //////////////////////////////////////////////////////////////////////////
    // UiEditorEntityContextNotificationBus implementation
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& ticket) override;
    //~UiEditorEntityContextNotificationBus implementation

    //! Instantiate an existing slice asset into the UI canvas
    void InstantiateSlice(const AZ::Data::AssetId& assetId, AZ::Vector2 viewportPosition, int childIndex = -1);

    //! Instantiate an existing slice asset into the UI canvas using a file browser
    void InstantiateSliceUsingBrowser(HierarchyWidget* hierarchy, AZ::Vector2 viewportPosition);

    //! Create a new slice from the selected items and replace the selected items
    //! with an instance of the slice
    void MakeSliceFromSelectedItems(HierarchyWidget* hierarchy, bool inheritSlices);

    // Get the root slice for the canvas
    AZ::SliceComponent* GetRootSlice() const;

    //! Given a set of entities return a set that contains these entities plus all of their descendants
    AzToolsFramework::EntityIdSet GatherEntitiesAndAllDescendents(const AzToolsFramework::EntityIdList& inputEntities);
    
    //! Brings up the Push to Slice (advanced) dialog
    void PushEntitiesModal(const AzToolsFramework::EntityIdList& entities,
                           AZ::SerializeContext* serializeContext = nullptr);

    //! Detach the given entities from the slice instance(s) that they are part of
    void DetachSliceEntities(const AzToolsFramework::EntityIdList& entities);

    //! Detach all entities in the slice instances that the give entities are part of from their slice instances
    void DetachSliceInstances(const AzToolsFramework::EntityIdList& entities);

    //! Returns true if the entity has a null parent pointer
    bool IsRootEntity(const AZ::Entity& entity) const;

    //! Set the entity context that this UI slice manager is operating on
    void SetEntityContextId(AzFramework::EntityContextId entityContextId);

    //! Get the entity context that this UI slice manager is operating on
    AzFramework::EntityContextId GetEntityContextId() const { return m_entityContextId; }

    //! Push the given entities back to the given slice asset (they must be part of an instance of that slice)
    //! No adds or removes are performed by this operation
    AZ::Outcome<void, AZStd::string> PushEntitiesBackToSlice(const AzToolsFramework::EntityIdList& entityIdList, const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset);

    //! Push the given set of entities to the given slice instance (handles adds and removes).
    AZ::Outcome<void, AZStd::string> QuickPushSliceInstance(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress,
        const AzToolsFramework::EntityIdList& entityIdList);
    
private:    // member functions

    static AZStd::string MakeTemporaryFilePathForSave(const char* targetFilename);

    void MakeSliceFromEntities(AzToolsFramework::EntityIdList& entities, bool inheritSlices);

    bool MakeNewSlice(const AzToolsFramework::EntityIdSet& entities, 
                        const char* targetDirectory, 
                        bool inheritSlices, 
                        AZ::SerializeContext* serializeContext = nullptr);

    void GetTopLevelEntities(const AZ::SliceComponent::EntityList& entities, AZ::SliceComponent::EntityList& topLevelEntities);

    //! Used in slice creation validation/preparation - checks single root for selected entities, generates
    //! ordered list of entities to use in slice
    AZ::Entity* ValidateSingleRootAndGenerateOrderedEntityList(const AzToolsFramework::EntityIdSet& liveEntities,
        AzToolsFramework::EntityIdList& outOrderedEntityList, AZ::Entity*& insertBefore);

    //! \return whether user confirmed detach, false if cancelled
    bool ConfirmDialog_Detach(const QString& title, const QString& text);

private:    // data

    AzFramework::EntityContextId m_entityContextId;
};

