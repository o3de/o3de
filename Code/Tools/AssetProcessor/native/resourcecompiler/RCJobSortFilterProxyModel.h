/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QSortFilterProxyModel>
#include <AzCore/base.h>
#endif

namespace AzQtComponents
{
    struct SearchTypeFilter;
    using SearchTypeFilterList = QVector<SearchTypeFilter>;
}

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        enum class JobStatus;
    }
}

namespace AssetProcessor
{
    struct CustomJobStatusFilter
    {
        CustomJobStatusFilter() = default;
        CustomJobStatusFilter(bool completeWithWarnings)
            : m_completedWithWarnings(completeWithWarnings)
        {

        }

        bool m_completedWithWarnings = false;
    };

    struct JobStatusInfo
    {
        AzToolsFramework::AssetSystem::JobStatus m_status;
        AZ::u32 m_warningCount;
        AZ::u32 m_errorCount;
    };

    class JobSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        explicit JobSortFilterProxyModel(QObject* parent = nullptr);
        void OnJobStatusFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        QList<AzToolsFramework::AssetSystem::JobStatus> m_activeTypeFilters = {};
        bool m_completedWithWarningsFilter = false;
    };
} // namespace AssetProcessor

Q_DECLARE_METATYPE(AssetProcessor::CustomJobStatusFilter);
Q_DECLARE_METATYPE(AssetProcessor::JobStatusInfo);
