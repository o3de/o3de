/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/SelectionProxyModel.h>
#include <Editor/SkeletonSortFilterProxyModel.h>
#include <QAbstractItemView>
#include <QWidget>
#endif


QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class JointSelectionWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        JointSelectionWidget(QAbstractItemView::SelectionMode selectionMode, QWidget* parent);
        ~JointSelectionWidget();

        void SelectByJointName(const AZStd::string& jointName, bool clearSelection = true);
        void SelectByJointNames(const AZStd::vector<AZStd::string>& jointNames, bool clearSelection = true);
        AZStd::vector<AZStd::string> GetSelectedJointNames() const;

        void SetFilterState(const QString& category, const QString& displayName, bool enabled);
        void HideIcons();

    signals:
        void ItemDoubleClicked(const QModelIndex& modelIndex);

    public slots:
        void Reinit();

    private slots:
        // QTreeView
        void OnItemDoubleClicked(const QModelIndex& modelIndex)          { emit ItemDoubleClicked(modelIndex); }

        // AzQtComponents::FilteredSearchWidget
        void OnTextFilterChanged(const QString& text);
        void OnTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

    private:
        AzQtComponents::FilteredSearchWidget*   m_searchWidget;
        QTreeView*                              m_treeView;
        AZStd::unique_ptr<SkeletonModel>        m_skeletonModel;
        SkeletonSortFilterProxyModel*           m_filterProxyModel;
        QLabel*                                 m_noSelectionLabel;
    };
} // namespace EMotionFX
