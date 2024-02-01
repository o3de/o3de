/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    /**
     * Bus for making requests to the edit-time entity context component.
     */
    class EditorEntityContextRequests
        : public AZ::EBusTraits
    {
    public:
        // This bus itself is thread safe, and function like GetEditorEntityContextId() is thread safe. But be careful
        // with function like AddEntity, DestroyEntity - those might not be thread safe due the implementation.
        static constexpr bool LocklessDispatch = true;

        virtual ~EditorEntityContextRequests() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Retrieve the Id of the editor entity context.
        virtual AzFramework::EntityContextId GetEditorEntityContextId() = 0;

        virtual AzFramework::EntityContext* GetEditorEntityContextInstance() = 0;

        /// Creates an entity in the editor context.
        /// \return the EntityId for the created Entity
        virtual AZ::EntityId CreateNewEditorEntity(const char* name) = 0;

        /// Creates an entity in the editor context.
        /// \param name The name to give the newly created entity.
        /// \param entityId The entity ID to create the new entity with.
        /// \return the EntityId for the created Entity
        virtual AZ::EntityId CreateNewEditorEntityWithId(const char* name, const AZ::EntityId& entityId) = 0;

        /// Registers an existing entity with the editor context.
        virtual void AddEditorEntity(AZ::Entity* entity) = 0;

        /// Registers an existing set of entities with the editor context.
        virtual void AddEditorEntities(const EntityList& entities) = 0;

        /// Triggers registered callbacks for an existing set of entities with the editor context.
        virtual void HandleEntitiesAdded(const EntityList& entities) = 0;

        /// Creates an editor ready entity, and sends out notification for the creation.
        virtual void FinalizeEditorEntity(AZ::Entity* entity) = 0;

        /// Destroys an entity in the editor context.
        /// \return whether or not the entity was destroyed. A false return value signifies the entity did not belong to the game context.
        virtual bool DestroyEditorEntity(AZ::EntityId entityId) = 0;

        /// Clones a set of entities and optionally creates the Sandbox objects to wrap them.
        /// This function doesn't automatically add new entities to any entity context, callers are responsible for that.
        /// \param sourceEntities - the source set of entities to clone
        /// \param resultEntities - the set of entities cloned from the source
        /// \param sourceToCloneEntityIdMap[out] The map between source entity ids and clone entity ids
        /// \return true means cloning succeeded, false otherwise
        virtual bool CloneEditorEntities(const EntityIdList& sourceEntities, 
                                         EntityList& resultEntities, 
                                         AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) = 0;

        /// Saves the context's slice root to the specified buffer. Entities are saved as-is (with editor components).
        /// \param stream The stream to save the editor entity context to.
        /// \param entitiesInLayers A list of entities that were saved into layers. These won't be saved to the editor entity context stream.
        /// \return true if successfully saved. Failure is only possible if serialization data is corrupt.
        virtual bool SaveToStreamForEditor(
            AZ::IO::GenericStream& stream,
            const EntityList& entitiesInLayers,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& instancesInLayers) = 0;

        /// Returns entities that are not part of a slice.
        /// \param entityList The entity list to populate with loose entities.
        virtual void GetLooseEditorEntities(EntityList& entityList) = 0;

        /// Saves the context's slice root to the specified buffer. Entities undergo conversion for game: editor -> game components.
        /// \return true if successfully saved. Failure is only possible if serialization data is corrupt.
        virtual bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) = 0;

        /// Loads the context's slice root from the specified buffer.
        /// \return true if successfully loaded. Failure is possible if the source file is corrupt or data could not be up-converted.
        virtual bool LoadFromStream(AZ::IO::GenericStream& stream) = 0;

        /// Loads the context's slice root from the specified buffer, and loads any layers referenced by the context.
        /// \param stream The stream to load from.
        /// \param levelPakFile The path to the level's pak file, which is used to find the layers.
        /// \return true if successfully loaded.
        virtual bool LoadFromStreamWithLayers(AZ::IO::GenericStream& stream, QString levelPakFile) = 0;

        /// Completely resets the context (slices and entities deleted).
        virtual void ResetEditorContext() = 0;

        /// Play-in-editor transitions.
        /// When starting, we deactivate the editor context and startup the game context.
        virtual void StartPlayInEditor() = 0;

        /// When stopping, we shut down the game context and re-activate the editor context.
        virtual void StopPlayInEditor() = 0;

        /// Is used to check whether the Editor is running the game simulation or in normal edit mode.
        virtual bool IsEditorRunningGame() = 0;

        /// Has the Editor been requested to move to Game Mode (but may not have fully entered it yet).
        virtual bool IsEditorRequestingGame() = 0;

        /// \return true if the entity is owned by the editor entity context.
        virtual bool IsEditorEntity(AZ::EntityId id) = 0;

        /// Adds the required editor components to the entity.
        virtual void AddRequiredComponents(AZ::Entity& entity) = 0;

        /// Returns an array of the required editor component types added by AddRequiredComponents()
        virtual const AZ::ComponentTypeList& GetRequiredComponentTypes() = 0;

        /// Maps an editor Id to a runtime entity Id. Relevant only during in-editor simulation.
        /// \param Id of editor entity
        /// \param destination parameter for runtime entity Id
        /// \return true if editor Id was found in the editor Id map
        virtual bool MapEditorIdToRuntimeId(const AZ::EntityId& editorId, AZ::EntityId& runtimeId) = 0;

        /// Maps an runtime Id to a editor entity Id. Relevant only during in-editor simulation.
        /// \param Id of runtime entity
        /// \param destination parameter for editor entity Id
        /// \return true if runtime Id was found in the Id map
        virtual bool MapRuntimeIdToEditorId(const AZ::EntityId& runtimeId, AZ::EntityId& editorId) = 0;
    };

    using EditorEntityContextRequestBus = AZ::EBus<EditorEntityContextRequests>;

    /**
     * Bus for receiving events/notifications from the editor entity context component.
     */
    class EditorEntityContextNotification
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityContextNotification() = default;

        /// Called before the context is reset.
        virtual void OnPrepareForContextReset() {}

        /// Fired after the context is reset.
        virtual void OnContextReset() {}

        //! Fired when an Editor entity is created
        virtual void OnEditorEntityCreated(const AZ::EntityId& /*entityId*/) {}

        //! Fired when an Editor entity is deleted
        virtual void OnEditorEntityDeleted(const AZ::EntityId& /*entityId*/) {}

        //Fired when an entity is duplicated. newEntity is the new duplicate of oldEntity
        virtual void OnEditorEntityDuplicated(const AZ::EntityId& /*oldEntity*/, const AZ::EntityId& /*newEntity*/) {}

        //! Fired when the editor begins going into 'Simulation' mode.
        virtual void OnStartPlayInEditorBegin() {}

        //! Fired when the editor finishes going into 'Simulation' mode.
        virtual void OnStartPlayInEditor() {}

        //! Fired when the editor begins coming out of 'Simulation' mode.
        virtual void OnStopPlayInEditorBegin() {}

        //! Fired when the editor comes out of 'Simulation' mode
        virtual void OnStopPlayInEditor() {}

        //! Fired when the entity stream begins loading.
        virtual void OnEntityStreamLoadBegin() {}

        //! Fired when the entity stream has been successfully loaded.
        virtual void OnEntityStreamLoadSuccess() {}

        //! Fired when the entity stream load has failed
        virtual void OnEntityStreamLoadFailed() {}

        //! Fired when the entities needs to be focused in Entity Outliner
        virtual void OnFocusInEntityOutliner(const EntityIdList& /*entityIdList*/) {}

        //! Fired before the EditorEntityContext exports the root level slice to the game stream
        virtual void OnSaveStreamForGameBegin(AZ::IO::GenericStream& /*gameStream*/, AZ::DataStream::StreamType /*streamType*/, AZStd::vector<AZStd::unique_ptr<AZ::Entity>>& /*levelEntities*/) {}

        //! Fired after the EditorEntityContext exports the root level slice to the game stream
        virtual void OnSaveStreamForGameSuccess(AZ::IO::GenericStream& /*gameStream*/) {}

        //! Fired after the EditorEntityContext fails to export the root level slice to the game stream
        virtual void OnSaveStreamForGameFailure(AZStd::string_view /*failureString*/) {}

        //! Preserve entity order when re-parenting entities
        virtual void SetForceAddEntitiesToBackFlag(bool /*forceAddToBack*/) {}

    };

    using EditorEntityContextNotificationBus = AZ::EBus<EditorEntityContextNotification>;

    //! Notification bus to notify Editor systems when the Legacy Editor
    //! has been requested to enter Game Mode.
    class EditorLegacyGameModeNotifications
        : public AZ::EBusTraits
    {
    public:
        //! The initial request has been made to start Game Mode.
        virtual void OnStartGameModeRequest() {}
        //! The initial request has been made to stop Game Mode.
        virtual void OnStopGameModeRequest() {}

    protected:
        ~EditorLegacyGameModeNotifications() = default;
    };

    using EditorLegacyGameModeNotificationBus = AZ::EBus<EditorLegacyGameModeNotifications>;
} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN(AzToolsFramework::EditorEntityContextRequests);
