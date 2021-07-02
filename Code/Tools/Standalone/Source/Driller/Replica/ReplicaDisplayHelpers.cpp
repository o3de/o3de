/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/chrono/chrono.h>

#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"

#include "Source/Driller/StripChart.hxx"

#include <QPainter>
#include <QPixmap>

namespace Driller
{
    QColor GetRandomDisplayColor()
    {
        static AZ::SimpleLcgRandom s_randomGenerator(AZStd::chrono::system_clock::now().time_since_epoch().count());
        // Narrowing the range to avoid excessively dark colors
        return QColor(50 + s_randomGenerator.GetRandom() % 206, 50 + s_randomGenerator.GetRandom() % 206, 50 + s_randomGenerator.GetRandom() % 206);
    }

    //////////////////////
    // BaseDisplayHelper
    //////////////////////

    BaseDisplayHelper::BaseDisplayHelper()
        : m_graphEnabled(true)
        , m_selected(false)
        , m_inspected(false)
        , m_color(GetRandomDisplayColor())
        , m_areaGraphPlotHelper(m_color)
        , m_sentGraphPlot(m_color)
        , m_iconEnabled(true)
        , m_parent(nullptr)
    {
        QPixmap pixmap(16, 16);
        QPainter painter(&pixmap);
        painter.setBrush(m_color);
        painter.drawRect(0, 0, 16, 16);

        m_icon.addPixmap(pixmap);
    }

    BaseDisplayHelper::~BaseDisplayHelper()
    {
        for (BaseDisplayHelper* helper : m_children)
        {
            // Can't remove objects from the tree display since it messes with the
            // index ordering. So if we get into this error, just keep it there, but we won't delete it.
            if (helper->m_parent == this)
            {
                delete helper;
            }
        }

        m_children.clear();
    }

    void BaseDisplayHelper::Reset()
    {
        m_graphEnabled = true;
        m_selected = false;

        for (BaseDisplayHelper* helper : m_children)
        {
            helper->Reset();
        }

        OnReset();
        ResetGraphConfiguration();
        ResetBandwidthUsage();
    }

    void BaseDisplayHelper::ResetGraphConfiguration()
    {
        m_areaGraphPlotHelper.Reset();

        for (BaseDisplayHelper* helper : m_children)
        {
            helper->ResetGraphConfiguration();
        }

        OnResetGraphConfiguration();
    }

    void BaseDisplayHelper::ResetBandwidthUsage()
    {
        m_bandwidthUsageAggregator.Reset();

        for (BaseDisplayHelper* helper : m_children)
        {
            helper->ResetBandwidthUsage();
        }

        OnResetBandwidthUsage();
    }

    int BaseDisplayHelper::AddChild(BaseDisplayHelper* baseDisplayHelper)
    {
        if (baseDisplayHelper == nullptr)
        {
            return -1;
        }

        AZ_Assert(baseDisplayHelper->m_parent == nullptr, "Adding Leaf node to two parents in tree.");

        int index = static_cast<int>(m_children.size());
        m_children.push_back(baseDisplayHelper);

        baseDisplayHelper->m_parent = this;

        return index;
    }

    void BaseDisplayHelper::DetachChild(BaseDisplayHelper* baseDisplayHelper)
    {
        if (baseDisplayHelper == nullptr)
        {
            return;
        }

        AZ_Error("BaseDisplayHelper", baseDisplayHelper->m_parent == this, "Detaching a leaf node from the wrong parent.");

        if (baseDisplayHelper->m_parent == this)
        {
            for (AZStd::vector< BaseDisplayHelper* >::iterator displayIter = m_children.begin();
                 displayIter != m_children.end();
                 ++displayIter)
            {
                if ((*displayIter) == baseDisplayHelper)
                {
                    baseDisplayHelper->m_parent = nullptr;
                    m_children.erase(displayIter);
                    break;
                }
            }
        }
    }

    void BaseDisplayHelper::InspectSeries(size_t seriesId)
    {
        m_inspected = m_areaGraphPlotHelper.IsSeries(seriesId);

        for (BaseDisplayHelper* child : m_children)
        {
            child->InspectSeries(seriesId);
        }
    }

    BaseDisplayHelper* BaseDisplayHelper::FindChildByRow(int row)
    {
        BaseDisplayHelper* retVal = nullptr;

        if (row >= 0 && row < m_children.size())
        {
            retVal = m_children[row];
        }

        return retVal;
    }

    const BaseDisplayHelper* BaseDisplayHelper::FindChildByRow(int row) const
    {
        const BaseDisplayHelper* retVal = nullptr;

        if (row >= 0 && row < m_children.size())
        {
            retVal = m_children[row];
        }

        return retVal;
    }

    size_t BaseDisplayHelper::GetTreeRowCount() const
    {
        return m_children.size();
    }

    int BaseDisplayHelper::GetChildIndex(const BaseDisplayHelper* helper) const
    {
        int index = -1;

        for (size_t i = 0; i < m_children.size(); ++i)
        {
            if (m_children[i] == helper)
            {
                index = static_cast<int>(i);
                break;
            }
        }

        return index;
    }

    void BaseDisplayHelper::SetIconEnabled(bool iconEnabled)
    {
        m_iconEnabled = iconEnabled;
    }

    bool BaseDisplayHelper::HasIcon() const
    {
        return m_iconEnabled;
    }

    const QIcon& BaseDisplayHelper::GetIcon() const
    {
        if (m_graphEnabled)
        {
            return m_icon;
        }
        else
        {
            static QIcon s_blackIcon;
            static bool s_doOnce = true;

            if (s_doOnce)
            {
                s_doOnce = false;

                QPixmap pixmap(16, 16);
                QPainter painter(&pixmap);
                painter.setBrush(Qt::black);
                painter.drawRect(0, 0, 16, 16);

                s_blackIcon.addPixmap(pixmap);
            }

            return s_blackIcon;
        }
    }

    const AZStd::vector< BaseDisplayHelper* >& BaseDisplayHelper::GetChildren() const
    {
        return m_children;
    }

    const BaseDisplayHelper* BaseDisplayHelper::GetParent() const
    {
        return m_parent;
    }

    void BaseDisplayHelper::DetachAllChildren()
    {
        for (BaseDisplayHelper* helper : m_children)
        {
            helper->m_parent = nullptr;
        }

        m_children.clear();
    }

    void BaseDisplayHelper::OnReset()
    {
    }

    void BaseDisplayHelper::OnResetGraphConfiguration()
    {
    }

    void BaseDisplayHelper::OnResetBandwidthUsage()
    {
    }

    /////////////////////////
    // DataSetDisplayHelper
    /////////////////////////

    DataSetDisplayHelper::DataSetDisplayHelper(size_t dataSetIndex)
        : KeyedDisplayHelper(dataSetIndex)
    {
    }

    void DataSetDisplayHelper::SetDisplayName(const char* displayName)
    {
        if (displayName)
        {
            m_dataSetName = displayName;
        }
        else
        {
            m_dataSetName.clear();
        }
    }

    const char* DataSetDisplayHelper::GetDisplayName() const
    {
        return m_dataSetName.c_str();
    }

    /////////////////////
    // RPCDisplayHelper
    /////////////////////

    ///////////////////////////////
    // RPCInvokationDisplayHelper
    ///////////////////////////////

    RPCDisplayHelper::RPCInvokationDisplayHelper::RPCInvokationDisplayHelper(const AZStd::string& rpcName, int counter)        
    {
        SetIconEnabled(false);
        m_rpcName = AZStd::string::format("%s_%i",rpcName.c_str(),counter);
    }

    const char* RPCDisplayHelper::RPCInvokationDisplayHelper::GetDisplayName() const
    {
        return m_rpcName.c_str();
    }

    RPCDisplayHelper::RPCDisplayHelper(size_t rpcIndex)
        : KeyedDisplayHelper(rpcIndex)
    {
    }

    void RPCDisplayHelper::AddInvokation(const BandwidthUsage& bandwidthUsage)
    {
        m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
        m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;

        RPCInvokationDisplayHelper* invokationDisplayHelper = aznew RPCInvokationDisplayHelper(m_rpcName,static_cast<int>(GetChildren().size()));

        invokationDisplayHelper->m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
        invokationDisplayHelper->m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;

        AddChild(invokationDisplayHelper);
    }

    void RPCDisplayHelper::SetDisplayName(const char* displayName)
    {
        m_rpcName = displayName;
    }

    const char* RPCDisplayHelper::GetDisplayName() const
    {
        return m_rpcName.c_str();
    }

    void RPCDisplayHelper::OnResetBandwidthUsage()
    {
        DetachAllChildren();
    }

    ////////////////////////////
    // BaseDetailDisplayHelper
    ////////////////////////////

    BaseDetailDisplayHelper::BaseDetailDisplayHelper()
        : m_rpcDisplayFilter(aznew RPCDisplayFilter())
        , m_dataSetDisplayFilter(aznew DataSetDisplayFilter())
    {
        AddChild(m_rpcDisplayFilter);
        AddChild(m_dataSetDisplayFilter);
    }

    BaseDetailDisplayHelper::~BaseDetailDisplayHelper()
    {
        m_rpcDisplayFilter = nullptr;
        m_dataSetDisplayFilter = nullptr;
    }

    RPCDisplayHelper* BaseDetailDisplayHelper::FindRPC(size_t rpcIndex)
    {
        AZ_Error("BaseDetailDisplayHelper", m_rpcDisplayFilter->HasDisplayHelperForKey(rpcIndex), "Invalid RPC Index");
        return m_rpcDisplayFilter->FindDisplayHelperFromKey(rpcIndex);
    }

    const RPCDisplayHelper* BaseDetailDisplayHelper::FindRPC(size_t rpcIndex) const
    {
        return m_rpcDisplayFilter->FindDisplayHelperFromKey(rpcIndex);
    }

    void BaseDetailDisplayHelper::SetupRPC(size_t index, const char* rpcName)
    {
        if (!m_rpcDisplayFilter->HasDisplayHelperForKey(index))
        {
            RPCDisplayHelper* rpcDisplayHelper = m_rpcDisplayFilter->CreateDisplayHelperFromKey(index);
            rpcDisplayHelper->SetDisplayName(rpcName);
        }
    }

    void BaseDetailDisplayHelper::AddRPCUsage(const BandwidthUsage& bandwidthUsage)
    {
        m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
        m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;

        m_rpcDisplayFilter->m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
        m_rpcDisplayFilter->m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;

        RPCDisplayHelper* rpcDisplay = FindRPC(bandwidthUsage.m_index);

        if (rpcDisplay)
        {
            rpcDisplay->AddInvokation(bandwidthUsage);
        }
    }

    RPCDisplayFilter* BaseDetailDisplayHelper::GetRPCDisplayHelper()
    {
        return m_rpcDisplayFilter;
    }

    DataSetDisplayHelper* BaseDetailDisplayHelper::FindDataSet(size_t dataSetIndex)
    {
        AZ_Error("BaseDetailDisplayHelper", m_dataSetDisplayFilter->HasDisplayHelperForKey(dataSetIndex), "Invalid DataSetIndex");
        return m_dataSetDisplayFilter->FindDisplayHelperFromKey(dataSetIndex);
    }

    const DataSetDisplayHelper* BaseDetailDisplayHelper::FindDataSet(size_t dataSetIndex) const
    {
        return m_dataSetDisplayFilter->FindDisplayHelperFromKey(dataSetIndex);
    }

    void BaseDetailDisplayHelper::SetupDataSet(size_t dataSetIndex, const char* dataSetName)
    {
        if (!m_dataSetDisplayFilter->HasDisplayHelperForKey(dataSetIndex))
        {
            DataSetDisplayHelper* displayHelper = m_dataSetDisplayFilter->CreateDisplayHelperFromKey(dataSetIndex);
            displayHelper->SetDisplayName(dataSetName);
        }
    }

    void BaseDetailDisplayHelper::AddDataSetUsage(const BandwidthUsage& bandwidthUsage)
    {
        m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
        m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;

        m_dataSetDisplayFilter->m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
        m_dataSetDisplayFilter->m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;

        DataSetDisplayHelper* dataSetDisplay = FindDataSet(bandwidthUsage.m_index);

        if (dataSetDisplay)
        {
            dataSetDisplay->m_bandwidthUsageAggregator.m_bytesSent += bandwidthUsage.m_usageAggregator.m_bytesSent;
            dataSetDisplay->m_bandwidthUsageAggregator.m_bytesReceived += bandwidthUsage.m_usageAggregator.m_bytesReceived;
        }
    }

    DataSetDisplayFilter* BaseDetailDisplayHelper::GetDataSetDisplayHelper()
    {
        return m_dataSetDisplayFilter;
    }


    ////////////////////////////////////
    // ReplicaChunkDetailDisplayHelper
    ////////////////////////////////////

    ReplicaChunkDetailDisplayHelper::ReplicaChunkDetailDisplayHelper(const char* chunkTypeName, AZ::u32 chunkIndex)
        : m_chunkTypeName(chunkTypeName)
        , m_chunkIndex(chunkIndex)
    {
    }

    AZ::u32 ReplicaChunkDetailDisplayHelper::GetChunkIndex() const
    {
        return m_chunkIndex;
    }

    const char* ReplicaChunkDetailDisplayHelper::GetChunkTypeName() const
    {
        return m_chunkTypeName.c_str();
    }

    const char* ReplicaChunkDetailDisplayHelper::GetDisplayName() const
    {
        return GetChunkTypeName();
    }

    ///////////////////////////////
    // ReplicaDetailDisplayHelper
    ///////////////////////////////

    ReplicaDetailDisplayHelper::ReplicaDetailDisplayHelper(const char* replicaName, AZ::u64 replicaId)
        : m_replicaName(replicaName)
        , m_replicaId(replicaId)
    {
    }

    AZ::u64 ReplicaDetailDisplayHelper::GetReplicaId() const
    {
        return m_replicaId;
    }

    const char* ReplicaDetailDisplayHelper::GetReplicaName() const
    {
        return m_replicaName.c_str();
    }

    const char* ReplicaDetailDisplayHelper::GetDisplayName() const
    {
        return GetReplicaName();
    }

    //////////////////////////////////////
    // OverallReplicaDetailDisplayHelper
    //////////////////////////////////////

    OverallReplicaDetailDisplayHelper::OverallReplicaDetailDisplayHelper(const char* replicaName, AZ::u64 replicaId)
        : m_replicaName(replicaName)
        , m_replicaId(replicaId)
    {
    }

    OverallReplicaDetailDisplayHelper::~OverallReplicaDetailDisplayHelper()
    {        
        DetachAllChildren();

        for (auto& mapPair : m_replicaChunks)
        {
            delete mapPair.second;      
        }
    }

    AZ::u64 OverallReplicaDetailDisplayHelper::GetReplicaId() const
    {
        return m_replicaId;
    }

    const char* OverallReplicaDetailDisplayHelper::GetReplicaName() const
    {
        return m_replicaName.c_str();
    }

    const char* OverallReplicaDetailDisplayHelper::GetDisplayName() const
    {
        return GetReplicaName();
    }    

    ReplicaChunkDetailDisplayHelper* OverallReplicaDetailDisplayHelper::CreateReplicaChunkDisplayHelper(const AZStd::string& chunkName, AZ::u32 chunkIndex)
    {
        ReplicaChunkDetailDisplayHelper* displayHelper = nullptr;

        auto chunkIter = m_replicaChunks.find(chunkIndex);

        AZ_Error("OverallReplicaDetailDisplayHelper", chunkIter == m_replicaChunks.end(), "Trying to create two replica chunks with the same chunk index for a given replica.");

        if (chunkIter == m_replicaChunks.end())
        {
            displayHelper = aznew ReplicaChunkDetailDisplayHelper(chunkName.c_str(), chunkIndex);
            m_replicaChunks.emplace(chunkIndex, displayHelper);

            AddChild(displayHelper);
        }

        return displayHelper;
    }

    ReplicaChunkDetailDisplayHelper* OverallReplicaDetailDisplayHelper::FindReplicaChunk(AZ::u32 chunkIndex)
    {
        ReplicaChunkDetailDisplayHelper* detailDisplayHelper = nullptr;

        auto chunkIter = m_replicaChunks.find(chunkIndex);
        if (chunkIter != m_replicaChunks.end())
        {
            detailDisplayHelper = chunkIter->second;
        }

        return detailDisplayHelper;
    }
}
