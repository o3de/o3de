#include "BuilderDataItem.h"
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

    BuilderDataItem::BuilderDataItem(
        ItemType itemType, const AZStd::string& name,
        AZ::s64 jobCount,
        AZ::s64 totalDuration,
        AZStd::shared_ptr<BuilderDataItem> parent)
        : m_itemType(itemType), m_name(name)
        , m_jobCount(jobCount)
        , m_totalDuration(totalDuration)
        , m_parent(parent)
    {
        if (itemType == ItemType::Builder)
        {
            for (int i = 0; i < aznumeric_cast<int>(JobType::Max); ++i)
            {
                int numJobType = sizeof(jobTypeDisplayNames) / sizeof(jobTypeDisplayNames[0]);
                const AZStd::string& jobTypeDisplayName =
                    i < numJobType ? jobTypeDisplayNames[i] : invalidJobTypeDisplayName;
                if (i >= numJobType)
                {
                    AZ_Warning("Asset Processor", false, "Invalid job type name. Job type indexed %d in scoped enum JobType does not have a matching display name in jobTypeDisplayNames. Update jobTypeDisplayNames vector in BuilderDataItem.cpp.");
                }

                m_children.emplace_back(new BuilderDataItem(
                    ItemType::JobType, jobTypeDisplayName, 0, 0, AZStd::shared_ptr<BuilderDataItem>(this)));
            }
        }
    }

    int BuilderDataItem::ChildCount() const
    {
        return aznumeric_cast<int>(m_children.size());
    }

    const char* BuilderDataItem::GetName() const
    {
        return m_name.c_str();
    }

    AZ::s64 BuilderDataItem::GetJobCount() const
    {
        return m_jobCount;
    }

    AZ::s64 BuilderDataItem::GetTotalDuration() const
    {
        return m_totalDuration;
    }

    BuilderDataItem* BuilderDataItem::GetChild(int row) const
    {
        if (row >= m_children.size())
        {
            return nullptr;
        }

        return m_children[row].get();
    }

    BuilderDataItem* BuilderDataItem::GetParent() const
    {
        return m_parent.get();
    }

    BuilderDataItem* BuilderDataItem::UpdateOrInsertEntry(
        JobType entryjobType, const AZStd::string& entryName, AZ::s64 entryJobCount, AZ::s64 entryTotalDuration)
    {
        //! only allowed to insert from builder, with a valid JobType
        if (m_itemType != ItemType::Builder || entryjobType >= JobType::Max)
        {
            return nullptr;
        }

        // jobType is either CreateJob or ProcessJob
        const auto& jobType = m_children[aznumeric_cast<int>(entryjobType)];

        BuilderDataItem* entry = nullptr;
        if (jobType->m_childNameToIndex.contains(entryName))
        {
            entry = jobType->m_children[jobType->m_childNameToIndex[entryName]].get();
            AZ::s64 jobCountDiff = entryJobCount - entry->m_jobCount;
            AZ::s64 totalDurationDiff = entryTotalDuration - entry->m_totalDuration;
            entry->m_jobCount = entryJobCount;
            entry->m_totalDuration= entryTotalDuration;
            jobType->UpdateMetrics(jobCountDiff, totalDurationDiff);
        }
        else
        {
            entry = jobType->m_children
                        .emplace_back(new BuilderDataItem(ItemType::Entry, entryName, entryJobCount, entryTotalDuration, jobType))
                        .get();
            jobType->m_childNameToIndex[entryName] = aznumeric_cast<int>(jobType->m_children.size() - 1);
            jobType->UpdateMetrics(entryJobCount, entryTotalDuration);
        }

        return entry;
    }
    void BuilderDataItem::UpdateMetrics(AZ::s64 jobCountDiff, AZ::s64 totalDurationDiff)
    {
        m_jobCount += jobCountDiff;
        m_totalDuration += totalDurationDiff;
        if (m_parent)
        {
            m_parent->UpdateMetrics(jobCountDiff, totalDurationDiff);
        }
    }
    int BuilderDataItem::GetRow() const
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
    bool BuilderDataItem::SetChild(AZStd::shared_ptr<BuilderDataItem> builder)
    {
        if (m_itemType != ItemType::InvisibleRoot)
        {
            return false;
        }

        m_children.resize(1);
        m_children[0] = builder;
        return true;
    }
}
