/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphItemDelegate.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphSortFilterProxyModel.h>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>


namespace EMStudio
{
    AnimGraphHierarchyWidget::AnimGraphHierarchyWidget(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // Create the display button group.
        QHBoxLayout* displayLayout = new QHBoxLayout();

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        displayLayout->addWidget(m_searchWidget);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &AnimGraphHierarchyWidget::OnTextFilterChanged);

        // Create the tree widget.
        m_treeView = new QTreeView();

        m_filterProxyModel = new AnimGraphSortFilterProxyModel(m_treeView);
        m_filterProxyModel->setDisableSelectionForFilteredButShowedElements(true);
        EMStudioPlugin* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AZ_Assert(animGraphPlugin, "Could not find anim graph plugin!");
        m_filterProxyModel->setSourceModel(&static_cast<AnimGraphPlugin*>(animGraphPlugin)->GetAnimGraphModel());
        m_filterProxyModel->setFilterKeyColumn(-1);
        m_filterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
        m_treeView->setModel(m_filterProxyModel);
        m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        {
            // hide all sections and just enable the ones we are interested about
            const int sectionCount = m_treeView->header()->count();
            for (int i = 0; i < sectionCount; ++i)
            {
                m_treeView->header()->hideSection(i);
            }

            m_treeView->header()->showSection(AnimGraphModel::COLUMN_NAME);
            m_treeView->header()->setSectionResizeMode(AnimGraphModel::COLUMN_NAME, QHeaderView::ResizeMode::ResizeToContents);

            m_treeView->header()->showSection(AnimGraphModel::COLUMN_PALETTE_NAME);
            m_treeView->header()->setSectionResizeMode(AnimGraphModel::COLUMN_PALETTE_NAME, QHeaderView::ResizeMode::ResizeToContents);
        }
        // Set the custom delegate
        m_treeView->setStyleSheet("font-size: 11px; color: #e9e9e9;");
        m_treeView->setItemDelegate(new AnimGraphItemDelegate(m_treeView));

        // m_treeView->setAlternatingRowColors(true); TODO: do we want this?
        m_treeView->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_treeView->setExpandsOnDoubleClick(false);
        m_treeView->expandAll();

        layout->addLayout(displayLayout);
        layout->addWidget(m_treeView);
        setLayout(layout);

        connect(m_treeView, &QTreeView::doubleClicked, this, &AnimGraphHierarchyWidget::OnItemDoubleClicked);
        connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AnimGraphHierarchyWidget::OnSelectionChanged);
    }

    void AnimGraphHierarchyWidget::SetSingleSelectionMode(bool useSingleSelection)
    {
        m_treeView->setSelectionMode(useSingleSelection ? QAbstractItemView::SingleSelection : QAbstractItemView::ExtendedSelection);
    }

    void AnimGraphHierarchyWidget::SetFilterNodeType(const AZ::TypeId& filterNodeType)
    {
        if (!filterNodeType.IsNull())
        {
            m_filterProxyModel->setFilterNodeTypes({ filterNodeType });
        }
    }

    void AnimGraphHierarchyWidget::SetFilterStatesOnly(bool showStatesOnly)
    {
        m_filterProxyModel->setFilterStatesOnly(showStatesOnly);
    }

    void AnimGraphHierarchyWidget::SetRootIndex(const QModelIndex& index)
    {
        m_filterProxyModel->setNonFilterableIndex(index);
        QModelIndex proxyIndex = m_filterProxyModel->mapFromSource(index);
        m_treeView->setRootIndex(proxyIndex);
    }

    void AnimGraphHierarchyWidget::SetRootAnimGraph(const EMotionFX::AnimGraph* graph)
    {
        if (graph)
        {
            EMotionFX::AnimGraphStateMachine* graphRoot = graph->GetRootStateMachine();
            EMStudioPlugin* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
            AZ_Assert(animGraphPlugin, "Could not find anim graph plugin!");
            QModelIndex rootIndex = static_cast<AnimGraphPlugin*>(animGraphPlugin)->GetAnimGraphModel().FindFirstModelIndex(graphRoot);
            SetRootIndex(rootIndex);
        }
        else
        {
            SetRootIndex(QModelIndex());
        }
    }

    void AnimGraphHierarchyWidget::OnItemDoubleClicked(const QModelIndex& index)
    {
        if (m_treeView->selectionMode() == QAbstractItemView::SingleSelection)
        {
            AZStd::vector<AnimGraphSelectionItem> selectedNodes;
            
            EMotionFX::AnimGraphNode* node = m_treeView->model()->data(index, AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();         
            selectedNodes.emplace_back(node->GetAnimGraph()->GetID(), node->GetNameString());

            emit OnSelectionDone(selectedNodes);
        }
    }


    void AnimGraphHierarchyWidget::OnTextFilterChanged(const QString& text)
    {
        m_filterProxyModel->setFilterWildcard(text);
    }

    AZStd::vector<AnimGraphSelectionItem> AnimGraphHierarchyWidget::GetSelectedItems() const
    {
        const QModelIndexList modelIndexes = m_treeView->selectionModel()->selectedRows();

        AZStd::vector<AnimGraphSelectionItem> selectedNodes;
        for (const QModelIndex& modelIndex : modelIndexes)
        {
            EMotionFX::AnimGraphNode* node = m_treeView->model()->data(modelIndex, AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            selectedNodes.emplace_back(node->GetAnimGraph()->GetID(), node->GetNameString());
        }

        return selectedNodes;
    }

    bool AnimGraphHierarchyWidget::HasSelectedItems() const
    {
        return !m_treeView->selectionModel()->selectedRows().empty();
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_AnimGraphHierarchyWidget.cpp>
