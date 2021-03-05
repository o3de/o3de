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
#ifndef AZFRAMEWORK_SCRIPT_NET_BINDINGS_H
#define AZFRAMEWORK_SCRIPT_NET_BINDINGS_H

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <GridMate/Replica/ReplicaChunkInterface.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/RemoteProcedureCall.h>

#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Script/ScriptPropertyTable.h>
#include <AzCore/Script/ScriptPropertyWatcherBus.h>
#include <AzFramework/Script/ScriptMarshal.h>

namespace AzFramework
{
    class ScriptPropertyDataSet;
    class ScriptComponentReplicaChunk;

    // ScriptNetBindingTable will act as the go between for the ScriptComponent and the Replica's.
    // It will also allow for holding of values in the case where you haven't been bound to a replica chunk yet and the
    // script tries to interact with something that is networked.
    //
    // Allows for scripts to be re-used seamlessly in a offline vs online scenario(and support for going from offline to online),
    // including RPCs(will alawys call the master version if offline)
    class ScriptNetBindingTable
        : public GridMate::ReplicaChunkInterface
    {
    private:
        friend class ScriptComponentReplicaChunk;
        friend class ScriptPropertyDataSet;

        // Helper struct to keep track of a a ScriptConctext
        // and the entityTableReference. Mainly used for
        // calling in to functions in LUA where we want
        // to push in the table reference as the first parameter
        struct EntityScriptContext
        {
        public:
            EntityScriptContext();

            void Unload();

            bool HasEntityTableRegistryIndex() const;
            int GetEntityTableRegistryIndex() const;

            bool HasScriptContext() const;
            AZ::ScriptContext* GetScriptContext() const;

            void ConfigureContext(AZ::ScriptContext* scriptContext, int entityTableRegistryIndex);

        private:

            bool SanityCheckContext() const;

            AZ::ScriptContext* m_scriptContext;
            int m_entityTableRegistryIndex;
        };

        class NetworkedTableValue;
        friend NetworkedTableValue;

        typedef AZStd::unordered_map<AZStd::string, NetworkedTableValue> NetworkedTableMap;        

        class RPCBindingHelper;
        friend RPCBindingHelper;

        typedef AZStd::unordered_map<AZStd::string, RPCBindingHelper> RPCHelperMap;

        // Helper class that will wrap up our interactions with the actual stored value
        // to hide the general use case of if we are connected to a replica or not.        
        //
        // Additionally this will serve as a holding ground for a 'networked'
        // value that doesn't have a dataset.
        //
        // Lastly holds onto the Callback references.
        class NetworkedTableValue
        {
        public:
            AZ_CLASS_ALLOCATOR(NetworkedTableValue, AZ::SystemAllocator, 0);

            NetworkedTableValue(AZ::ScriptProperty* initialValue = nullptr);
            ~NetworkedTableValue();

            void Destroy();

            // Methods to register this value to a chunk
            bool HasDataSet() const;            
            void RegisterDataSet(ScriptPropertyDataSet* dataSet);
            void UnbindFromDataSet();
            ScriptPropertyDataSet* GetDataSet() const;

            // Information kept in order to force these values to use a particular dataset for debugging.
            bool HasForcedDataSetIndex() const;
            void SetForcedDataSetIndex(int index);
            int GetForcedDataSetIndex() const;            

            // Callback functions
            bool HasCallback() const;
            void RegisterCallback(int functionReference);
            void ReleaseCallback(AZ::ScriptContext& scriptContext);
            void InvokeCallback(EntityScriptContext& scriptContext, const GridMate::TimeContext& timeContext);
            
            bool AssignValue(AZ::ScriptDataContext& scriptDataContext, const AZStd::string& propertyName);
            bool InspectValue(AZ::ScriptContext* scriptContext) const;            

        private:

            // This value will be used if we have a networked property, but don't have a valid chunk yet.
            // Works as a temporary store, which will be resolved once we get assigned to a DataSet
            AZ::ScriptProperty* m_shimmedScriptProperty;

            // The data set we are bound to
            ScriptPropertyDataSet* m_dataSet;
            int     m_forcedDataSetIndex;
            
            int     m_functionReference;
        };

        // Future thoughts
        // - Move the actual RPC meta table creation
        //   into this guy
        class RPCBindingHelper
        {
        public:
            AZ_CLASS_ALLOCATOR(RPCBindingHelper, AZ::SystemAllocator, 0);

            RPCBindingHelper();
            ~RPCBindingHelper();            

            void ReleaseTableIndex(AZ::ScriptContext& scriptContext);

            bool IsValid() const;
            
            void SetMasterFunction(int masterReference);
            bool InvokeMaster(EntityScriptContext& entityScriptContext, const ScriptRPCMarshaler::Container& params);

            void SetProxyFunction(int masterReference);
            void InvokeProxy(EntityScriptContext& entityScriptContext, const ScriptRPCMarshaler::Container& params);

        private:            
            int m_masterReference;            
            int m_proxyReference;
        };

    public:
        AZ_CLASS_ALLOCATOR(ScriptNetBindingTable, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* reflect);

        ScriptNetBindingTable();
        ~ScriptNetBindingTable();

        void Unload();

        void CreateNetworkBindingTable(AZ::ScriptContext* scriptContext, int baseTableIndex, int entityTableIndex);
        void FinalizeNetworkTable(AZ::ScriptContext* scriptContext, int entityTableRegistryIndex);

        AZ::ScriptContext* GetScriptContext() const;

        bool IsMaster() const;

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // DataSet Functionality
        //
        // Called when the script wants to bind a function callback to when
        // a value changes
        //
        // Might change this to just be register DataSet
        bool RegisterDataSet(AZ::ScriptDataContext& stackContext, AZ::ScriptProperty* scriptProperty);        

        // Called when the script wants to assign a value to the script value
        bool AssignTableValue(AZ::ScriptDataContext& stackContext);

        // Called when the script wants to know the value of a script value.
        bool InspectTableValue(AZ::ScriptDataContext& stackContext) const;
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        /// RPC Functionality
        void RegisterRPC(AZ::ScriptDataContext& rpcTableContext, const AZStd::string& rpcName, int elementIndex, int tableStackIndex);
        bool InvokeRPC(AZ::ScriptDataContext& stackContext);
        //////////////////////////////////////////////////////////////////////////////////////////////////////

        // Netbinding Interface duplication here to be called from the ScriptComponent
        GridMate::ReplicaChunkPtr GetNetworkBinding();
        void SetNetworkBinding(GridMate::ReplicaChunkPtr chunk);
        void UnbindFromNetwork();    

        void OnPropertyUpdate(AZ::ScriptProperty*const& scriptProperty, const GridMate::TimeContext& tc);
        bool OnInvokeRPC(AZStd::string functionName, AZStd::vector< AZ::ScriptProperty*> properties, const GridMate::RpcContext& rpcContext);

    private:

        void RegisterMetaTableCache();

        template<typename PropertyType, typename PropertyArrayType>
        AZ::ScriptPropertyTable* ConvertPropertyArrayToTable(PropertyArrayType* arrayProperty)
        {
            AZ::ScriptPropertyTable* scriptPropertyTable = aznew AZ::ScriptPropertyTable(arrayProperty->m_name.c_str());

            PropertyType propertyType;

            for (unsigned int i=0; i < arrayProperty->m_values.size(); ++i)
            {
                propertyType.m_value = arrayProperty->m_values[i];

                // Offset by  1 to deal with lua 1 indexing.
                // Table will make a clone of our object.
                scriptPropertyTable->SetTableValue(i+1, &propertyType);
            }            

            return scriptPropertyTable;
        }

        void AssignDataSets();

        NetworkedTableValue* FindTableValue(const AZStd::string& name);
        const NetworkedTableValue* FindTableValue(const AZStd::string& name) const;

        EntityScriptContext       m_entityScriptContext;

        GridMate::ReplicaChunkPtr m_replicaChunk;

        NetworkedTableMap         m_networkedTable;
        RPCHelperMap              m_rpcHelperMap;
    };

    // Typedeffing out the RPC and DataSet definitions.
    typedef GridMate::Rpc< GridMate::RpcArg< AZStd::string >, GridMate::RpcArg< ScriptRPCMarshaler::Container, ScriptRPCMarshaler > >::BindInterface<ScriptNetBindingTable, &ScriptNetBindingTable::OnInvokeRPC> ScriptPropertyRPC;
    typedef GridMate::DataSet<AZ::ScriptProperty*, ScriptPropertyMarshaler, ScriptPropertyThrottler>::BindInterface<ScriptNetBindingTable, &ScriptNetBindingTable::OnPropertyUpdate> ScriptPropertyDataSetType;

    class ScriptComponentReplicaChunk;

    // Specialized DataSet used by the ScriptProperties, just to add some wrapped around functionality
    // and to allow me to manipulate the DataSet throttler in order to properly manage a dirty flag
    class ScriptPropertyDataSet
        : public ScriptPropertyDataSetType
        , public AZ::ScriptPropertyWatcherBus::Handler
        , public AZ::ScriptPropertyWatcher
    {
    private:
        friend class ScriptComponentReplicaChunk;
        friend class ScriptNetBindingTable::NetworkedTableValue;

        const char* GetDataSetName();

    public:
        ScriptPropertyDataSet();
        ~ScriptPropertyDataSet();
        bool IsReserved() const;

        bool UpdateScriptProperty(AZ::ScriptDataContext& scriptDataContext, const AZStd::string& propertyName);
        void SetScriptProperty(AZ::ScriptProperty* scriptProperty);                

        void OnObjectModified() override;

    private:
        void Reserve(ScriptNetBindingTable::NetworkedTableValue* reserver);
        void Release(ScriptNetBindingTable::NetworkedTableValue* reserver);

        ScriptNetBindingTable::NetworkedTableValue* m_reserver;
    };

    // The actual ReplicaChunk that the script will use
    class ScriptComponentReplicaChunk
        : public GridMate::ReplicaChunkBase
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptComponentReplicaChunk, AZ::SystemAllocator,0);
        static const int k_maxScriptableDataSets = GM_MAX_DATASETS_IN_CHUNK;
        
        static const char* GetChunkName() { return "ScriptComponentReplicaChunk"; }
        
        // Might want to add some type of comment field into the various fields so this can be properly parsed
        // and determined what we are actually sending.
        ScriptComponentReplicaChunk();
        ~ScriptComponentReplicaChunk();
        
        bool IsReplicaMigratable() override;

        AZ::u32 CalculateDirtyDataSetMask(GridMate::MarshalContext& marshalContext) override;
        
        // Called from the Master, will assign the table value to the DataSet specified by the helper.
        bool AssignDataSet(ScriptNetBindingTable::NetworkedTableValue& helper);        

        // Called from teh Proxy. Will Assign the TableValue to the DataSet that contains the target property        
        void AssignDataSetForProperty(ScriptNetBindingTable::NetworkedTableValue& helper, AZ::ScriptProperty* targetProperty);

        // Only called inside of an assert, checks that the DataSet that the targetProperty is in is the same as the assumedDataSet
        // Used to confirm that we don't get a confusion between master/proxy about which ScriptProperty is assigned to which DataSet.
        bool SanityCheckDataSet(AZ::ScriptProperty* targetProperty, ScriptPropertyDataSet* assumedDataSet);

        ScriptPropertyRPC       m_scriptRPC;
        
    private:        
        AZ::u32                 m_enabledDataSetMask;
        ScriptPropertyDataSet   m_propertyDataSets[k_maxScriptableDataSets];
    };
}

#endif
