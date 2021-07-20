/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/JointSelectionWidget.h>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QTreeView>


namespace EMotionFX
{
    JointSelectionWidget::JointSelectionWidget(QAbstractItemView::SelectionMode selectionMode, QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        setLayout(mainLayout);

        m_noSelectionLabel = new QLabel("Select an actor instance", this);
        m_noSelectionLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        mainLayout->addWidget(m_noSelectionLabel, 0, Qt::AlignCenter);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        mainLayout->addWidget(m_searchWidget);

        m_skeletonModel = AZStd::make_unique<SkeletonModel>();

        m_treeView = new QTreeView(this);

        m_filterProxyModel = new SkeletonSortFilterProxyModel(m_skeletonModel.get(), &m_skeletonModel->GetSelectionModel(), m_treeView);
        m_filterProxyModel->setFilterKeyColumn(-1);
        m_filterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        m_treeView->setModel(m_filterProxyModel);
        m_treeView->setSelectionModel(m_filterProxyModel->GetSelectionProxyModel());

        m_filterProxyModel->ConnectFilterWidget(m_searchWidget);

        m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        m_treeView->setSelectionMode(selectionMode);
        m_treeView->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_treeView->setExpandsOnDoubleClick(false);

        QHeaderView* header = m_treeView->header();
        header->setStretchLastSection(false);
        header->resizeSection(1, SkeletonModel::s_defaultIconSize);
        header->resizeSection(2, SkeletonModel::s_defaultIconSize);
        header->resizeSection(3, SkeletonModel::s_defaultIconSize);
        header->setSectionResizeMode(0, QHeaderView::Stretch);
        header->hide();

        connect(m_treeView, &QTreeView::doubleClicked, this, &JointSelectionWidget::OnItemDoubleClicked);

        // Connect after the tree view connected to the model.
        connect(m_skeletonModel.get(), &QAbstractItemModel::modelReset, this, &JointSelectionWidget::Reinit);

        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &JointSelectionWidget::OnTextFilterChanged);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &JointSelectionWidget::OnTypeFilterChanged);

        mainLayout->addWidget(m_treeView);

        Reinit();
    }

    JointSelectionWidget::~JointSelectionWidget()
    {
    }

    void JointSelectionWidget::SetFilterState(const QString& category, const QString& displayName, bool enabled)
    {
        m_searchWidget->SetFilterState(category, displayName, enabled);
    }

    void JointSelectionWidget::HideIcons()
    {
        m_treeView->hideColumn(SkeletonModel::COLUMN_RAGDOLL_LIMIT);
        m_treeView->hideColumn(SkeletonModel::COLUMN_RAGDOLL_COLLIDERS);
        m_treeView->hideColumn(SkeletonModel::COLUMN_HITDETECTION_COLLIDERS);
    }

    void JointSelectionWidget::SelectByJointName(const AZStd::string& jointName, [[maybe_unused]] bool clearSelection)
    {
        AZStd::vector<AZStd::string> jointNames;
        jointNames.emplace_back(jointName);
        SelectByJointNames(jointNames);
    }

    void JointSelectionWidget::SelectByJointNames(const AZStd::vector<AZStd::string>& jointNames, bool clearSelection)
    {
        QItemSelectionModel& selectionModel = m_skeletonModel->GetSelectionModel();

        if (clearSelection)
        {
            selectionModel.clearSelection();
        }

        AZStd::string itemName;
        m_skeletonModel->ForEach([&jointNames, this, &selectionModel, &itemName](const QModelIndex& modelIndex)
            {
                itemName = this->m_skeletonModel->data(modelIndex, Qt::DisplayRole).toString().toUtf8().data();
                if (AZStd::find(jointNames.begin(), jointNames.end(), itemName) != jointNames.end())
                {
                    selectionModel.select(modelIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                }
            });
    }

    AZStd::vector<AZStd::string> JointSelectionWidget::GetSelectedJointNames() const
    {
        AZStd::vector<AZStd::string> result;
        const QItemSelectionModel& selectionModel = m_skeletonModel->GetSelectionModel();
        const QModelIndexList selectedRows = selectionModel.selectedRows(SkeletonModel::COLUMN_NAME);

        for (const QModelIndex& modelIndex : selectedRows)
        {
            result.emplace_back(m_skeletonModel->data(modelIndex, Qt::DisplayRole).toString().toUtf8().data());
        }

        return result;
    }

    void JointSelectionWidget::Reinit()
    {
        ActorInstance* actorInstance = m_skeletonModel->GetActorInstance();
        if (actorInstance)
        {
            m_treeView->setVisible(true);
            m_searchWidget->setVisible(true);
            m_noSelectionLabel->setVisible(false);
        }
        else
        {
            m_treeView->setVisible(false);
            m_searchWidget->setVisible(false);
            m_noSelectionLabel->setVisible(true);
        }

        m_treeView->expandAll();
    }

    void JointSelectionWidget::OnTextFilterChanged([[maybe_unused]] const QString& text)
    {
        m_treeView->expandAll();
    }

    void JointSelectionWidget::OnTypeFilterChanged([[maybe_unused]] const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
    {
        m_treeView->expandAll();
    }
} // namespace EMotionFX
