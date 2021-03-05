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
