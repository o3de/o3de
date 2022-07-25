/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AZCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

namespace AssetProcessor
{
    class BuilderDataItem
    {
    public:
        enum class JobType
        {
            CreateJob,
            ProcessJob,
            Max
        };

        enum class ItemType
        {
            InvisibleRoot,
            Builder,
            JobType,
            Entry,
            Max
        };

        BuilderDataItem(
            ItemType itemType, const AZStd::string& name,
            AZ::s64 jobCount,
            AZ::s64 totalDuration,
            AZStd::shared_ptr<BuilderDataItem> parent);
        int ChildCount() const;
        const char* GetName() const;
        AZ::s64 GetJobCount() const;
        AZ::s64 GetTotalDuration() const;
        BuilderDataItem* GetChild(int row) const;
        BuilderDataItem* GetParent() const;
        int GetRow() const; //! Returns this item's row number in its parent's children list.
        bool SetChild(AZStd::shared_ptr<BuilderDataItem> builder);
        BuilderDataItem* UpdateOrInsertEntry(JobType jobType, const AZStd::string& name, AZ::s64 jobCount, AZ::s64 totalDuration);
    private:
        void UpdateMetrics(AZ::s64 jobCountDiff, AZ::s64 totalDurationDiff);

        AZStd::vector<AZStd::shared_ptr<BuilderDataItem>> m_children;
        AZStd::shared_ptr<BuilderDataItem> m_parent;
        AZStd::unordered_map<AZStd::string, int> m_childNameToIndex;
        AZStd::string m_name;
        AZ::s64 m_jobCount = 0;
        AZ::s64 m_totalDuration = 0;
        ItemType m_itemType = ItemType::Max;
    };
}
