/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class AnimGraphSortFilterProxyModel;
    class SelectionProxyModel;

    class NavigateWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigateWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        NavigateWidget(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~NavigateWidget();

    private:
        void keyReleaseEvent(QKeyEvent* event) override;

    private slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);
        void OnItemDoubleClicked(const QModelIndex& targetModelIndex);
        void OnContextMenuEvent(const QPoint& point);
        void OnTextFilterChanged(const QString& text);

    private:
        AnimGraphPlugin* m_plugin;

        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        QTreeView* m_treeView;
        AnimGraphSortFilterProxyModel* m_filterProxyModel;
        SelectionProxyModel* m_selectionProxyModel;
    };

} // namespace EMStudio