/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>
#include <QtCore/QSortFilterProxyModel>
#endif


namespace EMStudio
{
    // This class serves as a proxy model for filtering and sorting AnimGraphModel
    //
    class AnimGraphSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        explicit AnimGraphSortFilterProxyModel(QObject* parent = nullptr);
        ~AnimGraphSortFilterProxyModel() override = default;

        void setFilterRecursiveMode(bool enabled);
        void setDisableSelectionForFilteredButShowedElements(bool enabled);
        void setFilterStatesOnly(bool enabled);

        void setFilterNodeTypes(const AZStd::unordered_set<AZ::TypeId>& filterNodeTypes);

        /*
         * @brief Set an index in the source model that should never be filtered out
         *
         * This is useful for views that want to set a root index that is not
         * the top-most index in the source model. Normally, if the filter
         * removes all indexes from the source model, the index that the view
         * is using for its root will become invalid. When the filter is
         * cleared, bringing back all the rows, the view's root index is still
         * the invalid one, losing the original setting. This method allows the
         * view to use a root index that will always stay valid.
         */
        void setNonFilterableIndex(const QModelIndex& sourceIndex);

        // TODO: improve the filtering by caching and invalidating the cache with new entries

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;

    private:
        bool IsFiltered(const QModelIndex& index, bool recursiveMode) const;

        // In recursive mode (true by default), we filter-in (leave) the entries that have any child
        // that matches the filter
        bool m_recursiveMode;

        // In cases where we are showing elements that are filtered (e.g. because recursive mode is enabled) we may want
        // to disable selection of those items.
        bool m_disableSelectionForFiltered;

        // Show states only (false by default) will filter-out (take out) entries that are not states
        bool m_showStatesOnly;

        // Shows nodes only (true by default) will filter-out (take out) entries that are transitions
        bool m_showNodesOnly;

        // If not empty, will filter-in (leave) the node entries that match the specified type
        AZStd::unordered_set<AZ::TypeId> m_filterNodeTypes;

        // A source index that cannot be filtered. This allows views to have a
        // root index that is never filtered out, so that their root always
        // stays valid and isn't reset
        QPersistentModelIndex m_nonFilterableSourceIndex;
     };

}   // namespace EMStudio
