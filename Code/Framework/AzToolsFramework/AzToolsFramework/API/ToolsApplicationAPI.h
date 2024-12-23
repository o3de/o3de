/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <QObject>

namespace AZ
{
    class Entity;
    class Vector2;
} // namespace AZ

struct IEditor;
class QApplication;
class QDockWidget;
class QMainWindow;
class QMenu;
class QString;
class QWidget;

namespace AzToolsFramework
{
    struct ViewPaneOptions;
    class EntityPropertyEditor;

    namespace UndoSystem
    {
        class UndoStack;
        class URSequencePoint;
    }

    namespace AssetBrowser
    {
        class AssetSelectionModel;
    }

    using ClassDataList = AZStd::vector<const AZ::SerializeContext::ClassData*>;

    //! Return true to accept this type of component.
    using ComponentFilter = AZStd::function<bool(const AZ::SerializeContext::ClassData&)>;

    //! Controls how much to rebuild the property display when a change is made
    enum PropertyModificationRefreshLevel : int
    {
        Refresh_None,                   //! No refresh is required
        Refresh_Values,                 //! Repopulate the values from components into the UI
        Refresh_AttributesAndValues,    //! In addition to the above, also check if attributes such as visibility have changed
        Refresh_EntireTree,             //! Discard the entire UI and rebuild it from scratch
        Refresh_EntireTree_NewContent,  //! In addition to the above, scroll to the bottom of the view.
    };

    /**
     * Bus owned by the ToolsApplication. Listen for general ToolsApplication events.
     */
    class ToolsApplicationEvents
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<ToolsApplicationEvents>;

        /*!
         * Fired prior to committing a change in entity selection set.
         */
        virtual void BeforeEntitySelectionChanged() {}

        /*!
         * Fired after committing a change in entity selection set.
         * \param EntityIdList the list of newly selected entity Ids
         * \param EntityIdList the list of newly deselected entity Ids
         */
        virtual void AfterEntitySelectionChanged(const EntityIdList& /*newlySelectedEntities*/, const EntityIdList& /*newlyDeselectedEntities*/) {}

        /*!
        * Fired before committing a change in entity highlighting set.
        */
        virtual void BeforeEntityHighlightingChanged() {}

        /*!
        * Fired after committing a change in entity highlighting set.
        */
        virtual void AfterEntityHighlightingChanged() {}

        /*!
         * Fired when an entity's transform parent has changed.
         */
        virtual void EntityParentChanged(AZ::EntityId /*entityId*/, AZ::EntityId /*newParentId*/, AZ::EntityId /*oldParentId*/) {}

        /*!
         * Fired when a given entity has been unregistered from the application.
         * \param entityId - The Id of the subject entity.
         */
        virtual void EntityDeregistered(AZ::EntityId /*entity*/) {}

        /*!
         * Fired when a given entity has been registered with the application.
         * \param entityId - The Id of the subject entity.
         */
        virtual void EntityRegistered(AZ::EntityId /*entity*/) {}

        /*!
         * Broadcast when the user has created an entity as a child of another entity.
         * This event is broadcast after the entity has been created and activated and
         * all relevant transform component information has been set.
         * \param entityId - The Id of the new entity
         * \param parentId - The Id of the new entity's parent
         */
        virtual void EntityCreatedAsChild(AZ::EntityId /*entityId*/, AZ::EntityId /*parentId*/) {}

        /*!
         * Fired just prior to applying a requested undo or redo operation.
         */
        virtual void BeforeUndoRedo() {}

        /*!
         * Fired just after applying a requested undo or redo operation.
         * Note that prefab propagation will not have occurred at this point, so data may not yet be updated.
         * Consider listening to OnPrefabInstancePropagationEnd on PrefabPublicNotificationBus instead.
         */
        virtual void AfterUndoRedo() {}

        /*!
         * Fired when a new undo batch has been started.
         * \param label - description of the batch.
         */
        virtual void OnBeginUndo(const char* /*label*/) {}

        /*!
         * Fired when an undo batch has been ended..
         * \param label - description of the batch.
         * \param changed - did anything change during the batch
         */
        virtual void OnEndUndo(const char* /*label*/, bool /*changed*/) {}

        /*!
         * Notify property UI to refresh the property tree.  Note that this will go out to every
         * property UI control in every window in the entire application.
         * Use InvalidatePropertyDisplayForComponent() instead when possible for faster results.
         */
        virtual void InvalidatePropertyDisplay(PropertyModificationRefreshLevel /*level*/) {}

        /*!
         * Notify property UI to refresh the properties displayed for a specific component.
         * You should prefer to use this call over the above one, except in circumstances where
         * you need to refresh every UI element in every property tree in every window in the entire application.
         */
        virtual void InvalidatePropertyDisplayForComponent(AZ::EntityComponentIdPair /*entityComponentIdPair*/, PropertyModificationRefreshLevel /*level*/) {}

        /*!
         * Process source control status for the specified file.
         */
        virtual void GotSceneSourceControlStatus(SourceControlFileInfo& /*fileInfo*/) {}

        /*!
         * Process scene status.
         */
        virtual void PerformActionsBasedOnSceneStatus(bool /*sceneIsNew*/, bool /*readOnly*/) {}

        /*!
         * Highlight the specified asset in the asset browser.
         */
        virtual void ShowAssetInBrowser(AZStd::string /*assetName*/) {}

        /*!
         * Event sent when the editor is set to Isolation Mode where only selected entities are visible
         */
        virtual void OnEnterEditorIsolationMode() {}

        /*!
         * Event sent when the editor quits Isolation Mode
         */
        virtual void OnExitEditorIsolationMode() {}

        /*!
         * Sets  the position of the next entity to be instantiated, used by the EditorEntityModel when dragging from asset browser
         */
        virtual void SetEntityInstantiationPosition(const AZ::EntityId& /*parent*/, const AZ::EntityId& /*beforeEntity*/) {}

        /*!
        * Clears the position of the next entity to be instantiated, used by the EditorEntityModel if entity instantiation fails
        * after SetEntityInstantiationPosition is called. This makes sure entities created after the initial event don't end up with a parent out of
        * sync in the outliner and transform component.
        */
        virtual void ClearEntityInstantiationPosition() {}

        /*!
         * Called when the level is saved.
         */
        virtual void OnSaveLevel() {}
    };

    using ToolsApplicationNotificationBus = AZ::EBus<ToolsApplicationEvents>;

    /**
     * Bus used to make general requests to the ToolsApplication.
     */
    class ToolsApplicationRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<ToolsApplicationRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /*!
         * Handles pre-export tasks for an entity, such as generating runtime components on the target.
         */
        virtual void PreExportEntity(AZ::Entity& /*source*/, AZ::Entity& /*target*/) = 0;

        /*!
         * Handles post-export tasks for an entity.
         */
        virtual void PostExportEntity(AZ::Entity& /*source*/, AZ::Entity& /*target*/) = 0;

        /*!
         * Marks an entity as dirty.
         * \param target - The Id of the entity to mark as dirty.
         */
        virtual void AddDirtyEntity(AZ::EntityId target) = 0;

        /*!
        * Removes an entity from the dirty entity set.
        * \param target - The Id of the entity to remove
        * \return 1 if target EntityId was removed successfully, otherwise 0
        */
        virtual int RemoveDirtyEntity(AZ::EntityId target) = 0;

        /*!
         * Clears the dirty entity set.
         */
        virtual void ClearDirtyEntities() = 0;

        /*!
         * Marks an entity as ignored suppressing entity addition to the dirty entity set.
         * \param target - The Id of the entity to mark as ignored.
         */
        virtual void AddIgnoredEntity(AZ::EntityId target) = 0;

        /*!
         * Removes an entity from the ignored entity set.
         * \param target - The Id of the entity to remove
         * \return 1 if target EntityId was removed successfully, otherwise 0
         */
        virtual int RemoveIgnoredEntity(AZ::EntityId target) = 0;

        /*!
         * Clears the ignored entity set.
         */
        virtual void ClearIgnoredEntities() = 0;

        /*!
         * \return true if an undo/redo operation is in progress.
         */
        virtual bool IsDuringUndoRedo() = 0;

        /*!
         * Notifies the application the user intends to undo the last undo-able operation.
         */
        virtual void UndoPressed() = 0;

        /*!
         * Notifies the application the user intends to reapply the last redo-able operation.
         */
        virtual void RedoPressed() = 0;

        /*!
         * Notifies the application that the undo stack needs to be flushed
         */
        virtual void FlushUndo() = 0;

        /*!
        * Notifies the application that the redo stack needs to be sliced (removed)
        */
        virtual void FlushRedo() = 0;

        /*!
         * Notifies the application that the user intends to select an entity.
         * \param entityId - the Id of the newly selected entity.
         */
        virtual void MarkEntitySelected(AZ::EntityId entityId) = 0;

        /*!
         * Notifies the application that the user intends to select a list of entities.
         * This should be used any time multiple entities are selected, as this is 
         * a large performance improvement over calling MarkEntitySelected more than once.
         * \param entitiesToSelect - the vector of newly selected entities
         */
        virtual void MarkEntitiesSelected(const EntityIdList& entitiesToSelect) = 0;

        /*!
         * Notifies the application that the user intends to deselect an entity.
         * \param entityId - the Id of the now deselected entity.
         */
        virtual void MarkEntityDeselected(AZ::EntityId entityId) = 0;

        /*!
         * Notifies the application that the user intends to deselect a list of entities.
         * This should be used any time multiple entities are deselected, as this is
         * a large performance improvement over calling MarkEntityDeselected more than once.
         * \param entitiesToDeselect - the vector of newly deselected entities
         */
        virtual void MarkEntitiesDeselected(const EntityIdList& entitiesToDeselect) = 0;

        /*!
         * Notifies the application that editor has highlighted an entity, or removed
         * a highlight. This is used for mouse-hover behavior in Sandbox.
         */
        virtual void SetEntityHighlighted(AZ::EntityId entityId, bool highlighted) = 0;

        /*!
         * Starts a new undo batch.
         * \param label - description of the operation.
         * \return a handle for the new batch, which can be used with ResumeUndoBatch().
         */
        virtual UndoSystem::URSequencePoint* BeginUndoBatch(const char* label) = 0;

        /*!
         * Attempts to continue adding to an existing undo batch command.
         * If the specified batchId is on the top of the stack, it is used, otherwise a new
         * handle is returned.
         * \param batchId - the Id of the undo batch (returned from BeginUndoBatch).
         * \param label - description of the operation.
         * \return a handle for the batch.
         */
        virtual UndoSystem::URSequencePoint* ResumeUndoBatch(UndoSystem::URSequencePoint* batchId, const char* label) = 0;

        /*!
         * Completes the current undo batch.
         * It's still possible to resume the batch as long as it's still the most recent one.
         */
        virtual void EndUndoBatch() = 0;

        /*!
         * \return true if the entity (or entities) can be edited/modified.
         */
        virtual bool IsEntityEditable(AZ::EntityId entityId) = 0;
        virtual bool AreEntitiesEditable(const EntityIdList& entityIds) = 0;

        /*!
         * Notifies the tools application that the user wishes to checkout selected entities.
         */
        virtual void CheckoutPressed() = 0;

        /*!
         * Returns source control info for the current world/scene.
         * Not yet implemented in ToolsApplication.
         */
        virtual SourceControlFileInfo GetSceneSourceControlInfo() = 0;

        /*!
         * Returns true if any entities are selected, false if no entities are selected.
         */
        virtual bool AreAnyEntitiesSelected() = 0;

        /*!
         * Returns the number of selected entities.
         */
        virtual int GetSelectedEntitiesCount() = 0;

        /*!
         * Retrieves the set of selected entities.
         * \return a list of entity Ids.
         */
        virtual const EntityIdList& GetSelectedEntities() = 0;

        /*!
         * Retrieves the set of highlighted (but not selected) entities.
         * \return a list of entity Ids.
         */
        virtual const EntityIdList& GetHighlightedEntities() = 0;

        /*!
         * Explicitly specifies the set of selected entities.
         * \param a list of entity Ids.
         */
        virtual void SetSelectedEntities(const EntityIdList& selectedEntities) = 0;

        /*!
        * Functionality removed, function call left in to prevent compile issues if anybody's using it.
        * \param entityId
        */
        virtual bool IsSelectable(const AZ::EntityId& entityId) = 0;

        /*!
         * Returns true if the specified entity is currently selected.
         * \param entityId
         */
        virtual bool IsSelected(const AZ::EntityId& entityId) = 0;

        /*!
         * Returns true if the specified entity is a slice root.
         * \param entityId
         */
        virtual bool IsSliceRootEntity(const AZ::EntityId& entityId) = 0;

        /*!
         * Retrieves the undo stack.
         * \return a pointer to the undo stack.
         */
        virtual UndoSystem::UndoStack* GetUndoStack() = 0;

        /*!
         * Retrieves the current undo batch.
         * \returns a pointer to the top of the undo stack.
         */
        virtual UndoSystem::URSequencePoint* GetCurrentUndoBatch() = 0;

        /*!
         * Given a list of input entity Ids, gather their children and all descendants as well.
         * \param inputEntities list of entities whose children should be gathered.
         * \return set of entity Ids including input entities and their descendants.
         */
        virtual EntityIdSet GatherEntitiesAndAllDescendents(const EntityIdList& inputEntities) = 0;

        //! Create a new entity at a default position
        virtual AZ::EntityId CreateNewEntity(AZ::EntityId parentId = AZ::EntityId()) = 0;

        //! Create a new entity at a specified position
        virtual AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& pos, AZ::EntityId parentId = AZ::EntityId()) = 0;

        //! Gets an existing entity id from a known id.
        virtual AZ::EntityId GetExistingEntity(AZ::u64 id) = 0;

        //! Returns if an entity with the given id exists
        virtual bool EntityExists(AZ::EntityId id) = 0;

        /*!
         * Delete all currently-selected entities.
         */
        virtual void DeleteSelected() = 0;

        /*!
         * Deletes the specified entity.
         */
        virtual void DeleteEntityById(AZ::EntityId entityId) = 0;

        /*!
         * Deletes all specified entities.
         */
        virtual void DeleteEntities(const EntityIdList& entities) = 0;

        /*!
        * Deletes the specified entity, as well as any transform descendants.
        */
        virtual void DeleteEntityAndAllDescendants(AZ::EntityId entityId) = 0;

        /*!
        * Deletes all entities in the provided list, as well as their transform descendants.
        */
        virtual void DeleteEntitiesAndAllDescendants(const EntityIdList& entities) = 0;

        /*!
        * \brief Finds the Common root of an entity list; Also finds the top level entities in a given list of active entities (who share the common root)
        * Example : A(B[D,E{F}],C),G (Letter is entity name, braces hold children)
        *           Sample run | entitiesToBeChecked:(B,D,E,F,C)
        *                           commonRootEntityId: <A> , topLevelEntities: <B,C>, return : <true>
        *           Sample run | entitiesToBeChecked:(E,C)
        *                           commonRootEntityId:<InvalidEntityId> , topLevelEntities: <E,C>, return : <false>
        *           Sample run | entitiesToBeChecked:(A,G,B,E,C)
        *                          commonRootEntityId:<InvalidEntityId> , topLevelEntities: <A,G>, return : <true> (True because both of the top level entities have no parent , which for us is the common parent)
        *           Sample run | entitiesToBeChecked:(A,D)
        *                          commonRootEntityId:<InvalidEntityId> , topLevelEntities: <A,D>, return : <false>
        * \param entitiesToBeChecked List of entities whose parentage is to be found
        * \param commonRootEntityId [Out] Entity id of the common root for the entitiesToBeChecked
        * \param topLevelEntities [Out] List of entities at the top of the hierarchy in entitiesToBeChecked
        * \return boolean value indicating whether entities have a common root or not
        *           IF True commonRootEntityId is the common root of all rootLevelEntities
        *           IF False commonRootEntityId is an invalid entity id
        * NOTE: Requires that the entities to be checked are live, they must be active and available via TransformBus.
        *       \ref entitiesToBeChecked cannot contain nested entities with gaps, see Sample run 4
        */
        virtual bool FindCommonRoot(const AzToolsFramework::EntityIdSet& entitiesToBeChecked, AZ::EntityId& commonRootEntityId
            , AzToolsFramework::EntityIdList* topLevelEntities = nullptr) = 0;

        /**
        * \brief Finds the Common root of an entity list; Also finds the top level entities in a given list of inactive entities (who share the common root)
        * Example : A(B[D,E{F}],C),G (Letter is entity name, braces hold children)
        *           Sample run | entitiesToBeChecked:(B,D,E,F,C)
        *                           commonRootEntityId: <A> , topLevelEntities: <B,C>, return : <true>
        *           Sample run | entitiesToBeChecked:(E,C)
        *                           commonRootEntityId:<InvalidEntityId> , topLevelEntities: <E,C>, return : <false>
        *           Sample run | entitiesToBeChecked:(A,G,B,E,C)
        *                          commonRootEntityId:<InvalidEntityId> , topLevelEntities: <A,G>, return : <true> (True because both of the top level entities have no parent , which for us is the common parent)
        *           Sample run | entitiesToBeChecked:(A,D)
        *                          commonRootEntityId:<InvalidEntityId> , topLevelEntities: <A,D>, return : <false>
        * \param entitiesToBeChecked List of entities whose parentage is to be found
        * \param commonRootEntityId [Out] Entity id of the common root for the entitiesToBeChecked
        * \param topLevelEntities [Out] List of entities at the top of the hierarchy in entitiesToBeChecked
        * \return boolean value indicating whether entities have a common root or not
        *           IF True commonRootEntityId is the common root of all rootLevelEntities
        *           IF False commonRootEntityId is an invalid entity id
        * NOTE: Does not require that the entities to be checked are live, they could be temp or asset entities.
        *       \ref entitiesToBeChecked cannot contain nested entities with gaps, see Sample run 4
        */
        virtual bool FindCommonRootInactive(const AzToolsFramework::EntityList& entitiesToBeChecked, AZ::EntityId& commonRootEntityId, AzToolsFramework::EntityList* topLevelEntities = nullptr) = 0;

        /**
         * Find all top level entities in the transform hierarchy of a list of entities, whether they are active or not.
         * Different from the function FindCommonRootInactive, this function returns all top level entities even if \ref entityIdsToCheck contains gaps in its transform hierarchy, at the cost of performance.
         * @param entityIdsToCheck A list of entity ids.
         * @param[out] A list of entity ids of top level entities.
         */
        virtual void FindTopLevelEntityIdsInactive(const EntityIdList& entityIdsToCheck, EntityIdList& topLevelEntityIds) = 0;

        /**
         * Check every entity to see if they all belong to the same slice instance, if so return that slice instance address, otherwise return null.
         * @param entityIds An group of EntityIds.
         * @return The slice instance address if all of \ref entityIds belongs to the same slice instance, otherwise null.
         */
        virtual AZ::SliceComponent::SliceInstanceAddress FindCommonSliceInstanceAddress(const EntityIdList& entityIds) = 0;

        /**
         * Get the id of the root entity of a slice instance. 
         * This function ignores any unpushed change made to the transform hierarchy of the entities in the slice instance in question.
         * @param sliceAddress The address of a slice instance.
         * @return The root entity id.
         */
        virtual AZ::EntityId GetRootEntityIdOfSliceInstance(AZ::SliceComponent::SliceInstanceAddress sliceAddress) = 0;

        /**
         * Get the id of the level that is loaded currently in the editor.
         * This is a "singleton" type of EntityId that represents the current level.
         * It can be used to add level components to it.
         */
        virtual AZ::EntityId GetCurrentLevelEntityId() = 0;

        /**
        * Prepares a file for editability. Interacts with source-control if the asset is not already writable, in a blocking fashion.
        * \param path full path of the asset to be made editable.
        * \param progressMessage progress message to display during checkout operation.
        * \param progressCallback user callback for retrieving progress information, provide RequestEditProgressCallback() if no progress reporting is required.
        * \return boolean value indicating if the file is writable after the operation.
        */
        using RequestEditProgressCallback = AZStd::function<void(int& current, int& max)>;
        virtual bool RequestEditForFileBlocking(const char* assetPath, const char* progressMessage, const RequestEditProgressCallback& progressCallback) = 0;

        /**
        * Same as RequestEditForBlocking, but intentionally fails operation when source control is offline
        * We add this function as convenience to side step the behavior of removing write protection when LocalFileSCComponent is used
        * \param path full path of the asset to be made editable.
        * \param progressMessage progress message to display during checkout operation.
        * \param progressCallback user callback for retrieving progress information, provide RequestEditProgressCallback() if no progress reporting is required.
        * \return boolean value indicating if the file is writable after the operation.
        */
        virtual bool CheckSourceControlConnectionAndRequestEditForFileBlocking(const char* assetPath, const char* progressMessage, const RequestEditProgressCallback& progressCallback) = 0;

        /**
        * Prepares a file for editability. Interacts with source-control if the asset is not already writable.
        * \param path full path of the asset to be made editable.
        * \param resultCallback user callback to be notified when source control operation is complete. Callback will be invoked with a true success value if the file was made writable.
        *        If the file is already writable at the time the function is called, resultCallback(true) will be invoked immediately.
        */
        using RequestEditResultCallback = AZStd::function<void(bool success)>;
        virtual void RequestEditForFile(const char* assetPath, RequestEditResultCallback resultCallback) = 0;

        /**
        * Same as RequestEditForFile, but intentionally fails operation when source control is offline
        * We add this function as convenience to side step the behavior of removing write protection when LocalFileSCComponent is used
        * \param path full path of the asset to be made editable.
        * \param resultCallback user callback to be notified when source control operation is complete. Callback will be invoked with a true success value if the file was made writable.
        *        If the file is already writable at the time the function is called, resultCallback(true) will be invoked immediately.
        */
        virtual void CheckSourceControlConnectionAndRequestEditForFile(const char* assetPath, RequestEditResultCallback resultCallback) = 0;

        /*!
         * Enter the Isolation Mode and hide entities that are not selected.
         */
        virtual void EnterEditorIsolationMode() = 0;

        /*!
         * Exit the Isolation Mode and stop hiding entities.
         */
        virtual void ExitEditorIsolationMode() = 0;

        /*!
         * Request if the editor is currently in Isolation Mode
         * /return boolean indicating if the editor is currently in Isolation Mode
         */
        virtual bool IsEditorInIsolationMode() = 0;

        /**
        * Creates and adds a new entity to the tools application from components which match at least one of the requiredTags
        * The tag matching occurs on AZ::Edit::SystemComponentTags attribute from the reflected class data in the serialization context
        */
        virtual void CreateAndAddEntityFromComponentTags(const AZStd::vector<AZ::Crc32>& requiredTags, const char* entityName) = 0;

        /**
        * Attempts to resolve a path to an executable using the current executable's folder
        */
        using ResolveToolPathOutcome = AZ::Outcome<AZStd::string, AZStd::string>;
        virtual ResolveToolPathOutcome ResolveConfigToolsPath(const char* toolApplicationName) const = 0;

        /**
         * Open 3D Engine Internal use only.
         *
         * Run a specific redo command separate from the undo/redo system.
         * In many cases before a modification on an entity takes place, it is first packaged into 
         * undo/redo commands. Running the modification's redo command separate from the undo/redo 
         * system simulates its execution, and avoids some code duplication.
         */
        virtual void RunRedoSeparately(UndoSystem::URSequencePoint* redoCommand) = 0;
    };

    using ToolsApplicationRequestBus = AZ::EBus<ToolsApplicationRequests>;

    /**
     * Bus keyed on entity Id for selection events.
     * Note that upon connection OnSelected() may be immediately invoked.
     */
    class EntitySelectionEvents
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides

        /**
         * Custom connection policy notifies handler if entity is already selected.
         */
        template<class Bus>
        struct SelectionConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);
                EntityIdList selectedEntities;
                ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities,
                    &ToolsApplicationRequests::Bus::Events::GetSelectedEntities);
                if (AZStd::find(selectedEntities.begin(), selectedEntities.end(), id) != selectedEntities.end())
                {
                    handler->OnSelected();
                }
            }
        };
        template<typename Bus>
        using ConnectionPolicy = SelectionConnectionPolicy<Bus>;

        using Bus = AZ::EBus<EntitySelectionEvents>;
        ////////////////////////////////////////////////////////////////////////

        virtual void OnSelected() {}
        virtual void OnDeselected() {}
    };

    /**
     * Bus for editor requests related to Pick Mode.
     */
    class EditorPickModeRequests
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorPickModeRequests>;

        // EBusTraits overrides
        using BusIdType = AzFramework::EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /**
         * Move the Editor out of Pick Mode.
         * Note: The Editor is moved into Pick Mode by a button in the Entity Inspector UI.
         */
        virtual void StopEntityPickMode() = 0;
        /**
         * When in Pick Mode, set the picked entity to the assigned slot(s).
         * @note It is only valid to make this request when the editor is in Pick Mode.
         */
        virtual void PickModeSelectEntity(AZ::EntityId entityId) = 0;
    
    protected:
        ~EditorPickModeRequests() = default;
    };

    /// Type to inherit to implement EditorPickModeRequests
    using EditorPickModeRequestBus = AZ::EBus<EditorPickModeRequests>;

    /**
     * Bus for editor notifications related to Pick Mode.
     */
    class EditorPickModeNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        using BusIdType = AzFramework::EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        /**
         * Notify other systems that the editor has entered Pick Mode select.
         */
        virtual void OnEntityPickModeStarted() {}
        /**
         * Notify other systems that the editor has left Pick Mode select.
         */
        virtual void OnEntityPickModeStopped() {}

    protected:
        ~EditorPickModeNotifications() = default;
    };

    /// Type to inherit to implement EditorPickModeNotifications
    using EditorPickModeNotificationBus = AZ::EBus<EditorPickModeNotifications>;

    /**
     * Bus for general editor requests to be intercepted by the application (e.g. Sandbox).
     */
    class EditorRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<EditorRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides

        // PLEASE PLEASE PLEASE don't change this to multiple unless you change all of the
        // calls to this E-BUS that expect a returned value to handle multiple
        // buses listening.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Registers a view pane (generally a QMainWindow-derived class) with the main editor.
        /// It's easier to use AzToolsFramework::RegisterViewPane, as it does not require a widget creation function to be supplied.
        /// \param name - display name for the pane. Will appear in the window header bar, as well as the context menu.
        /// \param category - category under the Tools menu that will contain the option to open the pane.
        /// \param viewOptions - structure defining various UI options for the pane.
        /// \param widgetCreationFunc - function callback for constructing the pane.
        typedef AZStd::function<QWidget*(QWidget*)> WidgetCreationFunc;
        virtual void RegisterViewPane(const char* /*name*/, const char* /*category*/, const ViewPaneOptions& /*viewOptions*/, const WidgetCreationFunc& /*widgetCreationFunc*/) {};

        //! Similar to EditorRequests::RegisterViewPane, although instead of specifying a widget creation function, the user
        //! must connect to the ViewPaneCallbacks bus and respond to the CreateViewPaneWidget event that is called when
        //! the view pane needs to be constructed.
        virtual void RegisterCustomViewPane(const char* /*name*/, const char* /*category*/, const ViewPaneOptions& /*viewOptions*/) {};

        /// Unregisters a view pane by name from the main editor.
        /// \param name - the name of the pane to be unregistered. This must match the name used for registration.
        virtual void UnregisterViewPane(const char* /*name*/) {};

        /// Returns the widget contained/wrapped in a view pane.
        /// \param viewPaneName - the name of the pane which contains the widget to be retrieved. This must match the name used for registration.
        virtual QWidget* GetViewPaneWidget(const char* /*viewPaneName*/) { return nullptr; }

        /// Show an Editor window by name.
        void ShowViewPane(const char* paneName) { OpenViewPane(paneName); }

        /// Opens an Editor window by name. Shows it if it was previously hidden, and activates it even if it's already visible.
        virtual void OpenViewPane(const char* /*paneName*/) {}

        /// Opens a new instance of an Editor window by name and returns the dock widget container
        virtual QDockWidget* InstanceViewPane(const char* /*paneName*/) { return nullptr; }

        /// Closes an Editor window by name.
        /// If the view pane was registered with the ViewPaneOptions.isDeletable set to true (the default), this will delete the view pane, if it was open.
        /// If the view pane was not registered with the ViewPaneOptions.isDeletable set to true, the view pane will be hidden instead.
        virtual void CloseViewPane(const char* /*paneName*/) {}

        //! Spawn asset browser for the appropriate asset types.
        virtual void BrowseForAssets(AssetBrowser::AssetSelectionModel& /*selection*/) = 0;

        /// Adds the components that are required for editor representation to the entity.
        virtual void CreateEditorRepresentation(AZ::Entity* /*entity*/) {}

        /// Clone selected entities/slices.
        virtual void CloneSelection(bool& /*handled*/) {}

        /// Delete selected entities/slices
        virtual void DeleteSelectedEntities(bool /*includeDescendants*/) {}

        /// Create a new entity at a default position
        virtual AZ::EntityId CreateNewEntity(AZ::EntityId parentId = AZ::EntityId()) { (void)parentId; return AZ::EntityId(); }

        /// Create a new entity as a child of an existing entity - Intended only to handle explicit requests from the user
        virtual AZ::EntityId CreateNewEntityAsChild(AZ::EntityId /*parentId*/) { return AZ::EntityId(); }

        /// Create a new entity at a specified position
        virtual AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& /*pos*/, AZ::EntityId parentId = AZ::EntityId()) { (void)parentId; return AZ::EntityId(); }

        /// Gets and existing EntityId from a known id passed ad a u64
        virtual AZ::EntityId GetExistingEntity(AZ::u64 id) {return AZ::EntityId{id}; }

        virtual AzFramework::EntityContextId GetEntityContextId() { return AzFramework::EntityContextId::CreateNull(); }

        /// Retrieve the main application window.
        virtual QWidget* GetMainWindow() { return nullptr; }

        /// Retrieve main editor interface.
        virtual IEditor* GetEditor() { return nullptr; }

        virtual bool GetUndoSliceOverrideSaveValue() { return false; }

        /// Retrieve the setting for messaging
        virtual bool GetShowCircularDependencyError() { return true; }

        /// Hide or show the circular dependency error when saving slices
        virtual void SetShowCircularDependencyError(const bool& /*showCircularDependencyError*/) {}

        /// Launches the Lua editor and opens the specified (space separated) files.
        virtual void LaunchLuaEditor(const char* /*files*/) {}

        /// Returns whether a level document is open.
        virtual bool IsLevelDocumentOpen() { return false; }

        /// Return the name of a level document.
        virtual AZStd::string GetLevelName() { return AZStd::string(); }

        /// Return default icon to show in the viewport for components that haven't specified an icon.
        virtual AZStd::string GetDefaultComponentViewportIcon() { return AZStd::string(); }

        /// Return default icon to show in the palette, etc for components that haven't specified an icon.
        virtual AZStd::string GetDefaultComponentEditorIcon() { return AZStd::string(); }

        /// Return default entity icon to show both in viewport and entity-inspector.
        virtual AZStd::string GetDefaultEntityIcon() { return AZStd::string(); }

        /// Return path to icon for component.
        /// Path will be empty if component should have no icon.
        virtual AZStd::string GetComponentEditorIcon(const AZ::Uuid& /*componentType*/, const AZ::Component* /*component*/) { return AZStd::string(); }

        //! Return path to icon for component type.
        //! Path will be empty if component type should have no icon.
        virtual AZStd::string GetComponentTypeEditorIcon(const AZ::Uuid& /*componentType*/) { return AZStd::string(); }

        /**
         * Return the icon image path based on the component type and where it is used.
         * \param componentType         component type
         * \param componentIconAttrib   edit attribute describing where the icon is used, it could be one of Icon, Viewport and HidenIcon
         * \return the path of the icon image
         */
        virtual AZStd::string GetComponentIconPath(const AZ::Uuid& /*componentType*/, AZ::Crc32 /*componentIconAttrib*/, const AZ::Component* /*component*/) { return AZStd::string(); }

        /**
         * Calculate the navigation 2D radius in units of an agent given its Navigation Type Name
         * @param angentTypeName         the name that identifies the agent navigation type
         * @return the 2D horizontal radius of the agent, -1 if not found
         */
        virtual float CalculateAgentNavigationRadius(const char* /*angentTypeName*/) { return -1; }

        /**
         * Retrieve the default agent Navigation Type Name
         * @return the string identifying an agent navigation type
         */
        virtual const char* GetDefaultAgentNavigationTypeName() { return ""; }
        
        virtual void OpenPinnedInspector(const AzToolsFramework::EntityIdSet& /*entities*/) { }

        virtual void ClosePinnedInspector(AzToolsFramework::EntityPropertyEditor* /*editor*/) {}

        /// Return all available agent types defined in the Navigation xml file.
        virtual AZStd::vector<AZStd::string> GetAgentTypes() { return AZStd::vector<AZStd::string>(); }

        /// Focus all viewports on the selected and highlighted entities
        virtual void GoToSelectedOrHighlightedEntitiesInViewports() { }

        /// Focus all viewports on the selected entities
        virtual void GoToSelectedEntitiesInViewports() { }

        /// Returns true if the selected entities can be moved to, and false if not.
        virtual bool CanGoToSelectedEntitiesInViewports() { return true; }

        /// Returns the world-space position under the center of the render viewport.
        virtual AZ::Vector3 GetWorldPositionAtViewportCenter() { return AZ::Vector3::CreateZero(); }

        //! Retrieves the position in world space corresponding to the point interacted with by the user.
        //! Will take context menus and cursor position into account as appropriate.
        virtual AZ::Vector3 GetWorldPositionAtViewportInteraction() const { return AZ::Vector3::CreateZero(); };

        /// Clears current redo stack
        virtual void ClearRedoStack() {}
    };

    using EditorRequestBus = AZ::EBus<EditorRequests>;

    /**
     * Bus for general editor events.
     */
    class EditorEvents
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorEvents>;

        virtual void OnEscape() {}

        /// The editor has changed performance specs.
        virtual void OnEditorSpecChange() {}

        enum EditorContextMenuFlags
        {
            eECMF_NONE = 0,
            eECMF_HIDE_ENTITY_CREATION = 0x1,
            eECMF_USE_VIEWPORT_CENTER = 0x2,
        };

        /// Populate slice portion of edit-time context menu
        virtual void PopulateEditorGlobalContextMenu_SliceSection(QMenu * /*menu*/, const AZ::Vector2& /*point*/, int /*flags*/) {}

        /// Anything can override this and return true to skip over the WelcomeScreenDialog
        virtual bool SkipEditorStartupUI() { return false; }

        /// Notify that it's ok to register views
        virtual void NotifyRegisterViews() {}

        /// Notify that central widget has been initialized
        virtual void NotifyCentralWidgetInitialized() {}

        /// Notify that the Qt Application object is now ready to be used
        virtual void NotifyQtApplicationAvailable(QApplication* /* application */) {}

        /// Notify that the IEditor is ready
        virtual void NotifyIEditorAvailable(IEditor* /*editor*/) {}

        /// Notify that the MainWindow has been fully initialized
        virtual void NotifyMainWindowInitialized(QMainWindow* /*mainWindow*/) {}

        /// Notify that the Editor has been fully initialized
        virtual void NotifyEditorInitialized() {}

        /// Signal that an asset should be highlighted / selected
        virtual void SelectAsset(const QString& /* assetPath */) {}

        // Notify that a viewpane has just been opened.
        virtual void OnViewPaneOpened(const char* /*viewPaneName*/) {}

        // Notify that a viewpane has just been closed.
        virtual void OnViewPaneClosed(const char* /*viewPaneName*/) {}
    };

    using EditorEventsBus = AZ::EBus<EditorEvents>;

    class ViewPaneCallbacks
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZStd::string;
        //////////////////////////////////////////////////////////////////////////

        //! Return the window ID of the created view pane widget
        virtual AZ::u64 CreateViewPaneWidget() { return AZ::u64(); }
    };

    using ViewPaneCallbackBus = AZ::EBus<ViewPaneCallbacks>;

    /**
     * RAII Helper class for undo batches.
     *
     * AzToolsFramework::ScopedUndoBatch undoBatch("Batch Name");
     * entity->ChangeData(...);
     * undoBatch.MarkEntityDirty(entity->GetId());
     */
    class ScopedUndoBatch
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedUndoBatch, AZ::SystemAllocator);
        explicit ScopedUndoBatch(const char* batchName);
        ~ScopedUndoBatch();

        // utility/convenience function for adding dirty entity
        static void MarkEntityDirty(const AZ::EntityId& id);
        UndoSystem::URSequencePoint* GetUndoBatch() const;

    private:
        AZ_DISABLE_COPY_MOVE(ScopedUndoBatch);

        UndoSystem::URSequencePoint* m_undoBatch = nullptr;
    };

    /// Registers a view pane with the main editor. It will be listed under the "Tools" menu on the main window's menubar.
    /// Note that if a view pane is registered with it's ViewPaneOptions.isDeletable set to true, the widget will be deallocated and destructed on close.
    /// Otherwise, it will be hidden instead.
    /// If you'd like to be able to veto the close (for instance, if the user has unsaved data), override the closeEvent() on your custom
    /// viewPane widget and call ignore() on the QCloseEvent* parameter.
    ///
    /// \param viewPaneName - name for the pane. This is what will appear in the dock window's title bar, as well as in the main editor window's menubar, if the optionalMenuText is not set in the viewOptions parameter.
    /// \param category - category under the "Tools" menu that will contain the option to open the newly registered pane.
    /// \param viewOptions - structure defining various options for the pane.
    template <typename TWidget>
    inline void RegisterViewPane(const char* name, const char* category, const ViewPaneOptions& viewOptions)
    {
        AZStd::function<QWidget*(QWidget*)> windowCreationFunc = [](QWidget* parent = nullptr)
        {
            return new TWidget(parent);
        };

        EditorRequests::Bus::Broadcast(&EditorRequests::RegisterViewPane, name, category, viewOptions, windowCreationFunc);
    }

    /// Registers a view pane with the main editor. It will be listed under the "Tools" menu on the main window's menubar.
    /// This variant is most useful when dealing with singleton view widgets.
    /// Note that if the new view is a singleton, and shouldn't be destroyed by the view pane manager, viewOptions.isDeletable must be set to false.
    /// Note that if a view pane is registered with it's ViewPaneOptions.isDeletable set to true, the widget will be deallocated and destructed on close.
    /// Otherwise, it will be hidden instead.
    /// If you'd like to be able to veto the close (for instance, if the user has unsaved data), override the closeEvent() on your custom
    /// viewPane widget and call ignore() on the QCloseEvent* parameter.
    ///
    /// \param viewPaneName - name for the pane. This is what will appear in the dock window's title bar, as well as in the main editor window's menubar, if the optionalMenuText is not set in the viewOptions parameter.
    /// \param category - category under the "Tools" menu that will contain the option to open the newly registered pane.
    /// \param viewOptions - structure defining various options for the pane.
    template <typename TWidget>
    inline void RegisterViewPane(const char* viewPaneName, const char* category, const ViewPaneOptions& viewOptions, AZStd::function<QWidget*(QWidget*)> windowCreationFunc)
    {
        EditorRequests::Bus::Broadcast(&EditorRequests::RegisterViewPane, viewPaneName, category, viewOptions, windowCreationFunc);
    }

    /// Unregisters a view pane with the main editor. It will no longer be listed under the "Tools" menu on the main window's menubar.
    /// Any currently open view panes of this type will be closed before the view pane handlers are unregistered.
    ///
    /// \param viewPaneName - name of the pane to unregister. Must be the same as the name previously registered with RegisterViewPane.
    void UnregisterViewPane(const char* viewPaneName);

    /// Returns the widget contained/wrapped in a view pane.
    /// \param name - the name of the pane which contains the widget to be retrieved. This must match the name used for registration.
    template <typename TWidget>
    inline TWidget* GetViewPaneWidget(const char* viewPaneName)
    {
        QWidget* ret = nullptr;
        EditorRequests::Bus::BroadcastResult(ret, &EditorRequests::GetViewPaneWidget, viewPaneName);

        return qobject_cast<TWidget*>(ret);
    }

    /// Opens a view pane if not already open, and activating the view pane if it was already opened.
    ///
    /// \param viewPaneName - name of the pane to open/activate. Must be the same as the name previously registered with RegisterViewPane.
    void OpenViewPane(const char* viewPaneName);

    /// Opens a view pane if not already open, and activating the view pane if it was already opened.
    ///
    /// \param viewPaneName - name of the pane to open/activate. Must be the same as the name previously registered with RegisterViewPane.
    QDockWidget* InstanceViewPane(const char* viewPaneName);

    /// Closes a view pane if it is currently open.
    ///
    /// \param viewPaneName - name of the pane to open/activate. Must be the same as the name previously registered with RegisterViewPane.
    void CloseViewPane(const char* viewPaneName);

    /**
     * Helper to wrap checking if an undo/redo operation is in progress.
     */
    bool UndoRedoOperationInProgress();
} // namespace AzToolsFramework

AZ_DECLARE_BUDGET(AzToolsFramework);
DECLARE_EBUS_EXTERN(AzToolsFramework::EditorRequests);
DECLARE_EBUS_EXTERN(AzToolsFramework::ToolsApplicationEvents);
DECLARE_EBUS_EXTERN(AzToolsFramework::EntitySelectionEvents);
