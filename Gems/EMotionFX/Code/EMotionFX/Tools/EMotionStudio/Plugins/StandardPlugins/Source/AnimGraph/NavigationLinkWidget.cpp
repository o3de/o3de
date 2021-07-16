/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationLinkWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/RoleFilterProxyModel.h>
#include <QHBoxLayout>

namespace EMStudio
{
    NavigationLinkWidget::NavigationLinkWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
        , m_plugin(plugin)
    {
        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(2, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        mainLayout->setAlignment(Qt::AlignLeft);
        setLayout(mainLayout);

        // Use the breadcrumbs component to visualize and interact with the navigation
        m_breadCrumbs = new AzQtComponents::BreadCrumbs(this);
        m_breadCrumbs->setPushPathOnLinkActivation(false);
        connect(m_breadCrumbs, &AzQtComponents::BreadCrumbs::linkClicked, this, &NavigationLinkWidget::OnBreadCrumbsLinkClicked);
        layout()->addWidget(m_breadCrumbs);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        setMaximumHeight(28);
        setFocusPolicy(Qt::ClickFocus);

        m_roleFilterProxyModel = new RoleFilterProxyModel(m_plugin->GetAnimGraphModel(), this);
        m_roleFilterProxyModel->setFilteredRoles({ Qt::DecorationRole });

        connect(&m_plugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &NavigationLinkWidget::OnFocusChanged);
        connect(&m_plugin->GetAnimGraphModel(), &AnimGraphModel::dataChanged, this, &NavigationLinkWidget::OnDataChanged);
    }

    NavigationLinkWidget::~NavigationLinkWidget()
    {
    }

    void NavigationLinkWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        AZ_UNUSED(newFocusIndex);
        AZ_UNUSED(oldFocusIndex);

        if (!newFocusParent.isValid() || newFocusParent != oldFocusParent)
        {
            m_modelIndexes.clear();
        }

        if (newFocusParent != oldFocusParent)
        {
            // If we are focusing on a new parent, add all the hierarchy 
            if (newFocusParent.isValid())
            {
                AddToNavigation(newFocusParent);
                UpdateBreadCrumbsPath();
            }
        }
    }

    void NavigationLinkWidget::OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, [[maybe_unused]] const QVector<int>& roles)
    {
        // Check if breadcrumbs path needs to be updated
        const QItemSelectionRange range(topLeft, bottomRight);
        for (const QPersistentModelIndex& modelIndex : m_modelIndexes)
        {
            if (range.contains(modelIndex))
            {
                UpdateBreadCrumbsPath();
                break;
            }
        }
    }

    void NavigationLinkWidget::OnBreadCrumbsLinkClicked(const QString& linkPath, int linkIndex)
    {
        AZ_UNUSED(linkPath);

        if (linkIndex >= 0 && linkIndex < m_modelIndexes.size())
        {
            m_plugin->GetAnimGraphModel().Focus(m_modelIndexes[linkIndex]);
        }
    }

    void NavigationLinkWidget::AddToNavigation(const QModelIndex& modelIndex)
    {
        QModelIndex parent = modelIndex.parent();
        if (parent.isValid())
        {
            AddToNavigation(parent);
        }

        m_modelIndexes.push_back(modelIndex);
    }

    void NavigationLinkWidget::UpdateBreadCrumbsPath()
    {
        QString breadCrumbsPath;
        for (int i = 0; i < m_modelIndexes.size(); ++i)
        {
            if (i > 0)
            {
                breadCrumbsPath.append('/');
            }
            breadCrumbsPath.append(m_modelIndexes[i].data(Qt::DisplayRole).toString());
        }
        m_breadCrumbs->setCurrentPath(breadCrumbsPath);
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NavigationLinkWidget.cpp>
