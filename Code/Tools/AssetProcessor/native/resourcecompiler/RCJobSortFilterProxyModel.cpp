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

#include <native/resourcecompiler/RCJobSortFilterProxyModel.h>
#include <native/resourcecompiler/JobsModel.h> //for jobsModel column enum

#include <AzQtComponents/Components/FilteredSearchWidget.h>

namespace AssetProcessor
{
    JobSortFilterProxyModel::JobSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        setSortCaseSensitivity(Qt::CaseInsensitive);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    bool JobSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        using namespace AzToolsFramework::AssetSystem;

        const QModelIndex jobStateIndex = sourceModel()->index(sourceRow,
            AssetProcessor::JobsModel::Column::ColumnStatus, sourceParent);

        JobStatusInfo jobStatus = sourceModel()->data(jobStateIndex, AssetProcessor::JobsModel::statusRole)
            .value<JobStatusInfo>();

        if (jobStatus.m_status == JobStatus::Failed_InvalidSourceNameExceedsMaxLimit)
        {
            jobStatus.m_status = JobStatus::Failed;
        }
        
        // Checks our custom filters. 
        // true = passed our filter, send on to the default filter
        // false = reject the row
        auto filterFunc = [&]()
        {
            if (m_completedWithWarningsFilter && jobStatus.m_status == JobStatus::Completed && (jobStatus.m_errorCount > 0 || jobStatus.m_warningCount > 0))
            {
                return true;
            }
            
            if (!m_activeTypeFilters.isEmpty() && m_activeTypeFilters.contains(jobStatus.m_status))
            {
                return true;
            }

            return false;
        };

        bool hasFilters = !m_activeTypeFilters.isEmpty() || m_completedWithWarningsFilter == true;
        bool useDefaultFilter = hasFilters ? filterFunc() : true;

        return useDefaultFilter ? QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent) : false;
    }

    void JobSortFilterProxyModel::OnJobStatusFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
    {
        m_completedWithWarningsFilter = false;
        m_activeTypeFilters.clear();

        for (const auto& filter : activeTypeFilters)
        {
            if (filter.metadata.canConvert<AzToolsFramework::AssetSystem::JobStatus>())
            {
                m_activeTypeFilters << filter.metadata.value<AzToolsFramework::AssetSystem::JobStatus>();
            }
            else if (filter.metadata.canConvert<AssetProcessor::CustomJobStatusFilter>())
            {
                m_completedWithWarningsFilter = filter.metadata.value<AssetProcessor::CustomJobStatusFilter>().m_completedWithWarnings;
            }
        }

        invalidateFilter();
    }

    bool JobSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        // Only the completed column has an override, because it displays time in a different format
        // than what works best to sort.
        if (left.column() != JobsModel::ColumnCompleted || right.column() != JobsModel::ColumnCompleted)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        QVariant leftTime = sourceModel()->data(left, JobsModel::SortRole);
        QVariant rightTime = sourceModel()->data(right, JobsModel::SortRole);

        if (leftTime.type() != QVariant::DateTime || rightTime.type() != QVariant::DateTime)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        return leftTime.toDateTime() < rightTime.toDateTime();
    }
} //namespace AssetProcessor


