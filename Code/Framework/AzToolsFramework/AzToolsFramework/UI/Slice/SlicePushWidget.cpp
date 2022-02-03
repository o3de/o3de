/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Slice/SliceEntityBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/Slice/SlicePushWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/UICore/ClickableLabel.hxx>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>

namespace AzToolsFramework
{
    using EditorEntitySortComponent = AzToolsFramework::Components::EditorEntitySortComponent;

    /**
     * Simple overlay control for when no data changes are present.
     */
    NoChangesOverlay::NoChangesOverlay(QWidget* pParent)
        : QWidget(pParent)
    {
        AZ_Assert(pParent, "There must be a parent.");
        setAttribute(Qt::WA_StyledBackground, true);
        int width = parentWidget()->geometry().width();
        int height =  parentWidget()->geometry().height();
        this->setGeometry(0, 0, width, height);
        pParent->installEventFilter(this);

        QLabel* label = new QLabel(tr("No saveable changes detected"), this);
        label->setAlignment(Qt::AlignCenter);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignHCenter);
        layout->addWidget(label);
        QPushButton* button = new QPushButton(tr("Close"), this);
        button->setFixedSize(200, 40);
        layout->addWidget(button);

        connect(button, &QPushButton::released, this, &NoChangesOverlay::OnCloseClicked);

        setLayout(layout);

        UpdateGeometry();
    }

    NoChangesOverlay::~NoChangesOverlay()
    {
        parentWidget()->removeEventFilter(this);
    }

    void NoChangesOverlay::UpdateGeometry()
    {
        int width = this->parentWidget()->geometry().width();
        int height =  this->parentWidget()->geometry().height();
        this->setGeometry(0, 0, width, height);
    }

    bool NoChangesOverlay::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::Resize)
        {
            UpdateGeometry();
        }

        return QObject::eventFilter(obj, event);
    }
   
    static const int kRowSize = 22;

    using SliceAssetPtr = AZ::Data::Asset<AZ::SliceAsset>;

     /**
     * Represents an item in the slice target tree (right pane).
     * Each item represents a single valid slice asset target for the selected node in the left pane/tree.
     */
    class SliceTargetTreeItem
        : public QTreeWidgetItem
    {
    public:

        explicit SliceTargetTreeItem(const SliceAssetPtr& asset, QTreeWidget* view = nullptr, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(view, type)
            , m_selectButton(nullptr)
        {
            m_asset = asset;
            m_selectButton = new QRadioButton();
            setSizeHint(0, QSize(0, kRowSize));
        }

        explicit SliceTargetTreeItem(const SliceAssetPtr& asset, QTreeWidgetItem* item = nullptr, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(item, type)
            , m_selectButton(nullptr)
        {
            m_asset = asset;
            m_selectButton = new QRadioButton();
            setSizeHint(0, QSize(0, kRowSize));
        }

        SliceAssetPtr m_asset;
        QRadioButton* m_selectButton;
    };

    /**
     * Represents an entry in the data tree. There will be a root item for each entity included in the push operation,
     * with child hierarchies for all fields that differ from the base slice.
     * Root items that represent entities store slightly different data from leaf entities. See comments below
     * on class members for details.
     */
    class FieldTreeItem
        : public QTreeWidgetItem
    {
    private:
        explicit FieldTreeItem(QTreeWidget* parent, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(parent, type)
            , m_pushedHiddenAutoPushNodes(false)
            , m_entity(nullptr)
            , m_entityItem(nullptr)
            , m_node(nullptr)
            , m_slicePushType(SlicePushType::Changed)
        {
            setSizeHint(0, QSize(0, kRowSize));
        }
        explicit FieldTreeItem(FieldTreeItem* parent, int type = QTreeWidgetItem::Type)
            : QTreeWidgetItem(parent, type)
            , m_pushedHiddenAutoPushNodes(false)
            , m_entity(nullptr)
            , m_entityItem(nullptr)
            , m_node(nullptr)
            , m_slicePushType(SlicePushType::Changed)
        {
            setSizeHint(0, QSize(0, kRowSize));
        }

    public:

        static FieldTreeItem* CreateEntityRootItem(AZ::Entity* entity, QTreeWidget* view = nullptr, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(view, type);
            item->m_entity = entity;
            item->m_entityItem = item;
            item->m_node = &item->m_hierarchy;
            item->m_address = item->m_node->ComputeAddress();
            return item;
        }
        
        static FieldTreeItem* CreateFieldItem(AzToolsFramework::InstanceDataNode* node, FieldTreeItem* entityItem, FieldTreeItem* parentItem, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(parentItem, type);
            item->m_entity = entityItem->m_entity;
            item->m_entityItem = entityItem;
            item->m_node = node;
            item->m_address = item->m_node->ComputeAddress();
            item->m_selectedAsset = entityItem->m_selectedAsset;
            return item;
        }
        
        static FieldTreeItem* CreateEntityRemovalItem(AZ::Entity* assetEntityToRemove, 
                                                      const SliceAssetPtr& targetAsset, 
                                                      const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, 
                                                      int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(static_cast<QTreeWidget*>(nullptr), type);
            item->m_entity = assetEntityToRemove;
            item->m_entityItem = item;
            item->m_slicePushType = SlicePushType::Removed;
            item->m_sliceAddress = sliceAddress;
            item->m_selectedAsset = targetAsset.GetId();
            return item;
        }

        static FieldTreeItem* CreateEntityAddItem(AZ::Entity* entity, int type = QTreeWidgetItem::Type)
        {
            FieldTreeItem* item = new FieldTreeItem(static_cast<QTreeWidget*>(nullptr), type);
            item->m_entity = entity;
            item->m_entityItem = item;
            item->m_slicePushType = SlicePushType::Added;
            item->m_node = &item->m_hierarchy;
            item->m_address = item->m_node->ComputeAddress();
            return item;
        }

        bool IsPushableItem() const 
        {
            // Either entity add/removal, or leaf item (for updates)
            return m_slicePushType == SlicePushType::Added || m_slicePushType == SlicePushType::Removed || childCount() == 0;
        }

        bool IsConflicted() const
        {
            //item cannot be conflicted if the user has deactivated it 
            if (checkState(0) != Qt::CheckState::Checked)
            {
                return false;
            }

            //conflicts can only happen with multiple changes to the same fields in multiple entity instances, ignore additions/deletions
            if (m_slicePushType != SlicePushType::Changed)
            {
                return false;
            }

            //check to see whether any entity marked as a potential conflict is checked in the UI
            for (int itemIndex = 0; itemIndex < m_conflictsWithItems.size(); itemIndex++)
            {
                FieldTreeItem *item = m_conflictsWithItems.at(itemIndex);

                if (item->checkState(0) == Qt::CheckState::Checked)
                {
                    return true;
                }
            }
            return false;
        }

        bool HasPotentialConflicts() const
        {
            return m_conflictsWithItems.size() > 0;
        }

        bool AllLeafNodesShareSelectedAsset(AZ::Data::AssetId targetAssetId) const
        {
            if (IsPushableItem())
            {
                return m_selectedAsset == targetAssetId;
            }

            for (int childIndex = 0; childIndex < childCount(); childIndex++)
            {
                FieldTreeItem* childItem = static_cast<FieldTreeItem*>(child(childIndex));
                if (childItem)
                {
                    if (!childItem->AllLeafNodesShareSelectedAsset(targetAssetId))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool AllLeafNodesArePushableToThisAncestor(AZ::Entity& entity) const
        {
            if (IsPushableItem())
            {
                if (m_slicePushType == FieldTreeItem::SlicePushType::Removed)
                {
                    return true; // Removal of entities is guaranteed to be pushable.
                }
                else
                {
                    return SliceUtilities::IsNodePushable(*m_node, SliceUtilities::IsRootEntity(entity));
                }
                
            }

            for (int childIndex = 0; childIndex < childCount(); childIndex++)
            {
                FieldTreeItem* childItem = static_cast<FieldTreeItem*>(child(childIndex));
                if (childItem)
                {
                    if (!childItem->AllLeafNodesArePushableToThisAncestor(entity))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        // For (root) Entity-level items, contains the entities built IDH.
        AzToolsFramework::InstanceDataHierarchy m_hierarchy;

        // For (root) Entity-level item, points to hierarchy. For child items, points to the IDH node for the field.
        AzToolsFramework::InstanceDataNode* m_node;

        //adress of m_node
        AzToolsFramework::InstanceDataNode::Address m_address;

        //list of all field items with (potential) conflicts
        AZStd::vector<FieldTreeItem*> m_conflictsWithItems;

        // For (root) Entity-level item, hidden-but-pushable field nodes (these aren't displayed, so if a push is happening, they're automatically included in push).
        AZStd::unordered_set<AzToolsFramework::InstanceDataNode*> m_hiddenAutoPushNodes;

        // For (root) Entity-level item, whether the hidden auto-push nodes have been pushed yet
        bool m_pushedHiddenAutoPushNodes;

        // Entity associated with this node, or the entity parent of this node.
        AZ::Entity* m_entity;

        enum class SlicePushType : int
        {
            Changed, // for entities with changes in itself and/or its components
            Added, // for entities that are newly added to the target slice
            Removed // for entities that are removed from the target slice
        };
        SlicePushType m_slicePushType;

        // For entity-level items only, contains slice ancestor list.
        AZ::SliceComponent::EntityAncestorList m_ancestors;
        AZ::SliceComponent::SliceInstanceAddress m_sliceAddress;

        // Relevant for leaf items and entity add/removals, stores Id of target slice asset.
        AZ::Data::AssetId m_selectedAsset;

        // Points to item at root of this entity hieararchy.
        FieldTreeItem* m_entityItem;
    };

    namespace Internal
    {
        QString PushChangedItemToSliceTransaction(SliceUtilities::SliceTransaction::TransactionPtr sliceTransaction, FieldTreeItem* item)
        {
            QString pushError;
            SliceUtilities::SliceTransaction::Result result = sliceTransaction->UpdateEntityField(item->m_entity, item->m_node->ComputeAddress());
            if (!result)
            {
                pushError = QString("Unable to add entity for push.\r\n\r\nDetails:\r\n%1").arg(result.GetError().c_str());
            }
            else
            {
                // Auto-push any hidden auto-push fields for the root entity item that need pushed
                FieldTreeItem* entityRootItem = item->m_entityItem;
                if (entityRootItem && !entityRootItem->m_hiddenAutoPushNodes.empty() && !entityRootItem->m_pushedHiddenAutoPushNodes)
                {
                    // We also need to ensure we're only pushing hidden pushes to the direct slice ancestor of the entity; we don't want to
                    // be pushing things like component order to an ancestor that wouldn't have the same components
                    AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
                    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityRootItem->m_entity->GetId(),
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
                    if (item->m_selectedAsset == sliceAddress.first->GetSliceAsset().GetId())
                    {
                        for (auto& hiddenAutoPushNode : entityRootItem->m_hiddenAutoPushNodes)
                        {
                            result = sliceTransaction->UpdateEntityField(entityRootItem->m_entity, hiddenAutoPushNode->ComputeAddress());

                            // Explicitly not warning of hidden field pushes - a user cannot act or change these failures, and we don't want
                            // to prevent valid non-hidden pushes to fail because of something someone can't change.
                        }
                        entityRootItem->m_pushedHiddenAutoPushNodes = true;
                    }
                }
            }

            return pushError;
        }

        /**
         * Check/uncheck all top level items of the same FieldTreeItem::SlicePushType.
         */
        void CheckAllItemsBySlicePushType(QTreeWidget* fieldTree, FieldTreeItem::SlicePushType slicePushType, Qt::CheckState checked)
        {
            for (int topLevelIndex = 0; topLevelIndex < fieldTree->topLevelItemCount(); ++topLevelIndex)
            {
                FieldTreeItem* item = static_cast<FieldTreeItem*>(fieldTree->topLevelItem(topLevelIndex));
                if (item->m_slicePushType == slicePushType && !item->isDisabled())
                {
                    item->setCheckState(0, checked);
                }
            }
        }

        /**
         * Test if all top level items of the same FieldTreeItem::SlicePushType have the same Qt::CheckState.
         */
        Qt::CheckState TestCheckStateOfAllTopLevelItemsBySlicePushType(QTreeWidget* fieldTree, FieldTreeItem::SlicePushType slicePushType)
        {
            int checkedNum = 0;
            int slicePushTypeItemNum = 0;
            for (int topLevelIndex = 0; topLevelIndex < fieldTree->topLevelItemCount(); ++topLevelIndex)
            {
                FieldTreeItem* item = static_cast<FieldTreeItem*>(fieldTree->topLevelItem(topLevelIndex));
                if (item->m_slicePushType == slicePushType)
                {
                    slicePushTypeItemNum++;
                    Qt::CheckState itemCheckState = item->checkState(0);
                    if (itemCheckState == Qt::CheckState::Checked)
                    {
                        checkedNum++;
                    }
                }
            }

            if (checkedNum == 0)
            {
                return Qt::CheckState::Unchecked;
            }
            else if (checkedNum == slicePushTypeItemNum)
            {
                return Qt::CheckState::Checked;
            }
            else
            {
                return Qt::CheckState::PartiallyChecked;
            }
        }
    } // namespace Internal

    //=========================================================================
    // SlicePushWidget
    //=========================================================================
    SlicePushWidget::SlicePushWidget(const EntityIdList& entities, const SlicePushWidgetConfigPtr& config, AZ::SerializeContext* serializeContext, QWidget* parent)
        : QWidget(parent)
        , m_config(config)
    {
        const int defaultContentsMargin = 5;

        AZ_Assert(m_config != nullptr, "SlicePushWidget created without configuration!");

        QVBoxLayout* layout = new QVBoxLayout();
        // The warning foldout goes to the edge of the slice push widget.
        layout->setContentsMargins(0, 0, 0, 0);

        // Window layout:
        // ===============================================
        // ||-------------------------------------------||
        // || (Data tree label)    (slice tree label)   ||
        // ||-------------------------------------------||
        // ||                     |                     ||
        // ||                (splitter)                 ||
        // ||                     |                     ||
        // ||    (Data tree)      | (Slice target tree) ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||-------------------------------------------||
        // || (optional status messages)                ||
        // || (legend)                     (button box) ||
        // ===============================================

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_fieldTree = new QTreeWidget();
        m_fieldTree->setObjectName("SlicePushWidget.m_fieldTree");
        m_fieldTree->setColumnCount(2);
        m_fieldTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_fieldTree->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_fieldTree->setDragEnabled(false);
        m_fieldTree->setSelectionMode(QAbstractItemView::SingleSelection);
        m_fieldTree->header()->setStretchLastSection(false);
        m_fieldTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_fieldTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_fieldTree->installEventFilter(this);

        m_sliceTree = new QTreeWidget();
        m_sliceTree->setColumnCount(3);
        m_sliceTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_sliceTree->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_sliceTree->setDragEnabled(false);
        m_sliceTree->header()->setStretchLastSection(false);
        m_sliceTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_sliceTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_sliceTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_sliceTree->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
        m_sliceTree->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        connect(m_sliceTree, &QTreeWidget::itemClicked, this, 
            [](QTreeWidgetItem* item, int)
            {
                SliceTargetTreeItem* sliceItem = static_cast<SliceTargetTreeItem*>(item);
                sliceItem->m_selectButton->setChecked(true);
            });

        QStringList headers;
        headers << tr("Property");
        headers << tr("Target Slice Asset");
        m_fieldTree->setHeaderLabels(headers);
        m_fieldTree->setToolTip(tr("Choose which properties to push, and to which assets."));

        headers = QStringList();
        headers << tr("Selected");
        headers << tr("Slice Asset");
        headers << tr("Level References"); // Count is only relevant in level scope. Revisit as part of Slice-Editor effort.
        m_sliceTree->setHeaderLabels(headers);
        m_sliceTree->setToolTip(tr("Choose target assets for selected property groups."));

        m_iconGroup = QIcon(":/PropertyEditor/Resources/browse_on.png");
        m_iconChangedDataItem = QIcon(":/PropertyEditor/Resources/changed_data_item.png");
        m_iconChangedDataItem.addFile(":/PropertyEditor/Resources/changed_data_item.png", QSize(), QIcon::Selected);
        m_iconConflictedDataItem = QIcon(":/PropertyEditor/Resources/cant_save.png");
        m_iconConflictedDataItem.addFile(":/PropertyEditor/Resources/cant_save.png", QSize(), QIcon::Selected);
        m_iconConflictedDisabledDataItem = QIcon(":/PropertyEditor/Resources/cant_save_disabled.png");
        m_iconConflictedDisabledDataItem.addFile(":/PropertyEditor/Resources/cant_save_disabled.png", QSize(), QIcon::Selected);
        m_iconNewDataItem = QIcon(":/PropertyEditor/Resources/new_data_item.png");
        m_iconNewDataItem.addFile(":/PropertyEditor/Resources/new_data_item.png", QSize(), QIcon::Selected);
        m_iconRemovedDataItem = QIcon(":/PropertyEditor/Resources/removed_data_item.png");
        m_iconRemovedDataItem.addFile(":/PropertyEditor/Resources/removed_data_item.png", QSize(), QIcon::Selected);
        m_iconSliceItem = QIcon(":/PropertyEditor/Resources/slice_item.png");
        m_iconWarning = QIcon(":/PropertyEditor/Resources/warning.png");
        m_iconOpen = QIcon(":/PropertyEditor/Resources/open_arrow.png");
        m_iconClosed = QIcon(":/PropertyEditor/Resources/close_arrow.png");

        // Main splitter contains left & right widgets (the two trees).
        QSplitter* splitter = new QSplitter();
        splitter->setContentsMargins(defaultContentsMargin, defaultContentsMargin, defaultContentsMargin, defaultContentsMargin);

        QWidget* leftWidget = new QWidget();
        QVBoxLayout* leftLayout = new QVBoxLayout();
        QLabel* fieldTreeLabel = new QLabel(tr("Select fields and assign target slices:"));
        fieldTreeLabel->setProperty("Title", true);
        leftLayout->addWidget(fieldTreeLabel);
        leftLayout->addWidget(m_fieldTree);
        leftWidget->setLayout(leftLayout);

        QWidget* rightWidget = new QWidget();
        QVBoxLayout* rightLayout = new QVBoxLayout();
        m_infoLabel = new QLabel();
        m_infoLabel->setProperty("Title", true);
        rightLayout->addWidget(m_infoLabel);
        rightLayout->addWidget(m_sliceTree);
        rightWidget->setLayout(rightLayout);

        splitter->addWidget(leftWidget);
        splitter->addWidget(rightWidget);
        
        QList<int> sizes;
        const int totalWidth = size().width();
        const int firstColumnSize = totalWidth / 2;
        sizes << firstColumnSize << (totalWidth - firstColumnSize);
        splitter->setSizes(sizes);
        splitter->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

        QHBoxLayout* bottomLegendAndButtonsLayout = new QHBoxLayout();

        // Create/add legend to bottom layout.
        {
            QLabel* imageChanged = new QLabel();
            if (!m_iconChangedDataItem.availableSizes().empty())
            {
                imageChanged->setPixmap(m_iconChangedDataItem.pixmap(m_iconChangedDataItem.availableSizes().first()));
            }
            m_checkboxAllChangedItems = new QCheckBox("Changed", this);
            m_checkboxAllChangedItems->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
            connect(m_checkboxAllChangedItems, &QCheckBox::stateChanged, this, [this](int state)
            {
                Internal::CheckAllItemsBySlicePushType(m_fieldTree, FieldTreeItem::SlicePushType::Changed, Qt::CheckState(state));
            });

            QLabel* imageAdded = new QLabel();
            if (!m_iconNewDataItem.availableSizes().empty())
            {
                imageAdded->setPixmap(m_iconNewDataItem.pixmap(m_iconNewDataItem.availableSizes().first()));
            }
            m_checkboxAllAddedItems = new QCheckBox("Added", this);
            m_checkboxAllAddedItems->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
            connect(m_checkboxAllAddedItems, &QCheckBox::stateChanged, this, [this](int state)
            {
                Internal::CheckAllItemsBySlicePushType(m_fieldTree, FieldTreeItem::SlicePushType::Added, Qt::CheckState(state));
            });

            QLabel* imageRemoved = new QLabel();
            if (!m_iconRemovedDataItem.availableSizes().empty())
            {
                imageRemoved->setPixmap(m_iconRemovedDataItem.pixmap(m_iconRemovedDataItem.availableSizes().first()));
            }
            m_checkboxAllRemovedItems = new QCheckBox("Removed", this);
            m_checkboxAllRemovedItems->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
            connect(m_checkboxAllRemovedItems, &QCheckBox::stateChanged, this, [this](int state)
            {
                Internal::CheckAllItemsBySlicePushType(m_fieldTree, FieldTreeItem::SlicePushType::Removed, Qt::CheckState(state));
            });

            bottomLegendAndButtonsLayout->addWidget(imageChanged, 0);
            bottomLegendAndButtonsLayout->addWidget(m_checkboxAllChangedItems, 0);
            bottomLegendAndButtonsLayout->addWidget(imageAdded, 0);
            bottomLegendAndButtonsLayout->addWidget(m_checkboxAllAddedItems, 0);
            bottomLegendAndButtonsLayout->addWidget(imageRemoved, 0);
            bottomLegendAndButtonsLayout->addWidget(m_checkboxAllRemovedItems, 0);
        }

        // Create/populate button box.
        {
            QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
            m_pushSelectedButton = new QPushButton(tr("Save Selected Overrides"));
            m_pushSelectedButton->setToolTip(tr("Save selected overrides to the specified slice(es)."));
            m_pushSelectedButton->setDefault(false);
            m_pushSelectedButton->setAutoDefault(false);
            QPushButton* cancelButton = new QPushButton(tr("Cancel"));
            cancelButton->setDefault(true);
            cancelButton->setAutoDefault(true);
            buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
            buttonBox->addButton(m_pushSelectedButton, QDialogButtonBox::ApplyRole);
            connect(m_pushSelectedButton, &QPushButton::clicked, this, &SlicePushWidget::OnPushClicked);
            connect(cancelButton, &QPushButton::clicked, this, &SlicePushWidget::OnCancelClicked);

            bottomLegendAndButtonsLayout->addWidget(buttonBox, 1);
        }

        m_bottomLayout = new QVBoxLayout();
        m_bottomLayout->setContentsMargins(defaultContentsMargin, defaultContentsMargin, defaultContentsMargin, defaultContentsMargin);
        m_bottomLayout->setAlignment(Qt::AlignBottom);
        m_bottomLayout->addLayout(bottomLegendAndButtonsLayout);

        CreateWarningLayout();

        CreateConflictLayout();

        // Add everything to main layout.
        layout->addWidget(m_conflictWidget, 0);
        layout->addWidget(m_warningWidget, 0);
        layout->addWidget(splitter, 1);
        layout->addLayout(m_bottomLayout, 0);

        connect(m_fieldTree, &QTreeWidget::itemSelectionChanged, this, &SlicePushWidget::OnFieldSelectionChanged);
        connect(m_fieldTree, &QTreeWidget::itemChanged, this, &SlicePushWidget::OnFieldDataChanged);

        setLayout(layout);

        Setup(entities, serializeContext);

        FinalizeWarningLayout();

        m_conflictWidget->hide();

        // Only test m_checkboxAllChangedItems, the other two CheckBoxes are defaulted to unchecked.
        Qt::CheckState checkStateOfChangedItems = Internal::TestCheckStateOfAllTopLevelItemsBySlicePushType(m_fieldTree, FieldTreeItem::SlicePushType::Changed);
        if (checkStateOfChangedItems == Qt::CheckState::Checked)
        {
            m_checkboxAllChangedItems->setCheckState(Qt::CheckState::Checked);
        }

        ShowConflictMessage();
    }
    
    //=========================================================================
    // ~SlicePushWidget
    //=========================================================================
    SlicePushWidget::~SlicePushWidget()
    {
        if (AZ::Data::AssetBus::MultiHandler::BusIsConnected())
        {
            StopAssetMonitoring();
        }
    }

    QWidget* SlicePushWidget::CreateMessageWidget()
    {
        QWidget* widget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignBottom);

        widget->setLayout(layout);

        return widget;
    }

    void SlicePushWidget::CreateConflictLayout()
    {
        m_conflictWidget = CreateMessageWidget();

        QLayout* layout = m_conflictWidget->layout();
        QVBoxLayout* boxLayout = static_cast<QVBoxLayout*>(layout);

        CreateConflictMessage(boxLayout);
    }

    void SlicePushWidget::CreateWarningLayout()
    {
        m_warningWidget = CreateMessageWidget();

        QLayout* layout = m_warningWidget->layout();
        QVBoxLayout* boxLayout = static_cast<QVBoxLayout*>(layout);

        CreateWarningLayoutHeader(boxLayout);
        CreateWarningFoldout(boxLayout);
    }

    void SlicePushWidget::SetupTopAreaMessage(QVBoxLayout* layout, QLabel* label, bool addPullDown)
    {
        if (!layout)
        {
            return;
        }

        const int foldoutImageSize = 24;

        // The warning layout is separated from the rest of the slice push widget by a thin line on the top and bottom.
        QWidget* topHorizontalLine = new QWidget();
        topHorizontalLine->setObjectName("WarningTopLine");
        topHorizontalLine->setFixedHeight(s_messageSeperatorLineHeight);
        topHorizontalLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setContentsMargins(s_messageHeaderLeftMargin, s_messageHheaderMargins, s_messageHheaderMargins, s_messageHheaderMargins);

        // The warning layout header has an icon, a header message, and a details label and foldout that can
        // be clicked to view additional information about warnings.
        QLabel* conflictIcon = new QLabel();
        conflictIcon->setAlignment(Qt::AlignLeft);
        conflictIcon->setPixmap(m_iconConflictedDataItem.pixmap(m_iconConflictedDataItem.availableSizes().first()));
        conflictIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // The warning title should be replaced later with a specific number of missing references, seed it with something useful in case that fails.
        label->setAlignment(Qt::AlignLeft);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        // Add all of the widgets in order to the layout.
        topLayout->addWidget(conflictIcon, Qt::AlignLeft);
        topLayout->addWidget(label, Qt::AlignLeft);

        if (addPullDown)
        {
            ClickableLabel* detailsLabel = new ClickableLabel();
            detailsLabel->setText(tr("Details"));
            detailsLabel->setAlignment(Qt::AlignRight);
            detailsLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            connect(detailsLabel, &ClickableLabel::clicked, this, &SlicePushWidget::OnToggleWarningClicked);

            m_toggleWarningButton = new ClickableLabel();
            m_toggleWarningButton->setPixmap(m_iconClosed.pixmap(m_iconClosed.availableSizes().first()));
            m_toggleWarningButton->setFixedSize(foldoutImageSize, foldoutImageSize);
            m_toggleWarningButton->setAlignment(Qt::AlignCenter);
            m_toggleWarningButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            connect(m_toggleWarningButton, &ClickableLabel::clicked, this, &SlicePushWidget::OnToggleWarningClicked);

            topLayout->addWidget(detailsLabel, Qt::AlignRight);
            topLayout->addWidget(m_toggleWarningButton, Qt::AlignRight);
        }
        

        // The warning layout header is separated on the bottom by a thin line from the rest of the window.
        QWidget* bottomHorizontalLine = new QWidget();
        bottomHorizontalLine->setObjectName("WarningBottomLine");
        bottomHorizontalLine->setFixedHeight(s_messageSeperatorLineHeight);
        bottomHorizontalLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // Add everything to the warning layout.
        layout->addWidget(topHorizontalLine);
        layout->addLayout(topLayout);
        layout->addWidget(bottomHorizontalLine);
    }
    
    void SlicePushWidget::CreateConflictMessage(QVBoxLayout* conflictLayout)
    {
        if (!conflictLayout)
        {
            return;
        }
        
        m_conflictTitle = new QLabel(tr("You can't save the same component or property override to the same slice. Deselect conflicting overrides before saving."));

        SetupTopAreaMessage(conflictLayout, m_conflictTitle, false);
    }

    void SlicePushWidget::CreateWarningLayoutHeader(QVBoxLayout* warningLayout)
    {
        if (!warningLayout)
        {
            return;
        }

        m_warningTitle = new QLabel(tr("One or more slice files have missing file references. Cancel to update your references or \"Save...\" to remove the missing references."));
        m_warningTitle->setObjectName("SlicePushWidget.m_warningTitle");
        SetupTopAreaMessage(warningLayout, m_warningTitle, true);
    }

    void SlicePushWidget::CreateWarningFoldout(QVBoxLayout* warningLayout)
    {
        // The warning foldout contains a scrollable list of warning messages.
        // It has two columns, the first is the slice file that has an error in it.
        // The second column is the associated warning message.
        // This entire section of the warning header can be hidden away or shown using the details button
        // on the warning header.
        m_warningFoldout = new QWidget();

        QVBoxLayout* warningFoldoutLayout = new QVBoxLayout();
        warningFoldoutLayout->setContentsMargins(0, 0, 0, 0);

        m_warningMessages = new QTreeWidget();
        m_warningMessages->setObjectName("WarningTreeWidget");
        m_warningMessages->setUniformRowHeights(true);
        m_warningMessages->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        // There are two columns for the warning list, the first is the slice file with an error, and the second is the message.
        m_warningMessages->setColumnCount(2);
        // The warning display can't really have things dragged and dropped into it or out of it.
        m_warningMessages->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_warningMessages->setDragEnabled(false);
        // The slice file name column should be resizable, so assets buried deep in a directory structure don't
        // eat up all of the visible space and hide away the warning message.
        m_warningMessages->header()->setSectionResizeMode(0, QHeaderView::Interactive);
        // Have the warning message fill up the rest of the available view.
        m_warningMessages->header()->setSectionResizeMode(1, QHeaderView::Stretch);

        QStringList headerLabels;
        headerLabels << tr("Slice file");
        headerLabels << tr("Description");
        m_warningMessages->setHeaderLabels(headerLabels);

        // When the user clicks on a warning message, the slice push widget's tree view should scroll to show the
        // associated entity, and select that entity.
        connect(m_warningMessages,
            &QTreeWidget::itemSelectionChanged,
            this, [this]() { this->OnWarningMessageSelectionChanged(); });

        warningFoldoutLayout->addWidget(m_warningMessages, 0);
        // The warning list has a maximum size, to allow the rest of the slice push widget to expand to fill the space.
        // Include a stretch here to help keep the warning message list from over-growing.
        warningFoldoutLayout->addStretch(1);

        m_warningFoldout->setLayout(warningFoldoutLayout);
        m_warningFoldout->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

        // The detailed view of the warnings are hidden by default.
        m_warningFoldout->hide();

        warningLayout->addWidget(m_warningFoldout);
    }

    void SlicePushWidget::FinalizeWarningLayout()
    {
        // The SlicePushWidget UI had to be built before building the treeview of pushable content.
        // Once that treeview of pushable content has been built, the final warning count will have been generated.
        // At this point, the detailed warning view's final size can be determined, and the warning header can be finalized.
        const int warningMessageCount = m_warningMessages->topLevelItemCount();

        if (warningMessageCount == 0)
        {
            m_warningWidget->hide();
            return;
        }

        // Update the warning header to display the number of slices with invalid references.
        m_warningTitle->setText(tr("%1 missing reference(s). To update your references, click Cancel. To remove the missing references, click Save.").arg(
            m_entityToSliceComponentWithInvalidReferences.size()));

        // Include a bit of padding when generating the height of the warning message treeview so the bottom elements
        // don't sit uncomfortably close to the rest of the slice push widget.
        const int padding = 6;

        // Start building the final display height of the tree view, start with the size of the header and the padding.
        int finalHeight = m_warningMessages->header()->height() + padding;

        // Clamp the treeview to a maximum number of elements to display. If there are less warnings than this, then the warning
        // view shouldn't take up empty space. If there are more warnings than this, let the scroll view handle it. The rest
        // of the height of the slice push widget should be used by the interface for pushing slices, and not all of these warnings.
        const int maxItemsDisplayedAtOnce = 5;
        int itemsToDisplay = warningMessageCount < maxItemsDisplayedAtOnce ? warningMessageCount : maxItemsDisplayedAtOnce;
        // Check the height of each row to get an exact height. Generally the row heights will be the same, but this allows
        // for some flexibility, in case a long message causes the text to wrap and the cell to grow in height.
        for(int i = 0; i < itemsToDisplay; ++i)
        {
            QTreeWidgetItem* row = m_warningMessages->topLevelItem(i);
            QRect rowRect = m_warningMessages->visualItemRect(row);
            finalHeight += rowRect.height();
        }
        m_warningMessages->setMaximumHeight(finalHeight);
    }

    void SlicePushWidget::OnWarningMessageSelectionChanged()
    {
        QList<QTreeWidgetItem*> selectedItems = m_warningMessages->selectedItems();
        if (selectedItems.size() <= 0)
        {
            return;
        }
        AZStd::unordered_map<QTreeWidgetItem*, FieldTreeItem*>::iterator entityTreeItem =
            m_warningsToEntityTree.find(selectedItems[0]);
        if (entityTreeItem == m_warningsToEntityTree.end())
        {
            return;
        }
        m_fieldTree->scrollToItem(entityTreeItem->second);
        m_fieldTree->setCurrentItem(entityTreeItem->second);
    }

    //=========================================================================
    // SlicePushWidget::OnAssetReloaded
    //=========================================================================
    void SlicePushWidget::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Slice has changed underneath us, transaction invalidated
        AZStd::string sliceAssetPath;
        EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());

        QMessageBox::warning(this, QStringLiteral("Push to Slice Aborting"), 
            QString("Slice asset changed on disk during push configuration, transaction canceled.\r\n\r\nAsset:\r\n%1").arg(sliceAssetPath.c_str()), 
            QMessageBox::Ok);
        
        // Force cancel the push
        emit OnCanceled();
    }

    void SlicePushWidget::SetupConflictData(AZ::Data::AssetId targetSliceAssetId)
    {
        // Remove all previous conflicts data before re-calculating them.
        QTreeWidgetItemIterator conflictItemIter(m_fieldTree);
        for (; *conflictItemIter; ++conflictItemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*conflictItemIter);
            AZ_Assert(item, "FieldTreeItem pointer is null.");
            item->m_conflictsWithItems.clear();
        }

        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);

            if (!item->IsPushableItem())
            {
                continue;
            }

            QTreeWidgetItemIterator conflictTestIter = itemIter;
            conflictTestIter++;

            for (; *conflictTestIter; ++conflictTestIter)
            {
                FieldTreeItem* confictTestItem = static_cast<FieldTreeItem*>(*conflictTestIter);

                if (!confictTestItem->IsPushableItem() ||
                    item->m_slicePushType != FieldTreeItem::SlicePushType::Changed)
                {
                    // Only items of 'SlicePushType::Changed' needs to be checked. Additions and 
                    // removals don't generate override conflict.
                    continue;
                }

                // Check if components added are incompatible.
                bool incompatibleComponents = false;

                AZ::ComponentDescriptor* componentDesc = nullptr;
                AZ::ComponentDescriptorBus::EventResult(componentDesc, item->m_node->GetClassMetadata()->m_typeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                if (componentDesc)
                {
                    AZ::ComponentDescriptor* componentDesc_conflictTest = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(componentDesc_conflictTest, confictTestItem->m_node->GetClassMetadata()->m_typeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                    if (componentDesc_conflictTest)
                    {
                        AZ::ComponentDescriptor::DependencyArrayType providedServices_conflictTest;
                        const AZ::Component* conflictInstance = confictTestItem->m_node->GetNumInstances() > 0 ? static_cast<AZ::Component*>(confictTestItem->m_node->FirstInstance()) : nullptr;
                        componentDesc_conflictTest->GetProvidedServices(providedServices_conflictTest, conflictInstance);

                        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
                        const AZ::Component* itemInstance = item->m_node->GetNumInstances() > 0 ? static_cast<AZ::Component*>(item->m_node->FirstInstance()) : nullptr;
                        componentDesc->GetIncompatibleServices(incompatibleServices, itemInstance);

                        auto foundItr = AZStd::find_first_of(incompatibleServices.begin(), incompatibleServices.end(), providedServices_conflictTest.begin(), providedServices_conflictTest.end());
                        if (foundItr != incompatibleServices.end())
                        {
                            incompatibleComponents = true;
                        }
                    }
                }

                if (incompatibleComponents || item->m_address == confictTestItem->m_address)
                {
                    // Check whether both changes will be pushed to the same entity down in the target slice.

                    AZ::Entity* rootEntity_item = nullptr;
                    FieldTreeItem* entityField_item = item->m_entityItem;
                    for (const AZ::SliceComponent::Ancestor& ancestor : entityField_item->m_ancestors)
                    {
                        if (ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId() == targetSliceAssetId)
                        {
                            rootEntity_item = ancestor.m_entity;
                        }
                    }
                    
                    if (!rootEntity_item)
                    {
                        continue;//must be new instance of slice which has been changed
                    }

                    AZ::Entity* rootEntity_conflictTest = nullptr;
                    FieldTreeItem* entityField_conflictTest = confictTestItem->m_entityItem;
                    for (const AZ::SliceComponent::Ancestor& ancestor : entityField_conflictTest->m_ancestors)
                    {
                        if (ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId() == targetSliceAssetId)
                        {
                            rootEntity_conflictTest = ancestor.m_entity;
                        }
                    }
                    AZ_Assert(rootEntity_conflictTest, "Cannot find corresponding entity in the target slice while setting up override items in Advanced Push Window.");

                    if (rootEntity_item == rootEntity_conflictTest)
                    {
                        item->m_conflictsWithItems.push_back(confictTestItem);
                        confictTestItem->m_conflictsWithItems.push_back(item);
                    }
                }
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::Setup
    //=========================================================================
    void SlicePushWidget::Setup(const EntityIdList& entities, AZ::SerializeContext* serializeContext)
    {
        m_serializeContext = serializeContext;

        if (!m_serializeContext)
        {
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        }

        PopulateFieldTree(entities);

        SetupAssetMonitoring();

        // Update asset name column for all pushable items (leaf update items, add/remove entity items)
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (item->IsPushableItem())
            {
                AZStd::vector<SliceAssetPtr> validSliceAssets = GetValidTargetAssetsForField(*item);
                if (!validSliceAssets.empty())
                {
                    item->m_selectedAsset = validSliceAssets.back().GetId();

                    AZStd::string sliceAssetPath;
                    EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, item->m_selectedAsset);

                    SetFieldIcon(item);

                    const char* shortPath = sliceAssetPath.c_str();
                    auto pos = sliceAssetPath.find_last_of('/');
                    if (pos == AZStd::string::npos)
                    {
                        pos = sliceAssetPath.find_last_of('\\');
                    }
                    if (pos != AZStd::string::npos)
                    {
                        shortPath += pos + 1;
                    }

                    item->setText(1, shortPath);

                    const QString tooltip =
                        tr("Field \"") + item->text(0) + tr("\" is assigned to asset \"") + sliceAssetPath.c_str() + "\".";
                    item->setToolTip(0, tooltip);
                    item->setToolTip(1, tooltip);
                }
            }
        }

        // Auto-select the first entity.
        const QModelIndex firstIndex = m_fieldTree->model()->index(0, 0);
        if (firstIndex.isValid())
        {
            m_fieldTree->selectionModel()->select(firstIndex, QItemSelectionModel::Select);
        }

        m_fieldTree->expandAll();

        // Display an overlay if no pushable options were found.
        if (0 == m_fieldTree->topLevelItemCount())
        {
            NoChangesOverlay* overlay = new NoChangesOverlay(this);
            connect(overlay, &NoChangesOverlay::OnCloseClicked, this, &SlicePushWidget::OnCancelClicked);
        }

        DisplayStatusMessages();
    }

    //=========================================================================
    // SlicePushWidget::SetupAssetMonitoring
    //=========================================================================
    void SlicePushWidget::SetupAssetMonitoring()
    {
        for (const auto& sliceAddress : m_sliceInstances)
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(sliceAddress.GetReference()->GetSliceAsset().GetId());
        }
    }

    //=========================================================================
    // SlicePushWidget::StopAssetMonitoring
    //=========================================================================
    void SlicePushWidget::StopAssetMonitoring()
    {
        for (const auto& sliceAddress : m_sliceInstances)
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(sliceAddress.GetReference()->GetSliceAsset().GetId());
        }
    }

    //=========================================================================
    // SlicePushWidget::GetValidTargetAssetsForField
    //=========================================================================
    AZStd::vector<SliceAssetPtr> SlicePushWidget::GetValidTargetAssetsForField(const FieldTreeItem& item) const
    {
        AZStd::vector<SliceAssetPtr> validSliceAssets;
        validSliceAssets.reserve(m_fieldTree->model()->rowCount() * 2);

        const AZ::SliceComponent::EntityAncestorList& ancestors = item.m_entityItem->m_ancestors;
        for (auto iter = ancestors.begin(); iter != ancestors.end(); ++iter)
        {
            auto& ancestor = *iter;
            if (item.AllLeafNodesArePushableToThisAncestor(*ancestor.m_entity))
            {
                validSliceAssets.push_back(ancestor.m_sliceAddress.GetReference()->GetSliceAsset());
            }
        }

        // If the entity doesn't yet belong to a slice instance, allow targeting any of
        // the slices in the current slice push operation.
        if (ancestors.empty())
        {
            QTreeWidgetItemIterator itemIter(m_fieldTree);
            for (; *itemIter; ++itemIter)
            {
                FieldTreeItem* pItem = static_cast<FieldTreeItem*>(*itemIter);
                if (!pItem->parent())
                {
                    for (auto iter = pItem->m_ancestors.begin(); iter != pItem->m_ancestors.end(); ++iter)
                    {
                        auto& ancestor = *iter;
                        const AZ::Data::AssetId& sliceAssetId = ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId();

                        auto assetCompare =
                            [ &sliceAssetId ] (const SliceAssetPtr& asset)
                        {
                            return asset.GetId() == sliceAssetId;
                        };

                        if (validSliceAssets.end() == AZStd::find_if(validSliceAssets.begin(), validSliceAssets.end(), assetCompare))
                        {
                            validSliceAssets.push_back(ancestor.m_sliceAddress.GetReference()->GetSliceAsset());
                        }
                    }
                }
            }
        }

        return validSliceAssets;
    }

    //=========================================================================
    // SlicePushWidget::OnFieldSelectionChanged
    //=========================================================================
    void SlicePushWidget::OnFieldSelectionChanged()
    {
        m_sliceTree->blockSignals(true);

        m_sliceTree->clear();
        m_infoLabel->setText("");

        if (!m_fieldTree->selectedItems().isEmpty())
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front());

            bool isLeaf = (item->childCount() == 0);

            if (item->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
            {
                m_infoLabel->setText(QString("Remove entity \"%1\" from slice:").arg(item->m_entity->GetName().c_str()));
            }
            else if (item->m_slicePushType == FieldTreeItem::SlicePushType::Added)
            {
                m_infoLabel->setText(QString("Add entity \"%1\" %2to slice:").arg(item->m_entity->GetName().c_str()).arg((isLeaf && item->parent() == nullptr) ? "" : "and its hierarchy "));
            }
            else
            {
                m_infoLabel->setText(QString("Choose target slice for %1\"%2\":")
                    .arg(isLeaf ? "" : "children of ")
                    .arg((item->parent() == nullptr) ? item->m_entity->GetName().c_str() : GetNodeDisplayName(*item->m_node).c_str()));
            }

            AZStd::vector<SliceAssetPtr> validSliceAssets = GetValidTargetAssetsForField(*item);

            // For the selected item populate the tree of all valid slice targets.
            size_t level = 0;
            for (const SliceAssetPtr& sliceAsset : validSliceAssets)
            {
                const AZ::Data::AssetId& sliceAssetId = sliceAsset.GetId();

                AZStd::string assetPath;
                AZStd::string itemText;
                EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, sliceAssetId);
                AzFramework::StringFunc::Path::GetFullFileName(assetPath.c_str(), itemText);
                if (itemText.empty())
                {
                    itemText = sliceAssetId.ToString<AZStd::string>();
                }

                // We don't actually embed dependents as children, but just indent them.
                // Overall it's visually more clear alongside the radio buttons.

                SliceTargetTreeItem* sliceItem = new SliceTargetTreeItem(sliceAsset, m_sliceTree);

                QWidget* widget = new QWidget(this);
                QHBoxLayout* advancedPushRowLayout = new QHBoxLayout(this);
                widget->setLayout(advancedPushRowLayout);

                if (level > 0)
                {        
                    QLabel* indentLabel = new QLabel(this);
                    int LShapeIconWidgetWidth = advancedPushRowLayout->contentsMargins().left() + SliceUtilities::GetLShapeIconSize().width() + advancedPushRowLayout->contentsMargins().right();
                    int indentLabelWidth = SliceUtilities::GetSliceHierarchyMenuIdentationPerLevel() + static_cast<int>(level - 1) * LShapeIconWidgetWidth;
                    indentLabel->setFixedSize(indentLabelWidth, SliceUtilities::GetSliceItemHeight());
                    advancedPushRowLayout->addWidget(indentLabel);

                    // Add the L shape icon to show the slice hierarchy
                    QLabel* lShapeIconLabel = new QLabel(this);
                    lShapeIconLabel->setPixmap(QPixmap(SliceUtilities::GetLShapeIconPath()));
                    lShapeIconLabel->setFixedSize(SliceUtilities::GetLShapeIconSize());
                    advancedPushRowLayout->addWidget(lShapeIconLabel);
                }

                QLabel* iconLabel = new QLabel(this);
                QIcon sliceChangedItemIcon(SliceUtilities::GetSliceItemChangedIconPath());
                iconLabel->setPixmap(sliceChangedItemIcon.pixmap(SliceUtilities::GetSliceItemIconSize()));
                iconLabel->setFixedSize(SliceUtilities::GetSliceItemIconSize());
                advancedPushRowLayout->addWidget(iconLabel);

                QLabel* sliceLabel = new QLabel(itemText.c_str(), this);
                advancedPushRowLayout->addWidget(sliceLabel);

                m_sliceTree->setItemWidget(sliceItem, 1, widget);

                const QString tooltip =
                    tr("Check to save field \"") + item->text(0) + tr("\" to asset \"") + assetPath.c_str() + "\".";
                sliceItem->setToolTip(0, tooltip);
                sliceItem->setToolTip(1, tooltip);

                const size_t instanceCount = CalculateReferenceCount(sliceAssetId, m_config->m_rootSlice);
                const AZStd::string instanceCountDescription = AZStd::string::format("%zu", instanceCount);
                sliceItem->setText(2, instanceCountDescription.c_str());

                QRadioButton* selectButton = sliceItem->m_selectButton;
                connect(selectButton, &QRadioButton::toggled, this, [sliceAssetId, this, selectButton] (bool checked) { this->OnSliceRadioButtonChanged(selectButton, sliceAssetId, checked); });
                m_sliceTree->setItemWidget(sliceItem, 0, selectButton);

                // Auto-select the slice item associated with the currently assigned target slice Id.
                if ((item->IsPushableItem() && item->m_selectedAsset == sliceAssetId) || item->AllLeafNodesShareSelectedAsset(sliceAssetId))
                {
                    selectButton->setChecked(true);
                }

                ++level;
            }
        }
        else
        {
            m_infoLabel->setText(tr("Select a leaf field to assign target slice."));
        }

        m_sliceTree->blockSignals(false);

        m_sliceTree->expandAll();
    }

    //=========================================================================
    // SlicePushWidget::ShowConflictMessage
    //=========================================================================
    void SlicePushWidget::ShowConflictMessage()
    {
        int numConflicts = GetConflictCount();
        if (numConflicts > 0)
        {
            QString conflictMsg = QString("<font color=\"red\">%1 ").arg(numConflicts);
            if (numConflicts > 1)
            {
                conflictMsg += QObject::tr("conflicts: ");
            }
            else
            {
                conflictMsg += QObject::tr("conflict: ");
            }
            conflictMsg += "</font>";
            conflictMsg += QObject::tr("You can't save the same component or property override to the same slice. Deselect conflicting overrides before saving.");
            m_conflictTitle->setText(conflictMsg);
            m_conflictWidget->show();
            m_pushSelectedButton->setDisabled(true);
        }
        else
        {
            m_conflictWidget->hide();
            m_pushSelectedButton->setDisabled(false);
        }
    }

    //=========================================================================
    // SlicePushWidget::SetFieldIcon
    //=========================================================================
    void SlicePushWidget::SetFieldIcon(FieldTreeItem* item)
    {
        const Qt::CheckState checkState = item->checkState(0);

        if (item->IsConflicted())
        {
            item->setIcon(0, m_iconConflictedDataItem);
        }
        else
        {
            if (checkState != Qt::CheckState::Checked && item->HasPotentialConflicts())
            {
                item->setIcon(0, m_iconConflictedDisabledDataItem);
            }
            else
            {
                QVariant iconVariant = item->data(0, s_iconStorageRole);
                QIcon icon = iconVariant.value<QIcon>();
                item->setIcon(0, icon);
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::GetConflictCount
    //=========================================================================
    bool SlicePushWidget::GetConflictCount()
    {
        //work out total number of active conflicts, i.e. for checked leaf items only.
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        AZStd::vector<AzToolsFramework::InstanceDataNode::Address> conflictedAddresses;

        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);

            if (item->m_slicePushType != AzToolsFramework::FieldTreeItem::SlicePushType::Changed)
            {
                continue;
            }

            if (item->checkState(0) != Qt::CheckState::Checked)
            {
                continue;
            }

            for (int conflict = 0; conflict < item->m_conflictsWithItems.size(); conflict++)
            {
                FieldTreeItem* conflictedItem = item->m_conflictsWithItems.at(conflict);
                if (conflictedItem->checkState(0) != Qt::CheckState::Checked)
                {
                    continue;
                }
                bool alreadySeen = false;
                for (int addrIndex = 0; addrIndex < conflictedAddresses.size(); addrIndex++)
                {
                    if (conflictedAddresses.at(addrIndex) == conflictedItem->m_address)
                    {
                        alreadySeen = true;
                        break;
                    }
                }
                if (!alreadySeen)
                {
                    conflictedAddresses.push_back(conflictedItem->m_address);
                }
            }
        }
        return !conflictedAddresses.empty();
    }

    //=========================================================================
    // SlicePushWidget::OnFieldDataChanged
    //=========================================================================
    void SlicePushWidget::OnFieldDataChanged(QTreeWidgetItem* item, int column)
    {
        (void)column;

        // The user updated a checkbox for a data field. Propagate to child items.

        FieldTreeItem* fieldItem = static_cast<FieldTreeItem*>(item);

        FieldTreeItem::SlicePushType fieldItemSlicePushType = fieldItem->m_slicePushType;

        const Qt::CheckState checkState = fieldItem->checkState(0);

        SetFieldIcon(fieldItem);

        for (int conflict = 0; conflict < fieldItem->m_conflictsWithItems.size(); conflict++)
        {
            FieldTreeItem* conflictItem = fieldItem->m_conflictsWithItems.at(conflict);
            SetFieldIcon(conflictItem);
        }

        m_fieldTree->blockSignals(true);

        AZStd::vector<FieldTreeItem*> stack;
        for (int i = 0; i < fieldItem->childCount(); ++i)
        {
            stack.push_back(static_cast<FieldTreeItem*>(fieldItem->child(i)));
        }

        while (!stack.empty())
        {
            FieldTreeItem* pItem = stack.back();
            stack.pop_back();

            if (!pItem->isDisabled())
            {
                pItem->setCheckState(0, checkState);
            }

            for (int i = 0; i < pItem->childCount(); ++i)
            {
                stack.push_back(static_cast<FieldTreeItem*>(pItem->child(i)));
            }
        }

        // For additions, we also need to sync "checked" state up the tree and prevent any children 
        // from being added whose parents aren't also added. You can't add a child item and not its parent
        // For removals, we need to sync "unchecked" state up the tree and prevent any parents
        // from being removed whose children aren't also being removed. No orphaning.
        if (fieldItem->m_slicePushType == FieldTreeItem::SlicePushType::Added 
            || fieldItem->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
        {
            FieldTreeItem* hierarchyRootItem = fieldItem;
            FieldTreeItem* currentItem = hierarchyRootItem;
            while (currentItem != nullptr)
            {
                hierarchyRootItem = currentItem;
                if (fieldItem->m_slicePushType == FieldTreeItem::SlicePushType::Added)
                {
                    if (checkState == Qt::CheckState::Checked)
                    {
                        currentItem->setCheckState(0, checkState);
                    }
                }
                else if (fieldItem->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
                {
                    if (checkState == Qt::CheckState::Unchecked)
                    {
                        currentItem->setCheckState(0, checkState);
                    }
                }

                currentItem = static_cast<FieldTreeItem*>(currentItem->parent());
            }

            // Go through all children in this hierarchy and:
            // for addition trees, ensure if a parent is not being added, the child should not be added
            // for removal trees, ensure if a parent is being removed, the children should also be removed
            if (hierarchyRootItem)
            {
                AZStd::vector<FieldTreeItem*> verificationStack;
                for (int i = 0; i < hierarchyRootItem->childCount(); ++i)
                {
                    verificationStack.push_back(static_cast<FieldTreeItem*>(hierarchyRootItem->child(i)));
                }

                while (!verificationStack.empty())
                {
                    FieldTreeItem* pItem = verificationStack.back();
                    verificationStack.pop_back();

                    FieldTreeItem* parentItem = static_cast<FieldTreeItem*>(pItem->parent());
                    if (fieldItem->m_slicePushType == FieldTreeItem::SlicePushType::Added)
                    {
                        if (parentItem->checkState(0) == Qt::CheckState::Unchecked)
                        {
                            pItem->setCheckState(0, Qt::CheckState::Unchecked);
                        }
                    }
                    else if (fieldItem->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
                    {
                        if (parentItem->checkState(0) == Qt::CheckState::Checked)
                        {
                            pItem->setCheckState(0, Qt::CheckState::Checked);
                        }
                    }

                    for (int i = 0; i < pItem->childCount(); ++i)
                    {
                        verificationStack.push_back(static_cast<FieldTreeItem*>(pItem->child(i)));
                    }
                }
            }
        }

        // Test if all items of a FieldTreeItem::SlicePushType have the same Qt::CheckState, if so set their check-all CheckBox to the same state.
        // If the Qt::CheckState of all items are mixed, leave their check-all CheckBox with its original state.
        Qt::CheckState allItemsCheckState = Internal::TestCheckStateOfAllTopLevelItemsBySlicePushType(m_fieldTree, fieldItemSlicePushType);
        if (allItemsCheckState != Qt::CheckState::PartiallyChecked)
        {
            switch (fieldItemSlicePushType)
            {
            case AzToolsFramework::FieldTreeItem::SlicePushType::Changed:
                m_checkboxAllChangedItems->blockSignals(true);
                m_checkboxAllChangedItems->setCheckState(allItemsCheckState);
                m_checkboxAllChangedItems->blockSignals(false);
                break;
            case AzToolsFramework::FieldTreeItem::SlicePushType::Added:
                m_checkboxAllAddedItems->blockSignals(true);
                m_checkboxAllAddedItems->setCheckState(allItemsCheckState);
                m_checkboxAllAddedItems->blockSignals(false);
                break;
            case AzToolsFramework::FieldTreeItem::SlicePushType::Removed:
                m_checkboxAllRemovedItems->blockSignals(true);
                m_checkboxAllRemovedItems->setCheckState(allItemsCheckState);
                m_checkboxAllRemovedItems->blockSignals(false);
                break;
            default:
                break;
            }
        }

        ShowConflictMessage();

        m_fieldTree->blockSignals(false);
    }

    //=========================================================================
    // SlicePushWidget::OnSliceRadioButtonChanged
    //=========================================================================
    void SlicePushWidget::OnSliceRadioButtonChanged(QRadioButton* selectButton, const AZ::Data::AssetId assetId, bool checked)
    {
        if (!checked)
        {
            return;
        }

        // The user selected a new target slice asset. Assign to the select data item and sync related items in the tree

        if (!m_fieldTree->selectedItems().isEmpty())
        {
            SetupConflictData(assetId);

            QTreeWidgetItemIterator itemIter(m_fieldTree);
            for (; *itemIter; ++itemIter)
            {
                FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
                AZ_Assert(item, "FieldTreeItem pointer is null.");
                if (item->IsPushableItem() && item->m_slicePushType == FieldTreeItem::SlicePushType::Changed)
                {
                    SetFieldIcon(item);
                }
            }

            if (m_sliceTree->model()->rowCount() == 1)
            {
                selectButton->blockSignals(true);
                selectButton->setChecked(true);
                selectButton->blockSignals(false);
                return;
            }

            FieldTreeItem* currentItem = static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front());
            AZStd::vector<FieldTreeItem*> stack;
            if (currentItem->m_slicePushType == FieldTreeItem::SlicePushType::Added
                || currentItem->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
            {
                // Adds/removals have their entire hierarchy synced
                FieldTreeItem* highestLevelParent = currentItem;
                while (FieldTreeItem* parentItem = static_cast<FieldTreeItem*>(highestLevelParent->parent()))
                {
                    highestLevelParent = parentItem;
                }
                stack.push_back(highestLevelParent);
            }
            else
            {
                // Other updates sync data to their children only
                stack.push_back(currentItem);
            }

            while (!stack.empty())
            {
                FieldTreeItem* item = stack.back();
                stack.pop_back();

                if (item->IsPushableItem())
                {
                    item->m_selectedAsset = assetId;

                    AZStd::string assetPath;
                    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);

                    item->setText(1, assetPath.c_str());

                    QString tooltip;
                    if (item->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
                    {
                        tooltip = QString("Entity \"%1\" will be deleted from asset \"%2\".").arg(item->text(0)).arg(assetPath.c_str());
                    }
                    else if (item->m_slicePushType == FieldTreeItem::SlicePushType::Added)
                    {
                        tooltip = QString("Entity \"%1\" will be added to asset \"%2\".").arg(item->text(0)).arg(assetPath.c_str());
                    }
                    else
                    {
                        tooltip = QString("Field \"%1\" is assigned to asset \"%2\".").arg(item->text(0)).arg(assetPath.c_str());
                    }

                    item->setToolTip(0, tooltip);
                    item->setToolTip(1, tooltip);
                }

                for (int childIndex = 0, childCount = item->childCount(); childIndex < childCount; ++childIndex)
                {
                    stack.push_back(static_cast<FieldTreeItem*>(item->child(childIndex)));
                }
            }
       }
    }

    //=========================================================================
    // SlicePushWidget::OnPushClicked
    //=========================================================================
    void SlicePushWidget::OnPushClicked()
    {
        auto& assetDb = AZ::Data::AssetManager::Instance();

        // If there were no invalid references, the warning check can be skipped.
        if (m_entityToSliceComponentWithInvalidReferences.size() > 0)
        {
            // Track the number of uniquely referenced invalid slices by using their asset hint in an unordered_set.
            AZStd::unordered_set<AZStd::string> invalidReferences;
            AZStd::unordered_set<AZ::SliceAsset*> targetSlicesWithInvalidReferences;

            // Search through all of the field items to find what is selected.
            for (QTreeWidgetItemIterator itemIter(m_fieldTree); *itemIter; ++itemIter)
            {
                FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
                // Look for each item that has been marked to push.
                if (item->IsPushableItem() && item->checkState(0) == Qt::Checked && item->m_entity != nullptr)
                {
                    // Check if this item was previously marked as having an invalid reference.
                    AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent*>::iterator invalidReferenceIterator =
                        m_entityToSliceComponentWithInvalidReferences.find(item->m_entity->GetId());
                    if (invalidReferenceIterator != m_entityToSliceComponentWithInvalidReferences.end())
                    {
                        // Grab the slice component that had invalid references associated with this field item.
                        AZ::SliceComponent* sliceComponent = invalidReferenceIterator->second;
                        if (sliceComponent == nullptr)
                        {
                            AZ_Error("SlicePushWidget", 
                                false,
                                "Could not find SliceComponent for entity %s. This entity's slice references may not update correctly.",
                                item->m_entity != nullptr ? item->m_entity->GetName().c_str() : "not found");
                            continue;
                        }
                        const AZStd::list<AZ::SliceComponent::SliceReference> &invalidSlices = sliceComponent->GetInvalidSlices();
                        if (invalidSlices.size() > 0)
                        {
                            // Count the unique assets that will be modified in this push.
                            SliceAssetPtr targetSlice = assetDb.FindOrCreateAsset<AZ::SliceAsset>(item->m_selectedAsset, AZ::Data::AssetLoadBehavior::Default);
                            // SliceAssetPtr doesn't work as a key in sets, so grab the raw pointer out of it.
                            targetSlicesWithInvalidReferences.insert(targetSlice.Get());
                            // Count the unique references that exist to invalid slices.
                            for (const AZ::SliceComponent::SliceReference& sliceReference : invalidSlices)
                            {
                                AZStd::string referencedAssetHint = sliceReference.GetSliceAsset().GetHint();
                                invalidReferences.insert(referencedAssetHint);
                            }
                        }
                    }
                }
            }

            // At least one selected item to push has an invalid reference, verify that
            // the user actually wants to remove these references as part of this push.
            if (targetSlicesWithInvalidReferences.size() > 0)
            {
                SliceUtilities::InvalidSliceReferencesWarningResult warningResult =
                    SliceUtilities::DisplayInvalidSliceReferencesWarning(
                        this,
                        targetSlicesWithInvalidReferences.size(),
                        invalidReferences.size(),
                        /*showDetailsButton*/ false);

                switch (warningResult)
                {
                case SliceUtilities::InvalidSliceReferencesWarningResult::Save:
                    // Save selected, continue with the push.
                    break;
                case SliceUtilities::InvalidSliceReferencesWarningResult::Cancel:
                    // Cancel selected, bail.
                    return;
                default:
                    // Somehow the user clicked a button that isn't save or cancel, treat it as cancel.
                    // The warning popup window will already have marked this as an error, so do not throw another error.
                    return;
                }
            }
        }

        // Edit any ChildEntryOrderArrays to remove new entities that aren't going to be in the pushed slice.
        for (QTreeWidgetItemIterator itemIter(m_fieldTree); *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);

            // Skip this if it's a removed node
            if (item->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
            {
                continue;
            }

            if (!item->m_node)
            {
                AZ_Error("SlicePushWidget",
                    false,
                    "FieldTreeItem on entity '%s' has a null node pointer despite not being of SlicePushType Removed (this should not be possible!).\n\nIf this issue persists, delete the instance and recreate it.",
                    item->m_entity != nullptr ? item->m_entity->GetName().c_str() : "(not found)");
                return;
            }

            // Check whether this node is an EditorEntitySortComponent.
            if (item->m_node->GetClassMetadata() && item->m_node->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<EditorEntitySortComponent>::Uuid())
            {
                EditorEntitySortComponent* sortComponent = static_cast<EditorEntitySortComponent*>(item->m_node->FirstInstance());
                if (sortComponent)
                {
                    SliceAssetPtr targetSlice = assetDb.FindOrCreateAsset<AZ::SliceAsset>(item->m_selectedAsset, AZ::Data::AssetLoadBehavior::Default);

                    AzToolsFramework::EntityOrderArray orderArray = sortComponent->GetChildEntityOrderArray();
                    AzToolsFramework::EntityOrderArray prunedOrderArray;
                    prunedOrderArray.reserve(orderArray.size());

                    SliceUtilities::WillPushEntityCallback willPushEntityCallback =
                        [this](const AZ::EntityId entityId, const SliceAssetPtr& assetToPushTo) -> bool
                    {
                        if (WillPushEntityToAsset(entityId, assetToPushTo.GetId()))
                        {
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    };

                    SliceUtilities::RemoveInvalidChildOrderArrayEntries(orderArray, prunedOrderArray, targetSlice, willPushEntityCallback);

                    AZ_Assert(orderArray.size() >= prunedOrderArray.size(), "Pruned OrderArray should not be bigger than unpruned array.");

                    size_t numEntriesRemoved = orderArray.size() - prunedOrderArray.size();

                    if (numEntriesRemoved > 0)
                    {
                        // Replace the order array with the pruned version.
                        sortComponent->SetChildEntityOrderArray(prunedOrderArray);

                        AZ_Warning("Slice Push", false, "%d entries removed from Child Entity Array as they were unselected or invalid.\n", numEntriesRemoved);
                    }
                }
            }
        }

        if (PushSelectedFields())
        {
            emit OnFinished();
        }
    }
    
    //=========================================================================
    // SlicePushWidget::OnCancelClicked
    //=========================================================================
    void SlicePushWidget::OnCancelClicked()
    {
        emit OnCanceled();
    }

    //=========================================================================
    // SlicePushWidget::OnToggleWarningClicked
    //=========================================================================
    void SlicePushWidget::OnToggleWarningClicked()
    {
        if (m_warningFoldout->isHidden())
        {
            m_toggleWarningButton->setPixmap(m_iconOpen.pixmap(m_iconOpen.availableSizes().first()));
            m_warningFoldout->show();
        }
        else
        {
            m_toggleWarningButton->setPixmap(m_iconClosed.pixmap(m_iconClosed.availableSizes().first()));
            m_warningFoldout->hide();
        }
    }

    bool SlicePushWidget::CanPushEntityToAsset(const AZ::EntityId entityId, const AZ::Data::AssetId& assetId)
    {
        return m_unpushableNewChildEntityIdsPerAsset[assetId].find(entityId) == m_unpushableNewChildEntityIdsPerAsset[assetId].end();
    }

    bool SlicePushWidget::WillPushEntityToAsset(const AZ::EntityId entityId, const AZ::Data::AssetId& assetId)
    {
        if (!CanPushEntityToAsset(entityId, assetId))
        {
            return false;
        }

        for (QTreeWidgetItemIterator itemIter(m_fieldTree); *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (!item->IsPushableItem() 
                || item->checkState(0) != Qt::Checked 
                || item->m_slicePushType != FieldTreeItem::SlicePushType::Added)
            {
                continue;
            }

            if (item->m_entity->GetId() != entityId || item->m_selectedAsset != assetId)
            {
                return false;
            }

            return true;
        }

        return false;
    }

    //=========================================================================
    // SlicePushWidget::PopulateFieldTree
    //=========================================================================
    void SlicePushWidget::PopulateFieldTree(const EntityIdList& entities)
    {
        EntityIdSet processEntityIds;
        for (const AZ::EntityId& id : entities)
        {
            processEntityIds.insert(id);
        }

        EntityIdSet unpushableEntityIds;
        EntityIdSet pushableNewChildEntityIds;
        PopulateFieldTreeAddedEntities(processEntityIds, unpushableEntityIds, pushableNewChildEntityIds);

        // Add a root item for each entity:
        // - Pre-calculate the IDH for each entity, comparing against all ancestors.
        // When encountering a pushable leaf (differs from one or more ancestors):
        // - Insert items for all levels of the hierarchy between the entity and the leaf.
        
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> tempCompareEntities;

        for (const AZ::EntityId& entityId : processEntityIds)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

            if (entity)
            {
                AZ::SliceComponent::EntityAncestorList referenceAncestors;
                AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                AZ::SliceComponent::EntityAncestorList entitySliceAncestors;
                if (sliceAddress.IsValid())
                {
                    // If a new slice instance is being added to a slice we do not want any changes to its current ancestor
                    // to show up in this dialog. Any local changes will already be pushed into the slice that the addition is
                    // pushed to. We also do not want removals of entities from the new instance being pushed to show up
                    // for the same reason.
                    if (pushableNewChildEntityIds.find(entityId) != pushableNewChildEntityIds.end())
                    {
                        continue;
                    }

                    // Pre-gather all unique referenced assets.
                    if (m_sliceInstances.end() == AZStd::find(m_sliceInstances.begin(), m_sliceInstances.end(), sliceAddress))
                    {
                        m_sliceInstances.push_back(sliceAddress);
                    }

                    // Any entities that are unpushable don't need any further processing beyond gathering
                    // their slice instance (used in detecting entity removals)
                    if (unpushableEntityIds.find(entityId) != unpushableEntityIds.end())
                    {
                        continue;
                    }

                    sliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, entitySliceAncestors);
                }
                else
                {
                    // Entities not in slices will be handled in PopulateFieldTreeAddedEntities.
                    continue;
                }

                FieldTreeItem* entityItem = FieldTreeItem::CreateEntityRootItem(entity, m_fieldTree);

                // Generate the IDH for the entity.
                entityItem->m_hierarchy.AddRootInstance<AZ::Entity>(entity);

                // Use immediate ancestor as comparison instance.
                AZ::Entity* compareClone = nullptr;
                if (!entitySliceAncestors.empty())
                {
                    const AZ::SliceComponent::Ancestor& entitySliceAncestor = entitySliceAncestors.front();

                    tempCompareEntities.push_back(
                        AzToolsFramework::SliceUtilities::CloneSliceEntityForComparison(*entitySliceAncestor.m_entity, *sliceAddress.GetInstance(), *m_serializeContext));

                    compareClone = tempCompareEntities.back().get();
                    entityItem->m_hierarchy.AddComparisonInstance<AZ::Entity>(compareClone);
                }

                entityItem->m_hierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
                entityItem->m_ancestors = AZStd::move(entitySliceAncestors);
                entityItem->m_sliceAddress = sliceAddress;
                
                entityItem->setText(0, QString("%1 (changed)").arg(entity->GetName().c_str()));
                entityItem->setIcon(0, m_iconChangedDataItem);
                entityItem->setData(0, s_iconStorageRole, m_iconChangedDataItem);
                entityItem->setCheckState(0, Qt::CheckState::Checked);

                AZStd::unordered_map<AzToolsFramework::InstanceDataNode*, FieldTreeItem*> nodeItemMap;
                nodeItemMap[&entityItem->m_hierarchy] = entityItem;

                AZStd::vector<AzToolsFramework::InstanceDataNode*> nodeStack;
                AZStd::vector<FieldTreeItem*> itemStack;

                // For determining whether an entity is a root entity of a slice, we check its live version 
                // and if available the comparison instance root. We check the comparison because it's the asset
                // version of the slice - the live version may have a parentId to it (slice is childed to another entity)
                // which would fail the RootEntity check, but the clone will be free of those modifications
                AZ_Assert(m_config->m_isRootEntityCB, "No IsRootEntity callback configured!");
                const bool isRootEntity = m_config->m_isRootEntityCB(entity) || (compareClone && m_config->m_isRootEntityCB(compareClone));
                bool entityHasPushableChanges = false;

                //
                // Recursively traverse IDH and add all modified fields to the tree.
                //

                // Add the children in reverse order to the stack so that they get popped of it in the original order.
                // Otherwise we see the children in the wrong order in the dialog (which looks bad for vector elements)
                // and it actually causes bugs because we end up committing vector elements in reverse order which fails
                // for multiple adds (when not using persistent IDs)
                AZStd::list<InstanceDataNode>& children = entityItem->m_hierarchy.GetChildren();
                for (auto hierarchyItemChild = children.rbegin(); hierarchyItemChild != children.rend(); ++hierarchyItemChild)
                {
                    InstanceDataNode& child = *hierarchyItemChild;
                    nodeStack.push_back(&child);
                }

                while (!nodeStack.empty())
                {
                    InstanceDataNode* node = nodeStack.back();
                    nodeStack.pop_back();

                    // Determine if this node should be pushed and not displayed (which applies to all child nodes)
                    // Because they are hidden pushes (used by hidden editor-only components that users shouldn't
                    // have to even know about in the push UI) we don't want to count them as "pushable changes" -
                    // it would be confusing to see that you can push changes to an entity but you can't see what
                    // any of those changes are!
                    {
                        const AZ::u32 sliceFlags = SliceUtilities::GetNodeSliceFlags(*node);

                        bool hidden = false;
                        if ((sliceFlags & AZ::Edit::SliceFlags::HideOnAdd) != 0 && node->IsNewVersusComparison())
                        {
                            hidden = true;
                        }

                        if ((sliceFlags & AZ::Edit::SliceFlags::HideOnChange) != 0 && node->IsDifferentVersusComparison())
                        {
                            hidden = true;
                        }

                        if ((sliceFlags & AZ::Edit::SliceFlags::HideOnRemove) != 0 && node->IsRemovedVersusComparison())
                        {
                            hidden = true;
                        }

                        if ((sliceFlags & AZ::Edit::SliceFlags::HideAllTheTime) != 0)
                        {
                            hidden = true;
                        }

                        if (hidden && (sliceFlags & AZ::Edit::SliceFlags::PushWhenHidden) != 0)
                        {
                            entityItem->m_hiddenAutoPushNodes.insert(node);
                        }
                    }

                    if (!SliceUtilities::IsNodePushable(*node, isRootEntity))
                    {
                        continue;
                    }

                    // This works around bugs in the serialization system where multiple changes to a container being pushed in one transaction
                    // would cause addresses to be re-generated, causing the cached addresses for other changes in the transaction to resolve incorrectly.
                    // This also provides a cleaner interface, often times the interface for changes to containers wouldn't match what users expected.
                    // This also prevents users from creating strange situations in slices, by making big changes to a container and only partially pushing those changes.
                    const bool isContainer = node->GetClassMetadata() ? node->GetClassMetadata()->m_container != nullptr : false;
                    bool childrenHavePersistentId = true;
                    if (isContainer && node->GetChildren().size() > 0)
                    {
                        // Check if the first child of this container has a persistent ID.
                        InstanceDataNode& firstChild = node->GetChildren().front();
                        childrenHavePersistentId = firstChild.GetClassMetadata() &&
                            firstChild.GetClassMetadata()->GetPersistentId(*m_serializeContext);
                    }
                    // Only add children as individual items if they have persistent IDs.
                    if (childrenHavePersistentId)
                    {
                        // Add the children in reverse order to the stack so that they get popped of it in the original order
                        for (auto riter = node->GetChildren().rbegin(); riter != node->GetChildren().rend(); ++riter)
                        {
                            InstanceDataNode& child = *riter;
                            nodeStack.push_back(&child);
                        }
                    }
                    // If this node is a container, and the children do not have persistent IDs, then
                    // check for changes in the node recursively. This will make it so any changes to
                    // the contents of a container force the entire container to be pushed.
                    const bool checkForChangesRecursively = isContainer && !childrenHavePersistentId;
                    if (node->HasChangesVersusComparison(checkForChangesRecursively))
                    {
                        AZStd::vector<AzToolsFramework::InstanceDataNode*> walkStack;
                        walkStack.push_back(node);

                        AzToolsFramework::InstanceDataNode* runner = node->GetParent();
                        while (runner)
                        {
                            walkStack.push_back(runner);
                            runner = runner->GetParent();
                        }

                        while (!walkStack.empty())
                        {
                            AzToolsFramework::InstanceDataNode* walkNode = walkStack.back();
                            walkStack.pop_back();

                            if (nodeItemMap.find(walkNode) != nodeItemMap.end())
                            {
                                continue;
                            }

                            // Use the same visibility determination as the inspector.
                            AzToolsFramework::NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*walkNode, true);

                            if (visibility == AzToolsFramework::NodeDisplayVisibility::Visible)
                            {
                                AZStd::string fieldName = GetNodeDisplayName(*walkNode);

                                if (!fieldName.empty())
                                {
                                    // Find closest ancestor with a display item.
                                    AzToolsFramework::InstanceDataNode* parent = walkNode->GetParent();
                                    FieldTreeItem* displayParent = entityItem;
                                    while (parent)
                                    {
                                        auto iter = nodeItemMap.find(parent);
                                        if (iter != nodeItemMap.end())
                                        {
                                            displayParent = iter->second;
                                            break;
                                        }
                                        parent = parent->GetParent();
                                    }

                                    FieldTreeItem* item = FieldTreeItem::CreateFieldItem(walkNode, entityItem, displayParent);
                                    item->setText(0, fieldName.c_str());
                                    item->setData(0, s_iconStorageRole, m_iconChangedDataItem);
                                    item->setCheckState(0, Qt::CheckState::Checked);
                                    item->setIcon(0, m_iconGroup);
                                    item->m_sliceAddress = sliceAddress;
                                    nodeItemMap[walkNode] = item;

                                    entityHasPushableChanges = true;
                                }
                            }
                        }
                    }
                }

                const bool hasInvalidReferences = ProcessInvalidReferences(entityItem);
                if (hasInvalidReferences)
                {
                    // if the slice entity has invalid reference(s), the child sort order needs to be converted into
                    // a hidden auto push otherwise it can easily get out of sync 
                    using NodeItemPair = AZStd::pair<AzToolsFramework::InstanceDataNode*, FieldTreeItem*>;

                    auto childSortNodeItem = AZStd::find_if(nodeItemMap.begin(), nodeItemMap.end(),
                        [](NodeItemPair pair)
                        {
                            return (pair.first->GetClassMetadata() 
                                && pair.first->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<EditorEntitySortComponent>::Uuid());
                        });
                    if (childSortNodeItem != nodeItemMap.end())
                    {
                        entityItem->m_hiddenAutoPushNodes.insert(childSortNodeItem->first);

                        QTreeWidgetItem* removeWidget = childSortNodeItem->second;
                        while (removeWidget)
                        {
                            nodeItemMap.erase(AZStd::remove_if(nodeItemMap.begin(), nodeItemMap.end(),
                                [removeWidget](NodeItemPair pair)
                                {
                                    return (pair.second == removeWidget);
                                }),
                                nodeItemMap.end());

                            QTreeWidgetItem* parent = removeWidget->parent();
                            delete removeWidget;
                            removeWidget = nullptr;

                            if (parent && parent != entityItem && parent->childCount() == 0)
                            {
                                removeWidget = parent;
                            }
                        }
                    }

                    if (nodeItemMap.size() == 1)
                    {
                        entityItem->setText(0, QString(tr("%1 (invalid references will be removed)")).arg(entity->GetName().c_str()));
                        entityHasPushableChanges = true;
                    }
                }

                if (!entityHasPushableChanges)
                {
                    // Don't display the entity if no pushable changes were present.
                    delete entityItem;
                }
            }
        }

        PopulateFieldTreeRemovedEntities();
    }

    void SlicePushWidget::AddWarning(FieldTreeItem* entityItem, const QString& sliceFile, const QString& message)
    {
        if (entityItem == nullptr)
        {
            return;
        }
        QStringList messageList;

        messageList << sliceFile;
        messageList << message;
        QTreeWidgetItem* warningItem = new QTreeWidgetItem(messageList);
        m_warningMessages->insertTopLevelItem(0, warningItem);
        m_warningsToEntityTree[warningItem] = entityItem;
    }

    bool SlicePushWidget::ProcessInvalidReferences(FieldTreeItem* entityItem)
    {
        if (entityItem == nullptr)
        {
            return false;
        }
        bool result = false;
        for (const AZ::SliceComponent::Ancestor& ancestor : entityItem->m_ancestors)
        {
            SliceAssetPtr sliceAsset = ancestor.m_sliceAddress.GetReference()->GetSliceAsset();
            AZ::SliceComponent* referencedComponent = sliceAsset.Get()->GetComponent();
            const AZ::SliceComponent::SliceList& invalidSlices = referencedComponent->GetInvalidSlices();
            
            for (const AZ::SliceComponent::SliceReference& sliceReference : invalidSlices)
            {
                QString referencedAssetHint = sliceReference.GetSliceAsset().GetHint().c_str();
                AddWarning(
                    entityItem,
                    QString(sliceAsset.GetHint().c_str()),
                    tr("The %1 entity references an invalid slice: %2.").arg(
                        entityItem->m_entity->GetName().c_str(),
                        referencedAssetHint));
                result = true;
            }
            if (result && entityItem->m_entity)
            {
                m_entityToSliceComponentWithInvalidReferences[entityItem->m_entity->GetId()] = referencedComponent;
            }
        }
        return result;
    }

    using GetParentIdCB = AZStd::function<AZ::EntityId(const AZ::EntityId& entityId, FieldTreeItem* fieldTreeItem)>;
    void InsertHierarchicalItemsToFieldTree(const AZStd::unordered_map<AZ::EntityId, FieldTreeItem*>& entityToFieldTreeItemMap, const GetParentIdCB& getParentCB, QTreeWidget* fieldTree, AZStd::unordered_set<AZ::EntityId>& rootLevelEntitiesOut)
    {
        for (auto& entityItemPair : entityToFieldTreeItemMap)
        {
            AZ::EntityId entityId = entityItemPair.first;
            FieldTreeItem* entityItem = entityItemPair.second;
            AZ::EntityId parentId = getParentCB(entityId, entityItem);
            auto parentIt = entityToFieldTreeItemMap.find(parentId);
            if (parentIt != entityToFieldTreeItemMap.end())
            {
                FieldTreeItem* parentItem = parentIt->second;
                parentItem->addChild(entityItem);
            }
            else
            {
                fieldTree->insertTopLevelItem(0, entityItem);
                rootLevelEntitiesOut.insert(entityId);
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::PopulateFieldTreeAddedEntities
    //=========================================================================
    void SlicePushWidget::PopulateFieldTreeAddedEntities(const EntityIdSet& entities, EntityIdSet& unpushableNewChildEntityIds, EntityIdSet& pushableNewChildEntityIds)
    {
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;
        AZStd::unordered_map<AZ::EntityId, FieldTreeItem*> entityToFieldTreeItemMap;
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> unpushableChildEntityIdAncestorPairs; 

        AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList> sliceAncestryMapping;
        AzToolsFramework::EntityIdList entityIdList;
        for (const AZ::EntityId& entityId : entities)
        {
            entityIdList.emplace_back(entityId);
        }

        pushableNewChildEntityIds = SliceUtilities::GetPushableNewChildEntityIds(entityIdList, m_unpushableNewChildEntityIdsPerAsset, sliceAncestryMapping, newChildEntityIdAncestorPairs, m_newEntityIds);

        for (const auto& unpushableEntityIds : m_unpushableNewChildEntityIdsPerAsset)

        {
            for (const AZ::EntityId& entityId : unpushableEntityIds.second)
            {
                if (unpushableNewChildEntityIds.find(entityId) == unpushableNewChildEntityIds.end())
                {
                    unpushableNewChildEntityIds.insert(entityId);
                }
            }
        }
        
        bool hasUnpushableTransformDescendants = false;
        for (const auto& entityIdAncestorPair: newChildEntityIdAncestorPairs)
        {
            const AZ::EntityId& entityId = entityIdAncestorPair.first;
            const AZ::SliceComponent::EntityAncestorList& ancestors = entityIdAncestorPair.second;

            //check for entities that are unpushable
            if (!ancestors.empty())
            {
                AZ::SliceComponent::Ancestor rootAncestor = ancestors.back();
                AZ::Data::AssetId rootId = rootAncestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId();
                if (!CanPushEntityToAsset(entityId, rootId))
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

                    FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity);
                    item->setText(0, QString("%1 (added, unsaveable [see (A) below])").arg(entity->GetName().c_str()));
                    item->setIcon(0, m_iconNewDataItem);
                    item->m_ancestors = AZStd::move(sliceAncestryMapping[entityId]);
                    item->setCheckState(0, Qt::CheckState::Unchecked);
                    item->setDisabled(true);
                    entityToFieldTreeItemMap[entity->GetId()] = item;
                    continue;
                }
            }

            // Test if the newly added loose entity is a descendant of an entity that's unpushable, if so mark it as unpushable too.
            AZ::EntityId unpushableAncestorId(AZ::EntityId::InvalidEntityId);
            AZ::EntityId parentId;
            AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entityId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
            while (parentId.IsValid())
            {
                EntityIdSet::iterator foundItr = unpushableNewChildEntityIds.find(parentId);
                if (foundItr != unpushableNewChildEntityIds.end())
                {
                    unpushableAncestorId = *foundItr;
                    break;
                }

                AZ::EntityId currentParentId = parentId;
                parentId.SetInvalid();
                AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, currentParentId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
            }

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

            if (unpushableAncestorId.IsValid())
            {
                hasUnpushableTransformDescendants = true;

                AZStd::string parentName;
                AZ::ComponentApplicationBus::BroadcastResult(parentName, &AZ::ComponentApplicationRequests::GetEntityName, parentId);

                FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity);
                item->setText(0, QString("%1 (added, unsaveable [see (B) below])").arg(entity->GetName().c_str()));
                item->setIcon(0, m_iconNewDataItem);
                item->setData(0, s_iconStorageRole, m_iconNewDataItem);
                item->m_ancestors = AZStd::move(ancestors);
                item->setCheckState(0, Qt::CheckState::Unchecked);
                item->setDisabled(true);
                entityToFieldTreeItemMap[entity->GetId()] = item;
            }
            else
            {
                // Create field entry to add this entity to the asset.
                FieldTreeItem* item = FieldTreeItem::CreateEntityAddItem(entity);
                item->setText(0, QString("%1 (added)").arg(entity->GetName().c_str()));
                item->setIcon(0, m_iconNewDataItem);
                item->setData(0, s_iconStorageRole, m_iconNewDataItem);

                //add any potential ancestors that aren't in the unpushable list for this entity
                for (auto ancestor : ancestors)
                {
                    AZ::Data::AssetId id = ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId();

                    if (CanPushEntityToAsset(entityId, id))
                    {
                        item->m_ancestors.push_back(ancestor);
                    }
                }

                item->setCheckState(0, m_config->m_defaultAddedEntitiesCheckState ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
                entityToFieldTreeItemMap[entity->GetId()] = item;
            }
        }

        // We set up the "add tree" so that you can see the transform hierarchy you're pushing in the push widget
        auto getParentIdCB = [](const AZ::EntityId& entityId, FieldTreeItem* fieldTreeItem) -> AZ::EntityId
        {
            (void)fieldTreeItem;

            AZ::EntityId parentId;
            AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entityId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
            return parentId;
        };
        AZStd::unordered_set<AZ::EntityId> rootLevelAdditions;
        InsertHierarchicalItemsToFieldTree(entityToFieldTreeItemMap, getParentIdCB, m_fieldTree, rootLevelAdditions);

        // Show bottom warnings explaining unpushables
        {
            if (unpushableNewChildEntityIds.size() > 0)
            {
                AddStatusMessage(AzToolsFramework::SlicePushWidget::StatusMessageType::Warning, 
                                    QObject::tr("(A) Invalid additions detected. These are unsaveable because slices cannot contain instances of themselves. "
                                        "Saving these additions would create a cyclic asset dependency, causing infinite recursion."));
            }
            if (hasUnpushableTransformDescendants)
            {
                AddStatusMessage(AzToolsFramework::SlicePushWidget::StatusMessageType::Warning, 
                                    QObject::tr("(B) Invalid additions detected. These are unsaveable because they are transform children/descendants of other "
                                                "invalid-to-add entities."));
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::PopulateFieldTreeRemovedEntities
    //=========================================================================
    void SlicePushWidget::PopulateFieldTreeRemovedEntities()
    {
        AZStd::unordered_map<AZ::EntityId, FieldTreeItem*> entityToFieldTreeItemMap;
        SliceUtilities::IdToEntityMapping assetEntityIdtoAssetEntityMapping;
        SliceUtilities::IdToInstanceAddressMapping assetEntityIdtoInstanceAddressMapping;
        AZStd::unordered_set<AZ::EntityId> uniqueRemovedEntities = SliceUtilities::GetUniqueRemovedEntities(m_sliceInstances, assetEntityIdtoAssetEntityMapping, assetEntityIdtoInstanceAddressMapping);

        for (const AZ::EntityId& entityId : uniqueRemovedEntities)
        {
            AZ::SliceComponent::SliceInstanceAddress instanceAddr = assetEntityIdtoInstanceAddressMapping[entityId];
            AZ::Entity* assetEntity = assetEntityIdtoAssetEntityMapping[entityId];

            // Create field entry to remove this entity from the asset.
            FieldTreeItem* item = FieldTreeItem::CreateEntityRemovalItem(assetEntity, instanceAddr.GetReference()->GetSliceAsset(), instanceAddr);
            item->setText(0, QString("%1 (deleted)").arg(assetEntity->GetName().c_str()));
            item->setIcon(0, m_iconRemovedDataItem);
            item->setData(0, s_iconStorageRole, m_iconRemovedDataItem);
            item->setCheckState(0, m_config->m_defaultRemovedEntitiesCheckState ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
            item->m_ancestors.push_back(AZ::SliceComponent::Ancestor(assetEntity, instanceAddr));

            AZ::SliceComponent::SliceInstanceAddress assetInstanceAddress = instanceAddr.GetReference()->GetSliceAsset().Get()->GetComponent()->FindSlice(entityId);
            if (assetInstanceAddress.IsValid())
            {
                assetInstanceAddress.GetReference()->GetInstanceEntityAncestry(entityId, item->m_ancestors);
            }
            else
            {
                // This is a loose entity of the slice
                instanceAddr.GetReference()->GetInstanceEntityAncestry(entityId, item->m_ancestors);
            }

            entityToFieldTreeItemMap[entityId] = item;
        }

        // We set up the "removal tree" so that you can see the removal transform hierarchy you're pushing in the push widget
        auto getParentIdCB = [](const AZ::EntityId& entityId, FieldTreeItem* fieldTreeItem) -> AZ::EntityId
        {
            (void)entityId;

            // These are asset entities - not live entities, so we need to retrieve their parent id manually (they're not connected to buses)
            AZ::EntityId parentId;
            AZ::SliceEntityHierarchyInterface* sliceHierarchyInterface = AZ::EntityUtils::FindFirstDerivedComponent<AZ::SliceEntityHierarchyInterface>(fieldTreeItem->m_entity);
            if (sliceHierarchyInterface)
            {
                parentId = sliceHierarchyInterface->GetSliceEntityParentId();
            }
            else
            {
                AZ_Warning("Slice Push", false, "Attempting to get SliceEntityHierarchy parent id from entity that has no SliceEntityHierarchyInterface.");
            }
            return parentId;
        };
        AZStd::unordered_set<AZ::EntityId> rootLevelRemovals;
        InsertHierarchicalItemsToFieldTree(entityToFieldTreeItemMap, getParentIdCB, m_fieldTree, rootLevelRemovals);

        // For removal hierarchies we sync up target slice assets, so that if you're removing an entity in a transform hierarchy of
        // removals, any other removed entities also get removed from the same target asset. This constraint in the push UI is to help 
        // prevent accidentally orphaning entities in referenced slice assets. This isn't fool-proof yet, since if you re-arrange your entity
        // transform hierarchy with intermixed slices, you could still end up orphaning, but so far we've found most people
        // naturally keep slices and transform hierarchy pretty well in sync. The "target slice asset sync" is done in OnSliceRadioButtonChanged,
        // right here we need to make sure a given removal transform hierarchy shares a common set of ancestry to push to
        {
            for (auto& rootLevelRemovalEntityId : rootLevelRemovals)
            {
                // First, find the common set of ancestors that all removals in this hierarchy share
                AZStd::unordered_set<AZ::SliceComponent::SliceInstanceAddress> commonSlices;
                FieldTreeItem* rootFieldItem = entityToFieldTreeItemMap[rootLevelRemovalEntityId];
                {
                    for (auto& rootLevelAncestor : rootFieldItem->m_ancestors)
                    {
                        commonSlices.insert(rootLevelAncestor.m_sliceAddress);
                    }

                    AZStd::vector<FieldTreeItem*> stack;
                    for (int i = 0; i < rootFieldItem->childCount(); ++i)
                    {
                        stack.push_back(static_cast<FieldTreeItem*>(rootFieldItem->child(i)));
                    }

                    while (!stack.empty())
                    {
                        FieldTreeItem* item = stack.back();
                        stack.pop_back();

                        // Remove any items from the commonSlices that this child does not have
                        for (auto commonSliceIt = commonSlices.begin(); commonSliceIt != commonSlices.end(); )
                        {
                            bool found = false;
                            for (auto& ancestor : item->m_ancestors)
                            {
                                if (*commonSliceIt == ancestor.m_sliceAddress)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if (found)
                            {
                                ++commonSliceIt;
                            }
                            else
                            {
                                commonSliceIt = commonSlices.erase(commonSliceIt);
                            }
                        }

                        for (int i = 0; i < item->childCount(); ++i)
                        {
                            stack.push_back(static_cast<FieldTreeItem*>(item->child(i)));
                        }
                    }
                }

                // Next, cull all non-common ancestors from everyone in the hierarchy
                {
                    AZStd::vector<FieldTreeItem*> stack;
                    stack.push_back(rootFieldItem);

                    while (!stack.empty())
                    {
                        FieldTreeItem* item = stack.back();
                        stack.pop_back();

                        for (auto ancestorIt = item->m_ancestors.begin(); ancestorIt != item->m_ancestors.end(); )
                        {
                            auto foundIt = commonSlices.find(ancestorIt->m_sliceAddress);
                            if (foundIt == commonSlices.end())
                            {
                                ancestorIt = item->m_ancestors.erase(ancestorIt);
                            }
                            else
                            {
                                ++ancestorIt;
                            }
                        }

                        for (int i = 0; i < item->childCount(); ++i)
                        {
                            stack.push_back(static_cast<FieldTreeItem*>(item->child(i)));
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // SlicePushWidget::CalculateReferenceCount
    //=========================================================================
    size_t SlicePushWidget::CalculateReferenceCount(const AZ::Data::AssetId& assetId, const AZ::SliceComponent* levelSlice)
    {
        // This is an approximate measurement of how much this slice proliferates within the currently-loaded level.
        // Down the line we'll actually query the asset DB's dependency tree, summing up instances.
        AZ_UNUSED(levelSlice); // Prevent unused warning in release builds
        AZ_Warning("SlicePush", levelSlice, "SlicePushWidget::CalculateReferenceCount could not find root slice, displayed counts will be inaccurate!");
        size_t instanceCount = 0;
        AZ::Data::AssetBus::EnumerateHandlersId(assetId,
            [&instanceCount, assetId] (AZ::Data::AssetEvents* handler) -> bool
            {
                AZ::SliceComponent* component = azrtti_cast<AZ::SliceComponent*>(handler);
                if (component)
                {
                    AZ::SliceComponent::SliceReference* reference = component->GetSlice(assetId);

                    size_t count = reference->GetInstances().size();

                    instanceCount += count;
                }
                return true;
            });

        return instanceCount;
    }
    
    //=========================================================================
    // SlicePushWidget::CloneAsset
    //=========================================================================
    SliceAssetPtr SlicePushWidget::CloneAsset(const SliceAssetPtr& asset) const
    {
        AZ::Entity* clonedAssetEntity = aznew AZ::Entity(asset.Get()->GetEntity()->GetId());
        AZ::SliceComponent* clonedSlice = asset.Get()->GetComponent()->Clone(*m_serializeContext);
        clonedAssetEntity->AddComponent(clonedSlice);
        SliceAssetPtr clonedAsset(aznew AZ::SliceAsset(asset.GetId()), AZ::Data::AssetLoadBehavior::Default);
        clonedAsset.Get()->SetData(clonedAssetEntity, clonedSlice);
        return clonedAsset;
    }

    //=========================================================================
    // SlicePushWidget::PushSelectedFields
    //=========================================================================
    bool SlicePushWidget::PushSelectedFields()
    {
        // Identify all assets affected by the push, and check them out.
        AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr> uniqueAffectedAssets;
        if (!CheckoutAndGatherAffectedAssets(uniqueAffectedAssets))
        {
            return false;
        }

        using namespace AzToolsFramework::SliceUtilities;

        // Start a push transaction for each asset.
        AZStd::unordered_map<AZ::Data::AssetId, SliceTransaction::TransactionPtr> transactionMap;
        for (const auto& pair : uniqueAffectedAssets)
        {
            const SliceAssetPtr& asset = pair.second;
            transactionMap[asset.GetId()] = SliceTransaction::BeginSlicePush(asset);
            if (!transactionMap[asset.GetId()])
            {
                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString("Unable to create transaction for slice asset %1").arg(asset.ToString<AZStd::string>().c_str()),
                                     QMessageBox::Ok);
                return false;
            }
        }

        QString pushError;
        AZStd::unordered_map<AZ::Data::AssetId, EntityIdList> entitiesToRemove; // collection of entities to remove per slice asset transaction completed

        AZStd::vector<FieldTreeItem*> removalNodes;

        // Iterate over selected fields and synchronize data to target.
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (item->IsPushableItem())
            {
                if (item->checkState(0) == Qt::Checked)
                {
                    SliceTransaction::TransactionPtr transaction = transactionMap[item->m_selectedAsset];

                    // Add entity
                    if (item->m_slicePushType == FieldTreeItem::SlicePushType::Added)
                    {
                        AZ::EntityId parentId;
                        AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, item->m_entity->GetId(), &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                        if (!parentId.IsValid())
                        {
                            pushError = QString("A new root cannot be pushed to an existing slice. New entities should be added as children or descendants of the slice root.");
                            break;
                        }

                        entitiesToRemove[item->m_selectedAsset].push_back(item->m_entity->GetId());
                        SliceTransaction::Result result = transaction->AddEntity(item->m_entity);
                        if (!result)
                        {
                            pushError = QString("Entity \"%1\" could not be added to the target slice.\r\n\r\nDetails:\r\n%2")
                                .arg(item->m_entity->GetName().c_str()).arg(result.GetError().c_str());
                            break;
                        }
                    }
                    // Remove entity
                    else if (item->m_slicePushType == FieldTreeItem::SlicePushType::Removed)
                    {
                        // Need to provide the transaction with the correct entity to remove based on target slice asset
                        AZ::Entity* removalEntity = item->m_entity;
                        for (auto& ancestor : item->m_ancestors)
                        {
                            if (ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId() == item->m_selectedAsset)
                            {
                                removalEntity = ancestor.m_entity;
                            }
                        }

                        SliceTransaction::Result result = transaction->RemoveEntity(removalEntity);
                        if (!result)
                        {
                            pushError = QString("Entity \"%1\" could not be removed from the target slice.\r\n\r\nDetails:\r\n%2")
                                .arg(item->m_entity->GetName().c_str()).arg(result.GetError().c_str());
                            break;
                        }
                    }
                    // Update entity field
                    else
                    {
                        // Add all removed nodes in reverse order later, so nodes can be removed in reverse order. 
                        // Because removing nodes in a vector in the original order is unstable (each removal will cause subsequent nodes being shifted).
                        if (item->m_node->IsRemovedVersusComparison())
                        {
                            removalNodes.push_back(item);
                            continue;
                        }

                        pushError = AzToolsFramework::Internal::PushChangedItemToSliceTransaction(transaction, item);
                        if (!pushError.isEmpty())
                        {
                            break;
                        }
                    }
                }
            }
        }

        for (auto removalNodesItr = removalNodes.rbegin(); removalNodesItr != removalNodes.rend(); ++removalNodesItr)
        {
            FieldTreeItem* item = *removalNodesItr;
            SliceTransaction::TransactionPtr transaction = transactionMap[item->m_selectedAsset];

            pushError = AzToolsFramework::Internal::PushChangedItemToSliceTransaction(transaction, item);
            if (!pushError.isEmpty())
            {
                break;
            }
        }

        if (!pushError.isEmpty())
        {
            QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                 QString("Internal during slice push. No assets have been affected.\r\n\r\nDetails:\r\n%1").arg(pushError),
                                 QMessageBox::Ok);
            return false;
        }

        // If multiple slices are being pushed to and an error ocurrs in the commit of the second or later commits then the asset monitoring
        // would report a spurious error. Fix by stopping the asset monitoring before the commits are started
        // (and turning it back on if we go back to the dialog after an error).
        StopAssetMonitoring();

        // Commit all transactions.
        int transactionIndex = 0;
        for (auto& transactionPair : transactionMap)
        {
            ScopedUndoBatch undoBatch("Slice Push (Advanced)");

            SliceTransaction::TransactionPtr& transaction = transactionPair.second;

            bool undoSliceOverrideValue;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(undoSliceOverrideValue, &AzToolsFramework::EditorRequests::GetUndoSliceOverrideSaveValue);
            AZ::u32 sliceCommitFlags = 0;

            if (!undoSliceOverrideValue)
            {
                sliceCommitFlags = SliceTransaction::SliceCommitFlags::DisableUndoCapture;
            }

            SliceTransaction::Result result = transaction->Commit(
                transactionPair.first,
                m_config->m_preSaveCB, 
                m_config->m_postSaveCB,
                sliceCommitFlags);

            if (result)
            {
                // Successful commit
                // Remove any entities that were successfully pushed into a slice (since they'll be brought to life through slice reloading)
                if (m_config->m_deleteEntitiesCB)
                {
                    m_config->m_deleteEntitiesCB(entitiesToRemove[transactionPair.first]);
                }
            }
            else
            {
                // Unsuccessful commit
                AZStd::string sliceAssetPath;
                EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, transactionPair.first);
                if (sliceAssetPath.empty())
                {
                    sliceAssetPath = transactionPair.first.ToString<AZStd::string>();
                }

                QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"),
                                     QString("Unable to commit changes for slice asset %1.\r\n\r\nDetails:\r\n%2")
                                     .arg(sliceAssetPath.c_str()).arg(result.GetError().c_str()),
                                     QMessageBox::Ok);

                if (transactionIndex == 0)
                {
                    // If our first transaction fails, we want to return false so that the Push widget remains open and the user
                    // has the chance to reconfigure and repush the changes. So, we restart the asset monitoring as the widget
                    // is not going away.
                    SetupAssetMonitoring();
                    return false;
                }
                else
                {
                    // If we're failing on a transaction after the first, we need to return true so that the PushWidget closes 
                    // since there will now have been changes that will make the previous configuration of the widget invalid.
                    return true;
                }
            }

            ++transactionIndex;
        }

        return true;
    }

    //=========================================================================
    // SlicePushWidget::CheckoutAndGatherAffectedAssets
    //=========================================================================
    bool SlicePushWidget::CheckoutAndGatherAffectedAssets(AZStd::unordered_map<AZ::Data::AssetId, SliceAssetPtr>& assets)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("Slice Push", false, "File IO is not initialized.");
            return false;
        }

        auto& assetDb = AZ::Data::AssetManager::Instance();

        size_t pendingCheckouts = 0;
        bool checkoutSuccess = true;

        // Iterate over selected leaves. Identify all target slice assets, and check them out.
        QTreeWidgetItemIterator itemIter(m_fieldTree);
        for (; *itemIter; ++itemIter)
        {
            FieldTreeItem* item = static_cast<FieldTreeItem*>(*itemIter);
            if (item->IsPushableItem())
            {
                if (item->checkState(0) == Qt::Checked)
                {
                    SliceAssetPtr asset = assetDb.FindOrCreateAsset<AZ::SliceAsset>(item->m_selectedAsset, AZ::Data::AssetLoadBehavior::Default);

                    if (assets.find(asset.GetId()) != assets.end())
                    {
                        continue;
                    }

                    AZStd::string sliceAssetPath;
                    EBUS_EVENT_RESULT(sliceAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());

                    if (sliceAssetPath.empty())
                    {
                        QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"), 
                            QString("Failed to resolve path for asset \"%1\".").arg(asset.GetId().ToString<AZStd::string>().c_str()), 
                            QMessageBox::Ok);

                        return false;
                    }

                    assets[asset.GetId()] = asset;

                    AZStd::string assetFullPath;
                    EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, sliceAssetPath, assetFullPath);
                    if (fileIO->IsReadOnly(assetFullPath.c_str()))
                    {
                        // Issue checkout order for each affected slice.
                        ++pendingCheckouts;

                        using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;

                        ApplicationBus::Broadcast(&ApplicationBus::Events::CheckSourceControlConnectionAndRequestEditForFile,
                            assetFullPath.c_str(),
                            [&pendingCheckouts, &checkoutSuccess](bool success)
                            {
                                checkoutSuccess &= success;
                                --pendingCheckouts;
                            }
                        );
                    }
                }
            }
        }

        if (pendingCheckouts > 0)
        {
            const size_t totalAssets = assets.size();
            AzToolsFramework::ProgressShield::LegacyShowAndWait(this, this->tr("Checking out slices for edit..."),
                [&pendingCheckouts, totalAssets] (int& current, int& max)
                {
                    current = static_cast<int>(totalAssets) - static_cast<int>(pendingCheckouts);
                    max = static_cast<int>(totalAssets);
                    return pendingCheckouts == 0;
                }
            );
        }
        
        if (!checkoutSuccess)
        {
            QMessageBox::warning(this, QStringLiteral("Cannot Push to Slice"), 
                QString("Failed to check out one or more target slice files."), 
                QMessageBox::Ok);
            return false;
        }

        return (!assets.empty());
    }

    //=========================================================================
    // SlicePushWidget::eventFilter
    //=========================================================================
    bool SlicePushWidget::eventFilter(QObject* target, QEvent* event)
    {
        if (target == m_fieldTree)
        {
            // If the user hits the spacebar with a field selected, toggle its checkbox.
            if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_Space)
                {
                    if (!m_fieldTree->selectedItems().isEmpty())
                    {
                        FieldTreeItem* item = static_cast<FieldTreeItem*>(m_fieldTree->selectedItems().front());
                        const Qt::CheckState currentState = item->checkState(0);
                        const Qt::CheckState newState = (currentState == Qt::CheckState::Checked) ? 
                            Qt::CheckState::Unchecked : Qt::CheckState::Checked;
                        item->setCheckState(0, newState);

                        return true; // Eat the event
                    }
                }
            }
        }
        
        return QWidget::eventFilter(target, event);
    }

    //=========================================================================
    // SlicePushWidget::AddStatusMessage
    //=========================================================================
    void SlicePushWidget::AddStatusMessage(StatusMessageType messageType, const QString& messageText)
    {
        m_statusMessages.push_back();
        AzToolsFramework::SlicePushWidget::StatusMessage& newMessage = m_statusMessages.back();
        newMessage.m_type = messageType;
        newMessage.m_text = messageText;
    }

    //=========================================================================
    // SlicePushWidget::DisplayStatusMessages
    //=========================================================================
    void SlicePushWidget::DisplayStatusMessages()
    {
        int messageCount = 0;
        for (AzToolsFramework::SlicePushWidget::StatusMessage& message : m_statusMessages)
        {
            QHBoxLayout* bottomStatusMessagesLayout = new QHBoxLayout();
            {
                bottomStatusMessagesLayout->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

                QLabel* statusIconLabel = new QLabel();
                switch (message.m_type)
                {
                case StatusMessageType::Warning:
                default:
                {
                    if (!m_iconWarning.availableSizes().empty())
                    {
                        statusIconLabel->setPixmap(m_iconWarning.pixmap(m_iconWarning.availableSizes().first()));
                    }
                    break;
                }
                }
                statusIconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
                statusIconLabel->setMargin(3);
                statusIconLabel->setAlignment(Qt::AlignCenter);

                QLabel* statusLabel = new QLabel();
                statusLabel->setText(message.m_text);
                statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
                statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
                statusLabel->setWordWrap(true);

                bottomStatusMessagesLayout->setAlignment(Qt::AlignBottom);

                bottomStatusMessagesLayout->addWidget(statusIconLabel);
                bottomStatusMessagesLayout->addWidget(statusLabel);
            }

            m_bottomLayout->insertLayout(messageCount++, bottomStatusMessagesLayout, 1);
        }
    }

} // namespace AzToolsFramework

#include "UI/Slice/moc_SlicePushWidget.cpp"
