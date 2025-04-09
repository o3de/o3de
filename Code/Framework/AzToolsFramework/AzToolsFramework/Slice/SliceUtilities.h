/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>

// Suppresses the following warnings
// warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
// warning C4251: 'QGradient::m_stops': class 'QVector<QGradientStop>' needs to have dll-interface to be used by clients of class 'QGradient'
// warning C4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QString>
#include <QSize>
#include <QWidget>
#include <QLabel>
#include <QDialog>
#include <QWidgetAction>
AZ_POP_DISABLE_WARNING

class QMenu;

namespace AzToolsFramework
{
    class InstanceDataNode;

    namespace SliceUtilities
    {
        using EntityAncestorPair = AZStd::pair<AZ::EntityId, AZStd::shared_ptr<AZ::Entity>>;
        using IdToEntityMapping = AZStd::unordered_map<AZ::EntityId, AZ::Entity*>;
        using IdToInstanceAddressMapping = AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::SliceInstanceAddress>;
        
        using WillPushEntityCallback = AZStd::function<bool(const AZ::EntityId, const AZ::Data::Asset <AZ::SliceAsset>&)> ;

        enum class QuickPushMenuOverrideDisplayCount
        {
            ShowOverrideCountWhenSingle,
            ShowOverrideCountOnlyWhenMultiple
        };

        /// Options that can be passed to PupulateQuickPushMenu to affect appearance.
        struct QuickPushMenuOptions
        {
            QuickPushMenuOptions() = default;
            QuickPushMenuOptions(const AZStd::string& headerText, QuickPushMenuOverrideDisplayCount singleOverrideDisplayOption) :
                m_headerText(headerText),
                m_singleOverrideDisplayOption(singleOverrideDisplayOption)
            {
            }

            AZStd::string m_headerText = "Save Slice Overrides";
            QuickPushMenuOverrideDisplayCount m_singleOverrideDisplayOption = QuickPushMenuOverrideDisplayCount::ShowOverrideCountWhenSingle;
        };

        /**
         * Utility function to gather all referenced entities given a set of entities.
         * This flood searches entity id references, so if you pass in Entity1 that references
         * Entity2, and Entity2 references Entity3, Entity3 is also visited and added to output as
         * a referenced entity.
         * \param entitiesWithReferences - input/output set containing all referenced entities.
         * \param serializeContext - serialize context to use to traverse reflected data.
         */
        void GatherAllReferencedEntities(AzToolsFramework::EntityIdSet& entitiesWithReferences,
                                         AZ::SerializeContext& serializeContext);

        /**
         * Utility function to gather all referenced entities given a set of entities and determine
         * if there are references not in original set.
         * This flood searches entity id references, so if you pass in Entity1 that references
         * Entity2, and Entity2 references Entity3, Entity3 is also visited and added to output as
         * a referenced entity.
         * \param entities - input set containing all original entities to search.
         * \param entitiesAndReferencedEntities - input/output set containing all referenced entities.
         * \param hasExternalReferences output true if there are referenced entities not in original entities set
         * \param serializeContext - serialize context to use to traverse reflected data.
         */
        void GatherAllReferencedEntitiesAndCompare(const AzToolsFramework::EntityIdSet& entities,
                                                   AzToolsFramework::EntityIdSet& entitiesAndReferencedEntities,
                                                   bool& hasExternalReferences,
                                                   AZ::SerializeContext& serializeContext);


        /**
         * Clones a slice-owned entity for the purpose of comparison against a live entity.
         * The key aspect of this utility is the resulting clone has Ids remapped to that of the instance,
         * so entity references don't appear as changes/deltas due to Id remapping during slice instantiation.
         */
        AZStd::unique_ptr<AZ::Entity> CloneSliceEntityForComparison(const AZ::Entity& sourceEntity,
                                                                    const AZ::SliceComponent::SliceInstance& instance,
                                                                    AZ::SerializeContext& serializeContext);

        /**
         * Determines if adding instanceToAdd slice to targetInstanceToAddTo slice is safe to do with regard
         * to whether it would create a cyclic asset dependency. Slices cannot contain themselves.
         * \param instanceToAdd - the slice instance wanting to be added
         * \param targetInstanceToAddTo - the slice instance wanting to have instanceToAdd added to
         * \return true if safe to add. false if the slice addition would create a cyclic asset dependency (invalid).
         */
        bool CheckSliceAdditionCyclicDependencySafe(const AZ::SliceComponent::SliceInstanceAddress& instanceToAdd,
                                                    const AZ::SliceComponent::SliceInstanceAddress& targetInstanceToAddTo);


        /**
         * Push a set of entities to a given slice asset.
         * It is assumed that all provided entities belong to an instance of the provided slice, otherwise AZ::Failure will be returned.
         * \param entityIdList list of live entity Ids whose overrides will be pushed to the slice. Live entities must belong to instances of the specified slice.
         * \param sliceAsset the target slice asset.
         * \param preSaveCallback the callback to use prior to the save to check that it is valid
         * \return AZ::Success if push is completed successfully, otherwise AZ::Failure with an AZStd::string payload.
         */
        AZ::Outcome<void, AZStd::string> PushEntitiesBackToSlice(const AzToolsFramework::EntityIdList& entityIdList,
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, SliceTransaction::PreSaveCallback preSaveCallback);

        /**
        * Push a set of entities including addition and subtraction to a given slice asset.
        * \param sliceAsset the target slice asset.
        * \param entitiesToUpdate list of entities whose overrides will be pushed to the slice.
        * \param entitiesToAdd list of entities to add to the slice.
        * \param entitiesToRemove list of entities to remove from the slice.
        * \param preSaveCallback the callback to use prior to the save to check that it is valid
        * \return AZ::Success if push is completed successfully, otherwise AZ::Failure with an AZStd::string payload.
        */
        AZ::Outcome<void, AZStd::string> PushEntitiesIncludingAdditionAndSubtractionBackToSlice(
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToUpdate,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToAdd,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToRemove,
            SliceTransaction::PreSaveCallback preSaveCallback,
            SliceTransaction::PostSaveCallback postSaveCallback);

        /**
         * Push an individual entity field back to a given slice asset.
         * It is assumed that the provided entity belongs to an instance of the provided slice, otherwise AZ::Failure will be returned.
         * \param entityId live entity Id whose instance of the provided field will be pushed back to the slice.
         * \param sliceAsset the target slice asset.
         * \param fieldNodeAddress InstanceDataHierarchy address of the field to be pushed back to the slice.
         * \param preSaveCallback the callback to use prior to the save to check that it is valid
         * \return AZ::Success if push is completed successfully, otherwise AZ::Failure with an AZStd::string payload.
         */
        AZ::Outcome<void, AZStd::string> PushEntityFieldBackToSlice(AZ::EntityId entityId,
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, const InstanceDataNode::Address& fieldAddress, SliceTransaction::PreSaveCallback preSaveCallback);

        /**
         * PreSaveCallback for SliceTransaction::Commits with default world entity rules.
         * Rules include verifying there's a single root entity, making sure root entities have 
         * no translation and clearing out cached world transforms of all entities.
         *
         * If needing custom logic for a PreSaveCallback for world entities, make sure to include this with your own callback:
         *
         * auto myPreSaveCallback = [&](SliceTransaction::TransactionPtr transaction, const char* fullPath, SliceTransaction::SliceAssetPtr& asset) -> SliceTransaction::Result {
         *    // <pre-default-checks custom code here>
         *    auto result = SliceUtilities::SlicePreSaveCallbackForWorldEntities(transaction, fullPath, asset);
         *    if (!result)
         *       return result;
         *    // <post-default-checks custom code here>
         *    return AZ::Success();
         * }
         */
        SliceTransaction::Result SlicePreSaveCallbackForWorldEntities(SliceTransaction::TransactionPtr transaction, const char* fullPath, SliceTransaction::SliceAssetPtr& asset);

        void SlicePostPushCallback(SliceTransaction::TransactionPtr transaction, const char* fullSourcePath, const SliceTransaction::SliceAssetPtr& asset);

        void SlicePostSaveCallbackForNewSlice(SliceTransaction::TransactionPtr transaction, const char* fullPath, const SliceTransaction::SliceAssetPtr& transactionAsset);

        /**
         * Returns true if the entity has no transform parent.
         */
        bool IsRootEntity(const AZ::Entity& entity);

        /**
        * \brief Determines whether the provided entity id is the root of a slice or subslice
        * \param id The entity id to check
        */
        bool IsSliceOrSubsliceRootEntity(const AZ::EntityId& id);

        /**
         * Retrieves the \ref AZ::Edit::Attributes::SliceFlags assigned to a given data node.
         * \param editData - The specific element data to check for slice flags (can be nullptr)
         * \param classData - The class data to check for slice flags (some flags can cascade from class to all elements, can also be nullptr)
         * return ref AZ::Edit::SliceFlags
         */
        AZ::u32 GetSliceFlags(const AZ::Edit::ElementData* editData, const AZ::Edit::ClassData* classData);

        /**
         * Retrieves the \ref AZ::Edit::Attributes::SliceFlags assigned to a given data node.
         * \param node - instance data hierarchy node
         * return ref AZ::Edit::SliceFlags
         */
        AZ::u32 GetNodeSliceFlags(const InstanceDataNode& node);

        /**
         * Returns true if the specified node is slice-pushable.
         * \param node instance data node to evaluate.
         * \param isRootEntity (optional) specifies whether the parent entity is a transform root.
         */
        bool IsNodePushable(const InstanceDataNode& node, bool isRootEntity = false);

        /**
         * Displays "Save As" QFileDialog to user, generating suggested full slice path based on
         * suggestedName and last place user saved a slice of this type to. Does error checking on
         * user-chosen name like making sure it's ASCII only/it's not taken by slices in other gems
         * \param suggestedName - initial suggested name of the slice
         * \param initialTargetDirectory - default directory to suggest saving in (gets overridden by user settings if present)
         * \param sliceUserSettingsId - CRC id for the SliceUserSettings for a given type of slice, used by "last saved in" logic
         * \param activeWindow - active QWidget for parenting dialog windows to
         * \param outSliceName - [out] name of the slice chosen by user/error checked
         * \param outSliceFilePath - [out] full directory/filename path chosen by user/error checked
         * \return true if valid name chosen, false if not or if cancelled by user
         */
        bool QueryUserForSliceFilename(const AZStd::string& suggestedName, 
                                       const char* initialTargetDirectory,
                                       AZ::u32 sliceUserSettingsId,
                                       QWidget* activeWindow, 
                                       AZStd::string& outSliceName, 
                                       AZStd::string& outSliceFilePath);

        /**
        * Get pushable new child entity Ids
        * \param entityIdList input entities
        * \param unpushableEntityIdsPerAsset [out] unpushable new child entity Ids for each potential ancestor
        * \param sliceAncestryMapping [out] mappings from the entity id to the slice ancestry to push to
        * \param newChildEntityIdAncestorPairs [out] Pairs of new child entity Id and the entity ancestor list
        * \param newEntityIds [out] Set of all added newly entityIds, whether pushable or not
        */
        AZStd::unordered_set<AZ::EntityId> GetPushableNewChildEntityIds(
            const AzToolsFramework::EntityIdList& entityIdList,
            AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>& unpushableEntityIdsPerAsset,
            AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>& sliceAncestryMapping,
            AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>>& newChildEntityIdAncestorPairs,
            EntityIdSet& newEntityIds);

        /**
        * Get unique removed entities
        * \param sliceInstances slice instances which contain select entities
        * \param assetEntityIdtoAssetEntityMappings [out] mappings from asset entity Id to asset entity
        * \param assetEntityIdtoInstanceAddressMappings [out] mappings from asset entity Id to instance address
        */
        AZStd::unordered_set<AZ::EntityId> GetUniqueRemovedEntities(
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sliceInstances,
            IdToEntityMapping& assetEntityIdtoAssetEntityMapping,
            IdToInstanceAddressMapping& assetEntityIdtoInstanceAddressMapping);

        /** 
         * Defines a callback for PopulateFindSliceMenu. This assists in bridging module boundaries. If the AssetBrowser isn't open when PopulateFindSliceMenu
         * is called, then it should be opened. This can only be done from a Sandbox module. PopulateFindSliceMenu is in AzToolsFramework and not Sandbox so it can
         * share logic with similar menus, like the quick push menu.
         */ 
        typedef void(*SliceSelectedCallback)();

        /**
        * Populates two QMenus: one with sub-menu to select slices associated to the passed in entity list in the asset browser and one with a list of parents of the current entity.
        * \param outerMenu The menu used as the parent for the go to slice menu.
        * \param selectedEntity The Entity to use to populate menus with. If this has more than one entry, no menus will be created.
        * \param sliceSelectedCallback Callback for when a slice is selected, run before the asset selection. This allows this functionality to bridge module
        *                              boundaries. SliceUtilities is in AzToolsFramework, but the AssetBrowser largely exists in Sandbox.
        */
        void PopulateSliceSubMenus(QMenu& outerMenu, const AzToolsFramework::EntityIdList& inputEntities, SliceSelectedCallback sliceSelectedCallback, SliceSelectedCallback sliceRelationshipViewCallback);

        /**
        * Creates and popluates a menu item associted with the passed in entity.
        * \param selectedEntity The Entity to use to populate the go to slice menu with.
        * \param ancestor slice root ancestor to associate with this item
        * \param menu pointer to menu to add this menu item to.
        * \param indentation amount of whitespace to place in front of this item.
        * \param icon image to display for this menu item.
        * \param tooltip text to display when hovering over item
        * \param sliceAssetId [out] sliceAssetId of the passed in ancestor.
        */
        QWidgetAction* MakeSliceMenuItem(const AZ::EntityId& selectedEntity, const AZ::SliceComponent::Ancestor& ancestor, QMenu* menu, int indentation, const QPixmap icon, QString tooltip, AZ::Data::AssetId& sliceAssetId);

        /**
         * Populates a QMenu with a sub-menu to select slices associated to the passed in entity list in the asset browser.
         * \param outerMenu The menu used as the parent for the go to slice menu.
         * \param selectedEntity The Entity to use to populate the go to slice menu with. 
         * \param sliceSelectedCallback Callback for when a slice is selected, run before the asset selection. This allows this functionality to bridge module
         *                              boundaries. SliceUtilities is in AzToolsFramework, but the AssetBrowser largely exists in Sandbox.
         */
        void PopulateFindSliceMenu(QMenu& outerMenu, const AZ::EntityId& selectedEntity, const AZ::SliceComponent::EntityAncestorList& ancestors, SliceSelectedCallback sliceSelectedCallback);

        /**
        * Populates a QMenu with a sub-menu to select slices associated to the passed in entity list in the slice relationship view.
        * \param outerMenu The menu used as the parent for the go to slice menu.
        * \param selectedEntity The Entity to use to populate the go to slice menu with.
        * \param sliceSelectedCallback Callback for when a slice is selected, run before the asset selection. This allows this functionality to bridge module
        *                              boundaries. SliceUtilities is in AzToolsFramework, but the AssetBrowser largely exists in Sandbox.
        */
        void PopulateSliceRelationshipViewMenu(QMenu& outerMenu, const AZ::EntityId& selectedEntity, const AZ::SliceComponent::EntityAncestorList& ancestors, SliceSelectedCallback sliceSelectedCallback);

        /**
         * Returns true if any of the given entities have slice overrides for their immediate slice ancestor (may be slow on a large group of entities).
         * \param inputEntities The entities to test. If any are not in a slice instance they are ignored.
         */
        bool DoEntitiesHaveOverrides(const AzToolsFramework::EntityIdList& inputEntities);

        /**
         * Check to see if a pending reparent request is non-trivial e.g. requires full or partial clones
         *
         * [A]                          | Clones should occur under the following scenarios:
         *  |                           | A ->   |   |   |   |   |   |   |   |   |   |   |   |
         *  |----<B>                    | B ->   |   |   |   |   | F | G | H | I | J | K |   |
         *  |     |                     | C ->   |   |   |   |   |   |   |   |   |   |   |   |
         *  |     |----[C]              | D ->   |   |   |   |   |   |   |   |   |   |   |   |
         *  |     |     |               | E ->   |   |   |   |   |   |   |   |   |   |   |   |
         *  |     |     |---- D         | F ->   |   | C | D | E |   |   |   |   |   | K |   |
         *  |     |                     | G -> A | B | C | D | E |   |   |   |   |   | K | L |
         *  |     |---- E               | H -> A | B | C | D | E | F |   |   |   | J | K | L |
         *  |                           | I -> A | B | C | D | E | F |   |   |   | J | K | L |
         *  |----{F}                    | J ->   |   |   |   |   |   |   |   |   |   |   |   |
         *  |     |                     | K ->   |   | C | D | E | F | G | H | I | J |   |   |
         *  |     |----{G}              | L ->   |   |   |   |   |   |   |   |   |   |   |   |
         *  |           |               |
         *  |           |----<H>        |
         *  |           |               |
         *  |           |----<I>        |
         *  |           |               |--------------------------------
         *  |           |---- J         | Legend: 
         *  |                           | [] - slice root
         *  |----{K}                    | {} - sub slice root
         *  |                           | <> - slice entity
         *  |---- L                     |    - loose entity
         *
         * \param entityId The target entity to reparent
         * \param newParentId The target parent entity
         */
        bool IsReparentNonTrivial(const AZ::EntityId& entityId, const AZ::EntityId& newParentId);

        /**
        * Reflects slice tools related structures for serialization/editing.
        */
        void Reflect(AZ::ReflectContext* context);

        /**
        * Gets the path to the icon used for representing slices.
        */
        QString GetSliceItemIconPath();

        /**
        * Gets the path to the icon used for representing modified slices.
        */
        QString GetSliceItemChangedIconPath();

        /**
        * Gets the path to the L shape icon.
        */
        QString GetLShapeIconPath();

        /**
        * Gets the path to the blue 3d box shape icon.
        */
        QString GetSliceEntityIconPath();

        /**
        * Gets the path to the warning icon.
        */
        QString GetWarningIconPath();

        /**
        * Gets the height to use for rows that represent slices.
        */
        AZ::u32 GetSliceItemHeight();

        /**
        * Gets the size to use for icons representing slices.
        */
        QSize GetSliceItemIconSize();

        /**
        * Gets the size of the L shape icon.
        */
        QSize GetLShapeIconSize();

        /**
        * Gets the size of the warning icon.
        */
        QSize GetWarningIconSize();

        /**
        * Gets the minimum width of the warning label.
        */
        int GetWarningLabelMinimumWidth();

        /**
        * Gets the depth to indent slices per level of that slice's hierarchy.
        */
        AZ::u32 GetSliceHierarchyMenuIdentationPerLevel();

        /**
        * Gets the font size to make the slice submenu look like the parent menu
        */
        int GetSliceSelectFontSize();

        /**
        * Structure for saving/retrieving user settings related to slice workflows.
        */
        class SliceUserSettings
            : public AZ::UserSettings
        {
        public:
            AZ_CLASS_ALLOCATOR(SliceUserSettings, AZ::SystemAllocator);
            AZ_RTTI(SliceUserSettings, "{56EC1A8F-1ADB-4CC7-A3C3-3F462750C31F}", AZ::UserSettings);

            AZStd::string m_saveLocation;
            bool m_autoNumber = false; //!< Should the name of the slice file be automatically numbered. e.g SliceName_001.slice vs SliceName.slice.

            SliceUserSettings() = default;

            static void Reflect(AZ::ReflectContext* context);
        };

        class DetachMenuActionWidget
            : public QWidget
        {
        public:
            DetachMenuActionWidget(QWidget* parent, const int& indentation, const AZStd::string& sliceAssetName, const bool& isLastAncestor);
            void enterEvent(QEvent* event) override;
            void leaveEvent(QEvent* event) override;

        private:
            void setsliceLabelTextColor(QString color);

            QLabel* m_sliceLabel;
            QLabel* m_toLabel;
        };

        class MoveToSliceLevelConfirmation
            : public QDialog
        {
        public:
            MoveToSliceLevelConfirmation(QWidget* parent, const AZStd::string& currentSlice, const AZStd::string& destinationSlice);
        };

        /// InvalidSliceReferencesWarningResult is used to contain the result
        /// on calls to DisplayInvalidSliceReferencesWarning.
        enum class InvalidSliceReferencesWarningResult
        {
            Save,   ///< The user has chosen continue with the save operation.
            Cancel, ///< The user has chosen to cancel the current operation.
            Details ///< The user has chosen to open the advanced slice push dialog.
        };

        /**
        * Displays a message warning the user that the push they are attempting will remove missing invalid references.
        * \param parent The widget to parent the window to.
        * \param invalidSliceCount The number of slices to be saved with invalid references.
        * \param invalidReferenceCount The number of references that will be cleared.
        * \param showDetailsButton True to show a details button to be used to open the advanced slice push window.
        */
        InvalidSliceReferencesWarningResult DisplayInvalidSliceReferencesWarning(
            QWidget* parent,
            size_t invalidSliceCount,
            size_t invalidReferenceCount,
            bool showDetailsButton);

        /**
        * Calculates the number of changes in a list of slices to be used in push/revert menu options
        * \param inputEntities List of entities to produce information for
        * \param entitiesToRemove [out] List of entities that have been removed
        * \param numRelevantEntitiesInSlices [out] Number of entities that have changes
        * \param pushableChangesPerAsset [out] List of the number of changes for each entitiy in list
        * \param sliceDisplayOrder [out] List of changes slices in display order
        * \param assetEntityAncestorMap [out] List of ancestors of affected entities
        * \param unpushableEntityIds [out] Set of unpushable entity Ids
        * \return True if there are changes
        */
        bool CountPushableChangesToSlice(const AzToolsFramework::EntityIdList& inputEntities,
            const InstanceDataNode::Address* fieldAddress,
            AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>& entitiesToAddPerAsset,
            AZStd::unordered_set<AZ::EntityId>& entitiesToRemove,
            size_t& numRelevantEntitiesInSlices,
            AZStd::unordered_map<AZ::Data::AssetId, int>& numPushableChangesPerAsset,
            AZStd::vector<AZ::Data::AssetId>& sliceDisplayOrder,
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<EntityAncestorPair>>& assetEntityAncestorMap,
            AZStd::unordered_map < AZ::Data::AssetId, AZStd::unordered_set<AZ::EntityId>>& pushableEntityIdsPerAsset,
            AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>&unpushableEntityIdsPerAsset);

        /**
        * Returns the file extension (including .) used for slices.
        */
        AZStd::string GetSliceFileExtension();

        //! Returns whether or not a given asset is a dynamic slice or not
        bool IsDynamic(const AZ::Data::AssetId& assetId);

        //! Toggles if a slice asset is dynamic and re-saves the slice
        void SetIsDynamic(const AZ::Data::AssetId& assetId, bool isDynamic);

        /**
        * Creates the right click context menu for slices in the asset browser.
        * \param menu The menu to parent this to.
        * \param fullFilePath The full file path of the slice.
        */
        void CreateSliceAssetContextMenu(QMenu* menu, const AZStd::string& fullFilePath);

        /**
        * Retrieves the desired save format for slices.
        * \return The slice save format.
        */
        AZ::DataStream::StreamType GetSliceStreamFormat();

        /**
        * Prunes child order array entries that won't be in a pushed slice.
        * \param originalOrderArray Child array with all entities in
        * \param prunedOrderArray [out] Child array after removing unneeded entities.
        * \param targetSlice Slice that the children are being pushed to.
        * \param willPushEntityCallback Callback routine that is called per entity in the array and returns true if the entity will be pushed.
        */
        void RemoveInvalidChildOrderArrayEntries(const AZStd::vector<AZ::EntityId>& originalOrderArray,
            AZStd::vector<AZ::EntityId>& prunedOrderArray,
            const AZ::Data::Asset<AZ::SliceAsset>& targetSlice,
            WillPushEntityCallback willPushEntityCallback);

        static constexpr const char* splitterColor = "black";
        static constexpr const char* detachMenuItemHoverColor = "#4285F4";
        static constexpr const char* detachMenuItemDefaultColor = "#ffffff";
        static constexpr const char* detailWidgetBackgroundColor = "#303030";
        static constexpr const char* unsavableChangesTextColor = "#ff3f3f";
        static constexpr const char* conflictedChangesTextColor = "red";

    } // namespace SliceUtilities

} // AzToolsFramework
