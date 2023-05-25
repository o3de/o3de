/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef AZCORE_SCRIPT_SCRIPTPROPERTYTABLE_H
#define AZCORE_SCRIPT_SCRIPTPROPERTYTABLE_H

#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Script/ScriptPropertyWatcherBus.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/unordered_map.h>

// Weird forward declares for friending.
//
// Ideally, the Table would know how to marshal itself to avoid this.
namespace AzFramework
{
    class ScriptPropertyMarshaler;
    class ScriptPropertyTableMarshalerHelper;
}

namespace AZ
{
    // Helper class that allows the Map to use specific Custom classes as keys into the map.
    // This provides the interface to the templated class.
    class ScriptPropertyGenericClassMap
    {
    public:
        struct MapValuePair
        {
            MapValuePair()
                : m_keyProperty(nullptr)
                , m_valueProperty(nullptr)
            {
            }

            void CloneTo(MapValuePair& targetProperty)
            {
                targetProperty.Destroy();

                targetProperty.m_keyProperty = m_keyProperty->Clone();
                targetProperty.m_valueProperty = m_valueProperty->Clone();
            }

            void Destroy()
            {
                delete m_keyProperty;
                m_keyProperty = nullptr;

                delete m_valueProperty;
                m_valueProperty = nullptr;
            }

            AZ::ScriptPropertyGenericClass* m_keyProperty;
            AZ::ScriptProperty*             m_valueProperty;
        };

        AZ_CLASS_ALLOCATOR(ScriptPropertyGenericClassMap, SystemAllocator);

        ScriptPropertyGenericClassMap() = default;
        virtual ~ScriptPropertyGenericClassMap() = default;

        virtual void UpdateTableValue(const AZ::ScriptPropertyGenericClass* keyProperty, AZ::ScriptDataContext& scriptDataContext, int valueIndex) = 0;
        virtual void SetTableValue(const AZ::ScriptPropertyGenericClass* keyProperty, const AZ::ScriptProperty* scriptProperty) = 0;
        virtual AZ::ScriptProperty* FindTableValue(const AZ::ScriptPropertyGenericClass* keyProperty) const = 0;

        virtual void SetWatcher(AZ::ScriptPropertyWatcher* scriptPropertyWatcher) = 0;

        virtual ScriptPropertyGenericClassMap* Clone() = 0;
        virtual void CloneDataFrom(ScriptPropertyGenericClassMap* dataSource)= 0;
    };

    // Template implementation of the map interface, lets us use user defined classes as keys into the table.
    // And still maintain most of our in place data parsing.
    template<typename T>
    class ScriptPropertyGenericClassMapImpl
        : public ScriptPropertyGenericClassMap
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyGenericClassMapImpl<T>, SystemAllocator);

        ScriptPropertyGenericClassMapImpl()
            : m_scriptPropertyWatcher(nullptr)
            , m_keyTypeId(T::TYPEINFO_Uuid())
        {
        }

        ~ScriptPropertyGenericClassMapImpl() override
        {
            for (auto& mapPair : m_pairMapping)
            {
                mapPair.second.Destroy();
            }
        }

        void UpdateTableValue(const AZ::ScriptPropertyGenericClass* keyProperty, AZ::ScriptDataContext& scriptDataContext, int valueIndex) override
        {
            AZ_Assert(keyProperty->GetDataTypeUuid() == m_keyTypeId, "Passed in invalid GenericClass Type to ScriptPropertyMap implementaiton.");

            if (keyProperty->GetDataTypeUuid() == m_keyTypeId)
            {
                const T* dataSource = keyProperty->Get<T>();

                auto valueIter = m_pairMapping.find((*dataSource));

                if (valueIter == m_pairMapping.end())
                {
                    MapValuePair valuePair;

                    valuePair.m_keyProperty = keyProperty->Clone();
                    valuePair.m_valueProperty = scriptDataContext.ConstructScriptProperty(valueIndex,"mv");

                    WatchValueProperty(valuePair.m_valueProperty);

                    m_pairMapping.emplace((*dataSource),valuePair);

                }
                else
                {
                    if (!valueIter->second.m_valueProperty->TryRead(scriptDataContext,valueIndex))
                    {
                        delete valueIter->second.m_valueProperty;
                        valueIter->second.m_valueProperty = nullptr;

                        AZ::ScriptProperty* scriptProperty = scriptDataContext.ConstructScriptProperty(valueIndex,"mv");

                        if (!azrtti_istypeof<AZ::ScriptPropertyNil>(scriptProperty) || scriptProperty == nullptr)
                        {
                            valueIter->second.m_valueProperty = scriptProperty;
                            WatchValueProperty(scriptProperty);
                        }
                        else
                        {
                            delete scriptProperty;

                            valueIter->second.Destroy();
                            m_pairMapping.erase(valueIter);
                        }
                    }
                }
            }
        }

        void SetTableValue(const AZ::ScriptPropertyGenericClass* keyProperty, const AZ::ScriptProperty* scriptProperty) override
        {
            AZ_Assert(keyProperty->GetDataTypeUuid() == m_keyTypeId, "Passing wrong GenericClass Type to ScriptPropertyMap implementation");

            if (keyProperty->GetDataTypeUuid() == m_keyTypeId)
            {
                const T* dataSource = keyProperty->Get<T>();

                auto valueIter = m_pairMapping.find((*dataSource));

                if (valueIter == m_pairMapping.end())
                {
                    if (scriptProperty != nullptr && !azrtti_istypeof<AZ::ScriptPropertyNil>(scriptProperty))
                    {
                        MapValuePair valuePair;

                        valuePair.m_keyProperty = keyProperty->Clone();
                        valuePair.m_valueProperty = scriptProperty->Clone();

                        WatchValueProperty(valuePair.m_valueProperty);

                        m_pairMapping.emplace((*dataSource),valuePair);
                    }
                }
                else
                {
                    if (!valueIter->second.m_valueProperty->TryUpdate(scriptProperty))
                    {
                        delete valueIter->second.m_valueProperty;

                        if (scriptProperty != nullptr && !azrtti_istypeof<AZ::ScriptPropertyNil>(scriptProperty))
                        {
                            valueIter->second.m_valueProperty = scriptProperty->Clone();

                            WatchValueProperty(valueIter->second.m_valueProperty);
                        }
                        else
                        {
                            valueIter->second.Destroy();
                            m_pairMapping.erase(valueIter);
                        }
                    }
                }
            }
        }

        AZ::ScriptProperty* FindTableValue(const AZ::ScriptPropertyGenericClass* keyProperty) const override
        {
            AZ_Assert(keyProperty->GetDataTypeUuid() == m_keyTypeId, "Given the wrong generic key type.");
            AZ::ScriptProperty* retVal = nullptr;

            if (keyProperty->GetDataTypeUuid() == m_keyTypeId)
            {
                const T* dataSource = keyProperty->Get<T>();

                auto valueIter = m_pairMapping.find((*dataSource));

                if (valueIter !=  m_pairMapping.end())
                {
                    retVal = valueIter->second.m_valueProperty;
                }
            }

            return retVal;
        }

        ScriptPropertyGenericClassMap* Clone() override
        {
            ScriptPropertyGenericClassMapImpl<T>* retVal = aznew ScriptPropertyGenericClassMapImpl<T>();
            retVal->CloneDataFrom(this);
            return retVal;
        }

        void CloneDataFrom(ScriptPropertyGenericClassMap* dataBase) override
        {
            ScriptPropertyGenericClassMapImpl<T>* sourceData = static_cast<ScriptPropertyGenericClassMapImpl<T>*>(dataBase);
            AZStd::unordered_set<T> newKeys;

            for (auto& sourcePair : sourceData->m_pairMapping)
            {
                newKeys.insert(sourcePair.first);

                auto mapIter = m_pairMapping.find(sourcePair.first);

                if (mapIter != m_pairMapping.end())
                {
                    if (!mapIter->second.m_valueProperty->TryUpdate(sourcePair.second.m_valueProperty))
                    {
                        delete mapIter->second.m_valueProperty;
                        mapIter->second.m_valueProperty = sourcePair.second.m_valueProperty->Clone();

                        WatchValueProperty(mapIter->second.m_valueProperty);
                    }
                }
                else
                {
                    MapValuePair newPair;
                    sourcePair.second.CloneTo(newPair);

                    WatchValueProperty(newPair.m_valueProperty);
                    m_pairMapping.emplace(sourcePair.first, newPair);
                }
            }

            auto mapIter = m_pairMapping.begin();
            while (mapIter != m_pairMapping.end())
            {
                if (newKeys.find(mapIter->first) == newKeys.end())
                {
                    mapIter->second.Destroy();
                    mapIter = m_pairMapping.erase(mapIter);
                }
                else
                {
                    ++mapIter;
                }
            }
        }

        const AZStd::unordered_map<T, MapValuePair>& GetPairMapping() const
        {
            return m_pairMapping;
        }

        AZStd::unordered_map<T, MapValuePair>& GetPairMapping()
        {
            return m_pairMapping;
        }

        void SetWatcher(AZ::ScriptPropertyWatcher* scriptPropertyWatcher) override
        {
            AZ_Error("ScriptPropertyTable", m_scriptPropertyWatcher == nullptr || scriptPropertyWatcher == nullptr, "Trying to add two script property watchers to a Generic Keyed Script Property Map");
            if (m_scriptPropertyWatcher == nullptr || scriptPropertyWatcher == nullptr)
            {
                m_scriptPropertyWatcher = scriptPropertyWatcher;

                for (auto& mapPair : m_pairMapping)
                {
                    WatchValueProperty(mapPair.second.m_valueProperty);
                }
            }
        }

    private:

        void WatchValueProperty(AZ::ScriptProperty* scriptProperty)
        {
            AZ::FunctionalScriptProperty* functionalScriptProperty = azrtti_cast<AZ::FunctionalScriptProperty*>(scriptProperty);

            if (functionalScriptProperty)
            {
                if (m_scriptPropertyWatcher)
                {
                    functionalScriptProperty->EnableInPlaceControls();
                    functionalScriptProperty->AddWatcher(m_scriptPropertyWatcher);
                }
                else
                {
                    functionalScriptProperty->DisableInPlaceControls();
                    functionalScriptProperty->RemoveWatcher(m_scriptPropertyWatcher);
                }
            }
        }

        AZ::ScriptPropertyWatcher*                m_scriptPropertyWatcher;
        AZ::Uuid                                  m_keyTypeId;
        AZStd::unordered_map<T, MapValuePair>     m_pairMapping;
    };

    class ScriptPropertyTable
        : public FunctionalScriptProperty
        , public ScriptPropertyWatcher
        , public ScriptPropertyWatcherBus::Handler
    {
    private:
        friend class AzFramework::ScriptPropertyMarshaler;
        friend class AzFramework::ScriptPropertyTableMarshalerHelper;

    public:
        typedef AZStd::unordered_map<int, ScriptProperty*> ScriptPropertyIndexMap;
        typedef AZStd::unordered_map<AZ::Crc32, ScriptProperty*> ScriptPropertyKeyedMap;
        typedef AZStd::unordered_map<AZ::Uuid, ScriptPropertyGenericClassMap* > ScriptPropertyGenericMap;

        AZ_CLASS_ALLOCATOR(ScriptPropertyTable, SystemAllocator);
        AZ_RTTI(AZ::ScriptPropertyTable, "{0EB069C0-F6C6-4871-9BDB-CC1BBF0B5315}", FunctionalScriptProperty);

        // No reflection for this one. Since we would need to reflect pointers.
        static ScriptProperty* TryCreateProperty(ScriptDataContext& context, int valueIndex, const char* name);

        ScriptPropertyTable();
        explicit ScriptPropertyTable(const char* name);
        ScriptPropertyTable(const char* name, const ScriptPropertyTable* scriptPropertyTable);

        ~ScriptPropertyTable() override;

        ScriptProperty* FindTableValue(AZ::ScriptDataContext& scriptDataContext, int keyIndex) const;
        ScriptProperty* FindTableValue(const AZ::ScriptPropertyGenericClass* scriptProperty) const;
        ScriptProperty* FindTableValue(const AZStd::string_view& keyValue) const;
        ScriptProperty* FindTableValue(int index) const;

        // This is the generalized update method that will determine the correct type of key to use.
        void UpdateTableValue(AZ::ScriptDataContext& scriptDataContext, int keyIndex, int valueIndex);

        // These are the update methods coming that directly use the appropriate key.
        void UpdateTableValue(const AZ::ScriptPropertyGenericClass* scriptProperty, AZ::ScriptDataContext& scriptDataContext, int index);
        void UpdateTableValue(const AZStd::string_view& keyValue, AZ::ScriptDataContext& scriptDataContext, int index);
        void UpdateTableValue(int tableIndex, AZ::ScriptDataContext& scriptDataContext, int stackIndex);

        void SetTableValue(const AZ::ScriptProperty* keyProperty, const AZ::ScriptProperty* value);
        void SetTableValue(const AZ::ScriptPropertyGenericClass* keyProperty, const AZ::ScriptProperty* value);
        void SetTableValue(const AZStd::string_view& keyValue, const ScriptProperty* value);
        void SetTableValue(int index, const ScriptProperty* value);

        template<typename T>
        ScriptPropertyGenericClassMapImpl<T>* FindKeyedTableBase() const
        {
            ScriptPropertyGenericClassMapImpl<T>* retVal = nullptr;

            AZ::Uuid typeInfo = T::TYPEINFO_Uuid();

            auto mapIter = m_genericMapping.find(typeInfo);

            if (mapIter != m_genericMapping.end())
            {
                retVal = static_cast<ScriptPropertyGenericClassMapImpl<T>*>(mapIter->second);
            }

            return retVal;
        }

        // TODO: Investigate what is actually needed here.
        const void* GetDataAddress() const override { return nullptr; }
        AZ::TypeId GetDataTypeUuid() const override
        {
            static Uuid k_invalidUuid = Uuid();
            return k_invalidUuid;
        }

        ScriptPropertyTable* Clone(const char* name = nullptr) const override;

        // By default, Write will write out a hard copy of the table.
        // and be unaware of any values that are changing from the script.
        //
        // If the Property is going to be re-used(and owned somewhere), MetatableControl should be enabled
        // which will push out a lightweight table, that contains metamethods
        // to simulate the correct output from the object.
        bool Write(AZ::ScriptContext& context) override;

        // Bypasses the metatable indexing, and writes out the actual raw values of the table.
        bool WriteRawTable(AZ::ScriptContext& context);

        void EnableInPlaceControls() override;
        void DisableInPlaceControls() override;

        AZ::ScriptContext* GetScriptContext() const;

        // Doesn't return the actual size, but returns what lua defines as the length of the table.
        // http://www.lua.org/manual/5.2/manual.html#3.4.6
        //
        // tl;dr - Let the length of the table be N, where N is the the largest integer s.t. 1..N, are all keys present in the table.
        int GetLuaTableLength() const;


        // ScriptPropertyWatcherBus
         void OnObjectModified() override;

    protected:
        void CloneDataFrom(const ScriptProperty* scriptProperty) override;

    private:

        void CreateCachedTable(AZ::ScriptContext& scriptContext);
        void ReleaseCachedTable();

        void SetTableValuePropertiesWatched(bool watched);
        void SetTableValueScriptPropertyWatched(ScriptProperty* scriptProperty, bool watchProperty);

        void InitKeyProperties();
        void PopulateSupportedGenericKeyTypes();

        template<typename T>
        void RawWriteTableMap(AZ::ScriptContext& context)
        {
            ScriptPropertyGenericClassMapImpl<T>* mapBase = FindKeyedTableBase<T>();

            if (mapBase)
            {
                const auto& valueMap = mapBase->GetPairMapping();

                for (auto& valuePair : valueMap)
                {
                    RawTableWriterHelper(context, valuePair.second.m_keyProperty, valuePair.second.m_valueProperty);
                }
            }
        }

        // Mainly here to avoid needing to include the lua externs in the header
        void RawTableWriterHelper(AZ::ScriptContext& scriptContext, AZ::ScriptProperty* keyProperty, AZ::ScriptProperty* valueProperty) const;

        // Used to determine caching process.
        bool                   m_enableMetatableControl;
        AZ::ScriptContext*     m_scriptContext;
        int                    m_tableCache;

        ScriptPropertyIndexMap      m_indexMapping;
        ScriptPropertyKeyedMap      m_keyMapping;
        ScriptPropertyGenericMap    m_genericMapping;

        // Keep a property here I can read the generic class data into.
        // to avoid constantly needing to recreate the key to do look-ups.
        AZ::ScriptPropertyNumber*       m_numberProperty;
        AZ::ScriptPropertyString*       m_stringProperty;
        AZ::ScriptPropertyGenericClass* m_genericProperty;
    };
}

#endif
