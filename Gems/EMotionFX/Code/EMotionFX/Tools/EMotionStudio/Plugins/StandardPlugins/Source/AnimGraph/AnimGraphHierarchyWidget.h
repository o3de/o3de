/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#endif


// forward declarations
QT_FORWARD_DECLARE_CLASS(QTreeView);
QT_FORWARD_DECLARE_CLASS(QItemSelection);

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace CommandSystem
{
    class SelectionList;
}

struct AnimGraphSelectionItem
{
    AnimGraphSelectionItem(uint32 animGraphID, const AZStd::string& nodeName)
        : mAnimGraphID(animGraphID)
        , mNodeName(nodeName)
    {}

    uint32          mAnimGraphID;
    AZStd::string   mNodeName;
};

namespace EMotionFX
{
    class AnimGraph;
}

namespace EMStudio
{
    class AnimGraphPlugin;
    class AnimGraphSortFilterProxyModel;

    class AnimGraphHierarchyWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphHierarchyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        explicit AnimGraphHierarchyWidget(QWidget* parent = nullptr);

        void SetSingleSelectionMode(bool useSingleSelection);
        void SetFilterNodeType(const AZ::TypeId& filterNodeType);
        void SetFilterStatesOnly(bool showStatesOnly);
        void SetRootIndex(const QModelIndex& index);
        void SetRootAnimGraph(const EMotionFX::AnimGraph* graph);

        // Returns the current selected items.
        AZStd::vector<AnimGraphSelectionItem> GetSelectedItems() const;
        bool HasSelectedItems() const;

    signals:
        // Triggered when a selection is done in singleSelection mode
        void OnSelectionDone(AZStd::vector<AnimGraphSelectionItem> selectedNodes);
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    public slots:
        void OnItemDoubleClicked(const QModelIndex& index);
        void OnTextFilterChanged(const QString& text);

    private:
        QTreeView*                              m_treeView;
        AzQtComponents::FilteredSearchWidget*   m_searchWidget;
        AnimGraphSortFilterProxyModel*          m_filterProxyModel;
    };
} // namespace EMStudio
