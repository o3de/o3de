/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphItemDelegate.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphSortFilterProxyModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigateWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/SelectionProxyModel.h>
#include <QHeaderView>
#include <QKeyEvent>
#include <QTreeView>
#include <QVBoxLayout>


namespace EMStudio
{
    NavigateWidget::NavigateWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
        , m_plugin(plugin)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(2);
        setLayout(mainLayout);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &NavigateWidget::OnTextFilterChanged);
        mainLayout->addWidget(m_searchWidget);

        // create the tree widget
        m_treeView = new QTreeView();
        connect(m_treeView, &QTreeView::doubleClicked, this, &NavigateWidget::OnItemDoubleClicked);
        m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NavigateWidget::OnContextMenuEvent);

        mainLayout->addWidget(m_treeView);
        m_treeView->setExpandsOnDoubleClick(false);

        // tree's model
        m_filterProxyModel = new AnimGraphSortFilterProxyModel(m_treeView);
        m_filterProxyModel->setSourceModel(&plugin->GetAnimGraphModel());
        m_filterProxyModel->setFilterKeyColumn(-1);
        m_filterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
        m_treeView->setModel(m_filterProxyModel);
        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &NavigateWidget::OnFocusChanged);
        {
            // hide all sections and just enable the ones we are interested about
            int sectionCount = m_treeView->header()->count();
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

        // tree's selection model
        m_selectionProxyModel = new SelectionProxyModel(&plugin->GetAnimGraphModel().GetSelectionModel(), m_filterProxyModel, m_treeView);
        m_treeView->setSelectionModel(m_selectionProxyModel);
        m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    }

    NavigateWidget::~NavigateWidget()
    {
    }

    void NavigateWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        AZ_UNUSED(oldFocusIndex);
        AZ_UNUSED(oldFocusParent);

        if (newFocusParent.isValid())
        {
            const QModelIndex targetFocusParent = m_filterProxyModel->mapFromSource(newFocusParent);
            m_treeView->setExpanded(targetFocusParent, true);
            m_treeView->dataChanged(targetFocusParent, targetFocusParent);
        }
        if (newFocusIndex.isValid())
        {
            const QModelIndex targetFocusIndex = m_filterProxyModel->mapFromSource(newFocusIndex);
            m_treeView->scrollTo(targetFocusIndex, QAbstractItemView::PositionAtCenter);
        }
    }

    void NavigateWidget::OnItemDoubleClicked(const QModelIndex& targetModelIndex)
    {
        const QModelIndex sourceModelIndex = m_filterProxyModel->mapToSource(targetModelIndex);
        m_plugin->GetAnimGraphModel().Focus(sourceModelIndex);
    }

    void NavigateWidget::OnContextMenuEvent(const QPoint& point)
    {
        QModelIndex index = m_treeView->indexAt(point);
        if (index.isValid())
        {
            QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
            if (sourceIndex.isValid())
            {
                const AnimGraphModel::ModelItemType itemType = sourceIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                if (itemType == AnimGraphModel::ModelItemType::NODE)
                {
                    // TODO: we need some re-writting of BlendGraphWidget::OnContextMenuEvent and to move it to AnimGraphActionManager or use ContextMenu directly
                    EMotionFX::AnimGraphNode* node = sourceIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    const QPoint globalPoint = m_treeView->mapToGlobal(point);

                    m_plugin->GetGraphWidget()->OnContextMenuEvent(
                        m_treeView,
                        point,
                        globalPoint,
                        m_plugin,
                        { node },
                        false,
                        m_plugin->GetAnimGraphModel().CheckAnySelectedNodeBelongsToReferenceGraph(),
                        m_plugin->GetActionFilter());
                }
            }
        }
    }

    void NavigateWidget::keyReleaseEvent(QKeyEvent* event)
    {
        // delete selected items
        switch (event->key())
        {
            // when pressing delete, delete the selected items
            case Qt::Key_Delete:
            {
                if (m_plugin->GetActionFilter().m_delete)
                {
                    m_plugin->GetActionManager().DeleteSelectedNodes();
                }
                break;
            }
        }
    }


    // when the filter string changed
    void NavigateWidget::OnTextFilterChanged(const QString& text)
    {
        m_filterProxyModel->setFilterWildcard(text);
        m_treeView->expandAll();
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NavigateWidget.cpp>
