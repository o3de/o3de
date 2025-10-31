/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/string/string.h>
#include <QMetaType>

namespace AssetProcessor
{
    class BuilderDataItem
    {
    public:
        enum class TaskType
        {
            CreateJobs,
            ProcessJob,
            Max
        };

        enum class ItemType
        {
            InvisibleRoot, // Items of this type serve as the root of the tree view. It will not be shown.
            Builder,
            TaskType,
            Entry,
            Max
        };

        BuilderDataItem(
            ItemType itemType, AZStd::string name, AZ::s64 jobCount, AZ::s64 totalDuration, AZStd::weak_ptr<BuilderDataItem> parent);

        //! Metric getters
        const AZStd::string& GetName() const;
        AZ::s64 GetJobCount() const;
        AZ::s64 GetTotalDuration() const;
        ItemType GetItemType() const;

        //! methods querying the tree structure
        int ChildCount() const;
        AZStd::shared_ptr<BuilderDataItem> GetChild(int row) const;
        AZStd::weak_ptr<BuilderDataItem> GetParent() const;
        //! Returns this item's row number in its parent's children list.
        int GetRow() const; 

        //! methods that adds children to this item
        //! This method is only called on InvisibleRoot: set itemToBeInserted as a child, and update m_childNameToIndex.
        //! itemToBeInserted can only be of ItemType::InvisibleRoot or ItemType::Builder. Returns the inserted item.
        AZStd::shared_ptr<BuilderDataItem> InsertChild(AZStd::shared_ptr<BuilderDataItem>&& itemToBeInserted);
        //! This method is only called on Builder: create TaskType children. Returns whether the insertion succeeds.
        bool InsertTaskTypesAsChildren(AZStd::weak_ptr<BuilderDataItem> builderWeakPointer);
        //! This method is only called on Builder: inserts the entry as a child of entryTaskType in the tree. Returns the inserted item.
        AZStd::shared_ptr<BuilderDataItem> UpdateOrInsertEntry(
            TaskType entryTaskType, const AZStd::string& entryName, AZ::s64 entryJobCount, AZ::s64 entryTotalDuration);
    private:
        void UpdateMetrics(AZ::s64 jobCountDiff, AZ::s64 totalDurationDiff);

        AZStd::vector<AZStd::shared_ptr<BuilderDataItem>> m_children;
        AZStd::weak_ptr<BuilderDataItem> m_parent;
        AZStd::unordered_map<AZStd::string, int> m_childNameToIndex;
        AZStd::string m_name;
        AZ::s64 m_jobCount = 0;
        AZ::s64 m_totalDuration = 0;
        ItemType m_itemType = ItemType::Max;
    };
}

Q_DECLARE_METATYPE(AssetProcessor::BuilderDataItem*)
