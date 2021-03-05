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

#include <AzCore/RTTI/ReflectContext.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/ReplicaChunkInterface.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <ScriptCanvas/Variable/GraphVariableMarshal.h>

namespace ScriptCanvas
{
    class GraphVariable;
    class GraphVariableReplicaChunk;

    //! Core functionality for managing replicated Datums in a script canvas and the
    //! corresponding GridMate callbacks and data structs (DataSets).
    class GraphVariableNetBindingTable
        : public GridMate::ReplicaChunkInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphVariableNetBindingTable, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflect);

        GraphVariableNetBindingTable() = default;
        ~GraphVariableNetBindingTable() = default;

        GridMate::ReplicaChunkPtr GetNetworkBinding();
        void SetNetworkBinding(GridMate::ReplicaChunkPtr chunk);
        void UnbindFromNetwork();

        //! Gets called when the given Datum object is updated with a new value
        //! that was received over the network.
        void OnPropertyUpdate(const Datum* const & scriptProperty, const GridMate::TimeContext& tc);

        //! Adds the given Datum to the list of "synced datums" for this instance.
        void AddDatum(GraphVariable* variable);

        //! Called when local data changes for a Datum whose values should be replicated
        //! over the network.
        void OnDatumChanged(GraphVariable& variable);

        void SetVariableMappings(const AZStd::unordered_map<VariableId, VariableId>& assetToRuntimeVariableMap, const AZStd::unordered_map<VariableId, VariableId>& runtimeToAssetVariableMap);
        VariableId FindAssetVariableIdByRuntimeVariableId(VariableId runtimeVariableId);
        VariableId FindRuntimeVariableIdByAssetVariableId(VariableId assetVariableId);
        AZStd::unordered_map<VariableId, AZStd::pair<GraphVariable*, int>>& GetVariableIdMap();

    private:
        void SetGraphNetBindingTable();

    private:
        AZStd::unordered_map<VariableId, VariableId> m_assetToRuntimeVariableMap;
        AZStd::unordered_map<VariableId, VariableId> m_runtimeToAssetVariableMap;

        //! Replica chunk used for GridMate networking binding. See GraphVariableReplicaChunk.
        GridMate::ReplicaChunkPtr m_replicaChunk;

        //! Contains pointers to all replicated variables contained within the runtime component
        //! of the canvas this net binding is associated with.
        AZStd::unordered_map<VariableId, AZStd::pair<GraphVariable*, int>> m_variableIdMap;
    };

    typedef GridMate::DataSet<const Datum*, DatumMarshaler, DatumThrottler>::BindInterface<GraphVariableNetBindingTable, &GraphVariableNetBindingTable::OnPropertyUpdate> DatumDataSetType;

    class DatumDataSet
        : public DatumDataSetType
    {
    public:
        DatumDataSet();
        ~DatumDataSet() = default;

    private:
        const char* GetDataSetName();
    };

    class GraphVariableReplicaChunk
        : public GridMate::ReplicaChunkBase
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphVariableReplicaChunk, AZ::SystemAllocator, 0);

        static const char* GetChunkName();

        GraphVariableReplicaChunk() = default;
        ~GraphVariableReplicaChunk() = default;

        bool IsReplicaMigratable() override;

        DatumDataSet m_properties[GM_MAX_DATASETS_IN_CHUNK];
    };
}
