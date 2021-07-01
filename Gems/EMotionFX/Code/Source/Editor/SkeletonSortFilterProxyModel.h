/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <QtCore/QSortFilterProxyModel>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#endif


QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class SelectionProxyModel;
    class SkeletonModel;

    class SkeletonSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT // AUTOMOC

    public:
        SkeletonSortFilterProxyModel(SkeletonModel* sourceSkeletonModel, QItemSelectionModel* sourceSelectionModel, QObject* parent = nullptr);

        enum Filter
        {
            SHOW_BONES = 0,
            SHOW_MESHES,
            SHOW_RAGDOLLJOINTS,
            SHOW_HITDETECTIONJOINTS,
            SHOW_CLOTHCOLLIDERJOINTS,
            FILTER_COUNT
        };

        SelectionProxyModel* GetSelectionProxyModel() const { return m_selectionProxyModel; }

        void SetFilterRecursiveMode(bool enabled);
        void SetFilterFlag(Filter filterFlag, bool enabled);
        bool GetFilterFlag(Filter filterFlag) const;
        bool AllFiltersDisabled() const;

        void ConnectFilterWidget(AzQtComponents::FilteredSearchWidget* filterWidget);

        static const char* s_actorCategory;
        static const char* s_simulationCategory;

        static const char* s_bonesFilterName;
        static const char* s_meshesFilterName;
        static const char* s_ragdollNodesFilterName;
        static const char* s_hitDetectionNodesFilterName;
        static const char* s_clothFilterName;

    private slots:
        void OnTextFilterChanged(const QString& text);
        void OnTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    private:
        bool m_recursiveMode;           // In recursive mode (true by default), we filter-in (leave) the entries that have any child that matches the filter.
        bool m_filterFlags[FILTER_COUNT];
        SelectionProxyModel* m_selectionProxyModel;

        Actor* m_actor;
    };
} // namespace EMotionFX
