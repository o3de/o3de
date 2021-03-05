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
#include <AzFramework/Script/ScriptNetBindings.h>

#include <AzFramework/Network/NetworkContext.h>
#include <AzFramework/Script/ScriptComponent.h>

#include <GridMate/Replica/ReplicaFunctions.h>

extern "C" {
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

namespace AzFramework
{
    namespace Internal
    {
        static int NetBinding__IsAuthoritative(lua_State* lua)
        {
            ScriptNetBindingTable* netBindingTable = reinterpret_cast<ScriptNetBindingTable*>(lua_touserdata(lua,lua_upvalueindex(1)));
            
            //AZ_Assert(netBindingTable != nullptr && netBindingTable->GetScriptContext(),"Missing or misconfigured ScriptNetBindingTable as upvalue in lua_cclosure");
            if (netBindingTable && netBindingTable->GetScriptContext())
            {                
                lua_pushboolean(lua,netBindingTable->IsMaster());
            }
            else
            {
                // If we don't have a net binding table, or a a script context, return true
                // since we likely are the master.
                lua_pushboolean(lua,true);
            }

            return 1;
        }

        static int NetBinding__IsMaster(lua_State* lua)
        {
            AZ_Warning("ScriptNetBindings", false, "IsMaster deprecated for the more lexical consistent IsAuthoritative");
            return NetBinding__IsAuthoritative(lua);
        }

        static int NetBinding__CallRPC(lua_State* lua)
        {
            ScriptNetBindingTable* netBindingTable = reinterpret_cast<ScriptNetBindingTable*>(lua_touserdata(lua,lua_upvalueindex(1)));
            
            AZ_Error("ScriptComponent", netBindingTable != nullptr,"Missing ScriptNetBindingTable as upvalue in lua_cclosure");
            AZ_Error("ScriptComponent", netBindingTable == nullptr || netBindingTable->GetScriptContext(),"Missing ScriptNetBindingTable as upvalue in lua_cclosure");
            if (netBindingTable && netBindingTable->GetScriptContext())
            {
                AZ::ScriptContext* context = netBindingTable->GetScriptContext();

                AZ::ScriptDataContext stackContext;                
                bool validIndex = context->ReadStack(stackContext);

                if (validIndex)
                {
                    netBindingTable->InvokeRPC(stackContext);
                }
            }

            return 0;
        }        
    }

    //////////////////////////
    // ScriptPropertyDataSet
    //////////////////////////

    // Necessary since the field doesn't make a copy, but keeps the actual literal.
    // But overall :(

    const char* ScriptPropertyDataSet::GetDataSetName()
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
            s_chunkIndex = s_chunkIndex%AZ_ARRAY_SIZE(s_nameArray);
        }

        return s_nameArray[s_chunkIndex++];
    }

    ScriptPropertyDataSet::ScriptPropertyDataSet()
        : ScriptPropertyDataSetType(GetDataSetName())
        , m_reserver(nullptr)
    {
    }

    ScriptPropertyDataSet::~ScriptPropertyDataSet()
    {
        AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(Get());

        if (functionalScriptProperty)
        {
            functionalScriptProperty->DisableInPlaceControls();
            functionalScriptProperty->RemoveWatcher(this);
        }
    }

    void ScriptPropertyDataSet::Reserve(ScriptNetBindingTable::NetworkedTableValue* reserver)
    {
        AZ_Error("ScriptComponent",m_reserver == nullptr || reserver == m_reserver, "Trying to reserve the same DataSet for two NetworkedTableVaules.");

        if (m_reserver == nullptr)
        {
            m_reserver = reserver;
            AZ::ScriptPropertyWatcherBus::Handler::BusConnect(this);

            AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(Get());

            if (functionalScriptProperty)
            {
                functionalScriptProperty->EnableInPlaceControls();
                functionalScriptProperty->AddWatcher(this);
            }            
        }
    }

    void ScriptPropertyDataSet::Release(ScriptNetBindingTable::NetworkedTableValue* reserver)
    {
        AZ_Error("ScriptComponent",m_reserver == nullptr || m_reserver == reserver, "Incorrect NetworkedTableValue trying to release a reserver DataSet.");
        if (m_reserver == reserver)
        {
            m_reserver = nullptr;
            AZ::ScriptPropertyWatcherBus::Handler::BusDisconnect(this);
        }
    }

    bool ScriptPropertyDataSet::IsReserved() const
    {
        return m_reserver != nullptr;
    }

    bool ScriptPropertyDataSet::UpdateScriptProperty(AZ::ScriptDataContext& scriptDataContext, const AZStd::string& propertyName)
    {
        bool wroteValue = false;

        Modify([&](AZ::ScriptProperty*& scriptProperty)
        {
            wroteValue = true;
            if (scriptProperty == nullptr || !scriptProperty->TryRead(scriptDataContext,-1))
            {
                AZ::ScriptProperty* newPropertyType = scriptDataContext.ConstructScriptProperty(-1,propertyName.c_str());

                delete scriptProperty;

                if (newPropertyType == nullptr)
                {
                    scriptProperty = aznew AZ::ScriptPropertyNil();
                }
                else
                {
                    scriptProperty = newPropertyType;
                }

                AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(newPropertyType);

                if (functionalScriptProperty)
                {
                    functionalScriptProperty->EnableInPlaceControls();
                    functionalScriptProperty->AddWatcher(this);
                }                
            }

            m_throttler.SignalDirty();

            return wroteValue;
        });

        return wroteValue;
    }

    void ScriptPropertyDataSet::SetScriptProperty(AZ::ScriptProperty* scriptProperty)
    {        
        Modify([&](AZ::ScriptProperty*& dataSetProperty)
        {
            AZ::ScriptProperty* tempProperty = dataSetProperty;

            if (scriptProperty == nullptr)
            {
                if (tempProperty)
                {
                    dataSetProperty = aznew AZ::ScriptPropertyNil(tempProperty->m_name.c_str());
                }
                else
                {
                    dataSetProperty = aznew AZ::ScriptPropertyNil();
                }
            }
            else
            {
                dataSetProperty = scriptProperty;
            }

            if (tempProperty)
            {
                delete tempProperty;
            }            

            AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(dataSetProperty);

            if (functionalScriptProperty)
            {
                functionalScriptProperty->EnableInPlaceControls();
                functionalScriptProperty->AddWatcher(this);
            }

            m_throttler.SignalDirty();

            return true;
        });
    }    

    void ScriptPropertyDataSet::OnObjectModified()
    {
        m_throttler.SignalDirty();
        SetDirty();
    }
    
    ////////////////////////
    // EntityScriptContext
    ////////////////////////

    ScriptNetBindingTable::EntityScriptContext::EntityScriptContext()
        : m_scriptContext(nullptr)        
        , m_entityTableRegistryIndex(LUA_REFNIL)
    {
    }

    void ScriptNetBindingTable::EntityScriptContext::Unload()
    {
        m_scriptContext = nullptr;
        m_entityTableRegistryIndex = LUA_REFNIL;        
    }

    bool ScriptNetBindingTable::EntityScriptContext::HasEntityTableRegistryIndex() const
    {
        return m_entityTableRegistryIndex != LUA_REFNIL;
    }

    int ScriptNetBindingTable::EntityScriptContext::GetEntityTableRegistryIndex() const
    {
        return m_entityTableRegistryIndex;
    }

    bool ScriptNetBindingTable::EntityScriptContext::HasScriptContext() const
    {
        return m_scriptContext != nullptr;
    }

    AZ::ScriptContext* ScriptNetBindingTable::EntityScriptContext::GetScriptContext() const
    {
        return m_scriptContext;
    }

    void ScriptNetBindingTable::EntityScriptContext::ConfigureContext(AZ::ScriptContext* scriptContext, int entityTableRegistryIndex)
    {
        m_scriptContext = scriptContext;
        m_entityTableRegistryIndex = entityTableRegistryIndex;
        
        AZ_Error("ScriptComponent",SanityCheckContext(),"Invalid configuration given to ScriptNetBindingTable");
    }

    bool ScriptNetBindingTable::EntityScriptContext::SanityCheckContext() const
    {
        bool isSane = HasScriptContext();

        if (isSane)
        {
            // We want to check if our reference is actually pointing to a table
            // which we will assume is our entity table
            lua_State* nativeContext = m_scriptContext->NativeContext();
            
            lua_rawgeti(nativeContext,LUA_REGISTRYINDEX,m_entityTableRegistryIndex);
            isSane = lua_istable(nativeContext,-1);
            lua_pop(nativeContext,1);            
        }

        return isSane;
    }

    ////////////////////////
    // NetworkedTableValue
    ////////////////////////

    ScriptNetBindingTable::NetworkedTableValue::NetworkedTableValue(AZ::ScriptProperty* initialValue)
        : m_shimmedScriptProperty(initialValue)
        , m_dataSet(nullptr)     
        , m_forcedDataSetIndex(-1)        
        , m_functionReference(LUA_REFNIL)
    {
        AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(initialValue);        

        if (functionalScriptProperty)
        {
            functionalScriptProperty->EnableInPlaceControls();
        }
    }

    ScriptNetBindingTable::NetworkedTableValue::~NetworkedTableValue()
    {
        // Always want to release our dataset if we have one when we are destroyed.
        //
        // We do not want to delete our proeprty thought, since we might have been copied over.
        if (m_dataSet)
        {
            m_dataSet->Release(this);
        }
    }

    void ScriptNetBindingTable::NetworkedTableValue::Destroy()
    {        
        if (m_shimmedScriptProperty)
        {
            delete m_shimmedScriptProperty;
            m_shimmedScriptProperty = nullptr;
        }
        
        if (m_dataSet)
        {
            m_dataSet->Release(this);
        }
    }

    bool ScriptNetBindingTable::NetworkedTableValue::HasDataSet() const
    {
        return m_dataSet != nullptr;
    }

    void ScriptNetBindingTable::NetworkedTableValue::RegisterDataSet(ScriptPropertyDataSet* dataSet)
    {
        m_dataSet = dataSet;

        if (m_dataSet)
        {
            if (m_shimmedScriptProperty)
            {
                m_dataSet->SetScriptProperty(m_shimmedScriptProperty);
                m_shimmedScriptProperty = nullptr;
            }

            m_dataSet->Reserve(this);
        }
    }

    void ScriptNetBindingTable::NetworkedTableValue::UnbindFromDataSet()
    {
        if (m_dataSet)
        {
            // Take ownership of the DataSet script property into our shimmed value
            //
            // If we are the master, we can take ownership and set the data set value to null
            if (m_dataSet->CanSet())
            {
                m_shimmedScriptProperty = m_dataSet->Get();
                m_dataSet->Set(nullptr);
            }
            // Otherwise, we need to clone the data in the data set since we can't modify it and we need to avoid a double deletion.
            else
            {
                AZ::ScriptProperty* scriptProperty = m_dataSet->Get();
                m_shimmedScriptProperty = scriptProperty->Clone();
            }

            m_dataSet->Release(this);
            m_dataSet = nullptr;
        }
    }

    ScriptPropertyDataSet* ScriptNetBindingTable::NetworkedTableValue::GetDataSet() const
    {
        return m_dataSet;
    }

    bool ScriptNetBindingTable::NetworkedTableValue::HasForcedDataSetIndex() const
    {
        return m_forcedDataSetIndex >= 1;
    }

    void ScriptNetBindingTable::NetworkedTableValue::SetForcedDataSetIndex(int index)
    {
        m_forcedDataSetIndex = index;
    }

    int ScriptNetBindingTable::NetworkedTableValue::GetForcedDataSetIndex() const
    {
        return m_forcedDataSetIndex;
    }

    bool ScriptNetBindingTable::NetworkedTableValue::HasCallback() const
    {
        return m_functionReference != LUA_REFNIL;
    }

    void ScriptNetBindingTable::NetworkedTableValue::RegisterCallback(int functionReference)
    {
        AZ_Warning("ScriptComponent",!HasCallback() || functionReference == LUA_REFNIL,"Overriding an already registered callback for a DataSet.");

        m_functionReference = functionReference;
    }

    void ScriptNetBindingTable::NetworkedTableValue::ReleaseCallback(AZ::ScriptContext& scriptContext)
    {        
        if (HasCallback())
        {
            scriptContext.ReleaseCached(m_functionReference);
            m_functionReference = LUA_REFNIL;
        }
    }

    void ScriptNetBindingTable::NetworkedTableValue::InvokeCallback(EntityScriptContext& entityContext, const GridMate::TimeContext& timeContext)
    {
        (void)timeContext;

        AZ::ScriptContext* scriptContext = entityContext.GetScriptContext();

        AZ_Warning("ScriptComponent",scriptContext,"DataSetCallback given null ScriptContext.");
        AZ_Warning("ScriptComponent",entityContext.HasEntityTableRegistryIndex(),"DataSetCallback given invalid entity table reference");
        if (scriptContext && entityContext.HasEntityTableRegistryIndex())
        {
            AZ::ScriptDataContext callContext;

            if (scriptContext->CallCached(m_functionReference,callContext))
            {
                callContext.PushArgFromRegistryIndex(entityContext.GetEntityTableRegistryIndex());            
                callContext.CallExecute();
            } 
        }
    }

    bool ScriptNetBindingTable::NetworkedTableValue::AssignValue(AZ::ScriptDataContext& scriptDataContext, const AZStd::string& propertyName)
    {
        bool assignedValue = false;
        if (HasDataSet())
        {
            assignedValue = GetDataSet()->UpdateScriptProperty(scriptDataContext, propertyName);      
        }
        else
        {
            if (m_shimmedScriptProperty == nullptr || !m_shimmedScriptProperty->TryRead(scriptDataContext,-1))
            {
                delete m_shimmedScriptProperty;
                m_shimmedScriptProperty = nullptr;
                m_shimmedScriptProperty = scriptDataContext.ConstructScriptProperty(-1,propertyName.c_str());

                AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(m_shimmedScriptProperty);        

                if (functionalScriptProperty)
                {
                    functionalScriptProperty->EnableInPlaceControls();
                }                
            }

            assignedValue = (m_shimmedScriptProperty != nullptr);
        }

        return assignedValue;
    }

    bool ScriptNetBindingTable::NetworkedTableValue::InspectValue(AZ::ScriptContext* scriptContext) const
    {
        AZ::ScriptProperty* inspectedProperty = m_shimmedScriptProperty;

        if (HasDataSet())
        {
            inspectedProperty = GetDataSet()->Get();
        }

        bool inspectedValue = false;

        if (inspectedProperty)
        {
            inspectedValue = inspectedProperty->Write((*scriptContext));
        }
        
        if (!inspectedValue)
        {
            inspectedValue = true;

            AZ::ScriptPropertyNil nilProperty;
            nilProperty.Write((*scriptContext));
        }

        return inspectedValue;
    }

    //////////////
    // RPCHelper
    //////////////

    // Maybe make this guy create the table himself?
    // Encapsulate the whole binding process in here.
    ScriptNetBindingTable::RPCBindingHelper::RPCBindingHelper()
        : m_masterReference(LUA_REFNIL)        
        , m_proxyReference(LUA_REFNIL)
    {
    }

    ScriptNetBindingTable::RPCBindingHelper::~RPCBindingHelper()
    {
    }

    void ScriptNetBindingTable::RPCBindingHelper::ReleaseTableIndex(AZ::ScriptContext& scriptContext)
    {
        if (m_masterReference != LUA_REFNIL)
        {
            scriptContext.ReleaseCached(m_masterReference);
            m_masterReference = LUA_REFNIL;
        }

        if (m_proxyReference != LUA_REFNIL)
        {
            scriptContext.ReleaseCached(m_proxyReference);
            m_proxyReference = LUA_REFNIL;
        }
    }

    bool ScriptNetBindingTable::RPCBindingHelper::IsValid() const
    {
        // Proxy is optional, so we only care about having the master index.
        return m_masterReference != LUA_REFNIL;
    }

    void ScriptNetBindingTable::RPCBindingHelper::SetMasterFunction(int masterReference)
    {
        AZ_Error("ScriptComponent",m_masterReference == LUA_REFNIL || masterReference == LUA_REFNIL, "Trying to rebind RPC master callback");

        if (m_masterReference == LUA_REFNIL || masterReference == LUA_REFNIL)
        {
            m_masterReference = masterReference;
        }
    }

    bool ScriptNetBindingTable::RPCBindingHelper::InvokeMaster(EntityScriptContext& entityScriptContext, const ScriptRPCMarshaler::Container& params)
    {
        if (!entityScriptContext.HasScriptContext())
        {
            AZ_Error("ScriptComponent",false,"Invoking RPC with invalid ScriptContext");
            return false;
        }

        bool allowRPC = false;

        if (m_masterReference != LUA_REFNIL)
        {
            AZ::ScriptDataContext callContext;
            AZ::ScriptContext* scriptContext = entityScriptContext.GetScriptContext();

            if (scriptContext->CallCached(m_masterReference,callContext))
            {
                callContext.PushArgFromRegistryIndex(entityScriptContext.GetEntityTableRegistryIndex());

                for (AZ::ScriptProperty* property : params)
                {
                    callContext.PushArgScriptProperty(property);
                }

                if (callContext.CallExecute())
                {
                    bool hasResult = false;
                    if (callContext.GetNumResults() == 1)
                    {
                        hasResult = callContext.ReadResult(0,allowRPC);
                    }

                    AZ_Assert(hasResult,"Master RPC function needs to return a boolean value");
                    (void)hasResult;
                }
            }

        }

        return allowRPC;
    }

    void ScriptNetBindingTable::RPCBindingHelper::SetProxyFunction(int proxyReference)
    {
        AZ_Error("ScriptComponent",m_proxyReference == LUA_REFNIL || proxyReference == LUA_REFNIL,"Trying to rebind an RPC Proxy call.");

        if (m_proxyReference == LUA_REFNIL || proxyReference == LUA_REFNIL)
        {
            m_proxyReference = proxyReference;
        }
    }

    void ScriptNetBindingTable::RPCBindingHelper::InvokeProxy(EntityScriptContext& entityScriptContext, const ScriptRPCMarshaler::Container& params)
    {
        if (!entityScriptContext.HasScriptContext())
        {
            AZ_Error("ScriptComponent",false,"Trying to call a callback without a script context.");
            return;
        }

        AZ_Warning("ScriptComponent",m_proxyReference != LUA_REFNIL,"Trying to invoke an RPC on the proxy without a callback being set.");
        if (m_proxyReference != LUA_REFNIL)
        {
            AZ::ScriptDataContext callContext;
            AZ::ScriptContext* scriptContext = entityScriptContext.GetScriptContext();

            if (scriptContext->CallCached(m_proxyReference,callContext))
            {
                callContext.PushArgFromRegistryIndex(entityScriptContext.GetEntityTableRegistryIndex());

                for (AZ::ScriptProperty* property : params)
                {
                    callContext.PushArgScriptProperty(property);
                }

                callContext.CallExecute();
            }
        }
    }

    //////////////////////////
    // ScriptNetBindingTable
    //////////////////////////
    void ScriptNetBindingTable::Reflect(AZ::ReflectContext* reflection)
    {
        NetworkContext* netContext = azrtti_cast<NetworkContext*>(reflection);

        if (netContext)
        {
            // Using old method until we update the network context to allow for array offests.
            // Or find a better way to handle the script replica chunk
            if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(ScriptComponentReplicaChunk::GetChunkName())))
            {
                GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<ScriptComponentReplicaChunk>();
            }
        }
    }

    // Template specialization for the ConvertPropertyArrayToTable GenericClass support to deal with extra memory copies.
    template<>
    AZ::ScriptPropertyTable* ScriptNetBindingTable::ConvertPropertyArrayToTable<AZ::ScriptPropertyGenericClass,AZ::ScriptPropertyGenericClassArray>(AZ::ScriptPropertyGenericClassArray* arrayProperty)
    {
        AZ::ScriptPropertyTable* scriptPropertyTable = aznew AZ::ScriptPropertyTable(arrayProperty->m_name.c_str());

        AZ::ScriptPropertyGenericClass emptyGenericClass;

        for (unsigned int i=0; i < arrayProperty->m_values.size(); ++i)
        {
            // Offset by  1 to deal with lua 1 indexing.
            // Table will make a clone of our object.
            //
            // To avoid pointlessly double copying the data. Insert the empty class.
            scriptPropertyTable->SetTableValue(i+1, &emptyGenericClass);

            // Then do a find for the class back, to load up the data into the final object directly.
            AZ::ScriptPropertyGenericClass* ownedClass = static_cast<AZ::ScriptPropertyGenericClass*>(scriptPropertyTable->FindTableValue(i+1));

            const AZ::DynamicSerializableField& serializableField = arrayProperty->m_values[i];

            ownedClass->Set(serializableField);
        }

        return scriptPropertyTable;
    }

    ScriptNetBindingTable::ScriptNetBindingTable()
    {
    }

    ScriptNetBindingTable::~ScriptNetBindingTable()
    {
        
    }

    void ScriptNetBindingTable::Unload()
    {
        // Unbind our elements from our script context.
        if (m_entityScriptContext.HasScriptContext())
        {
            AZ::ScriptContext& scriptContext = (*m_entityScriptContext.GetScriptContext());
            for (NetworkedTableMap::value_type& tablePair : m_networkedTable)
            {
                NetworkedTableValue& tableValue = tablePair.second;
                tableValue.ReleaseCallback(scriptContext);
            }

            for (RPCHelperMap::value_type& rpcPair : m_rpcHelperMap)
            {
                RPCBindingHelper& bindingHelper = rpcPair.second;
                bindingHelper.ReleaseTableIndex(scriptContext);
            }            
        }

        // Destroy the table values, since they might contain some memory
        for (NetworkedTableMap::value_type& tablePair : m_networkedTable)
        {
            tablePair.second.Destroy();
        }

        m_networkedTable.clear();
        m_rpcHelperMap.clear();

        m_entityScriptContext.Unload();
    }

    void ScriptNetBindingTable::CreateNetworkBindingTable(AZ::ScriptContext* scriptContext, int baseTableIndex, int entityTableIndex)
    {
        (void)baseTableIndex;

        lua_State* nativeContext = scriptContext->NativeContext();

        lua_pushliteral(nativeContext,"IsMaster");
        lua_pushlightuserdata(nativeContext,this);
        lua_pushcclosure(nativeContext, &Internal::NetBinding__IsMaster,1);
        lua_rawset(nativeContext,entityTableIndex);

        lua_pushliteral(nativeContext,"IsAuthoritative");
        lua_pushlightuserdata(nativeContext,this);
        lua_pushcclosure(nativeContext, &Internal::NetBinding__IsAuthoritative,1);
        lua_rawset(nativeContext,entityTableIndex);

        lua_pushstring(nativeContext,ScriptComponent::NetRPCFieldName);
        lua_createtable(nativeContext,0,0);

        int rpcTableIndex = lua_gettop(nativeContext);

        // Read the stack
        AZ::ScriptDataContext stackContext;        
        if (scriptContext->ReadStack(stackContext))
        {
            // Inspect the element we know to be our table context
            AZ::ScriptDataContext entityDataContext;
            if (stackContext.IsTable(baseTableIndex) && stackContext.InspectTable(baseTableIndex,entityDataContext))
            {
                // Find our RPC table inside of baseTableIndex
                int tableIndex = 0;
                if (entityDataContext.PushTableElement(ScriptComponent::NetRPCFieldName,&tableIndex))
                {
                    // If it's a table, we want to inspect it.
                    AZ::ScriptDataContext rpcTable;
                    if (entityDataContext.IsTable(tableIndex) && entityDataContext.InspectTable(tableIndex,rpcTable))
                    {
                        // Iterate over the fields here, and register and RPC for each element inside of that table.
                        int fieldIndex;
                        int elementIndex;
                        const char* fieldName;

                        while (rpcTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
                        {
                            if (fieldName != nullptr)
                            {
                                RegisterRPC(rpcTable,fieldName,elementIndex,rpcTableIndex);   
                            }
                        }
                    }
                }
            }
        }

        lua_rawset(nativeContext,entityTableIndex);
    }

    void ScriptNetBindingTable::FinalizeNetworkTable(AZ::ScriptContext* scriptContext, int entityTableRegistryIndex)
    {
        m_entityScriptContext.ConfigureContext(scriptContext, entityTableRegistryIndex);

        if (m_replicaChunk)
        {
            AssignDataSets();
        }
    }

    AZ::ScriptContext* ScriptNetBindingTable::GetScriptContext() const
    {
        return m_entityScriptContext.GetScriptContext();
    }

    bool ScriptNetBindingTable::IsMaster() const
    {
        return (m_replicaChunk != nullptr) ? m_replicaChunk->IsMaster() : true;
    }

    bool ScriptNetBindingTable::AssignTableValue(AZ::ScriptDataContext& stackDataContext)
    {
        bool wroteValue = false;
        AZ_Error("ScriptComponent",stackDataContext.GetScriptContext() == m_entityScriptContext.GetScriptContext(),"Trying to use a ScriptNetBindings in the wrong context.");

        // We do not want to allow assignments on proxy replicas
        // since that information will be discarded anyway.
        if ((m_replicaChunk && !m_replicaChunk->IsMaster()) || (stackDataContext.GetScriptContext() != m_entityScriptContext.GetScriptContext()))
        {
            return false;
        }

        AZStd::string key;

        if (stackDataContext.ReadValue(-2,key))
        {
            NetworkedTableValue* networkedTableValue = FindTableValue(key);

            // If we don't have the value, it means we aren't trying to network it.
            if (networkedTableValue)
            {
                if (m_replicaChunk && !networkedTableValue->HasDataSet())
                {
                    ScriptComponentReplicaChunk* scriptChunk = static_cast<ScriptComponentReplicaChunk*>(m_replicaChunk.get());

                    scriptChunk->AssignDataSet((*networkedTableValue));
                }

                wroteValue = networkedTableValue->AssignValue(stackDataContext,key);
            }
        }
        
        return wroteValue;
    }

    bool ScriptNetBindingTable::InspectTableValue(AZ::ScriptDataContext& stackContext) const
    {
        bool readValue = false;
        AZStd::string key;
        
        if (stackContext.ReadValue(-1,key))
        {
            AZ::ScriptContext* scriptContext = stackContext.GetScriptContext();

            if (scriptContext)
            {
                const NetworkedTableValue* networkTableValue = FindTableValue(key);

                if (networkTableValue)
                {
                    readValue = networkTableValue->InspectValue((stackContext.GetScriptContext()));
                    
                    // Default to nil if we have a network table value registered, but couldn't
                    // inspect it for some reason.
                    if (!readValue)
                    {
                        readValue = true;
                        static AZ::ScriptPropertyNil s_nilValue;
                        s_nilValue.Write((*scriptContext));
                    }
                }                
            }
            else
            {
                AZ_Error("ScriptComponent",false,"Trying to read value into invalid ScriptContext");
            }
        }

        return readValue;
    }

    bool ScriptNetBindingTable::RegisterDataSet(AZ::ScriptDataContext& networkTableContext, AZ::ScriptProperty* scriptProperty)
    {
        if (scriptProperty == nullptr)
        {
            AZ_Error("ScriptComponent",false,"Trying to Create a dataset for a null script property.");
            return false;
        }

        AZ::Uuid typeId = azrtti_typeid(scriptProperty);
        
        if (    typeId == azrtti_typeid<AZ::ScriptPropertyEntityRef>()
            ||  typeId == azrtti_typeid<AZ::ScriptPropertyAsset>())
        {
            AZ_Error("ScriptComponent",false,"Using unsupported type for ScriptProperty(%s) net binding. Value will not be networked.", scriptProperty->m_name.c_str());
            return false;
        }

        // If we've already got an item inside of our network table.
        // We need to surpress that problem, since it may happen when we reload the script
        if (m_networkedTable.find(scriptProperty->m_name) != m_networkedTable.end())
        {
            return true;
        }

        int elementIndex;
        bool enabled = false;
        bool handledProperty = false;
        
        if (networkTableContext.PushTableElement("Enabled",&elementIndex))
        {
            // If we have the enabled field and it's not a boolean.
            // Assume false.
            if (networkTableContext.IsBoolean(elementIndex))
            {   
                // If we didn't read in the enabled value, assume it's false.
                if (!networkTableContext.ReadValue(elementIndex,enabled))
                {
                    enabled = false;
                }
            }
        }
        else
        {
            // If we don't have an enabled field, assume that we want to be bound.
            enabled = true;
        }

        if (enabled)
        {
            NetworkedTableValue networkedTableValue;

            // Convert the property arrays over to a table to simplify the logic flow of detecting
            // the various changes, and to more readily support what the general LUA syntax.            
            if (typeId == azrtti_typeid<AZ::ScriptPropertyBooleanArray>())
            {
                AZ::ScriptPropertyBooleanArray* sourceArray = static_cast<AZ::ScriptPropertyBooleanArray*>(scriptProperty);

                networkedTableValue = NetworkedTableValue(ConvertPropertyArrayToTable<AZ::ScriptPropertyBoolean>(sourceArray));
            }
            else if (typeId == azrtti_typeid<AZ::ScriptPropertyNumberArray>())
            {
                AZ::ScriptPropertyNumberArray* sourceArray = static_cast<AZ::ScriptPropertyNumberArray*>(scriptProperty);

                networkedTableValue = NetworkedTableValue(ConvertPropertyArrayToTable<AZ::ScriptPropertyNumber>(sourceArray));
            }
            else if (typeId == azrtti_typeid<AZ::ScriptPropertyStringArray>())
            {                
                AZ::ScriptPropertyStringArray* sourceArray = static_cast<AZ::ScriptPropertyStringArray*>(scriptProperty);

                networkedTableValue = NetworkedTableValue(ConvertPropertyArrayToTable<AZ::ScriptPropertyString>(sourceArray));
            }
            else if (typeId == azrtti_typeid<AZ::ScriptPropertyGenericClassArray>())
            {
                AZ::ScriptPropertyGenericClassArray* sourceArray = static_cast<AZ::ScriptPropertyGenericClassArray*>(scriptProperty);                

                networkedTableValue = NetworkedTableValue(ConvertPropertyArrayToTable<AZ::ScriptPropertyGenericClass>(sourceArray));
            }
            else
            {
                networkedTableValue = NetworkedTableValue(scriptProperty->Clone());
            }

            if (networkTableContext.PushTableElement("OnNewValue", &elementIndex)) 
            {
                if (networkTableContext.IsFunction(elementIndex))
                {
                    networkedTableValue.RegisterCallback( networkTableContext.CacheValue(elementIndex) );
                }
            }

            if (networkTableContext.PushTableElement("ForceIndex",&elementIndex))
            {
                if (networkTableContext.IsNumber(elementIndex))
                {
                    int forcedDataSetIndex = 0;
                    if (networkTableContext.ReadValue(elementIndex,forcedDataSetIndex))
                    {
                        AZ_Error("ScriptComponent",forcedDataSetIndex >= 1 && forcedDataSetIndex <= ScriptComponentReplicaChunk::k_maxScriptableDataSets,"Trying to force Property (%s) to an invalid DataSetIndex(%i).",scriptProperty->m_name.c_str(),forcedDataSetIndex);
                        if(forcedDataSetIndex >= 1 && forcedDataSetIndex <= ScriptComponentReplicaChunk::k_maxScriptableDataSets)
                        {
                            networkedTableValue.SetForcedDataSetIndex(forcedDataSetIndex);
                        }
                    }
                    else
                    {
                        AZ_Error("ScriptComponent",false,"Trying to force Property (%s) to unknown DataSetIndex. Ignoring field.", scriptProperty->m_name.c_str());
                    }
                }
            }
            
            AZStd::pair<NetworkedTableMap::iterator, bool> insertResult = m_networkedTable.insert(NetworkedTableMap::value_type(scriptProperty->m_name,networkedTableValue));            

            // If we failed to insert the object, we need to destroy the networked table value.
            // To avoid leaking memory
            if (!insertResult.second)
            {
                networkedTableValue.Destroy();
            }

            handledProperty = true;
        }

        return handledProperty;
    }  

    bool ScriptNetBindingTable::InvokeRPC(AZ::ScriptDataContext& stackContext)
    {
        lua_State* nativeContext = stackContext.GetScriptContext()->NativeContext();

        bool invokedRPC = false;
        AZStd::string rpcName;
        
        // Assuming this is coming from an __call metamethod
        // the lua stack will look as follows
        // 1 - Table
        // n - Params
        if (stackContext.IsTable(1))
        {
            // Get the RPC name
            lua_pushliteral(nativeContext,"_rpcName");
            lua_gettable(nativeContext,1);

            if (stackContext.IsString(-1))
            {
                if (stackContext.ReadValue(-1,rpcName))
                {
                    // Pop off the key value we just read in; It's no longer necessary
                    lua_pop(nativeContext,1);

                    RPCHelperMap::iterator rpcIter = m_rpcHelperMap.find(rpcName);

                    if (rpcIter != m_rpcHelperMap.end())
                    {
                        bool foundParams = true;

                        ScriptRPCMarshaler::Container paramContainer;
                        
                        // Need to start at 2, since 1 is our table.
                        for (int i=2; i <= lua_gettop(nativeContext); ++i)
                        {
                            AZ::ScriptProperty* property = stackContext.ConstructScriptProperty(i,"param");

                            if (property)
                            {
                                paramContainer.push_back(property);
                            }
                            else
                            {
                                foundParams = false;
                                break;
                            }
                        }

                        // The script marshaler will clean up the keys after it has marshalled them out.
                        if (foundParams)
                        {
                            invokedRPC = true;

                            if (m_replicaChunk)
                            {
                                ScriptComponentReplicaChunk* scriptReplicaChunk = static_cast<ScriptComponentReplicaChunk*>(m_replicaChunk.get());
                                scriptReplicaChunk->m_scriptRPC(rpcName,paramContainer);
                            }
                            else
                            {
                                rpcIter->second.InvokeMaster(m_entityScriptContext,paramContainer);
                            }
                        }
                        else
                        {
                            for (AZ::ScriptProperty* property : paramContainer)
                            {
                                delete property;
                            }

                            paramContainer.clear();
                        }
                    }
                }
                else
                {
                    // We failed to read, so we need to pop the value we pushed off the stack.
                    lua_pop(nativeContext,1);
                }
            }
            else
            {
                // Pop off the value we added to the top of the stack
                lua_pop(nativeContext,1);
            }
        }

        return invokedRPC;
    }

    void ScriptNetBindingTable::RegisterRPC(AZ::ScriptDataContext& rpcTableContext, const AZStd::string& rpc, int elementIndex, int tableStackIndex)
    {
        if (rpcTableContext.IsTable(elementIndex))
        {
            RPCBindingHelper helper;

            AZ::ScriptDataContext rpcContext;
            if (rpcTableContext.InspectTable(elementIndex,rpcContext))
            {
                int functionIndex = 0;
                if (rpcContext.PushTableElement("OnMaster",&functionIndex))
                {
                    helper.SetMasterFunction(rpcContext.CacheValue(functionIndex));
                }
                else
                {
                    AZ_Error("ScriptNetBinding", false, "Could not find OnMaster function for RPC (%s).", rpc.c_str());
                }

                if (rpcContext.PushTableElement("OnProxy",&functionIndex))
                {
                    helper.SetProxyFunction(rpcContext.CacheValue(functionIndex));
                }
            }
            else
            {
                AZ_Error("ScriptNetBinding", false, "Could inspect table for RPC (%s).", rpc.c_str());
            }

            if (helper.IsValid())
            {
                lua_State* nativeContext = rpcTableContext.GetScriptContext()->NativeContext();

                // Create the RPC Table inside of our entity table to allow for functions to be called on it.
                // <RPC Table>
                lua_pushlstring(nativeContext,rpc.c_str(),rpc.size());
                lua_createtable(nativeContext,0,1);

                // Set up a name field inside of the table
                lua_pushliteral(nativeContext,"_rpcName");
                lua_pushlstring(nativeContext,rpc.c_str(),rpc.size());
                lua_rawset(nativeContext,-3);
                
                // <metatable>
                lua_createtable(nativeContext,0,1);
                lua_pushliteral(nativeContext,"__call");
                lua_pushlightuserdata(nativeContext, this);
                lua_pushcclosure(nativeContext, &Internal::NetBinding__CallRPC,1);
                lua_rawset(nativeContext,-3);
        
                lua_setmetatable(nativeContext,-2);
                // </metatable>

                lua_rawset(nativeContext,tableStackIndex);
                // </RPC Table>

                m_rpcHelperMap.insert(RPCHelperMap::value_type(rpc,helper));
            }
        }
    }

    GridMate::ReplicaChunkPtr ScriptNetBindingTable::GetNetworkBinding()
    {
        m_replicaChunk = GridMate::CreateReplicaChunk<ScriptComponentReplicaChunk>();
        m_replicaChunk->SetHandler(this);

        if (m_entityScriptContext.HasScriptContext())
        {
            AssignDataSets();
        }

        return m_replicaChunk;
    }

    void ScriptNetBindingTable::SetNetworkBinding(GridMate::ReplicaChunkPtr chunk)
    {
        m_replicaChunk = chunk;
        m_replicaChunk->SetHandler(this);

        if (m_entityScriptContext.HasScriptContext())
        {
            AssignDataSets();
        }
    }

    void ScriptNetBindingTable::UnbindFromNetwork()
    {
        if (m_replicaChunk)
        {
            m_replicaChunk->SetHandler(nullptr);

            for (NetworkedTableMap::value_type& tablePair : m_networkedTable)
            {
                tablePair.second.UnbindFromDataSet();
            }

            m_replicaChunk = nullptr;
        }
    }

    void ScriptNetBindingTable::OnPropertyUpdate(AZ::ScriptProperty*const& scriptProperty, const GridMate::TimeContext& tc)
    {
        AZ_Error("ScriptComponent",m_replicaChunk != nullptr,"DataSet callback method called without ReplicaChunk present.");

        if (scriptProperty && m_replicaChunk)
        {
            ScriptComponentReplicaChunk* scriptReplicaChunk = static_cast<ScriptComponentReplicaChunk*>(m_replicaChunk.get());

            NetworkedTableValue* tableValue = FindTableValue(scriptProperty->m_name);

            if (tableValue)
            {
                if (!tableValue->HasDataSet())
                {
                    scriptReplicaChunk->AssignDataSetForProperty((*tableValue),scriptProperty);
                    AZ_Error("ScriptComponent",tableValue->HasDataSet(),"Unable to bind received ScriptProperty to DataSet.");
                }
                else
                {
                    AZ_Error("ScriptComponent",scriptReplicaChunk->SanityCheckDataSet(scriptProperty,tableValue->GetDataSet()),"Mismatch between DataSet being chagned and mapping to dataset index.");
                }

                if (tableValue->HasCallback())
                {
                    tableValue->InvokeCallback(m_entityScriptContext,tc);
                }
            }
            else
            {
                AZ_Error("ScriptComponent",false,"Receiving update for unknown ScriptProperty.");
            }
        }
    }

    bool ScriptNetBindingTable::OnInvokeRPC(AZStd::string functionName, AZStd::vector< AZ::ScriptProperty*> params, const GridMate::RpcContext& rpcContext)
    {
        (void)rpcContext;

        bool canProxyExecute = false;
        AZ_Error("ScriptComponent",m_replicaChunk,"Receiving RPC callback with null ReplicaChunk.");

        RPCHelperMap::iterator rpcIter = m_rpcHelperMap.find(functionName);

        if (rpcIter != m_rpcHelperMap.end())
        {
            RPCBindingHelper& rpcHelper = rpcIter->second;

            if (m_replicaChunk == nullptr || m_replicaChunk->IsMaster())
            {
                canProxyExecute = rpcHelper.InvokeMaster(m_entityScriptContext,params);
            }
            else
            {
                // The canProxyExecute return value is meaningless on proxies.
                rpcHelper.InvokeProxy(m_entityScriptContext,params);
            }
        }
        
        return canProxyExecute;
    }    

    void ScriptNetBindingTable::AssignDataSets()
    {
        if (m_replicaChunk)
        {
            ScriptComponentReplicaChunk* scriptComponentChunk = static_cast<ScriptComponentReplicaChunk*>(m_replicaChunk.get());

            // Going to do this in two passes, first to do all of the forced ones, then all of the arbitrary ones.
            for (NetworkedTableMap::value_type& tablePair : m_networkedTable)
            {
                if (tablePair.second.HasForcedDataSetIndex() && !tablePair.second.HasDataSet())
                {
                    if (!scriptComponentChunk->AssignDataSet(tablePair.second))
                    {
                        // Remove the forced DataSet index since it was invalid.
                        // Second pass will assign this an arbitrary one.
                        tablePair.second.SetForcedDataSetIndex(-1);
                    }
                }
            }

            for (NetworkedTableMap::value_type& tablePair : m_networkedTable)
            {
                // Try to assign everything that doesn't already have a dataset.
                if (!tablePair.second.HasDataSet())
                {
                    scriptComponentChunk->AssignDataSet(tablePair.second);
                }
            }
        }
    }

    ScriptNetBindingTable::NetworkedTableValue* ScriptNetBindingTable::FindTableValue(const AZStd::string& name)
    {
        NetworkedTableValue* retVal = nullptr;
        NetworkedTableMap::iterator tableIter = m_networkedTable.find(name);

        if (tableIter != m_networkedTable.end())
        {
            retVal = &(tableIter->second);
        }

        return retVal;
    }

    const ScriptNetBindingTable::NetworkedTableValue* ScriptNetBindingTable::FindTableValue(const AZStd::string& name) const
    {
        const NetworkedTableValue* retVal = nullptr;
        NetworkedTableMap::const_iterator tableIter = m_networkedTable.find(name);

        if (tableIter != m_networkedTable.end())
        {
            retVal = &(tableIter->second);
        }

        return retVal;
    }

    ////////////////////////////////
    // ScriptComponentReplicaChunk
    ////////////////////////////////

    ScriptComponentReplicaChunk::ScriptComponentReplicaChunk()
        : m_scriptRPC("ScriptRPC")    
        , m_enabledDataSetMask(0)
    {
    }

    ScriptComponentReplicaChunk::~ScriptComponentReplicaChunk()
    {
        for (int i=0; i < k_maxScriptableDataSets; ++i)
        {
            ScriptPropertyDataSet& dataSet = m_propertyDataSets[i];

            dataSet.Modify([](AZ::ScriptProperty*& scriptProperty)
            {
                delete scriptProperty;
                scriptProperty =  nullptr;
                return false;
            });
        }
    }

    bool ScriptComponentReplicaChunk::IsReplicaMigratable()
    {
        return true;
    }

    AZ::u32 ScriptComponentReplicaChunk::CalculateDirtyDataSetMask(GridMate::MarshalContext& marshalContext)
    {
        if ((marshalContext.m_marshalFlags & GridMate::ReplicaMarshalFlags::ForceDirty))
        {
            return m_enabledDataSetMask;
        }

        return GridMate::ReplicaChunkBase::CalculateDirtyDataSetMask(marshalContext);
    }

    bool ScriptComponentReplicaChunk::AssignDataSet(ScriptNetBindingTable::NetworkedTableValue& helper)
    {
        bool assigned = false;

        if (helper.HasForcedDataSetIndex())
        {
            int testIndex = helper.GetForcedDataSetIndex() - 1;

            if (testIndex >= 0 && testIndex < k_maxScriptableDataSets)
            {
                ScriptPropertyDataSet* testDataSet = &m_propertyDataSets[testIndex];

                if (!testDataSet->IsReserved())
                {
                    assigned = true;
                    helper.RegisterDataSet(testDataSet);
                    m_enabledDataSetMask |= (1 << testIndex);
                }
                else
                {
                    AZ_Error("ScriptComponent",false,"Trying to register a networked value to a previously used DataSet.");
                }
            }
            else
            {
                AZ_Error("ScriptComponent",false,"Trying to register a table value to an invalid DataSet index.");
            }
        }
        else
        {
            for (int i=0; i < k_maxScriptableDataSets; ++i)
            {
                if (!m_propertyDataSets[i].IsReserved())
                {
                    assigned = true;
                    helper.RegisterDataSet(&m_propertyDataSets[i]);
                    m_enabledDataSetMask |= (1 << i);
                    break;
                }
            }

            AZ_Error("ScriptComponent",assigned, "Trying to create more then %i datasets for a script",k_maxScriptableDataSets);
        }

        return assigned;
    }

    void ScriptComponentReplicaChunk::AssignDataSetForProperty(ScriptNetBindingTable::NetworkedTableValue& helper, AZ::ScriptProperty* targetProperty)
    {
        AZ_Error("ScriptComponent",!IsMaster(),"Binding table value to specified DataSet on Master(Master should be making that choice, not responding to a choice).");

        if (!IsMaster())
        {
            for (int i=0; i < k_maxScriptableDataSets; ++i)
            {
                if (m_propertyDataSets[i].Get() == targetProperty)
                {
                    helper.RegisterDataSet(&m_propertyDataSets[i]);
                    m_enabledDataSetMask |= (1 << i);
                    break;
                }
            }
        }
    }

    bool ScriptComponentReplicaChunk::SanityCheckDataSet(AZ::ScriptProperty* targetProperty, ScriptPropertyDataSet* assumedDataSet)
    {
        bool isSane = false;

        for (int i=0; i < k_maxScriptableDataSets; ++i)
        {
            if (m_propertyDataSets[i].Get() == targetProperty)
            {
                isSane = &(m_propertyDataSets[i]) == assumedDataSet;   
                break;
            }
        }

        return isSane;
    }
}
