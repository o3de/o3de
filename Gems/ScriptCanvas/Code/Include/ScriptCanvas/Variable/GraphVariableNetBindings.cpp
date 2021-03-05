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

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Variable/GraphVariable.h>
#include <ScriptCanvas/Variable/GraphVariableNetBindings.h>
#include <ScriptCanvas/Core/Datum.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <AzFramework/Network/NetworkContext.h>

namespace ScriptCanvas
{
    const char* DatumDataSet::GetDataSetName()
    {
        static size_t       s_chunkIndex = 0;
        static const char*  s_nameArray[] = {
            "DataSet1","DataSet2","DataSet3","DataSet4","DataSet5",
            "DataSet6","DataSet7","DataSet8","DataSet9","DataSet10",
            "DataSet11","DataSet12","DataSet13","DataSet14","DataSet15",
            "DataSet16","DataSet17","DataSet18","DataSet19","DataSet20",
            "DataSet21","DataSet22","DataSet23","DataSet24","DataSet25",
            "DataSet26","DataSet27","DataSet28","DataSet29","DataSet30",
            "DataSet31","DataSet32"
        };

        if (s_chunkIndex > AZ_ARRAY_SIZE(s_nameArray) && AZ_ARRAY_SIZE(s_nameArray) >= 0)
        {
            s_chunkIndex = s_chunkIndex % AZ_ARRAY_SIZE(s_nameArray);
        }

        return s_nameArray[s_chunkIndex++];
    }

    DatumDataSet::DatumDataSet()
        : DatumDataSetType(DatumDataSet::GetDataSetName())
    {
    }

    //////////////////////////
    // GraphVariableReplicaChunk
    //////////////////////////

    const char* GraphVariableReplicaChunk::GetChunkName()
    {
        return "GraphVariableReplicaChunk";
    }

    bool GraphVariableReplicaChunk::IsReplicaMigratable()
    {
        return true;
    }

    //////////////////////////
    // GraphVariableNetBindingTable
    //////////////////////////

    void GraphVariableNetBindingTable::Reflect([[maybe_unused]] AZ::ReflectContext* reflect)
    {
        GridMate::ReplicaChunkDescriptorTable& descriptorTable = GridMate::ReplicaChunkDescriptorTable::Get();
        AZ::Crc32 hash = GridMate::ReplicaChunkClassId(GraphVariableReplicaChunk::GetChunkName());

        if (!descriptorTable.FindReplicaChunkDescriptor(hash))
        {
            descriptorTable.RegisterChunkType<GraphVariableReplicaChunk>();
        }
    }

    GridMate::ReplicaChunkPtr GraphVariableNetBindingTable::GetNetworkBinding()
    {
        if (!m_replicaChunk)
        {
            m_replicaChunk = GridMate::CreateReplicaChunk<GraphVariableReplicaChunk>();
            m_replicaChunk->SetHandler(this);
            SetGraphNetBindingTable();
        }

        return m_replicaChunk;
    }

    void GraphVariableNetBindingTable::SetNetworkBinding(GridMate::ReplicaChunkPtr chunk)
    {
        m_replicaChunk = chunk;
        m_replicaChunk->SetHandler(this);
        SetGraphNetBindingTable();
    }

    void GraphVariableNetBindingTable::UnbindFromNetwork()
    {
        if (m_replicaChunk)
        {
            m_replicaChunk->SetHandler(nullptr);
            m_replicaChunk = nullptr;
        }
    }

    void GraphVariableNetBindingTable::OnPropertyUpdate([[maybe_unused]] const Datum* const & scriptProperty, [[maybe_unused]] const GridMate::TimeContext& tc)
    {
    }

    void GraphVariableNetBindingTable::AddDatum(GraphVariable* variable)
    {
        size_t index = m_variableIdMap.size();

        m_variableIdMap[variable->GetVariableId()] = AZStd::make_pair(variable, static_cast<int>(index));
    }

    void GraphVariableNetBindingTable::OnDatumChanged(GraphVariable& variable)
    {
        if (m_replicaChunk && m_replicaChunk->IsMaster())
        {
            GraphVariableReplicaChunk* graphVarChunk = static_cast<GraphVariableReplicaChunk*>(m_replicaChunk.get());
            auto iter = m_variableIdMap.find(variable.GetVariableId());

            if (iter == m_variableIdMap.end())
            {
                AZ_TracePrintf("ScriptCanvasNetworking", "GraphVariableNetBindingTable::OnDatumChanged: variable not found");
                return;
            }

            const AZStd::pair<GraphVariable*, int>& pair = iter->second;
            DatumDataSet& datumDataSet = graphVarChunk->m_properties[pair.second];
            datumDataSet.GetThrottler().SignalDirty();
            datumDataSet.Set(variable.GetDatum());
        }
    }

    void GraphVariableNetBindingTable::SetVariableMappings(const AZStd::unordered_map<VariableId, VariableId>& assetToRuntimeVariableMap, const AZStd::unordered_map<VariableId, VariableId>& runtimeToAssetVariableMap)
    {
        m_assetToRuntimeVariableMap = assetToRuntimeVariableMap;
        m_runtimeToAssetVariableMap = runtimeToAssetVariableMap;
    }

    VariableId GraphVariableNetBindingTable::FindAssetVariableIdByRuntimeVariableId(VariableId runtimeVariableId)
    {
        auto iter = m_runtimeToAssetVariableMap.find(runtimeVariableId);

        if (iter != m_runtimeToAssetVariableMap.end())
        {
            return iter->second;
        }

        return VariableId();
    }

    VariableId GraphVariableNetBindingTable::FindRuntimeVariableIdByAssetVariableId(VariableId assetVariableId)
    {
        auto iter = m_assetToRuntimeVariableMap.find(assetVariableId);

        if (iter != m_assetToRuntimeVariableMap.end())
        {
            return iter->second;
        }

        return VariableId();
    }

    AZStd::unordered_map<VariableId, AZStd::pair<GraphVariable*, int>>& GraphVariableNetBindingTable::GetVariableIdMap()
    {
        return m_variableIdMap;
    }

    void GraphVariableNetBindingTable::SetGraphNetBindingTable()
    {
        GraphVariableReplicaChunk* graphVariableChunk = static_cast<GraphVariableReplicaChunk*>(m_replicaChunk.get());

        for (DatumDataSet& dataSet : graphVariableChunk->m_properties)
        {
            dataSet.GetMarshaler().SetNetBindingTable(this);
        }
    }
}