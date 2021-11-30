/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidget>
#include <QtGui/QIcon>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#endif

class QTreeWidget;
class QTreeWidgetItem;
class QRadioButton;
class QLabel;
class QVBoxLayout;

namespace AZ
{
    class SliceAsset;
}

namespace AzToolsFramework
{
    class InstanceDataNode;
    class FieldTreeItem;
    class ClickableLabel;

    using SliceAssetPtr = AZ::Data::Asset<AZ::SliceAsset>;

    /**
     * Overlay to display if no data changes were detected.
     */
    class NoChangesOverlay
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit NoChangesOverlay(QWidget* pParent);
        ~NoChangesOverlay() override;

    Q_SIGNALS:
        void OnCloseClicked();

    private:
        bool eventFilter(QObject* obj, QEvent* event) override;
        void UpdateGeometry();
    };

    /**
     * Configuration for SlicePushWidget so that it can be used for multiple types of entities (world entities, UI entities).
     * Different entity types can respond to different buses/have different ways of being deleted or gathering children/parents,
     * and have different required pre/post save callbacks for verifying that type of slice is valid (UI slices can have
     * different constraints than world slices)
     */
    struct SlicePushWidgetConfig
    {
        /// PreSaveCallback for SliceTransactions that are executed in this slice push. See SliceTransaction documentation.
        /// Typically contains validation/additional setup for slice assets.
        SliceUtilities::SliceTransaction::PreSaveCallback m_preSaveCB;

        /// PostSaveCallback for SliceTransactions that are executed in this slice push. See SliceTransaction documentation.
        SliceUtilities::SliceTransaction::PostSaveCallback m_postSaveCB;

        /// When adding entities to a slice in a slice push, we delete the original entities because they'll
        /// get born anew when the slice asset is saved and reloaded. DeleteEntitiesCallback takse a list
        /// of entity Ids, and should delete/remove those entities from existence
        using DeleteEntitiesCallback = AZStd::function<void(const AzToolsFramework::EntityIdList&)>;
        DeleteEntitiesCallback m_deleteEntitiesCB;

        /// Given an entity, determine if it's a "root entity", meaning it has no parents in the slice
        /// This is used to check whether changes on the root entity should be displayed in the push widget/allowed,
        /// since currently we have logic that special-cases root entity (the root entity of a world slice cannot
        /// have its TransformComponent translation be anything but the origin, for example)
        using IsRootEntityCallback = AZStd::function<bool(const AZ::Entity*)>;
        IsRootEntityCallback m_isRootEntityCB;

        /// True if entity additions should be checked by default in the Push Widget UI
        bool m_defaultAddedEntitiesCheckState;

        /// True if entity removals should be checked by default in the Push Widget UI
        bool m_defaultRemovedEntitiesCheckState;

        /// Root slice of the context for this Slice Push. Used for basic "how many instances are we about to impact" checks.
        AZ::SliceComponent* m_rootSlice;

        SlicePushWidgetConfig()
            : m_defaultAddedEntitiesCheckState(false)
            , m_defaultRemovedEntitiesCheckState(false)
            , m_rootSlice(nullptr)
        {
        }
    };
    using SlicePushWidgetConfigPtr = AZStd::shared_ptr<SlicePushWidgetConfig>;

    /**
     * Widget for pushing multiple entities/fields to slices.
     */
    class SlicePushWidget
        : public QWidget
        , public AZ::Data::AssetBus::MultiHandler
    {
        Q_OBJECT

    public:

        /**
         * \param entities - set of entity Ids to analyze/push.
         * \param config - Configuration for the SlicePush, in order to handle different types of entities/slices
         */
        explicit SlicePushWidget(const EntityIdList& entities, const SlicePushWidgetConfigPtr& config, AZ::SerializeContext* serializeContext, QWidget* parent = 0);
        ~SlicePushWidget() override;

        //////////////////////////////////////////////////////////////////////////
        // AssetBus
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        /// Determine the number of total references to this asset, providing an approximate
        /// measurement of the impact of pushing changes to this slice.
        static size_t CalculateReferenceCount(const AZ::Data::AssetId& assetId, const AZ::SliceComponent* levelSlice);
    Q_SIGNALS:

        void OnFinished();  ///< The push operation finished successfully.
        void OnCanceled();  ///< The push operation failed with errors.

    public Q_SLOTS:

        /// A field has been selected in the field tree.
        void OnFieldSelectionChanged();

        /// The user has selected a new slice target.
        void OnSliceRadioButtonChanged(QRadioButton* selectButton, const AZ::Data::AssetId assetId, bool checked);

        /// The user has checked/unchecked a field.
        void OnFieldDataChanged(QTreeWidgetItem* item, int column);

        /// The 'Push Selected' button has been clicked.
        void OnPushClicked();

        /// The 'Cancel' button has been clicked.
        void OnCancelClicked();

        /// The 'Toggle warnings' button has been clicked. This shows and hides the list of warnings.
        void OnToggleWarningClicked();

    private:
        //show or hide the conflict message as appropriate
        void ShowConflictMessage();

        /// set the correct icon for changed items (changed/conflicted etc)
        void  SetFieldIcon(FieldTreeItem* item);

        ///returns number of conflicted changes for this fieldtype
        bool GetConflictCount();

        /// Populate tree fields with data about which nods conflict with each other
        void SetupConflictData(AZ::Data::AssetId assetId);

        /// Conduct initial analysis for entities.
        void Setup(const EntityIdList& entities, AZ::SerializeContext* serializeContext);

        /// Creates the UI for displaying warnings to the user.
        void CreateWarningLayout();

        /// Creates the UI for displaying messages to the user.
        QWidget* CreateMessageWidget();

        /// Creates the UI for displaying conflict messages to the user.
        void CreateConflictLayout();

        /// Creates the top area message layout
        void SetupTopAreaMessage(QVBoxLayout* layout, QLabel* label, bool addPullDown);

        /// Creates the conflict display.
        /// \param warningLayout The layout the header will be added to.
        void CreateConflictMessage(QVBoxLayout* conflictLayout);

        /// Creates the header of the warning display.
        /// \param warningLayout The layout the header will be added to.
        void CreateWarningLayoutHeader(QVBoxLayout* warningLayout);

        /// Creates the foldout of the warning display.
        /// \param warningLayout The layout the foldout will be added to.
        void CreateWarningFoldout(QVBoxLayout* warningLayout);

        /// Finalizes the warning layout that was created based on warnings generated during widget setup.
        void FinalizeWarningLayout();

        /// Checks if the entity associated with the passed in field tree item has any slice references that are invalid.
        /// If it does have invalid references, caches information about the references to use for building
        /// the list of warnings for those references, and the pop-up dialog if the user wishes to save and erase them.
        /// \param entityItem The entity to check for invalid references.
        /// \returns True if an invalid reference was found, false if not.
        bool ProcessInvalidReferences(FieldTreeItem* entityItem);

        /// Adds the passed in warning message and slice file to the list of warnings to be displayed.
        /// \param entityItem The entity that generated the warning.
        /// \param sliceFile The slice file that has the warning associated with it.
        /// \aram message The warning message.
        void AddWarning(FieldTreeItem* entityItem, const QString& sliceFile, const QString& message);

        /// Callback used when the user selects a warning message. This will scroll to the associated entity
        /// in the field tree and select that entity.
        void OnWarningMessageSelectionChanged();

        /// Sets up handlers for asset changes on target entities. Invoked by Setup().
        void SetupAssetMonitoring();

        /// Stops listening for asset changes. Invoked before committing transactions.
        void StopAssetMonitoring();

        /// Populate field tree items and cache InstanceDataHierarchies. Invoked by Setup().
        void PopulateFieldTree(const EntityIdList& entities);

        /// Gather new entities added to the current slice instance and populate the field tree accordingly.
        /// @param      entities                     a set of ids of entities 
        /// @param[out] unpushableNewChildEntityIds  ids of new entities that cannot be pushed, for example entities from different instances of the same slice asset you're pushing to
        /// @param[out] pushableNewChildEntityIds    ids of new entities that can be pushed, we need to know the entities being added when populating the changed section
        void PopulateFieldTreeAddedEntities(const EntityIdSet& entities, EntityIdSet& unpushableNewChildEntityIds, EntityIdSet& pushableNewChildEntityIds);

        void PopulateFieldTreeRemovedEntities();

        /// Event filter for key presses.
        bool eventFilter(QObject* target, QEvent *event) override;

        /// Conduct the push operation for all selected fields.
        bool PushSelectedFields();

        bool CheckoutAndGatherAffectedAssets(AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr>& assets);

        /// Gather valid target assets for a given field entry.
        AZStd::vector<SliceAssetPtr> GetValidTargetAssetsForField(const FieldTreeItem& item) const;

        AZ::Data::Asset<AZ::SliceAsset> CloneAsset(const AZ::Data::Asset<AZ::SliceAsset>& asset) const;

        /// Status message data types
        enum class StatusMessageType : int
        {
            Warning
        };

        struct StatusMessage
        {
            StatusMessageType       m_type;
            QString                 m_text;
        };

        void AddStatusMessage(StatusMessageType messageType, const QString& messageText);
        void DisplayStatusMessages();

        /// Checks whether the entity can be pushed to the slice asset, or whether it is blocked 
        /// e.g. would cause loop when loading
        /// \param entityId The entity to check for pushability.
        /// \param assetId The slice asset to check against.
        /// \returns True if the entity can be pushed
        bool CanPushEntityToAsset(const AZ::EntityId entityId, const AZ::Data::AssetId& assetId);

        /// Checks whether the entity will be pushed to the slice asset, or whether it is blocked 
        /// (see CanPushEntityToAsset) or is simply unchecked in the tree by the user.
        /// \param entityId The entity to check for pushability.
        /// \param assetId The slice asset to check against.
        /// \returns True if the entity can be pushed
        bool WillPushEntityToAsset(const AZ::EntityId entityId, const AZ::Data::AssetId& assetId);


        const SlicePushWidgetConfigPtr                              m_config;               ///< Push configuration, containing entity type-specific callbacks/settings

        QTreeWidget*                                                m_fieldTree;            ///< Tree widget for fields (left side)
        QTreeWidget*                                                m_sliceTree;            ///< Tree widget for slice targets (right side)
        QLabel*                                                     m_infoLabel;            ///< Label above slice tree describing selection
        AZ::SerializeContext*                                       m_serializeContext;     ///< Serialize context set for the widget
        AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>     m_sliceInstances;       ///< Cached slice instances for transform fixup
        QVBoxLayout*                                                m_bottomLayout;         ///< Bottom layout containing optional status messages, legend and buttons
        QWidget*                                                    m_warningWidget;        ///< Top level item for warning messages, disabled when there are no warnings.
        QWidget*                                                    m_conflictWidget;       ///< Top level item for conflict messages, disabled when there are no conflicts.
        QWidget*                                                    m_warningFoldout;       ///< Top level item for information displayed when detailed warnings are enabled.
        ClickableLabel*                                             m_toggleWarningButton;  ///< The icon users can click on to toggle detailed warnings.
        QTreeWidget*                                                m_warningMessages;      ///< The list of warning messages.
        QLabel*                                                     m_warningTitle;         ///< If any warnings were encountered, this displays the number of warnings.
        QLabel*                                                     m_conflictTitle;        ///< If any conflicts were encountered, this displays the number of conflicts.
        AZStd::vector<StatusMessage>                                m_statusMessages;       ///< Status messages to be displayed

        QIcon                                                       m_iconGroup;            ///< Icon to use for field groups (parents)
        QIcon                                                       m_iconChangedDataItem;  ///< Icon to use for modified field leafs that're pushable.
        QIcon                                                       m_iconConflictedDataItem;///< Icon to use for modified but conflicted field
        QIcon                                                       m_iconConflictedDisabledDataItem;///< Icon to use for modified, conflicted, and disabled field
        QIcon                                                       m_iconNewDataItem;      ///< Icon to use for new child elements/data under pushable parent fields.
        QIcon                                                       m_iconRemovedDataItem;  ///< Icon to use for removed field leafs that're pushable.
        QIcon                                                       m_iconSliceItem;        ///< Icon to use for slice targets in the slice tree
        QIcon                                                       m_iconWarning;          ///< Icon to use for warnings
        QIcon                                                       m_iconOpen;             ///< Icon to use for opening views
        QIcon                                                       m_iconClosed;           ///< Icon to use for closing views

        QCheckBox*                                                  m_checkboxAllChangedItems; ///< CheckBox to check/uncheck all items that have changes
        QCheckBox*                                                  m_checkboxAllAddedItems;   ///< CheckBox to check/uncheck all items that are newly added to the slice
        QCheckBox*                                                  m_checkboxAllRemovedItems;   ///< CheckBox to check/uncheck all items that are removed from the slice

        QPushButton*                                                m_pushSelectedButton;

        AZStd::unordered_map<QTreeWidgetItem*, FieldTreeItem*>      m_warningsToEntityTree; ///< A map tracking warning messages to elements in the field tree.
        AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent*>     m_entityToSliceComponentWithInvalidReferences; ///< A map tracking entity IDs to components that have invalid references.
    
        //warning/conflict layout settings
        static const int s_messageSeperatorLineHeight = 1;
        // The left margin of the header should be pushed in to the right a bit, by design.
        static const int s_messageHeaderLeftMargin = 24;
        static const int s_messageHheaderMargins = 3;

        // role used to store icons in the QTreeWidgetItem data so correct icon can be restored
        // e.g. when conflict is resolved.
        static const int s_iconStorageRole = Qt::UserRole;

        AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet> m_unpushableNewChildEntityIdsPerAsset; ///< EntityIds that can't be pushed to each slice asset.
        EntityIdSet m_newEntityIds; ///< List of all EntityIds that are new additions.
    };

} // namespace AzToolsFramework
