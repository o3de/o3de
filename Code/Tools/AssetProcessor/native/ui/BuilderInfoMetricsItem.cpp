#include "BuilderInfoMetricsItem.h"
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AssetProcessor
{
    const AZStd::string jobTypeDisplayNames[] = { "Analysis Jobs", "Processing Jobs" };
    const AZStd::string invalidJobTypeDisplayName{ "Invalid Job Type" };

    BuilderInfoMetricsItem::BuilderInfoMetricsItem(
        ItemType itemType, const AZStd::string& name,
        AZ::s64 jobCount,
        AZ::s64 totalDuration,
        AZStd::shared_ptr<BuilderInfoMetricsItem> parent)
        : m_itemType(itemType), m_name(name)
        , m_jobCount(jobCount)
        , m_totalDuration(totalDuration)
        , m_parent(parent)
    {
        if (itemType == ItemType::Root)
        {
            for (int i = 0; i < aznumeric_cast<int>(JobType::Max); ++i)
            {
                int n = sizeof(jobTypeDisplayNames) / sizeof(jobTypeDisplayNames[0]);
                const AZStd::string& jobTypeDisplayName =
                    i < n ? jobTypeDisplayNames[i] : invalidJobTypeDisplayName;
                if (i >= n)
                {
                    AZ_Warning("Asset Processor", false, "Invalid job type name. Job type indexed %d in scoped enum JobType does not have a matching display name in jobTypeDisplayNames. Update jobTypeDisplayNames vector in BuilderInfoMetricsItem.cpp.");
                }

                m_children.emplace_back(new BuilderInfoMetricsItem(
                    ItemType::JobType, jobTypeDisplayName, 0, 0, AZStd::shared_ptr<BuilderInfoMetricsItem>(this)));
            }
        }
    }

    int BuilderInfoMetricsItem::ChildCount() const
    {
        return aznumeric_cast<int>(m_children.size());
    }

    const char* BuilderInfoMetricsItem::GetName() const
    {
        return m_name.c_str();
    }

    AZ::s64 BuilderInfoMetricsItem::GetJobCount() const
    {
        return m_jobCount;
    }

    AZ::s64 BuilderInfoMetricsItem::GetTotalDuration() const
    {
        return m_totalDuration;
    }

    BuilderInfoMetricsItem* BuilderInfoMetricsItem::GetChild(int row) const
    {
        if (row >= m_children.size())
        {
            return nullptr;
        }

        return m_children[row].get();
    }

    BuilderInfoMetricsItem* BuilderInfoMetricsItem::GetParent() const
    {
        return m_parent.get();
    }

    bool BuilderInfoMetricsItem::UpdateOrInsertEntry(
        JobType entryjobType, const AZStd::string& entryName, AZ::s64 entryJobCount, AZ::s64 entryTotalDuration)
    {
        //! only allowed to insert from root, with a valid JobType
        if (m_itemType != ItemType::Root || entryjobType >= JobType::Max)
        {
            return false;
        }

        const auto& jobType = m_children[aznumeric_cast<int>(entryjobType)];

        if (jobType->m_childNameToIndex.contains(entryName))
        {
            const auto& entry = jobType->m_children[jobType->m_childNameToIndex[entryName]];
            AZ::s64 jobCountDiff = entryJobCount - entry->m_jobCount;
            AZ::s64 totalDurationDiff = entryTotalDuration - entry->m_totalDuration;
            entry->m_jobCount = entryJobCount;
            entry->m_totalDuration= entryTotalDuration;
            jobType->UpdateMetrics(jobCountDiff, totalDurationDiff);
        }
        else
        {
            jobType->m_children.emplace_back(new BuilderInfoMetricsItem(ItemType::Entry, entryName, entryJobCount, entryTotalDuration, jobType));
            jobType->m_childNameToIndex[entryName] = aznumeric_cast<int>(jobType->m_children.size() - 1);
            jobType->UpdateMetrics(entryJobCount, entryTotalDuration);
        }

        return true;
    }
    void BuilderInfoMetricsItem::UpdateMetrics(AZ::s64 jobCountDiff, AZ::s64 totalDurationDiff)
    {
        m_jobCount += jobCountDiff;
        m_totalDuration += totalDurationDiff;
        if (m_parent)
        {
            m_parent->UpdateMetrics(jobCountDiff, totalDurationDiff);
        }
    }
    int BuilderInfoMetricsItem::GetRow() const
    {
        if (m_parent)
        {
            int index = 0;
            for (const auto& item : m_parent->m_children)
            {
                if (item.get() == this)
                {
                    return index;
                }
                ++index;
            }
        }
        return 0;
    }
}
