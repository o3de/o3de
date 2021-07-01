/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <QtWidgets/QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace EMStudio
{
    class AnimGraphPlugin;
    class RoleFilterProxyModel;

    class NavigationLinkWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigationLinkWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        NavigationLinkWidget(AnimGraphPlugin* plugin, QWidget* parent);
        ~NavigationLinkWidget();

    private slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
        void OnBreadCrumbsLinkClicked(const QString& linkPath, int linkIndex);

    private:
        void AddToNavigation(const QModelIndex& modelIndex);
        void UpdateBreadCrumbsPath();

        AzQtComponents::BreadCrumbs* m_breadCrumbs = nullptr;
        AZStd::vector<QPersistentModelIndex> m_modelIndexes;
        AnimGraphPlugin* m_plugin;
        RoleFilterProxyModel* m_roleFilterProxyModel;
    };

} // namespace EMStudio
