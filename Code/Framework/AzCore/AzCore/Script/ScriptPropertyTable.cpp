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
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Internal/MathTypes.h>

#include "ScriptPropertyTable.h"



extern "C" {
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

namespace AZ
{
    namespace Internal
    {
        // Used to set the appropriate key inside of our table to whatever value is specified.
        static int ScriptPropertyTable__NewIndex(lua_State* lua)
        {
            ScriptPropertyTable* scriptPropertyTable = reinterpret_cast<ScriptPropertyTable*>(lua_touserdata(lua, lua_upvalueindex(1)));

            if (scriptPropertyTable != nullptr)
            {
                AZ::ScriptContext* scriptContext = scriptPropertyTable->GetScriptContext();

                AZ_Error("ScriptPropertyTable", scriptContext != nullptr, "Trying to use a ScriptPropertyTable(%s) without giving it a ScriptContext to work in.\n", scriptPropertyTable->m_name.c_str());
                AZ_Error("ScriptPropertyTable", scriptContext == nullptr || scriptContext->NativeContext() == lua, "Trying to use a ScriptPropertyTable(%s) in the wrong lua context\n.", scriptPropertyTable->m_name.c_str());

                if (scriptContext && scriptContext->NativeContext() == lua)
                {
                    AZ::ScriptDataContext stackContext;
                    scriptContext->ReadStack(stackContext);

                    scriptPropertyTable->UpdateTableValue(stackContext, -2, -1);                    
                }
            }
            else
            {
                AZ_Assert(false, "Using a ScriptPropertyTable metamethod, without it being given a ScriptPropertyTable to work worth.\n");
            }

            return 0;
        }

        // Used to return the appropriate value for a given key from our table.
        static int ScriptPropertyTable__Index(lua_State* lua)
        {
            ScriptPropertyTable* scriptPropertyTable = reinterpret_cast<ScriptPropertyTable*>(lua_touserdata(lua, lua_upvalueindex(1)));

            if (scriptPropertyTable != nullptr)
            {
                AZ::ScriptContext* scriptContext = scriptPropertyTable->GetScriptContext();

                AZ_Error("ScriptPropertyTable", scriptContext != nullptr, "Trying to use a ScriptPropertyTable(%s) without giving it a ScriptContext to work in.\n", scriptPropertyTable->m_name.c_str());
                AZ_Error("ScriptPropertyTable", scriptContext == nullptr || scriptContext->NativeContext() == lua, "Trying to use a ScriptPropertyTable(%s) in the wrong lua context\n.", scriptPropertyTable->m_name.c_str());

                if (scriptContext && scriptContext->NativeContext() == lua)
                {
                    AZ::ScriptDataContext stackContext;
                    scriptContext->ReadStack(stackContext);

                    AZ::ScriptProperty* tableProperty = scriptPropertyTable->FindTableValue(stackContext,-1);                    

                    if (tableProperty != nullptr)
                    {
                        tableProperty->Write((*scriptPropertyTable->GetScriptContext()));
                    }
                    else
                    {
                        lua_pushnil(lua);
                    }
                }
                else
                {
                    lua_pushnil(lua);
                }
            }
            else
            {
                AZ_Assert(false, "Using a ScriptPropertyTable metamethod, without it being given a ScriptPropertyTable to work worth.\n");
                lua_pushnil(lua);
            }

            return 1;
        }

        // Used to return the list of key values pairs from our table.
        static int ScriptPropertyTable__Pairs(lua_State* lua)
        {
            ScriptPropertyTable* scriptPropertyTable = reinterpret_cast<ScriptPropertyTable*>(lua_touserdata(lua,lua_upvalueindex(1)));

            if (scriptPropertyTable)
            {
                AZ::ScriptContext* scriptContext = scriptPropertyTable->GetScriptContext();

                AZ_Error("ScriptPropertyTable", scriptContext != nullptr, "Trying to use a ScriptPropertyTable(%s) without giving it a ScriptContext to work in.\n", scriptPropertyTable->m_name.c_str());
                AZ_Error("ScriptPropertyTable", scriptContext == nullptr || scriptContext->NativeContext() == lua, "Trying to use a ScriptPropertyTable(%s) in the wrong lua context\n.", scriptPropertyTable->m_name.c_str());

                if (scriptContext != nullptr && scriptContext->NativeContext() == lua)
                {
                    // Expects 3 return values:
                    // stepFunc
                    // the object[the table to iterate on]
                    // initial_state[nil]

                    lua_pushvalue(lua, lua_upvalueindex(2));
                    scriptPropertyTable->WriteRawTable((*scriptPropertyTable->GetScriptContext()));                
                    lua_pushnil(lua);
                    return 3;
                }
            }

            return 0;
        }

        static int ScriptPropertyTable__IPairs(lua_State* lua)
        {
            ScriptPropertyTable* scriptPropertyTable = reinterpret_cast<ScriptPropertyTable*>(lua_touserdata(lua,lua_upvalueindex(1)));

            if (scriptPropertyTable)
            {
                AZ::ScriptContext* scriptContext = scriptPropertyTable->GetScriptContext();

                AZ_Error("ScriptPropertyTable", scriptContext != nullptr, "Trying to use a ScriptPropertyTable(%s) without giving it a ScriptContext to work in.\n", scriptPropertyTable->m_name.c_str());
                AZ_Error("ScriptPropertyTable", scriptContext == nullptr || scriptContext->NativeContext() == lua, "Trying to use a ScriptPropertyTable(%s) in the wrong lua context\n.", scriptPropertyTable->m_name.c_str());

                if (scriptContext != nullptr && scriptContext->NativeContext() == lua)
                {
                    // Expects 3 return values:
                    // stepFunc
                    // the object[the table to iterate on]
                    // initial_state[nil]

                    lua_pushvalue(lua, lua_upvalueindex(2));
                    scriptPropertyTable->WriteRawTable((*scriptPropertyTable->GetScriptContext()));
                    lua_pushnumber(lua,0);
                    return 3;
                }
            }

            return 0;
        }

        static int ScriptPropertyTable__Next(lua_State* lua)
        {
            luaL_checktype(lua,1,LUA_TTABLE);

            lua_settop(lua,2);

            if (lua_next(lua,1))
            {
                return 2;
            }
            else
            {
                lua_pushnil(lua);
                return 1;
            }
        }

        static int ScriptPropertyTable__INext(lua_State* lua)
        {
            luaL_checktype(lua,1,LUA_TTABLE);
            lua_settop(lua,2);

            lua_Number nextKey = lua_tonumber(lua, 2) + 1;

            // Push one to keep on the stack so we know what the next value is.
            lua_pushnumber(lua,nextKey);

            // Push one in order to look up in the table.
            lua_pushnumber(lua,nextKey);
            lua_gettable(lua,1);

            if (lua_isnil(lua,-1))
            {
                return 0;
            }
            else
            {
                return 2;
            }
        }

        static int ScriptPropertyTable__Length(lua_State* lua)
        {
            int tableLength = 0;

            ScriptPropertyTable* scriptPropertyTable = reinterpret_cast<ScriptPropertyTable*>(lua_touserdata(lua, lua_upvalueindex(1)));

            if (scriptPropertyTable != nullptr)
            {
                AZ::ScriptContext* scriptContext = scriptPropertyTable->GetScriptContext();

                AZ_Error("ScriptPropertyTable", scriptContext != nullptr, "Trying to use a ScriptPropertyTable(%s) without giving it a ScriptContext to work in.\n", scriptPropertyTable->m_name.c_str());
                AZ_Error("ScriptPropertyTable", scriptContext == nullptr || scriptContext->NativeContext() == lua, "Trying to use a ScriptPropertyTable(%s) in the wrong lua context\n.", scriptPropertyTable->m_name.c_str());

                if (scriptContext && scriptContext->NativeContext() == lua)
                {
                    tableLength = scriptPropertyTable->GetLuaTableLength();
                }
            }

            lua_pushnumber(lua,tableLength);
            return 1;
        }
    }

    ScriptProperty* ScriptPropertyTable::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        AZ::ScriptPropertyTable* retVal = nullptr;

        if (context.IsTable(valueIndex))
        {
            AZ::ScriptDataContext tableContext;
            if (context.InspectTable(valueIndex, tableContext))
            {
                retVal = aznew ScriptPropertyTable(name);

                int dataContextIndex = 0;
                const char* fieldName = nullptr;
                int keyIndex = 0;

                while (tableContext.InspectNextElement(dataContextIndex,fieldName,keyIndex))
                {
                    ScriptProperty* valueProperty = tableContext.ConstructScriptProperty(dataContextIndex,"mapValue");

                    // Bypassing our set methods here to avoid needing to double clone construct the value. Since I can just manage the memory here.
                    if (fieldName == nullptr)
                    {
                        auto insertResult = retVal->m_indexMapping.emplace(keyIndex,valueProperty);

                        if (!insertResult.second)
                        {
                            AZ_Error("ScriptPropertyTable",false,"Hit Collision on index %i when constructing table %s\n",keyIndex,name);
                            delete valueProperty;
                        }
                    }
                    else
                    {
                        AZ::Crc32 hashIndex = AZ::Crc32(fieldName);
                        auto insertResult = retVal->m_keyMapping.emplace(hashIndex, valueProperty);

                        if (!insertResult.second)
                        {
                            AZ_Error("ScriptPropertyTable", false, "Found collision on string %s when constructing table %s\n", fieldName, name);
                            delete valueProperty;
                        }
                    }
                }
            }
        }

        return retVal;
    }
    
    ScriptPropertyTable::ScriptPropertyTable()
        : FunctionalScriptProperty()        
        , m_enableMetatableControl(false)
        , m_scriptContext(nullptr)
        , m_tableCache(LUA_NOREF)
        , m_numberProperty(nullptr)
        , m_stringProperty(nullptr)
        , m_genericProperty(nullptr)
    {
        InitKeyProperties();
    }

    ScriptPropertyTable::ScriptPropertyTable(const char* name, const ScriptPropertyTable* scriptPropertyTable)
        : ScriptPropertyTable(name)
    {
        CloneDataFrom(scriptPropertyTable);

        m_enableMetatableControl = scriptPropertyTable->m_enableMetatableControl;
    }
    
    ScriptPropertyTable::ScriptPropertyTable(const char* name)
        : FunctionalScriptProperty(name)
        , m_enableMetatableControl(false)
        , m_scriptContext(nullptr)
        , m_tableCache(LUA_NOREF)
        , m_numberProperty(nullptr)
        , m_stringProperty(nullptr)
        , m_genericProperty(nullptr)
    {
        InitKeyProperties();        
    }
    
    ScriptPropertyTable::~ScriptPropertyTable()
    {
        delete m_genericProperty;
        delete m_numberProperty;
        delete m_stringProperty;
        
        for (auto& mapPair : m_indexMapping)
        {
            delete mapPair.second;
        }

        for (auto& mapPair: m_keyMapping)
        {
            delete mapPair.second;
        }

        ReleaseCachedTable();
    }

    ScriptProperty* ScriptPropertyTable::FindTableValue(AZ::ScriptDataContext& scriptDataContext, int keyIndex) const
    {
        AZ::ScriptProperty* retVal = nullptr;

        if (m_numberProperty->TryRead(scriptDataContext,keyIndex))
        {
            // Only want to support indexes for now.
            double integerPart = 0;
            if (AZ::IsClose(modf(m_numberProperty->m_value, &integerPart), 0 , std::numeric_limits<double>().epsilon()))
            {
                retVal = FindTableValue(static_cast<int>(m_numberProperty->m_value));
            }
        }
        else if (m_stringProperty->TryRead(scriptDataContext,keyIndex))
        {
            retVal = FindTableValue(m_stringProperty->m_name);
        }
        else if (m_genericProperty->TryRead(scriptDataContext,keyIndex))
        {
            retVal = FindTableValue(m_genericProperty);
        }

        return retVal;
    }

    ScriptProperty* ScriptPropertyTable::FindTableValue(const AZ::ScriptPropertyGenericClass* genericScriptProperty) const
    {
        AZ::ScriptProperty* retVal = nullptr;

        if (genericScriptProperty)
        {
            auto mapIter = m_genericMapping.find(genericScriptProperty->GetDataTypeUuid());

            if (mapIter != m_genericMapping.end())
            {
                retVal = mapIter->second->FindTableValue(genericScriptProperty);
            }
            else
            {
                AZ_Error("ScriptPropertyTable", false, "Trying to use unsupported generic type(%s) in ScriptPropertyTable", genericScriptProperty->GetDataTypeUuid().ToString<AZStd::string>().c_str());
            }
        }

        return retVal;
    }
    
    ScriptProperty* ScriptPropertyTable::FindTableValue(const AZStd::string_view& keyValue) const
    {
        AZ::ScriptProperty* retVal = nullptr;
        AZ::Crc32 crcIndex = AZ::Crc32(keyValue.data());
        
        auto mapIter = m_keyMapping.find(crcIndex);
        
        if (mapIter != m_keyMapping.end())
        {
            retVal = mapIter->second;
        }
        
        return retVal;
    }

    ScriptProperty* ScriptPropertyTable::FindTableValue(int index) const
    {
        AZ::ScriptProperty* retVal = nullptr;
        
        auto mapIter = m_indexMapping.find(index);        
        
        if (mapIter != m_indexMapping.end())
        {
            retVal = mapIter->second;
        }
        
        return retVal;
    }

    void ScriptPropertyTable::UpdateTableValue(AZ::ScriptDataContext& scriptDataContext, int keyIndex, int valueIndex)
    {
        if (m_numberProperty->TryRead(scriptDataContext,keyIndex))
        {
            // Only want to support indexes for now.
            double integerPart = 0;
            if (AZ::IsClose(modf(m_numberProperty->m_value, &integerPart), 0 , std::numeric_limits<double>().epsilon()))
            {
                UpdateTableValue(static_cast<int>(m_numberProperty->m_value),scriptDataContext,valueIndex);
            }
        }
        else if (m_stringProperty->TryRead(scriptDataContext,keyIndex))
        {
            UpdateTableValue(m_stringProperty->m_value,scriptDataContext,valueIndex);
        }
        else if (m_genericProperty->TryRead(scriptDataContext,keyIndex))
        {
            UpdateTableValue(m_genericProperty,scriptDataContext,valueIndex);
        }
        else
        {
            AZ_Error("ScriptPropertyTable", false, "Trying to use unsupported key type in ScriptPropertyTable");
        }        
    }

    void ScriptPropertyTable::UpdateTableValue(const AZ::ScriptPropertyGenericClass* genericScriptProperty, AZ::ScriptDataContext& scriptDataContext, int index)
    {
        auto mapIter = m_genericMapping.find(genericScriptProperty->GetDataTypeUuid());

        if (mapIter != m_genericMapping.end())
        {
            mapIter->second->UpdateTableValue(genericScriptProperty, scriptDataContext, index);
            SignalPropertyChanged();
        }
        else
        {
            AZ_Error("ScriptPropertyTable", false, "Trying to use unsupported generic type(%s) in ScriptPropertyTable", genericScriptProperty->GetDataTypeUuid().ToString<AZStd::string>().c_str());
        }
    }

    void ScriptPropertyTable::UpdateTableValue(const AZStd::string_view& keyValue, AZ::ScriptDataContext& scriptDataContext, int index)
    {
        AZ::Crc32 crcIndex = AZ::Crc32(keyValue.data());
        auto mapIter = m_keyMapping.find(crcIndex);

        if (mapIter != m_keyMapping.end())
        {
            if (!mapIter->second->TryRead(scriptDataContext,index))
            {
                delete mapIter->second;
                mapIter->second = scriptDataContext.ConstructScriptProperty(index,keyValue.data());

                if (mapIter->second == nullptr || azrtti_istypeof<AZ::ScriptPropertyNil>(mapIter->second))
                {
                    delete mapIter->second;
                    m_keyMapping.erase(mapIter);
                }
                else
                {
                    SetTableValueScriptPropertyWatched(mapIter->second, m_enableMetatableControl);
                }
            }
        }
        else
        {
            AZ::ScriptProperty* scriptProperty = scriptDataContext.ConstructScriptProperty(index,keyValue.data());

            if (scriptProperty != nullptr && !azrtti_istypeof<AZ::ScriptPropertyNil>(scriptProperty))
            {
                m_keyMapping.emplace(crcIndex,scriptProperty);
                SetTableValueScriptPropertyWatched(scriptProperty, m_enableMetatableControl);
            }
            else
            {
                delete scriptProperty;            
            }
        }

        // Some potential for optimization here by figuring out when values actually change and only signalling then.
        SignalPropertyChanged();
    }

    void ScriptPropertyTable::UpdateTableValue(int keyIndex, AZ::ScriptDataContext& scriptDataContext, int stackIndex)
    {
        auto mapIter = m_indexMapping.find(keyIndex);

        if (mapIter != m_indexMapping.end())
        {
            if (!mapIter->second->TryRead(scriptDataContext,stackIndex))
            {
                AZStd::string name = mapIter->second->m_name;
                delete mapIter->second;
                mapIter->second = scriptDataContext.ConstructScriptProperty(stackIndex,name.c_str());

                if (mapIter->second == nullptr || azrtti_istypeof<AZ::ScriptPropertyNil>(mapIter->second))
                {
                    delete mapIter->second;
                    m_indexMapping.erase(mapIter);
                }
                else
                {
                    SetTableValueScriptPropertyWatched(mapIter->second, m_enableMetatableControl);
                }
            }
        }
        else
        {
            AZ::ScriptProperty* scriptProperty = scriptDataContext.ConstructScriptProperty(stackIndex,"ip");

            if (scriptProperty != nullptr && !azrtti_istypeof<AZ::ScriptPropertyNil>(scriptProperty))
            {
                m_indexMapping.emplace(keyIndex,scriptProperty);
                SetTableValueScriptPropertyWatched(scriptProperty, m_enableMetatableControl);
            }
            else
            {
                delete scriptProperty;
            }
        }

        // Some potential for optimization here by figuring out when values actually change and only signalling then.
        SignalPropertyChanged();
    }

    void ScriptPropertyTable::SetTableValue(const AZ::ScriptProperty* keyProperty, const AZ::ScriptProperty* value)
    {
        AZ::Uuid typeId = azrtti_typeid(keyProperty);

        if (typeId == azrtti_typeid<AZ::ScriptPropertyNumber>())
        {
            // Only want to support indexes for now.
            double integerPart = 0;
            if (AZ::IsClose(modf(m_numberProperty->m_value, &integerPart), 0 , 0.01))
            {
                SetTableValue(static_cast<int>(static_cast<const AZ::ScriptPropertyNumber*>(keyProperty)->m_value), value);
            }
        }
        else if (typeId == azrtti_typeid<AZ::ScriptPropertyString>())
        {
            SetTableValue(static_cast<const AZ::ScriptPropertyString*>(keyProperty)->m_value, value);
        }
        else if (typeId == azrtti_typeid<AZ::ScriptPropertyGenericClass>())
        {
            SetTableValue(static_cast<const AZ::ScriptPropertyGenericClass*>(keyProperty), value);
        }
        else
        {
            AZ_Error("ScriptPropertyTable", false, "Trying to use unsupported type as a key into ScriptPropertyTable");
        }
    }

    void ScriptPropertyTable::SetTableValue(const AZ::ScriptPropertyGenericClass* keyProperty, const AZ::ScriptProperty* value)
    {
        auto mapIter = m_genericMapping.find(keyProperty->GetDataTypeUuid());

        if (mapIter != m_genericMapping.end())
        {
            mapIter->second->SetTableValue(keyProperty, value);
            SignalPropertyChanged();
        }
        else
        {
            AZ_Error("ScriptPropertyTable", false, "Trying to use unsupported generic type(%s) as a key to ScriptPropertyTable.", keyProperty->GetDataTypeUuid().ToString<AZStd::string>().c_str());
        }
    }
    
    void ScriptPropertyTable::SetTableValue(const AZStd::string_view& keyValue, const ScriptProperty* value)
    {
        AZ::Crc32 crcIndex = AZ::Crc32(keyValue.data());
        auto mapIter = m_keyMapping.find(crcIndex);

        if (mapIter != m_keyMapping.end())
        {
            // If we fail to copy in place inside of our ScriptProperty.
            // Delete the old one, and clone out the new value.
            if (!mapIter->second->TryUpdate(value))
            {
                delete mapIter->second;
            }
            else
            {
                return;
            }
        }
        else
        {
            auto insertResult = m_keyMapping.emplace(crcIndex,nullptr);
            mapIter = insertResult.first;
        }
        
        if (value != nullptr && !azrtti_istypeof<AZ::ScriptPropertyNil>(value))
        {
            mapIter->second = value->Clone(keyValue.data());
            SetTableValueScriptPropertyWatched(mapIter->second, m_enableMetatableControl);
        }
        else
        {
            m_keyMapping.erase(mapIter);
        }

        // Some potential for optimization here by figuring out when values actually change and only signalling then.
        SignalPropertyChanged();
    }

    void ScriptPropertyTable::SetTableValue(int index, const ScriptProperty* value)
    {
        auto mapIter = m_indexMapping.find(index);

        if (mapIter != m_indexMapping.end())
        {
            // If we fail to copy in place inside of our ScriptProperty.
            // Delete the old one, and clone out the new value.
            if (!mapIter->second->TryUpdate(value))
            {
                delete mapIter->second;
            }
            else
            {
                return;
            }
        }
        else
        {
            auto insertResult = m_indexMapping.emplace(index,nullptr);
            mapIter = insertResult.first;
        }
        
        // If we get a null or a nil pointer, just remove the value from our map.
        if (value != nullptr && !azrtti_istypeof<AZ::ScriptPropertyNil>(value))
        {
            mapIter->second = value->Clone();
            SetTableValueScriptPropertyWatched(mapIter->second, m_enableMetatableControl);
        }
        else
        {
            m_indexMapping.erase(mapIter);
        }

        // Some potential for optimization here by figuring out when values actually change and only signalling then.
        SignalPropertyChanged();
    }

    ScriptPropertyTable* ScriptPropertyTable::Clone(const char* name) const
    {
        if (name == nullptr)
        {
            name = m_name.c_str();
        }

        AZ::ScriptPropertyTable* scriptPropertyTable = aznew AZ::ScriptPropertyTable(name, this);

        if (m_scriptContext)
        {
            scriptPropertyTable->Write((*m_scriptContext));
        }

        return scriptPropertyTable;
    }

    bool ScriptPropertyTable::Write(AZ::ScriptContext& context)
    {
        bool wroteValue = false;
        
        if (m_enableMetatableControl)
        {
            CreateCachedTable(context);

            if (m_tableCache != LUA_NOREF && m_tableCache != LUA_REFNIL)
            {
                AZ_Error("ScriptPropertyTable", &context == m_scriptContext, "Trying to use a ScriptPropertyTable in multiple lua contextes then it was cached in.\n");

                if (m_scriptContext && &context == m_scriptContext)
                {
                    wroteValue = true;
                    lua_rawgeti(m_scriptContext->NativeContext(), LUA_REGISTRYINDEX, m_tableCache);
                    
                    AZ_Assert(lua_istable(m_scriptContext->NativeContext(),-1), "Table cache does not point to a valid Table Object for ScriptPropertyTable(%s)\n", m_name.c_str());
                }
            }
            else
            {
                wroteValue = WriteRawTable(context);
            }
        }
        else
        {
            wroteValue = WriteRawTable(context);
        }

        return wroteValue;
    }

    bool ScriptPropertyTable::WriteRawTable(AZ::ScriptContext& context)
    {
        lua_State* nativeContext = context.NativeContext();
        // Hard write out the table
        // This is our return value table. It will remain on the stack.
        lua_createtable(nativeContext,static_cast<int>(m_indexMapping.size()),static_cast<int>(m_keyMapping.size()));

        for (auto& mapPair : m_indexMapping)
        {
            // <element>
            lua_pushnumber(nativeContext,mapPair.first);
            mapPair.second->Write(context);
            lua_rawset(nativeContext,-3);
            // </element>
        }

        for (auto& mapPair: m_keyMapping)
        {
            // <element>
            // We know that the name of ScriptProperties is the actual name of the element from our push methods.
            lua_pushstring(nativeContext, mapPair.second->m_name.c_str());
            mapPair.second->Write(context);
            lua_rawset(nativeContext, -3);
            // </element>
        }

        RawWriteTableMap<AZ::EntityId>(context);

        return true;
    }

    void ScriptPropertyTable::EnableInPlaceControls()
    {
        if (!m_enableMetatableControl)
        {
            ScriptPropertyWatcherBus::Handler::BusConnect(this);
            m_enableMetatableControl = true;
            
            SetTableValuePropertiesWatched(true);
        }
    }

    void ScriptPropertyTable::DisableInPlaceControls()
    {
        if (m_enableMetatableControl)
        {
            m_enableMetatableControl = false;

            ReleaseCachedTable();
            ScriptPropertyWatcherBus::Handler::BusDisconnect(this);
        }
    }

    AZ::ScriptContext* ScriptPropertyTable::GetScriptContext() const
    {
        return m_scriptContext;
    }

    int ScriptPropertyTable::GetLuaTableLength() const
    {
        int elementCount = 0;

        while (true)
        {        
            auto indexIter = m_indexMapping.find((elementCount + 1));

            if (indexIter == m_indexMapping.end())
            {
                break;
            }

            ++elementCount;
        }

        return elementCount;
    }

    void ScriptPropertyTable::OnObjectModified()
    {
        SignalPropertyChanged();
    }

    void ScriptPropertyTable::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyTable* scriptPropertyTable = azrtti_cast<const AZ::ScriptPropertyTable*>(scriptProperty);
        AZ_Error("ScriptPropertyTable", scriptPropertyTable, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (scriptPropertyTable)
        {
            AZStd::unordered_set<int> newIndexes;        

            // Update all of our values.
            // While keeping track of what keys we are using. So we can clean up the unused values.
            // while trying out best to main not over zealously delete where we can update in place.
            for (auto& mapPair : scriptPropertyTable->m_indexMapping)
            {
                newIndexes.insert(mapPair.first);
                SetTableValue(mapPair.first,mapPair.second);
            }

            // scoping to re-use mapIter name
            {
                auto mapIter = m_indexMapping.begin();

                while (mapIter != m_indexMapping.end())
                {
                    if (newIndexes.find(mapIter->first) == newIndexes.end())
                    {
                        delete mapIter->second;
                        mapIter = m_indexMapping.erase(mapIter);
                    }
                    else
                    {
                        ++mapIter;
                    }
                }
            }

            AZStd::unordered_set<AZ::Crc32> newKeys;

            for (auto& mapPair : scriptPropertyTable->m_keyMapping)
            {
                newKeys.insert(mapPair.first);
                SetTableValue(mapPair.first,mapPair.second);
            }

            // scoping to re-use mapIter name
            {
                auto mapIter = m_keyMapping.begin();

                while (mapIter != m_keyMapping.end())
                {
                    if (newKeys.find(mapIter->first) == newKeys.end())
                    {
                        delete mapIter->second;
                        mapIter = m_keyMapping.erase(mapIter);
                    }
                    else
                    {
                        ++mapIter;
                    }
                }
            }

            for (auto& mapPair : scriptPropertyTable->m_genericMapping)
            {
                auto mapIter = m_genericMapping.find(mapPair.first);

                if (mapIter == m_genericMapping.end())
                {
                    ScriptPropertyGenericClassMap* genericMap = mapPair.second->Clone();

                    if (m_enableMetatableControl)
                    {
                        genericMap->SetWatcher(this);
                    }

                    m_genericMapping[mapPair.first] = genericMap;
                }
                else
                {
                    mapIter->second->CloneDataFrom(mapPair.second);
                }
            }
        }
    }

    void ScriptPropertyTable::CreateCachedTable(AZ::ScriptContext& scriptContext)
    {
        if (m_scriptContext == nullptr)
        {
            m_scriptContext = &scriptContext;
            lua_State* nativeContext = m_scriptContext->NativeContext();

            // <table>
            lua_createtable(nativeContext,0,1);

            // <table>
            // pairs() - used to mimic the functionality of the pairs method until we can support the __pairs metamethod
            lua_pushstring(nativeContext,"pairs");
            lua_pushlightuserdata(nativeContext, this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Next,0);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Pairs,2);
            lua_rawset(nativeContext,-3);

            // ipairs() = used to mimic the functionality of the ipairs method until we can support the __ipairs metamethod
            lua_pushstring(nativeContext,"ipairs");
            lua_pushlightuserdata(nativeContext, this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__INext,0);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__IPairs,2);
            lua_rawset(nativeContext,-3);

            // length() = used to mimic the functionality of the # operator until we can support the __length metamethod
            lua_pushstring(nativeContext,"length");
            lua_pushlightuserdata(nativeContext,this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Length, 1);
            lua_rawset(nativeContext,-3);

            // <metatable>
            // 0: __newindex - Hit whenever someone tries to write to a value in the table(i.e. m_table.myValue = "Foo")
            // 1: __index - Hit whenever someone tries to read a value from the table(i.e. local myValue = m_table.myValue)
            // 2: __pairs - Hit whenever someone calls the pairs() method with our table as the value(i.e. pairs(m_table))
            // 3: __next - The next iterator for the table.
            // 4: __length - Hit whenever someone calls the # method with our table as the target(i.e. #m_table)
            lua_createtable(nativeContext,0,5);

            // <Closure>
            lua_pushstring(nativeContext,"__newindex");
            lua_pushlightuserdata(nativeContext, this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__NewIndex,1);
            lua_rawset(nativeContext, -3);
            // </Closure>

            // <Closure>
            lua_pushstring(nativeContext,"__index");
            lua_pushlightuserdata(nativeContext, this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Index,1);
            lua_rawset(nativeContext, -3);
            // </Closure>

            // These metamethods aren't supported until Lua 5.2
            // <Closure>
            lua_pushstring(nativeContext, "__pairs");
            lua_pushlightuserdata(nativeContext, this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Next,0);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Pairs,2);
            lua_rawset(nativeContext, -3);
            // </Closure>

            // <Closure>
            lua_pushstring(nativeContext, "__ipairs");
            lua_pushlightuserdata(nativeContext, this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__INext,0);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__IPairs,2);
            lua_rawset(nativeContext, -3);
            // </Closure>

            // <Closure>
            lua_pushstring(nativeContext, "__next");
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Next,0);
            lua_rawset(nativeContext, -3);
            // </Closure>

            // <Closure>
            lua_pushstring(nativeContext, "__length");
            lua_pushlightuserdata(nativeContext,this);
            lua_pushcclosure(nativeContext, &Internal::ScriptPropertyTable__Length,1);
            lua_rawset(nativeContext, -3);
            // </Closure>

            lua_setmetatable(nativeContext,-2);
            // </metatable>
            
            // Keep the entity table in the registry
            m_tableCache = luaL_ref(m_scriptContext->NativeContext(), LUA_REGISTRYINDEX);
            AZ_Error("ScriptPropertyTable", m_tableCache != LUA_NOREF && m_tableCache != LUA_REFNIL, "Could not create cached table for metatable control on table (%s)\n", m_name.c_str());
        }

        AZ_Error("ScriptPropertyTable", &scriptContext == m_scriptContext, "Trying to use a ScriptPropertyTable in multiple lua contextes then it was cached in.\n");
    }

    void ScriptPropertyTable::ReleaseCachedTable()
    {
        if (m_tableCache != LUA_NOREF && m_scriptContext != nullptr)
        {
            luaL_unref(m_scriptContext->NativeContext(), LUA_REGISTRYINDEX, m_tableCache);
            m_tableCache = LUA_NOREF;
        }
    }
    
    void ScriptPropertyTable::SetTableValuePropertiesWatched(bool watchProperty)
    {
        for (auto& mapPair : m_indexMapping)
        {
            SetTableValueScriptPropertyWatched(mapPair.second, watchProperty);
        }

        for (auto& mapPair : m_keyMapping)
        {
            SetTableValueScriptPropertyWatched(mapPair.second, watchProperty);
        }

        for (auto& mapPair : m_genericMapping)
        {
            if (watchProperty)
            {
                mapPair.second->SetWatcher(this);
            }
            else
            {
                mapPair.second->SetWatcher(nullptr);
            }
        }
    }

    void ScriptPropertyTable::SetTableValueScriptPropertyWatched(AZ::ScriptProperty* scriptProperty, bool watchProperty)
    {
        AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(scriptProperty);

        if (functionalScriptProperty)
        {
            if (watchProperty)
            {
                functionalScriptProperty->EnableInPlaceControls();
                functionalScriptProperty->AddWatcher(this);
            }
            else
            {
                functionalScriptProperty->DisableInPlaceControls();
                functionalScriptProperty->RemoveWatcher(this);
            }
        }
    }

    // This method is here to init the values. Mostly so I can actually modify these values
    // in the const methods.
    //
    // Contains the 3 supported key types to avoid constant thrashing.
    void ScriptPropertyTable::InitKeyProperties()
    {
        m_numberProperty = aznew AZ::ScriptPropertyNumber();
        m_stringProperty = aznew AZ::ScriptPropertyString();
        m_genericProperty = aznew AZ::ScriptPropertyGenericClass();

        PopulateSupportedGenericKeyTypes();
    }

    // Only supporting a subset of keys for now.
    // To support arbitrary data sources, would need a cleverer
    // method of doing the marshaling.
    //
    // Simplest way would be forcing the marshaling into this object. But the buffers required for that
    // are in a project that aren't available in this project.
    // And the buffers and these classes can't be easily moved around.
    void ScriptPropertyTable::PopulateSupportedGenericKeyTypes()
    {
        m_genericMapping[AZ::EntityId::TYPEINFO_Uuid()] = aznew ScriptPropertyGenericClassMapImpl<AZ::EntityId>();
    }

    // Helper method called from the RawWriteTableMap template function.
    // Mainly here to avoid the need to including the lua functions in the header
    void ScriptPropertyTable::RawTableWriterHelper(AZ::ScriptContext& scriptContext, AZ::ScriptProperty* keyProperty, AZ::ScriptProperty* valueProperty) const
    {
        // <element>
        keyProperty->Write(scriptContext);
        valueProperty->Write(scriptContext);
        lua_rawset(scriptContext.NativeContext(),-3);
        // </element>
    }    
}
