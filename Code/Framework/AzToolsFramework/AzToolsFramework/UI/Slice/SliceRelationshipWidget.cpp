/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QTreeWidgetItemIterator::d_ptr': class 'QScopedPointer<QTreeWidgetItemIteratorPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QTreeWidgetItemIterator'
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSplitter>

#include "SliceRelationshipWidget.hxx"

#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

namespace SliceRelationshipWidgetInternal
{
    //! The icon to use when a cycle is detected.
    static const char* s_cycleDetectedIcon = ":/Cards/img/UI20/Cards/warning.png";
    //! the formatting string used a cycle is detected.  %1 is the name of the child item.
    static const char* s_cycleDetectedFormatter = " CYCLE DETECTED: %1";
}

namespace AzToolsFramework
{
    void SliceRelationshipWidget::closeEvent(QCloseEvent* event)
    {
        QWidget::closeEvent(event);
    }

    //=========================================================================
    // SlicePushWidget
    //=========================================================================
    SliceRelationshipWidget::SliceRelationshipWidget(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout();

        // Window layout:
        // ===============================================
        // ||-------------------------------------------||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||                (splitter)                 ||
        // ||                     |                     ||
        // ||  (Dependents tree)  | (Dependencies tree) ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||                     |                     ||
        // ||-------------------------------------------||
        // ||                              (button box) ||
        // ===============================================

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_sliceDependencyTree = new QTreeWidget();
        m_sliceDependencyTree->setColumnCount(1);
        m_sliceDependencyTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_sliceDependencyTree->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_sliceDependencyTree->setDragEnabled(false);
        m_sliceDependencyTree->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
        m_sliceDependencyTree->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        m_sliceDependencyTree->setColumnCount(1);
        m_sliceDependencyTree->setHeaderLabel(tr("Dependencies"));

        m_sliceDependentsTree = new QTreeWidget();
        m_sliceDependentsTree->setColumnCount(1);
        m_sliceDependentsTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_sliceDependentsTree->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
        m_sliceDependentsTree->setDragEnabled(false);
        m_sliceDependentsTree->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
        m_sliceDependentsTree->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        m_sliceDependentsTree->setColumnCount(1);
        m_sliceDependentsTree->setHeaderLabel(tr("Dependents"));

        // Main splitter contains left & right widgets (the two trees).
        QSplitter* splitter = new QSplitter();
        splitter->addWidget(m_sliceDependencyTree);
        splitter->addWidget(m_sliceDependentsTree);

        QList<int> sizes;
        const int totalWidth = size().width();
        const int firstColumnSize = totalWidth / 2;
        sizes << firstColumnSize << (totalWidth - firstColumnSize);
        splitter->setSizes(sizes);
        splitter->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

        QHBoxLayout* bottomLegendAndButtonsLayout = new QHBoxLayout();

        // Create/populate button box.
        {
            QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
            QPushButton* browseButton = new QPushButton(tr("Browse..."));
            browseButton->setToolTip(tr("Browse for a slice whose relationships you want to see"));
            browseButton->setDefault(false);
            browseButton->setAutoDefault(false);
            buttonBox->addButton(browseButton, QDialogButtonBox::ActionRole);
            connect(browseButton, &QPushButton::clicked, this, &SliceRelationshipWidget::OnBrowseClicked);

            QPushButton* currentlySelectedEntity = new QPushButton(tr("Current Entity"));
            currentlySelectedEntity->setToolTip(tr("Inspect slice relationships for the currently selected entity"));
            currentlySelectedEntity->setDefault(false);
            currentlySelectedEntity->setAutoDefault(false);
            buttonBox->addButton(currentlySelectedEntity, QDialogButtonBox::ActionRole);
            connect(currentlySelectedEntity, &QPushButton::clicked, this, &SliceRelationshipWidget::OnInspectSelectedEntity);

            bottomLegendAndButtonsLayout->addWidget(buttonBox, 1);
        }

        m_bottomLayout = new QVBoxLayout();
        m_bottomLayout->setAlignment(Qt::AlignBottom);
        m_bottomLayout->addLayout(bottomLegendAndButtonsLayout);

        // Add everything to main layout.
        layout->addWidget(splitter, 1);
        layout->addLayout(m_bottomLayout, 0);

        setLayout(layout);

        connect(m_sliceDependentsTree, &QTreeWidget::itemDoubleClicked, this, &SliceRelationshipWidget::OnSliceAssetDrilledDown);
        connect(m_sliceDependencyTree, &QTreeWidget::itemDoubleClicked, this, &SliceRelationshipWidget::OnSliceAssetDrilledDown);

        SliceDependencyBrowserNotificationsBus::Handler::BusConnect();
        SliceRelationshipRequestBus::Handler::BusConnect();
    }

    SliceRelationshipWidget::~SliceRelationshipWidget()
    {
        SliceDependencyBrowserNotificationsBus::Handler::BusDisconnect();
        SliceRelationshipRequestBus::Handler::BusDisconnect();
    }

    void SliceRelationshipWidget::OnSliceRelationshipModelUpdated(const AZStd::shared_ptr<SliceRelationshipNode>& focusNode)
    {
        GenerateTreesForNode(focusNode);
    }

    void SliceRelationshipWidget::OnSliceRelationshipViewRequested(const AZ::Data::AssetId& assetId)
    {
        ResetTrees(assetId);
    }

    QTreeWidgetItem* AddRootItem(QTreeWidget* widget, const QString& name)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(widget);
        item->setText(0, name);
        item->setExpanded(true);

        widget->addTopLevelItem(item);
        return item;
    }


    QTreeWidgetItem* AddChildItem(QTreeWidgetItem* parent, const QString& name)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        item->setText(0, name);

        parent->addChild(item);
        return item;
    }

    bool SliceRelationshipWidget::CheckForCycle(QTreeWidgetItem* parentItem, QString searchFor)
    {
        while (parentItem)
        {
            if (parentItem->text(0) == searchFor)
            {
                return true;
            }
            parentItem = parentItem->parent();
        }
        return false;
    }

    void SliceRelationshipWidget::GenerateDependentsTree(const AZStd::shared_ptr<SliceRelationshipNode>& node, QTreeWidgetItem* parentItem)
    {
        const SliceRelationshipNode::SliceRelationshipNodeSet& dependentNodes = node->GetDependents();
        for (const auto& dependentNode : dependentNodes)
        {
            // before we add this item, we need to make sure that there is no PARENT item (including 'parentItem') that already
            // includes the child node, to detect cycles.
            QString childItemName = QString((dependentNode->GetSliceRelativePath()).c_str());
            if (CheckForCycle(parentItem, childItemName))
            {
                childItemName = QString(SliceRelationshipWidgetInternal::s_cycleDetectedFormatter).arg(childItemName);
                QTreeWidgetItem* childItem = AddChildItem(parentItem, childItemName);
                childItem->setIcon(0, QIcon(SliceRelationshipWidgetInternal::s_cycleDetectedIcon));
            }
            else
            {
                QTreeWidgetItem* childItem = AddChildItem(parentItem, childItemName);
                GenerateDependentsTree(dependentNode, childItem);
            }
        }
    }

    void SliceRelationshipWidget::GenerateDependenciesTree(const AZStd::shared_ptr<SliceRelationshipNode>& node, QTreeWidgetItem* parentItem)
    {
        const SliceRelationshipNode::SliceRelationshipNodeSet& dependencyNodes = node->GetDependencies();
        for (const auto& dependencyNode : dependencyNodes)
        {
            QString childItemName = QString((dependencyNode->GetSliceRelativePath()).c_str());
            if (CheckForCycle(parentItem, childItemName))
            {
                childItemName = QString(SliceRelationshipWidgetInternal::s_cycleDetectedFormatter).arg(childItemName);
                QTreeWidgetItem* childItem = AddChildItem(parentItem, childItemName);
                childItem->setIcon(0, QIcon(SliceRelationshipWidgetInternal::s_cycleDetectedIcon));
            }
            else
            {
                QTreeWidgetItem* childItem = AddChildItem(parentItem, childItemName);
                GenerateDependenciesTree(dependencyNode, childItem);
            }
        }
    }

    void SliceRelationshipWidget::OnBrowseClicked()
    {
        AzToolsFramework::AssetBrowser::AssetSelectionModel selection = AzToolsFramework::AssetBrowser::AssetSelectionModel::AssetTypeSelection("Slice");
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);

        if (selection.IsValid())
        {
            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
            AZ_Assert(product, "Incorrect asset type selected. Expected product.");
            ResetTrees(product->GetAssetId());
        }
    }

    void SliceRelationshipWidget::OnInspectSelectedEntity()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        if(!selectedEntities.empty())
        {
            ResetTrees(selectedEntities.front());
        }
    }

    void SliceRelationshipWidget::OnSliceAssetDrilledDown(QTreeWidgetItem* item, int column)
    {
        if (item && column >= 0)
        {
            ResetTrees(AZStd::string(item->text(column).toUtf8().constData()));
        }
    }

    void SliceRelationshipWidget::ResetTrees(const AZ::EntityId& entityId)
    {
        if (entityId.IsValid())
        {
            AZ::SliceComponent::SliceInstanceAddress entitySliceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(entitySliceAddress, entityId,
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

            // Use the bottom most slice ancestor, so nested slices will show the closest ancestor rather than their root.
            if (!entitySliceAddress.IsValid())
            {
                return;
            }

            AZ::SliceComponent::EntityAncestorList ancestors;
            entitySliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, ancestors);

            if (ancestors.empty())
            {
                return;
            }

            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset = ancestors.back().m_sliceAddress.GetReference()->GetSliceAsset();

            ResetTrees(sliceAsset.GetId());
        }
    }

    void SliceRelationshipWidget::ResetTrees(const AZStd::string& assetRelativePath)
    {
        AZStd::shared_ptr<SliceRelationshipNode> node = nullptr;
        AzToolsFramework::SliceDependencyBrowserRequestsBus::BroadcastResult(node, &AzToolsFramework::SliceDependencyBrowserRequests::ReportSliceAssetDependenciesByPath, assetRelativePath);
        GenerateTreesForNode(node);
    }

    void SliceRelationshipWidget::ResetTrees(const AZ::Data::AssetId& assetId)
    {
        AZStd::shared_ptr<SliceRelationshipNode> node = nullptr;
        AzToolsFramework::SliceDependencyBrowserRequestsBus::BroadcastResult(node ,&AzToolsFramework::SliceDependencyBrowserRequests::ReportSliceAssetDependenciesById, assetId);
        GenerateTreesForNode(node);
    }

    void SliceRelationshipWidget::GenerateTreesForNode(const AZStd::shared_ptr<SliceRelationshipNode>& node)
    {
        m_sliceDependentsTree->clear();
        m_sliceDependencyTree->clear();

        if (node != nullptr)
        {
            // Dependents tree
            GenerateDependentsTree(node, AddRootItem(m_sliceDependentsTree, QString((node->GetSliceRelativePath()).c_str())));

            // Dependencies tree
            GenerateDependenciesTree(node, AddRootItem(m_sliceDependencyTree, QString((node->GetSliceRelativePath()).c_str())));
        }
    }

} // namespace AzToolsFramework

#include "UI/Slice/moc_SliceRelationshipWidget.cpp"
