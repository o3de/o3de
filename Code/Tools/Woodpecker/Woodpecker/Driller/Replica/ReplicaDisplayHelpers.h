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

#ifndef DRILLER_REPLICA_REPLICADISPLAYHELPERS_H
#define DRILLER_REPLICA_REPLICADISPLAYHELPERS_H

#include <AzCore/base.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <QColor>
#include <QIcon>

// QModelIndex
#include <QAbstractItemModel>

#include "Woodpecker/Driller/DrillerDataTypes.h"
#include "Woodpecker/Driller/Replica/ReplicaBandwidthChartData.h"

namespace StripChart
{
    class DataStrip;
}

namespace Driller
{
    QColor GetRandomDisplayColor();

    // Combo TreeView/Graph Display
    class BaseDisplayHelper;

    class BaseDisplayHelper
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseDisplayHelper, "{8F84783A-B924-4081-9F6E-51524071B7BB}");

        BaseDisplayHelper();
        virtual ~BaseDisplayHelper();

        void Reset();
        void ResetGraphConfiguration();
        void ResetBandwidthUsage();

        virtual int AddChild(BaseDisplayHelper* baseDisplayHelper);
        virtual void DetachChild(BaseDisplayHelper* baseDisplayHelper);

        void InspectSeries(size_t seriesId);

        virtual BaseDisplayHelper*  FindChildByRow(int row);
        virtual const BaseDisplayHelper* FindChildByRow(int row) const;
        virtual size_t GetTreeRowCount() const;

        virtual int GetChildIndex(const BaseDisplayHelper* helper) const;

        virtual const char* GetDisplayName() const = 0;

        void SetIconEnabled(bool iconEnabled);
        bool HasIcon() const;
        const QIcon& GetIcon() const;

        const AZStd::vector< BaseDisplayHelper* >& GetChildren() const;

        const BaseDisplayHelper* GetParent() const;

        // Information needed for graphing
        bool m_graphEnabled;
        bool m_selected;
        bool m_inspected;

        QColor m_color;

        BandwidthUsageAggregator m_bandwidthUsageAggregator;
        AreaGraphPlotHelper m_areaGraphPlotHelper;

        GraphPlotHelper m_sentGraphPlot;

    protected:

        virtual void DetachAllChildren();

        virtual void OnReset();
        virtual void OnResetGraphConfiguration();
        virtual void OnResetBandwidthUsage();

    private:

        bool   m_iconEnabled;
        QIcon  m_icon;

        BaseDisplayHelper*  m_parent;

        AZStd::vector< BaseDisplayHelper* >         m_children;
    };

    template<typename Key>
    class KeyedDisplayHelper
        : public BaseDisplayHelper
    {
    public:
        KeyedDisplayHelper(Key key)
            : m_key(key)
        {}

        const Key& GetKey() const
        {
            return m_key;
        }

    private:
        Key m_key;
    };

    template<typename Key, class DisplayType>
    class FilteredDisplayHelper
        : public BaseDisplayHelper
    {
        typedef AZStd::unordered_map< Key, int > KeyToIndexMapping;
    public:
        AZ_CLASS_ALLOCATOR(FilteredDisplayHelper, AZ::SystemAllocator, 0);

        FilteredDisplayHelper(const char* displayName)
            : m_displayName(displayName)
        {
        }

        virtual ~FilteredDisplayHelper()
        {
            DetachAllChildren();

            for (auto& mapPair : m_displayHelperMap)
            {
                delete mapPair.second;
            }
        }

        const char* GetDisplayName() const override
        {
            return m_displayName.c_str();
        }

        int AddChild(BaseDisplayHelper* baseDisplayHelper) override
        {
            AZ_Assert(false, "Unsupported behavior");
            (void)baseDisplayHelper;
            return -1;
        }

        int GetChildIndex(const BaseDisplayHelper* helper) const override
        {
            const KeyedDisplayHelper<Key>* keyedHelper = static_cast<const KeyedDisplayHelper<Key>*>(helper);

            const Key& key = keyedHelper->GetKey();

            int foundIndex = -1;

            for (size_t i = 0; i < m_displayOrdering.size(); ++i)
            {
                if (m_displayOrdering[i] == key)
                {
                    foundIndex = static_cast<int>(i);
                    break;
                }
            }

            return foundIndex;
        }

        bool HasDisplayHelperForKey(const Key& key) const
        {
            return m_keyMapping.find(key) != m_keyMapping.end();
        }

        DisplayType* CreateDisplayHelperFromKey(const Key& key)
        {
            DisplayType* retVal = nullptr;

            typename AZStd::unordered_map< Key, DisplayType* >::iterator helperIter = m_displayHelperMap.find(key);

            if (helperIter == m_displayHelperMap.end())
            {
                retVal = aznew DisplayType(key);
                m_displayHelperMap[key] = retVal;
            }
            else
            {
                retVal = m_displayHelperMap[key];
            }

            typename KeyToIndexMapping::iterator indexIter = m_keyMapping.find(key);

            if (indexIter == m_keyMapping.end())
            {
                if (retVal)
                {
                    int index = BaseDisplayHelper::AddChild(retVal);
                    m_keyMapping[key] = index;
                }
            }

            if (m_activeDisplay.insert(key).second)
            {
                m_displayOrdering.push_back(key);
            }

            return retVal;
        }

        DisplayType* FindDisplayHelperFromKey(const Key& key)
        {
            DisplayType* retVal = nullptr;

            typename KeyToIndexMapping::iterator indexIter = m_keyMapping.find(key);

            if (indexIter != m_keyMapping.end())
            {
                retVal = static_cast<DisplayType*>(BaseDisplayHelper::FindChildByRow(indexIter->second));
            }

            return retVal;
        }

        const DisplayType* FindDisplayHelperFromKey(const Key& key) const
        {
            const DisplayType* retVal = nullptr;

            typename KeyToIndexMapping::const_iterator indexIter = m_keyMapping.find(key);

            if (indexIter != m_keyMapping.end())
            {
                retVal = static_cast<const DisplayType*>(BaseDisplayHelper::FindChildByRow(indexIter->second));
            }

            return retVal;
        }

        BaseDisplayHelper* FindChildByRow(int row) override
        {
            DisplayType* retVal = nullptr;

            if (row >= 0 && row < m_displayOrdering.size())
            {
                Key key = m_displayOrdering[row];
                retVal = FindDisplayHelperFromKey(key);
            }

            return retVal;
        }

        const DisplayType* FindChildByRow(int row) const override
        {
            const DisplayType* retVal = nullptr;

            if (row >= 0 && row < m_displayOrdering.size())
            {
                Key key = m_displayOrdering[row];
                retVal = FindDisplayHelperFromKey(key);
            }

            return retVal;
        }

        size_t GetTreeRowCount() const override
        {
            return m_displayOrdering.size();
        }

        void ClearActiveDisplay()
        {
            DetachAllChildren();

            m_activeDisplay.clear();
            m_keyMapping.clear();
            m_displayOrdering.clear();
        }

    protected:
        void OnChildActivated(BaseDisplayHelper* helper, int index)
        {
            const KeyedDisplayHelper<Key>* keyedHelper = static_cast<const KeyedDisplayHelper<Key>*>(helper);
            const Key& key = keyedHelper->GetKey();

            m_keyMapping[key] = index;

            if (m_activeDisplay.insert(key).second)
            {
                m_displayOrdering.push_back(key);
            }
        }

        void OnChildDeactivated(BaseDisplayHelper* helper)
        {
            const KeyedDisplayHelper<Key>* keyedHelper = static_cast<const KeyedDisplayHelper<Key>*>(helper);
            const Key& key = keyedHelper->GetKey();

            m_displayOrdering.erase(m_displayOrdering.begin() + m_keyMapping[key]);
            m_keyMapping.erase(key);
            m_activeDisplay.erase(key);
        }

    private:

        void OnReset() override
        {
            m_keyMapping.clear();
            m_activeDisplay.clear();
            m_displayOrdering.clear();
        }

        void OnResetGraphConfiguration() override
        {
        }

        AZStd::string m_displayName;

        KeyToIndexMapping m_keyMapping;
        AZStd::unordered_set< Key > m_activeDisplay;
        AZStd::vector< Key > m_displayOrdering;

        AZStd::unordered_map< Key, DisplayType* > m_displayHelperMap;
    };

    class DataSetDisplayHelper
        : public KeyedDisplayHelper<size_t>
    {
    public:
        AZ_CLASS_ALLOCATOR(DataSetDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(DataSetDisplayHelper, "{74A47E69-1DF5-40E7-A471-BF84B62182A8}", BaseDisplayHelper);

        DataSetDisplayHelper(size_t dataSetIndex);

        void SetDisplayName(const char* displayName);
        const char* GetDisplayName() const override;

    private:

        AZStd::string m_dataSetName;
    };

    class DataSetDisplayFilter
        : public FilteredDisplayHelper<size_t, DataSetDisplayHelper>
    {
    public:
        AZ_CLASS_ALLOCATOR(DataSetDisplayFilter, AZ::SystemAllocator, 0);
        AZ_RTTI(DataSetDisplayFilter, "{C0B802CD-5551-48C0-95C2-41607D42A2E1}", BaseDisplayHelper);

        DataSetDisplayFilter()
            : FilteredDisplayHelper("DataSets")
        {
        }
    };

    class RPCDisplayHelper
        : public KeyedDisplayHelper<size_t>
    {      
    private:
        class RPCInvokationDisplayHelper
            : public BaseDisplayHelper
        {
        public:
            AZ_CLASS_ALLOCATOR(RPCInvokationDisplayHelper, AZ::SystemAllocator, 0);

            RPCInvokationDisplayHelper(const AZStd::string& name, int count);

            const char* GetDisplayName() const override;

        private:
            
            AZStd::string m_rpcName;
        };

    public:
        AZ_CLASS_ALLOCATOR(RPCDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(RPCDisplayHelper, "{564003A2-7880-441A-AC51-5397730C2E31}", BaseDisplayHelper);

        RPCDisplayHelper(size_t rpcName);

        void AddInvokation(const BandwidthUsage& bandwidthUsage);

        void SetDisplayName(const char* displayName);
        const char* GetDisplayName() const override;

    protected:

        void OnResetBandwidthUsage() override;

    private:

        AZStd::string m_rpcName;
    };

    class RPCDisplayFilter
        : public FilteredDisplayHelper<size_t, RPCDisplayHelper>
    {
    public:
        AZ_CLASS_ALLOCATOR(DataSetDisplayFilter, AZ::SystemAllocator, 0);
        AZ_RTTI(RPCDisplayFilter, "{1AF8368E-C5AF-4936-85C5-BA67E62FF871}", BaseDisplayHelper);

        RPCDisplayFilter()
            : FilteredDisplayHelper("RPCs")
        {
        }
    };    

    class BaseDetailDisplayHelper
        : public BaseDisplayHelper
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseDetailDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseDetailDisplayHelper, "{22B3809C-20A5-407B-9302-7890CEF4821D}", BaseDisplayHelper);

        BaseDetailDisplayHelper();
        virtual ~BaseDetailDisplayHelper();

        RPCDisplayHelper* FindRPC(size_t rpcIndex);
        const RPCDisplayHelper* FindRPC(size_t rpcIndex) const;
        void SetupRPC(size_t rpcIndex, const char* rpcName);
        void AddRPCUsage(const BandwidthUsage& currentUsage);

        RPCDisplayFilter* GetRPCDisplayHelper();

        DataSetDisplayHelper* FindDataSet(size_t dataSetIndex);
        const DataSetDisplayHelper* FindDataSet(size_t dataSetIndex) const;
        void SetupDataSet(size_t dataSetIndex, const char* dataSetName);
        void AddDataSetUsage(const BandwidthUsage& currentUsage);

        DataSetDisplayFilter* GetDataSetDisplayHelper();

    protected:

        RPCDisplayFilter* m_rpcDisplayFilter;
        DataSetDisplayFilter* m_dataSetDisplayFilter;
    };

    class ReplicaChunkDetailDisplayHelper
        : public BaseDetailDisplayHelper
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkDetailDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaChunkDetailDisplayHelper, "{9DBE2EFE-AA89-4527-A003-1EE08B9E3DB7}", BaseDetailDisplayHelper);

        ReplicaChunkDetailDisplayHelper(const char* chunkTypeName, AZ::u32 chunkIndex);

        AZ::u32     GetChunkIndex() const;
        const char* GetChunkTypeName() const;

        const char* GetDisplayName() const override;
    private:

        AZStd::string m_chunkTypeName;
        int m_chunkIndex;
    };    

    class ReplicaDetailDisplayHelper
        : public BaseDetailDisplayHelper
    {
    public:
        AZ_CLASS_ALLOCATOR(ReplicaDetailDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(ReplicaDetailDisplayHelper, "{9DBE2EFE-AA89-4527-A003-1EE08B9E3DB7}", BaseDetailDisplayHelper);

        ReplicaDetailDisplayHelper(const char* replicaName, AZ::u64 replicaId);

        AZ::u64 GetReplicaId() const;
        const char* GetReplicaName() const;

        const char* GetDisplayName() const override;

    private:

        AZStd::string   m_replicaName;
        AZ::u64         m_replicaId;
    };

    class OverallReplicaDetailDisplayHelper
        : public BaseDisplayHelper
    {
    public:
        AZ_CLASS_ALLOCATOR(OverallReplicaDetailDisplayHelper, AZ::SystemAllocator, 0);
        AZ_RTTI(OverallReplicaDetailDisplayHelper, "{1CE46BA7-DA92-4C4E-8294-F5E096D14622}", BaseDisplayHelper);

        OverallReplicaDetailDisplayHelper(const char* replicaName, AZ::u64 replicaId);
        ~OverallReplicaDetailDisplayHelper();

        AZ::u64 GetReplicaId() const;
        const char* GetReplicaName() const;

        const char* GetDisplayName() const override;
        
        bool HasReplicaChunk(int chunkIndex);
        ReplicaChunkDetailDisplayHelper* CreateReplicaChunkDisplayHelper(const AZStd::string& chunkName, AZ::u32 chunkIndex);
        ReplicaChunkDetailDisplayHelper* FindReplicaChunk(AZ::u32 chunkIndex);

    private:

        AZStd::string   m_replicaName;
        AZ::u64         m_replicaId;

        AZStd::unordered_map<AZ::u32, ReplicaChunkDetailDisplayHelper*> m_replicaChunks;
    };
}

#endif