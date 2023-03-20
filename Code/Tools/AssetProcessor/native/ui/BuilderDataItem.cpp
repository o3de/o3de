/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/ui/BuilderDataItem.h>

namespace AssetProcessor
{
    static constexpr AZStd::array jobTypeDisplayNames{ "Analysis Jobs", "Processing Jobs" };
    static constexpr char invalidJobTypeDisplayName[] = "Invalid Job Type";

    BuilderDataItem::BuilderDataItem(
        ItemType itemType, AZStd::string name,
        AZ::s64 jobCount,
        AZ::s64 totalDuration,
        AZStd::weak_ptr<BuilderDataItem> parent)
        : m_itemType(itemType)
        , m_name(AZStd::move(name))
        , m_jobCount(jobCount)
        , m_totalDuration(totalDuration)
        , m_parent(parent)
    {
    }

    int BuilderDataItem::ChildCount() const
    {
        return aznumeric_cast<int>(m_children.size());
    }

    const AZStd::string& BuilderDataItem::GetName() const
    {
        return m_name;
    }

    AZ::s64 BuilderDataItem::GetJobCount() const
    {
        return m_jobCount;
    }

    AZ::s64 BuilderDataItem::GetTotalDuration() const
    {
        return m_totalDuration;
    }

    BuilderDataItem::ItemType BuilderDataItem::GetItemType() const
    {
        return m_itemType;
    }

    AZStd::shared_ptr<BuilderDataItem> BuilderDataItem::GetChild(int row) const
    {
        if (row >= m_children.size())
        {
            return nullptr;
        }

        return m_children[row];
    }

    AZStd::weak_ptr<BuilderDataItem> BuilderDataItem::GetParent() const
    {
        return m_parent;
    }

    AZStd::shared_ptr<BuilderDataItem> BuilderDataItem::InsertChild(AZStd::shared_ptr<BuilderDataItem>&& itemToBeInserted)
    {
        //! It's the caller's responsibility to ensure the builder has a unique name
        if (m_itemType != ItemType::InvisibleRoot ||
            (itemToBeInserted->GetItemType() != ItemType::InvisibleRoot && itemToBeInserted->GetItemType() != ItemType::Builder) ||
            itemToBeInserted->GetParent().lock().get() != this || m_childNameToIndex.contains(itemToBeInserted->GetName()))
        {
            return nullptr;
        }

        m_childNameToIndex[itemToBeInserted->GetName()] = aznumeric_cast<int>(m_children.size());
        m_children.push_back(AZStd::move(itemToBeInserted));
        return m_children.back();
    }


    AZStd::shared_ptr<BuilderDataItem> BuilderDataItem::UpdateOrInsertEntry(
        TaskType entryTaskType, const AZStd::string& entryName, AZ::s64 entryJobCount, AZ::s64 entryTotalDuration)
    {
        //! only allowed to insert from builder, with a valid TaskType
        if (m_itemType != ItemType::Builder || entryTaskType >= TaskType::Max)
        {
            return nullptr;
        }

        // jobType is either CreateJobs or ProcessJob
        const auto& jobType = m_children[aznumeric_cast<int>(entryTaskType)];

        AZStd::shared_ptr<BuilderDataItem> entry = nullptr;
        if (jobType->m_childNameToIndex.contains(entryName))
        {
            entry = jobType->m_children[jobType->m_childNameToIndex[entryName]];
            AZ::s64 jobCountDiff = entryJobCount - entry->m_jobCount;
            AZ::s64 totalDurationDiff = entryTotalDuration - entry->m_totalDuration;
            entry->m_jobCount = entryJobCount;
            entry->m_totalDuration = entryTotalDuration;
            jobType->UpdateMetrics(jobCountDiff, totalDurationDiff);
        }
        else
        {
            entry = jobType->m_children.emplace_back(
                new BuilderDataItem(ItemType::Entry, entryName, entryJobCount, entryTotalDuration, jobType));
            jobType->m_childNameToIndex[entryName] = aznumeric_cast<int>(jobType->m_children.size() - 1);
            jobType->UpdateMetrics(entryJobCount, entryTotalDuration);
        }

        return entry;
    }

    bool BuilderDataItem::InsertTaskTypesAsChildren(AZStd::weak_ptr<BuilderDataItem> builderWeakPointer)
    {
        if (m_itemType != ItemType::Builder)
        {
            return false;
        }

        for (int jobTypeIndex = 0; jobTypeIndex < aznumeric_cast<int>(TaskType::Max); ++jobTypeIndex)
        {
            const AZStd::string& jobTypeDisplayName =
                jobTypeIndex < jobTypeDisplayNames.size() ? jobTypeDisplayNames[jobTypeIndex] : invalidJobTypeDisplayName;
            if (jobTypeIndex >= jobTypeDisplayNames.size())
            {
                AZ_Warning(
                    "Asset Processor",
                    false,
                    "Invalid job type name. Job type indexed %d in scoped enum TaskType does not have a matching display name in "
                    "jobTypeDisplayNames. Update jobTypeDisplayNames vector in BuilderDataItem.cpp.",
                    jobTypeIndex);
            }

            m_children.emplace_back(new BuilderDataItem(ItemType::TaskType, jobTypeDisplayName, 0, 0, builderWeakPointer));
        }

        return true;
    }

    void BuilderDataItem::UpdateMetrics(AZ::s64 jobCountDiff, AZ::s64 totalDurationDiff)
    {
        if (m_itemType == ItemType::InvisibleRoot)
        {
            return;
        }

        m_jobCount += jobCountDiff;
        m_totalDuration += totalDurationDiff;
        if (auto sharedParent = m_parent.lock())
        {
            sharedParent->UpdateMetrics(jobCountDiff, totalDurationDiff);
        }
    }
    int BuilderDataItem::GetRow() const
    {
        if (auto sharedParent = m_parent.lock())
        {
            int index = 0;
            for (const auto& item : sharedParent->m_children)
            {
                if (item.get() == this)
                {
                    return index;
                }
                ++index;
            }
        }
        return -1;
    }
}
