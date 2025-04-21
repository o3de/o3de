/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Script/ScriptProperty.h>

namespace AZ
{
    // Add TypeInfo and RTTI Reflection within the cpp file
    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptProperty, "AzFramework::ScriptProperty", "{D227D737-F1ED-4FB3-A1FB-38E4985D2E7A}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(FunctionalScriptProperty, "FunctionalScriptProperty", "{57D7418D-6B14-4A02-B50E-2E409D23CFC6}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(FunctionalScriptProperty, ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyNil, "AzFramework::ScriptPropertyNil", "{ACAD23F6-5E75-460E-BD77-1B477750264F}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyNil, ScriptProperty);

    // ScriptPropertyBoolean is serialized to a prefab file using the type name of AzFramework::ScriptPropertyBoolean
    // So make sure the old name is being used
    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyBoolean, "AzFramework::ScriptPropertyBoolean", "{EA7335F8-5B9F-4744-B805-FEF9240451BD}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyBoolean, ScriptProperty);

    // ScriptPropertyNumber is serialized to a prefab file using the type name of AzFramework::ScriptPropertyNumber
    // So make sure the old name is being used
    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyNumber, "AzFramework::ScriptPropertyNumber", "{5BCDFDEB-A75D-4E83-BB74-C45299CB9826}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyNumber, ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyString, "AzFramework::ScriptPropertyString", "{A0229C6D-B010-47E7-8985-EE220FC7BFAF}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyString, ScriptProperty);

    // ScriptPropertyGenericClass is serialized to a prefab file using the type name of AzFramework::ScriptPropertyGenericClass
    // So make sure the old name is being used
    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyGenericClass, "AzFramework::ScriptPropertyGenericClass", "{80618224-814C-44D4-A7B8-14B5A36F96ED}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyGenericClass, FunctionalScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyNumberArray, "AzFramework::ScriptPropertyNumberArray", "{76609A01-46CA-442E-8BA6-251D529886AF}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyNumberArray, ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyBooleanArray, "AzFramework::ScriptPropertyBooleanArray", "{3A83958C-26C7-4A59-B6D7-A7805B0EC756}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyBooleanArray, ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyStringArray, "AzFramework::ScriptPropertyStringArray", "{899993A5-D717-41BB-B89B-04A27952CA6D}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyStringArray, ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyGenericClassArray, "AZ::ScriptPropertyGenericClassArray", "{28E986DD-CF7C-404D-9BEE-EEE067180CD1}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyGenericClassArray, ScriptProperty);

    AZ_TYPE_INFO_WITH_NAME_IMPL(ScriptPropertyAsset, "AZ::ScriptPropertyAsset", "{4D4B7176-A6E1-4BB9-A7B0-5977EC724CCB}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ScriptPropertyAsset, ScriptProperty);

    //////////////////////////
    // ScriptPropertyReflect
    //////////////////////////
    void ScriptProperties::Reflect(AZ::ReflectContext* reflection)
    {
        ScriptProperty::Reflect(reflection);

        ScriptPropertyBoolean::Reflect(reflection);
        ScriptPropertyNumber::Reflect(reflection);
        ScriptPropertyString::Reflect(reflection);
        ScriptPropertyGenericClass::Reflect(reflection);

        ScriptPropertyBooleanArray::Reflect(reflection);
        ScriptPropertyNumberArray::Reflect(reflection);
        ScriptPropertyStringArray::Reflect(reflection);
        ScriptPropertyGenericClassArray::Reflect(reflection);

        ScriptPropertyAsset::Reflect(reflection);
    }

    template<class Iterator>
    bool WriteContainerToLuaTableArray(AZ::ScriptContext& context, Iterator first, Iterator last)
    {
        lua_State* lua = context.NativeContext();
        size_t numElements = AZStd::distance(first, last);
        if (numElements)
        {
            lua_createtable(lua, static_cast<int>(numElements), 0);

            for (size_t i = 0; i < numElements; ++i, ++first)
            {
                AZ::ScriptValue<typename AZStd::iterator_traits<Iterator>::value_type>::StackPush(lua, *first);
                lua_rawseti(lua, -2, static_cast<int>(i+1));
            }
        }
        else
        {
            lua_newtable(lua);
        }

        return true;
    }

    ///////////////////
    // ScriptProperty
    ///////////////////

    static bool ScriptPropertyVersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            // Generate persistent Id field.
            for (int i = 0; i < classElement.GetNumSubElements(); ++i)
            {
                AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);
                if (elementNode.GetName() == AZ_CRC_CE("name"))
                {
                    AZStd::string name;
                    if (elementNode.GetData(name))
                    {
                        const int idx = classElement.AddElement<AZ::u64>(context, "id");
                        const AZ::u32 crc = Crc32(name.c_str());
                        classElement.GetSubElement(idx).SetData<AZ::u64>(context, crc);
                    }
                }
            }
        }

        return true;
    }

    void ScriptProperty::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<AZ::ScriptProperty>()->
                Version(2, &AZ::ScriptPropertyVersionConverter)->
                    PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const AZ::ScriptProperty*>(instance)->m_id; })->
                    Field("id", &AZ::ScriptProperty::m_id)->
                    Field("name", &AZ::ScriptProperty::m_name);
        }
    }

    /////////////////////////////
    // FunctionalScriptProperty
    /////////////////////////////

    FunctionalScriptProperty::FunctionalScriptProperty()
    {
    }

    FunctionalScriptProperty::FunctionalScriptProperty(const char* name)
        : ScriptProperty(name)
    {
    }

    void FunctionalScriptProperty::AddWatcher(AZ::ScriptPropertyWatcher* watcher)
    {
        auto insertResult = m_watchers.insert(watcher);

        if (insertResult.second)
        {
            OnWatcherAdded(watcher);
        }
    }

    void FunctionalScriptProperty::RemoveWatcher(AZ::ScriptPropertyWatcher* watcher)
    {
        size_t eraseResult = m_watchers.erase(watcher);

        if (eraseResult > 0)
        {
            OnWatcherRemoved(watcher);
        }
    }

    void FunctionalScriptProperty::OnWatcherAdded(AZ::ScriptPropertyWatcher* scriptPropertyWatcher)
    {
        (void)scriptPropertyWatcher;
    }

    void FunctionalScriptProperty::OnWatcherRemoved(AZ::ScriptPropertyWatcher* scriptPropertyWatcher)
    {
        (void)scriptPropertyWatcher;
    }

    void FunctionalScriptProperty::SignalPropertyChanged()
    {
        for (AZ::ScriptPropertyWatcher* watcher : m_watchers)
        {
            ScriptPropertyWatcherBus::Event(watcher, &ScriptPropertyWatcherBus::Events::OnObjectModified);
        }
    }

    //////////////////////
    // ScriptPropertyNil
    //////////////////////

    void ScriptPropertyNil::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<AZ::ScriptPropertyNil, AZ::ScriptProperty>()
                ->Version(1)
                ->SerializeWithNoData();
        }
    }

    ScriptProperty* ScriptPropertyNil::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        ScriptProperty* retVal = nullptr;
        if (context.IsNil(valueIndex))
        {
            retVal = aznew AZ::ScriptPropertyNil(name);
        }

        return retVal;
    }

    const void* ScriptPropertyNil::GetDataAddress() const
    {
        return nullptr;
    }

    AZ::TypeId ScriptPropertyNil::GetDataTypeUuid() const
    {
        return AZ::SerializeTypeInfo<void*>::GetUuid();
    }

    AZ::ScriptPropertyNil* ScriptPropertyNil::Clone(const char* name) const
    {
        return aznew AZ::ScriptPropertyNil(name ? name : m_name.c_str());
    }

    bool ScriptPropertyNil::Write(AZ::ScriptContext& context)
    {
        lua_pushnil(context.NativeContext());
        return true;
    }

    bool ScriptPropertyNil::TryRead(AZ::ScriptDataContext& context, int valueIndex)
    {
        if (context.IsNil(valueIndex))
        {
            return true;
        }

        return false;
    }

    void ScriptPropertyNil::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        (void)scriptProperty;
    }

    //////////////////////////
    // ScriptPropertyBoolean
    //////////////////////////
    void ScriptPropertyBoolean::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyBoolean, ScriptProperty>()->
                Version(1)->
                Field("value", &ScriptPropertyBoolean::m_value);
        }
    }

    AZ::ScriptProperty* ScriptPropertyBoolean::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        AZ::ScriptProperty* retVal = nullptr;

        if (context.IsBoolean(valueIndex))
        {
            bool value;
            if (context.ReadValue(valueIndex, value))
            {
                retVal = aznew AZ::ScriptPropertyBoolean(name, value);
            }
        }

        return retVal;
    }

    bool ScriptPropertyBoolean::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return context.IsBoolean(valueIndex);
    }

    AZ::ScriptPropertyBoolean* ScriptPropertyBoolean::Clone(const char* name) const
    {
        return aznew AZ::ScriptPropertyBoolean(name ? name : m_name.c_str(),m_value);
    }

    bool ScriptPropertyBoolean::Write(AZ::ScriptContext& context)
    {
        AZ::ScriptValue<bool>::StackPush(context.NativeContext(), m_value);
        return true;
    }

    bool ScriptPropertyBoolean::TryRead(AZ::ScriptDataContext& context, int valueIndex)
    {
        if (context.IsBoolean(valueIndex))
        {
            context.ReadValue(valueIndex,m_value);
        }

        return false;
    }

    AZ::TypeId ScriptPropertyBoolean::GetDataTypeUuid() const
    {
        return AZ::SerializeTypeInfo<bool>::GetUuid();
    }

    void ScriptPropertyBoolean::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyBoolean* booleanProperty = azrtti_cast<const AZ::ScriptPropertyBoolean*>(scriptProperty);
        AZ_Error("ScriptPropertyBoolean", booleanProperty, "Invalid call to CloneData. Types must match before clone attempt is made.\n");

        if (booleanProperty)
        {
            m_value = booleanProperty->m_value;
        }
    }

    /////////////////////////
    // ScriptPropertyNumber
    /////////////////////////
    void ScriptPropertyNumber::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<AZ::ScriptPropertyNumber, AZ::ScriptProperty>()->
                Version(1)->
                Field("value", &AZ::ScriptPropertyNumber::m_value);
        }
    }

    AZ::ScriptProperty* ScriptPropertyNumber::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        AZ::ScriptProperty* retVal = nullptr;

        if (context.IsNumber(valueIndex))
        {
            double value;
            if (context.ReadValue(valueIndex, value))
            {
                retVal = aznew AZ::ScriptPropertyNumber(name,value);
            }
        }

        return retVal;
    }

    bool ScriptPropertyNumber::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return context.IsNumber(valueIndex);
    }

    AZ::ScriptPropertyNumber* ScriptPropertyNumber::Clone(const char* name) const
    {
        return aznew AZ::ScriptPropertyNumber(name ? name : m_name.c_str(),m_value);
    }

    bool ScriptPropertyNumber::Write(AZ::ScriptContext& context)
    {
        AZ::ScriptValue<double>::StackPush(context.NativeContext(), m_value);
        return true;
    }

    bool ScriptPropertyNumber::TryRead(AZ::ScriptDataContext& sdc, int index)
    {
        if (sdc.IsNumber(index))
        {
            sdc.ReadValue(index,m_value);
            return true;
        }

        return false;
    }

    AZ::TypeId ScriptPropertyNumber::GetDataTypeUuid() const
    {
        return AZ::SerializeTypeInfo<double>::GetUuid();
    }

    void ScriptPropertyNumber::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyNumber* numberProperty = azrtti_cast<const AZ::ScriptPropertyNumber*>(scriptProperty);
        AZ_Error("ScriptPropertyNumber", numberProperty, "Invalid call to CloneData. Types must match before clone attempt is made.\n");

        if (numberProperty)
        {
            m_value = numberProperty->m_value;
        }
    }

    /////////////////////////
    // ScriptPropertyString
    /////////////////////////
    void ScriptPropertyString::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyString, ScriptProperty>()->
                Version(1)->
                Field("value", &ScriptPropertyString::m_value);
        }
    }

    ScriptProperty* ScriptPropertyString::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        AZ::ScriptProperty* retVal = nullptr;

        if (context.IsString(valueIndex))
        {
            const char* value = nullptr;
            if (context.ReadValue(valueIndex, value))
            {
                retVal = aznew AZ::ScriptPropertyString(name,value);
            }
        }

        return retVal;
    }

    bool ScriptPropertyString::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return context.IsString(valueIndex);
    }

    AZ::ScriptPropertyString* ScriptPropertyString::Clone(const char* name) const
    {
        return aznew AZ::ScriptPropertyString(name ? name : m_name.c_str(), m_value.c_str());
    }

    bool ScriptPropertyString::Write(AZ::ScriptContext& context)
    {
        AZ::ScriptValue<const char*>::StackPush(context.NativeContext(), m_value.c_str());
        return true;
    }

    bool ScriptPropertyString::TryRead(AZ::ScriptDataContext& context, int valueIndex)
    {
        bool readValue = false;

        if (context.IsString(valueIndex))
        {
            const char* value = nullptr;
            if (context.ReadValue(valueIndex, value))
            {
                readValue = true;
                m_value = value;
            }
        }

        return readValue;
    }

    AZ::TypeId ScriptPropertyString::GetDataTypeUuid() const
    {
        return AZ::SerializeGenericTypeInfo<AZStd::string>::GetClassTypeId();
    }

    void ScriptPropertyString::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyString* stringProperty = azrtti_cast<const AZ::ScriptPropertyString*>(scriptProperty);
        AZ_Error("ScriptPropertyString", stringProperty, "Invalid call to CloneData. Types must match before clone attempt is made.\n");

        if (stringProperty)
        {
            m_value = stringProperty->m_value;
        }
    }

    ///////////////////////////////
    // ScriptPropertyGenericClass
    ///////////////////////////////
    void ScriptPropertyGenericClass::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyGenericClass, ScriptProperty>()->
                Version(1)->
                Field("value", &ScriptPropertyGenericClass::m_value);
        }
    }

    ScriptPropertyGenericClass::ScriptPropertyGenericClass()
        : m_scriptContext(nullptr)
        , m_cacheObject(false)
        , m_cachedValue(LUA_NOREF)
    {
    }

    ScriptPropertyGenericClass::ScriptPropertyGenericClass(const char* name, const AZ::DynamicSerializableField& value)
        : FunctionalScriptProperty(name)
        , m_scriptContext(nullptr)
        , m_cacheObject(false)
        , m_cachedValue(LUA_NOREF)
        , m_value(value)
    {
    }

    ScriptPropertyGenericClass::~ScriptPropertyGenericClass()
    {
        MonitorBehaviorSignals(false);
        ReleaseCache();
        m_value.DestroyData();
    }

    ScriptProperty* ScriptPropertyGenericClass::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        AZ::ScriptProperty* retVal = nullptr;

        if (context.IsRegisteredClass(valueIndex))
        {
            AZ::DynamicSerializableField value;
            value.m_data = Internal::LuaAnyClassFromStack(context.GetScriptContext()->NativeContext(), context.GetStartIndex() + valueIndex, &value.m_typeId);
            if (value.m_data)
            {
                // clone the data or take ownership from LUA
                const BehaviorClass* behaviorClass = nullptr;
                if (Internal::LuaGetClassInfo(context.GetScriptContext()->NativeContext(), context.GetStartIndex() + valueIndex, nullptr, &behaviorClass))
                {
                    BehaviorObject source;
                    source.m_address = value.m_data;
                    source.m_typeId = value.m_typeId;
                    BehaviorObject copy = behaviorClass->Clone(source);
                    value.m_data = copy.m_address;

                    retVal = aznew AZ::ScriptPropertyGenericClass(name, value);
                }
                else
                {
                    AZ_Error("Script", false, "Failed to locate behavior class for field %s.", name);
                }
            }
            else
            {
                AZ_Warning("Script", false, "Failed to read registered class %s", name);
            }
        }

        return retVal;
    }

    bool ScriptPropertyGenericClass::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return context.IsRegisteredClass(valueIndex);
    }

    AZ::ScriptPropertyGenericClass* ScriptPropertyGenericClass::Clone(const char* name) const
    {
        return aznew AZ::ScriptPropertyGenericClass(name ? name : m_name.c_str(),m_value);
    }

    bool ScriptPropertyGenericClass::Write(AZ::ScriptContext& context)
    {
        if (m_cacheObject)
        {
            CacheObject(context);

            if (m_cachedValue == LUA_NOREF || &context != m_scriptContext)
            {
                AZ_Error("ScriptPropertyGenericClass", false, "Failed to create cache of Generic Class Object.");
                Internal::LuaClassToStack(context.NativeContext(), m_value.m_data, m_value.m_typeId);
            }
            else
            {
                lua_rawgeti(m_scriptContext->NativeContext(), LUA_REGISTRYINDEX, m_cachedValue);
            }
        }
        else
        {
            Internal::LuaClassToStack(context.NativeContext(), m_value.m_data, m_value.m_typeId);
        }

        return true;
    }

    bool ScriptPropertyGenericClass::TryRead(AZ::ScriptDataContext& context, int valueIndex)
    {
        bool didRead = false;

        if (context.IsRegisteredClass(valueIndex))
        {
            AZ::DynamicSerializableField value;
            value.m_data = Internal::LuaAnyClassFromStack(context.GetScriptContext()->NativeContext(), context.GetStartIndex() + valueIndex, &value.m_typeId);
            if (value.m_data)
            {
                // clone the data or take ownership from LUA
                const BehaviorClass* behaviorClass = nullptr;
                if (Internal::LuaGetClassInfo(context.GetScriptContext()->NativeContext(), context.GetStartIndex() + valueIndex, nullptr, &behaviorClass))
                {
                    didRead = true;

                    MonitorBehaviorSignals(false);
                    ReleaseCache();

                    BehaviorObject source;
                    source.m_address = value.m_data;
                    source.m_typeId = value.m_typeId;

                    BehaviorObject copy = behaviorClass->Clone(source);

                    m_value.m_data = copy.m_address;
                    m_value.m_typeId = copy.m_typeId;

                    SignalPropertyChanged();
                    MonitorBehaviorSignals(true);
                }
            }
        }

        return didRead;
    }

    const AZ::DynamicSerializableField& ScriptPropertyGenericClass::GetSerializableField() const
    {
        return m_value;
    }

    void ScriptPropertyGenericClass::Set(const AZ::DynamicSerializableField& sourceField)
    {
        MonitorBehaviorSignals(false);
        ReleaseCache();

        m_value.m_typeId = sourceField.m_typeId;
        m_value.m_data = sourceField.CloneData();

        SignalPropertyChanged();
        MonitorBehaviorSignals(true);
    }

    void ScriptPropertyGenericClass::EnableInPlaceControls()
    {
        m_cacheObject = true;
    }

    void ScriptPropertyGenericClass::DisableInPlaceControls()
    {
        if (m_cacheObject)
        {
            ReleaseCache();
            m_cacheObject = false;
        }
    }

    void ScriptPropertyGenericClass::OnMemberMethodCalled(const BehaviorMethod* behaviorMethod)
    {
        if (!behaviorMethod->m_isConst)
        {
            SignalPropertyChanged();
        }
    }

    void ScriptPropertyGenericClass::OnWatcherAdded(ScriptPropertyWatcher* scriptPropertyWatcher)
    {
        (void)scriptPropertyWatcher;
        MonitorBehaviorSignals(true);
    }

    void ScriptPropertyGenericClass::OnWatcherRemoved(ScriptPropertyWatcher* scriptPropertyWatcher)
    {
        (void)scriptPropertyWatcher;
        if (m_watchers.empty())
        {
            MonitorBehaviorSignals(false);
        }
    }

    void ScriptPropertyGenericClass::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyGenericClass* genericClassProperty = azrtti_cast<const AZ::ScriptPropertyGenericClass*>(scriptProperty);
        AZ_Error("ScriptPropertyGenericClass", genericClassProperty, "Invalid call to CloneData. Types must match before clone attempt is made.\n");

        if (genericClassProperty)
        {
            MonitorBehaviorSignals(false);
            ReleaseCache();

            m_value.DestroyData();

            m_value.CopyDataFrom(genericClassProperty->m_value);

            SignalPropertyChanged();
            MonitorBehaviorSignals(true);
        }
    }

    void ScriptPropertyGenericClass::MonitorBehaviorSignals(bool enabled)
    {
        if (m_watchers.empty() || !enabled)
        {
            BehaviorObjectSignals::Handler::BusDisconnect(m_value.m_data);
        }
        else
        {
            BehaviorObjectSignals::Handler::BusConnect(m_value.m_data);
        }
    }

    void ScriptPropertyGenericClass::ReleaseCache()
    {
        if (m_scriptContext)
        {
            if (m_cachedValue != LUA_REFNIL && m_cachedValue != LUA_NOREF)
            {
                luaL_unref(m_scriptContext->NativeContext(), LUA_REGISTRYINDEX, m_cachedValue);
            }

            m_scriptContext = nullptr;
            m_cachedValue = LUA_NOREF;
        }
    }

    void ScriptPropertyGenericClass::CacheObject(AZ::ScriptContext& context)
    {
        AZ_Error("ScriptPropertyGenericClass",m_scriptContext == nullptr || &context == m_scriptContext, "ScriptProperty(%s) is trying to cache to two different Script Context's at once.", m_name.c_str());

        if (m_scriptContext == nullptr)
        {
            m_scriptContext = &context;
            m_cachedValue = Internal::LuaCacheRegisteredClass(m_scriptContext->NativeContext(), m_value.m_data, m_value.m_typeId);
        }
    }

    ////////////////////////////
    // ScriptPropertyBoolArray
    ////////////////////////////
    void ScriptPropertyBooleanArray::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyBooleanArray, ScriptProperty>()->
                Version(1)->
                Field("values", &ScriptPropertyBooleanArray::m_values);
        }
    }

    // This is kind of weird. This shouldn't really matter; Ideally we'd just
    // make the table of variants.
    bool ScriptPropertyBooleanArray::IsBooleanArray(AZ::ScriptDataContext& context, int valueIndex)
    {
        bool isBooleanArray = false;

        if (context.IsTable(valueIndex))
        {
            AZ::ScriptDataContext arrayTable;
            if (context.InspectTable(valueIndex,arrayTable))
            {
                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                // We only want to inspect the first element to inform us of what
                // type should be in our array for now.
                while (arrayTable.InspectNextElement(elementIndex,fieldName,fieldIndex))
                {
                    if (fieldIndex != -1)
                    {
                       break;
                    }
                }

                isBooleanArray = arrayTable.IsBoolean(elementIndex);
            }
        }

        return isBooleanArray;
    }

    ScriptProperty* ScriptPropertyBooleanArray::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        ScriptPropertyBooleanArray* retVal = nullptr;

        if (IsBooleanArray(context,valueIndex))
        {
            retVal = aznew ScriptPropertyBooleanArray(name);

            AZ::ScriptDataContext arrayTable;
            if (retVal && context.InspectTable(valueIndex,arrayTable))
            {
                ParseBooleanArray(arrayTable,retVal->m_values);
            }
        }

        return retVal;
    }

    void ScriptPropertyBooleanArray::ParseBooleanArray(AZ::ScriptDataContext& numberArrayTable, AZStd::vector<bool>& output)
    {
        const char* fieldName;
        int fieldIndex;
        int elementIndex;
        while (numberArrayTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
        {
            // check only the arrayElements
            if (fieldIndex == -1)
            {
                continue;
            }

            if (numberArrayTable.IsBoolean(elementIndex))
            {
                bool value;
                if (numberArrayTable.ReadValue(elementIndex, value))
                {
                    output.push_back(value);
                }
            }
            else
            {
                AZ_Warning("Script",false,"All types inside the array must the same type or have a common class! Element[%d] is not a Number!", fieldIndex);
            }
        }
    }

    AZ::TypeId ScriptPropertyBooleanArray::GetDataTypeUuid() const
    {
        return AZ::SerializeGenericTypeInfo< AZStd::vector<bool> >::GetClassTypeId();
    }

    bool ScriptPropertyBooleanArray::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return IsBooleanArray(context,valueIndex);
    }

    AZ::ScriptPropertyBooleanArray* ScriptPropertyBooleanArray::Clone(const char* name) const
    {
        AZ::ScriptPropertyBooleanArray* clonedValue = aznew AZ::ScriptPropertyBooleanArray(name ? name : m_name.c_str());
        clonedValue->m_values = m_values;
        return clonedValue;
    }

    bool ScriptPropertyBooleanArray::Write(AZ::ScriptContext& context)
    {
        return WriteContainerToLuaTableArray(context, m_values.begin(), m_values.end());
    }

    void ScriptPropertyBooleanArray::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyBooleanArray* booleanPropertyArray = azrtti_cast<const AZ::ScriptPropertyBooleanArray*>(scriptProperty);

        AZ_Error("ScriptPropertyBooleanArray", booleanPropertyArray, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (booleanPropertyArray)
        {
            m_values = booleanPropertyArray->m_values;
        }
    }

    //////////////////////////////
    // ScriptPropertyNumberArray
    //////////////////////////////
    void ScriptPropertyNumberArray::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyNumberArray, ScriptProperty>()->
                Version(1)->
                Field("values", &ScriptPropertyNumberArray::m_values);
        }
    }

    // This is kind of weird. This shouldn't really matter; Ideally we'd just
    // make the table of variants.
    bool ScriptPropertyNumberArray::IsNumberArray(AZ::ScriptDataContext& context, int valueIndex)
    {
        bool isNumberArray = false;

        if (context.IsTable(valueIndex))
        {
            AZ::ScriptDataContext arrayTable;
            if (context.InspectTable(valueIndex,arrayTable))
            {
                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                // We only want to inspect the first element to inform us of what
                // type should be in our array for now.
                while (arrayTable.InspectNextElement(elementIndex,fieldName,fieldIndex))
                {
                    if (fieldIndex != -1)
                    {
                       break;
                    }
                }

                isNumberArray = arrayTable.IsNumber(elementIndex);
            }
        }

        return isNumberArray;
    }

    ScriptProperty* ScriptPropertyNumberArray::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        ScriptPropertyNumberArray* retVal = nullptr;

        if (IsNumberArray(context,valueIndex))
        {
            retVal = aznew ScriptPropertyNumberArray(name);

            AZ::ScriptDataContext arrayTable;
            if (retVal && context.InspectTable(valueIndex,arrayTable))
            {
                ParseNumberArray(arrayTable,retVal->m_values);
            }
        }

        return retVal;
    }

    void ScriptPropertyNumberArray::ParseNumberArray(AZ::ScriptDataContext& numberArrayTable, AZStd::vector<double>& output)
    {
        const char* fieldName;
        int fieldIndex;
        int elementIndex;
        while (numberArrayTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
        {
            // check only the arrayElements
            if (fieldIndex == -1)
            {
                continue;
            }

            if (numberArrayTable.IsNumber(elementIndex))
            {
                double value;
                if (numberArrayTable.ReadValue(elementIndex, value))
                {
                    output.push_back(value);
                }
            }
            else
            {
                AZ_Warning("Script",false,"All types inside the array must the same type or have a common class! Element[%d] is not a Number!", fieldIndex);
            }
        }
    }

    AZ::TypeId ScriptPropertyNumberArray::GetDataTypeUuid() const
    {
        return AZ::SerializeGenericTypeInfo< AZStd::vector<double> >::GetClassTypeId();
    }

    bool ScriptPropertyNumberArray::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return IsNumberArray(context,valueIndex);
    }

    AZ::ScriptPropertyNumberArray* ScriptPropertyNumberArray::Clone(const char* name) const
    {
        AZ::ScriptPropertyNumberArray* clonedValue = aznew ScriptPropertyNumberArray(name ? name : m_name.c_str());
        clonedValue->m_values = m_values;
        return clonedValue;
    }

    bool ScriptPropertyNumberArray::Write(AZ::ScriptContext& context)
    {
        return WriteContainerToLuaTableArray(context, m_values.begin(), m_values.end());
    }

    void ScriptPropertyNumberArray::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyNumberArray* scriptNumberArray = azrtti_cast<const AZ::ScriptPropertyNumberArray*>(scriptProperty);

        AZ_Error("ScriptPropertyNumberArray", scriptNumberArray, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (scriptNumberArray)
        {
            m_values = scriptNumberArray->m_values;
        }
    }

    //////////////////////////////
    // ScriptPropertyStringArray
    //////////////////////////////
    void ScriptPropertyStringArray::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyStringArray, ScriptProperty>()->
                Version(1)->
                Field("values", &ScriptPropertyStringArray::m_values);
        }
    }

    // This is kind of weird. This shouldn't really matter; Ideally we'd just
    // make the table of variants.
    bool ScriptPropertyStringArray::IsStringArray(AZ::ScriptDataContext& context, int valueIndex)
    {
        bool isStringArray = false;

        if (context.IsTable(valueIndex))
        {
            AZ::ScriptDataContext arrayTable;
            if (context.InspectTable(valueIndex,arrayTable))
            {
                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                // We only want to inspect the first element to inform us of what
                // type should be in our array for now.
                while (arrayTable.InspectNextElement(elementIndex,fieldName,fieldIndex))
                {
                    if (fieldIndex != -1)
                    {
                       break;
                    }
                }

                isStringArray = arrayTable.IsString(elementIndex);
            }
        }

        return isStringArray;
    }

    ScriptProperty* ScriptPropertyStringArray::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        ScriptPropertyStringArray* retVal = nullptr;

        if (IsStringArray(context,valueIndex))
        {
            retVal = aznew ScriptPropertyStringArray(name);

            AZ::ScriptDataContext arrayTable;
            if (retVal && context.InspectTable(valueIndex,arrayTable))
            {
                ParseStringArray(arrayTable,retVal->m_values);
            }
        }

        return retVal;
    }

    void ScriptPropertyStringArray::ParseStringArray(AZ::ScriptDataContext& stringArrayTable, AZStd::vector<AZStd::string>& output)
    {
        const char* fieldName;
        int fieldIndex;
        int elementIndex;
        while (stringArrayTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
        {
            // check only the arrayElements
            if (fieldIndex == -1)
            {
                continue;
            }

            if (stringArrayTable.IsString(elementIndex))
            {
                const char* value;
                if (stringArrayTable.ReadValue(elementIndex, value))
                {
                    output.push_back(value);
                }
            }
            else
            {
                AZ_Warning("Script",false,"All types inside the array must the same type or have a common class! Element[%d] is not a String!", fieldIndex);
            }
        }
    }

    AZ::TypeId ScriptPropertyStringArray::GetDataTypeUuid() const
    {
        return AZ::SerializeGenericTypeInfo< AZStd::vector<AZStd::string> >::GetClassTypeId();
    }

    bool ScriptPropertyStringArray::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return IsStringArray(context,valueIndex);
    }

    AZ::ScriptPropertyStringArray* ScriptPropertyStringArray::Clone(const char* name) const
    {
        AZ::ScriptPropertyStringArray* clonedValue = aznew ScriptPropertyStringArray(name ? name : m_name.c_str());
        clonedValue->m_values = m_values;
        return clonedValue;
    }

    bool ScriptPropertyStringArray::Write(AZ::ScriptContext& context)
    {
        return WriteContainerToLuaTableArray(context, m_values.begin(), m_values.end());
    }

    void ScriptPropertyStringArray::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyStringArray* stringPropertyArray = azrtti_cast<const AZ::ScriptPropertyStringArray*>(scriptProperty);

        AZ_Error("ScriptPropertyStringArray", stringPropertyArray, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (stringPropertyArray)
        {
            m_values = stringPropertyArray->m_values;
        }
    }

    ////////////////////////////////////
    // ScriptPropertyGenericClassArray
    ////////////////////////////////////
    // The GenericClassArray version converter is defined later in this file
    static bool ScriptPropertyGenericClassArrayVersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);
    void ScriptPropertyGenericClassArray::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyGenericClassArray, ScriptProperty>()->
                Version(2, &ScriptPropertyGenericClassArrayVersionConverter)->
                Field("values", &ScriptPropertyGenericClassArray::m_values)->
                Field("elementType", &ScriptPropertyGenericClassArray::m_elementTypeId);
        }
    }

    // This is kind of weird. This shouldn't really matter; Ideally we'd just
    // make the table of variants.
    bool ScriptPropertyGenericClassArray::IsGenericClassArray(AZ::ScriptDataContext& context, int valueIndex)
    {
        bool isGenericClassArray = false;

        if (context.IsTable(valueIndex))
        {
            AZ::ScriptDataContext arrayTable;
            if (context.InspectTable(valueIndex,arrayTable))
            {
                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                // We only want to inspect the first element to inform us of what
                // type should be in our array for now.
                while (arrayTable.InspectNextElement(elementIndex,fieldName,fieldIndex))
                {
                    if (fieldIndex != -1)
                    {
                       break;
                    }
                }

                isGenericClassArray = arrayTable.IsRegisteredClass(elementIndex);
            }
        }

        return isGenericClassArray;
    }

    ScriptProperty* ScriptPropertyGenericClassArray::TryCreateProperty(AZ::ScriptDataContext& context, int valueIndex, const char* name)
    {
        ScriptPropertyGenericClassArray* retVal = nullptr;

        if (IsGenericClassArray(context,valueIndex))
        {
            retVal = aznew ScriptPropertyGenericClassArray(name);

            AZ::ScriptDataContext arrayTable;
            if (retVal && context.InspectTable(valueIndex,arrayTable))
            {
                ParseGenericClassArray(arrayTable,retVal->m_values);

                // Grab the type of what we're storing if it's not already known
                if (retVal->m_elementTypeId.IsNull() && retVal->m_values.size() > 0)
                {
                    AZ::DynamicSerializableField& value = retVal->m_values.front();
                    retVal->m_elementTypeId = value.m_typeId;
                }
            }
        }

        return retVal;
    }

    void ScriptPropertyGenericClassArray::ParseGenericClassArray(AZ::ScriptDataContext& genericClassArrayTable, ScriptPropertyGenericClassArray::ValueArrayType& output)
    {
        const char* fieldName;
        int fieldIndex;
        int elementIndex;
        while (genericClassArrayTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
        {
            // check only the arrayElements
            if (fieldIndex == -1)
            {
                continue;
            }

            if (genericClassArrayTable.IsRegisteredClass(elementIndex))
            {
                output.emplace_back();
                AZ::DynamicSerializableField& value = output.back();

                value.m_data = Internal::LuaAnyClassFromStack(genericClassArrayTable.GetScriptContext()->NativeContext(), genericClassArrayTable.GetStartIndex() + elementIndex, &value.m_typeId);

                // If we fail to read, we want to remove the invalid data from our vector
                if (value.m_data)
                {
                    // clone the data or take ownership from LUA
                    const BehaviorClass* behaviorClass = nullptr;
                    if (Internal::LuaGetClassInfo(genericClassArrayTable.GetScriptContext()->NativeContext(), genericClassArrayTable.GetStartIndex() + elementIndex, nullptr, &behaviorClass))
                    {
                        BehaviorObject source;
                        source.m_address = value.m_data;
                        source.m_typeId = value.m_typeId;
                        BehaviorObject copy = behaviorClass->Clone(source);
                        value.m_data = copy.m_address;
                    }
                    else
                    {
                        AZ_Error("Script", false, "Failed to locate behavior class for field %s. Ignoring element.", fieldName);
                        output.pop_back();
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "Failed to read registered class %s. Ignoring element.", fieldName);
                    output.pop_back();
                }
            }
            else
            {
                AZ_Warning("Script",false,"All types inside the array must the same type or have a common class! Element[%d] is different from the elemenent before it!", fieldIndex);
            }
        }
    }

    AZ::TypeId ScriptPropertyGenericClassArray::GetDataTypeUuid() const
    {
        return AZ::SerializeGenericTypeInfo< ValueArrayType >::GetClassTypeId();
    }

    AZ::Uuid ScriptPropertyGenericClassArray::GetElementTypeUuid() const
    {
        return m_elementTypeId;
    }

    void ScriptPropertyGenericClassArray::SetElementTypeUuid(const AZ::Uuid elementTypeId)
    {
        m_elementTypeId = elementTypeId;
    }

    bool ScriptPropertyGenericClassArray::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return IsGenericClassArray(context,valueIndex);
    }

    AZ::ScriptPropertyGenericClassArray* ScriptPropertyGenericClassArray::Clone(const char* name) const
    {
        AZ::ScriptPropertyGenericClassArray* clonedValue = aznew ScriptPropertyGenericClassArray(name ? name : m_name.c_str());
        clonedValue->m_values = m_values;
        clonedValue->m_elementTypeId = m_elementTypeId;
        return clonedValue;
    }

    bool ScriptPropertyGenericClassArray::Write(AZ::ScriptContext& context)
    {
        lua_State* lua = context.NativeContext();
        size_t numElements = m_values.size();
        if (numElements)
        {
            lua_createtable(lua, static_cast<int>(numElements), 0);
            for (size_t i = 0; i < numElements; ++i)
            {
                Internal::LuaClassToStack(context.NativeContext(), m_values[i].m_data, m_values[i].m_typeId);
                lua_rawseti(lua, -2, static_cast<int>(i+1));
            }

            return true;
        }
        return false;
    }

    void ScriptPropertyGenericClassArray::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyGenericClassArray* genericClassArray = azrtti_cast<const AZ::ScriptPropertyGenericClassArray*>(scriptProperty);

        AZ_Error("ScriptPropertyGenericClassArray", genericClassArray, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (genericClassArray)
        {
            m_values.clear();

            const AZ::ScriptPropertyGenericClassArray* sourceProperty = static_cast<const AZ::ScriptPropertyGenericClassArray*>(scriptProperty);
            m_values.reserve(sourceProperty->m_values.size());

            for (auto& serializableField : m_values)
            {
                m_values.emplace_back(serializableField);
            }

            m_elementTypeId = sourceProperty->m_elementTypeId;
        }
    }

    bool ScriptPropertyGenericClassArrayVersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {

        if (classElement.GetVersion() == 1)
        {
            // Generate dynamic element Id field.
            for (int classIdx = 0; classIdx < classElement.GetNumSubElements(); ++classIdx)
            {
                AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(classIdx);
                if (elementNode.GetName() == AZ_CRC_CE("values"))
                {
                    if (elementNode.GetNumSubElements() > 0)
                    {
                        // If there are elements to derive from, base the element type off that
                        AZ::SerializeContext::DataElementNode& dsfNode = elementNode.GetSubElement(0);
                        for (int dsfIdx = 0; dsfIdx < dsfNode.GetNumSubElements(); ++dsfIdx)
                        {
                            AZ::SerializeContext::DataElementNode& dataNode = dsfNode.GetSubElement(dsfIdx);
                            if (dataNode.GetName() == AZ_CRC_CE("m_data"))
                            {
                                const int idx = classElement.AddElement<AZ::Uuid>(context, "elementType");
                                const AZ::Uuid elementType = dataNode.GetId();
                                classElement.GetSubElement(idx).SetData<AZ::Uuid>(context, elementType);
                            }
                        }
                    }
                    else
                    {
                        // If there are not we'll CreateNull now and use that during LoadScript to create a "from script original"
                        // to get this info out of (see: ScriptEditorComponent::LoadProperties)
                        const int idx = classElement.AddElement<AZ::Uuid>(context, "elementType");
                        const AZ::Uuid elementType = AZ::Uuid::CreateNull();
                        classElement.GetSubElement(idx).SetData<AZ::Uuid>(context, elementType);
                    }
                }
            }
        }

        return true;
    }

    ////////////////////////
    // ScriptPropertyAsset
    ////////////////////////

    void ScriptPropertyAsset::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ScriptPropertyAsset, ScriptProperty>()->
                Version(1)->
                Field("value", &ScriptPropertyAsset::m_value);
        }
    }

    AZ::TypeId ScriptPropertyAsset::GetDataTypeUuid() const
    {
        return AZ::SerializeTypeInfo<AZ::Data::Asset<AZ::Data::AssetData> >::GetUuid();
    }

    bool ScriptPropertyAsset::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return context.IsRegisteredClass(valueIndex);
    }

    AZ::ScriptPropertyAsset* ScriptPropertyAsset::Clone(const char* name) const
    {
        AZ::ScriptPropertyAsset* clonedValue = aznew ScriptPropertyAsset(name ? name : m_name.c_str());
        clonedValue->m_value = m_value;
        return clonedValue;
    }

    bool ScriptPropertyAsset::Write(AZ::ScriptContext& context)
    {
        (void)context;
        // \todo - We don't yet support marshaling asset ptrs to/from lua. @Rosen.
        //ScriptValue<AZ::Data::Asset<AZ::Data::AssetData>>::StackPush(context.ToNativeContext(), m_value);
        return true;
    }

    void ScriptPropertyAsset::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyAsset* assetProperty = azrtti_cast<const AZ::ScriptPropertyAsset*>(scriptProperty);

        AZ_Error("ScriptPropertyAsset", assetProperty, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (assetProperty)
        {
            m_value = assetProperty->m_value;
        }
    }
    }
