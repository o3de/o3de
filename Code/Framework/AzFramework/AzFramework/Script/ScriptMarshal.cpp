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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/UuidMarshal.h>
#include <GridMate/Serialize/ContainerMarshal.h>

#include <AzFramework/Script/ScriptNetBindings.h>
#include <AzFramework/Network/DynamicSerializableFieldMarshaler.h>
#include <AzFramework/Network/EntityIdMarshaler.h>

#include "AzFramework/Script/ScriptMarshal.h"

namespace AzFramework
{
    ////////////////////////////
    // ScriptPropertyMarshaler
    ////////////////////////////

    template<class T>
    bool UnmarshalGenericType(AZ::DynamicSerializableField& serializableField, GridMate::ReadBuffer& rb)
    {
        bool valueChanged = true;

        GridMate::Marshaler<AZ::DynamicSerializableField> serializableFieldMarshaler;

        // Store the old value, to compare with the unmarshaled value, to signal
        T oldValue = (*serializableField.Get<T>());

        serializableFieldMarshaler.Unmarshal(serializableField,rb);

        // If our type hasn't changed, compare the values.
        if (serializableField.m_typeId == T::TYPEINFO_Uuid())
        {
            valueChanged = !(oldValue == (*serializableField.Get<T>()));
        }        

        return valueChanged;
    }

    class ScriptPropertyTableMarshalerHelper
    {
    public:
        template<typename T>
        static void MarshalScriptPropertyGenericMap(const ScriptPropertyMarshaler& scriptPropertyMarshaler, GridMate::WriteBuffer& wb, const AZ::ScriptPropertyTable* scriptPropertyTable)
        {
            GridMate::Marshaler<AZ::u32> sizeMarshaler;

            auto mapIter = scriptPropertyTable->m_genericMapping.find(T::TYPEINFO_Uuid());

            if (mapIter != scriptPropertyTable->m_genericMapping.end())
            {
                AZ::ScriptPropertyGenericClassMapImpl<T>* genericClassKeyMap = static_cast<AZ::ScriptPropertyGenericClassMapImpl<T>*>(mapIter->second);

                auto& valueMap = genericClassKeyMap->GetPairMapping();            

                // We will write out all of our keys. Since it is easier to write out nil values for the properties.
                sizeMarshaler.Marshal(wb,static_cast<AZ::u32>(valueMap.size()));

                GridMate::Marshaler<T> keyMarshaler;

                for (auto& mapPair : valueMap)
                {
                    keyMarshaler.Marshal(wb,mapPair.first);                
                    scriptPropertyMarshaler.Marshal(wb,mapPair.second.m_valueProperty);
                }
            }
            else
            {
                sizeMarshaler.Marshal(wb,0);
            }
        }    

        template<typename T>
        static bool UnmarshalScriptPropertyGenericMap(const ScriptPropertyMarshaler& scriptPropertyMarshaler, AZ::ScriptPropertyTable* scriptPropertyTable, GridMate::ReadBuffer& rb)
        {
            bool valueChanged = false;

            AZ::SerializeContext* useContext  = nullptr;
            EBUS_EVENT_RESULT(useContext, AZ::ComponentApplicationBus, GetSerializeContext);

            if (useContext)
            {
                const AZ::SerializeContext::ClassData* classData = useContext->FindClassData(T::TYPEINFO_Uuid());

                if (classData && classData->m_factory)
                {
                    auto mapIter = scriptPropertyTable->m_genericMapping.find(T::TYPEINFO_Uuid());

                    if (mapIter != scriptPropertyTable->m_genericMapping.end())
                    {
                        // This whole thing is an in-place map update.
                        // to try to minimize the number of allocations. We try to re-use objects as much as possible.
                        //
                        // Two phase approach: Step one, update all of the existing properties, while keeping track of all of the used keys.
                        // Step two, go through and delete any unupdated keys from the mapping.
                        AZ::ScriptPropertyGenericClassMapImpl<T>* genericClassKeyMap = static_cast<AZ::ScriptPropertyGenericClassMapImpl<T>*>(mapIter->second);

                        AZStd::unordered_set<T> newKeys;

                        GridMate::Marshaler<AZ::u32> sizeMarshaler;

                        AZ::u32 mapSize;
                        sizeMarshaler.Unmarshal(mapSize,rb);

                        auto& valueMap = genericClassKeyMap->GetPairMapping();            

                        GridMate::Marshaler<T> keyMarshaler;

                        for (unsigned int i=0; i < mapSize; ++i)
                        {
                            T propertyKey;
                            keyMarshaler.Unmarshal(propertyKey,rb);

                            newKeys.insert(propertyKey);

                            auto valueIter = valueMap.find(propertyKey);

                            if (valueIter != valueMap.end())
                            {
                                if (scriptPropertyMarshaler.UnmarshalToPointer(valueIter->second.m_valueProperty,rb))
                                {
                                    valueChanged = true;
                                }
                            }
                            else
                            {
                                valueChanged = true;
                    
                                AZ::ScriptProperty* newValueProperty = nullptr;
                                scriptPropertyMarshaler.UnmarshalToPointer(newValueProperty,rb);

                                AZ::ScriptPropertyGenericClassMap::MapValuePair newPair;

                                newPair.m_valueProperty = newValueProperty;                    

                                T* serializableData = nullptr;
                                serializableData = static_cast<T*>(classData->m_factory->Create("ScriptProperty"));
                                (*serializableData) = propertyKey;

                                AZ::ScriptPropertyGenericClass* genericPropertyClass = aznew AZ::ScriptPropertyGenericClass();

                                genericPropertyClass->Set<T>(serializableData);

                                newPair.m_keyProperty = genericPropertyClass;

                                valueMap.emplace(propertyKey,newPair);
                            }
                        }

                        // Delete all of the unused keyes from the map
                        auto valueIter = valueMap.begin();            

                        while (valueIter != valueMap.end())
                        {
                            if (newKeys.find(valueIter->first) == newKeys.end())
                            {
                                valueChanged = true;
                                valueIter->second.Destroy();                    
                                valueIter = valueMap.erase(valueIter);
                            }
                            else
                            {
                                ++valueIter;                    
                            }
                        }            
                    }                    
                }
            }

            return valueChanged;
        }
    };

    
    
    void ScriptPropertyMarshaler::Marshal(GridMate::WriteBuffer& wb, AZ::ScriptProperty*const& property) const
    {        
        GridMate::Marshaler<AZ::Uuid> typeMarshaler;
        GridMate::Marshaler<AZ::u64>  idMarshaler;
        GridMate::Marshaler<AZStd::string> nameMarshaler;

        if (property == nullptr)
        {
            // Write out a nil property if we have a nullptr property
            nameMarshaler.Marshal(wb,"");
            idMarshaler.Marshal(wb,0);
            typeMarshaler.Marshal(wb,AZ::ScriptPropertyNil::RTTI_Type());
            return;
        }

        // Common points: 
        // Always going to marshal the uuid of the type(or something similar)
        // so we know what type we have on the other side.
        //
        // Next need to pass along the name field.        
        const AZ::Uuid& typeId = azrtti_typeid(property);

        nameMarshaler.Marshal(wb,property->m_name);
        idMarshaler.Marshal(wb,property->m_id);
        
        // Method 1:        
        // - Allow each ScriptProperty to marshal itself.
        // - Currently unavailable since the ScriptProperties live in AZCore
        //   and the WriteBuffer is in GridMate.
        // cont.Marshal(wb);
        
        // Method 2:
        // - Process all of our known marshallable types and use the appropriate marshaler        
        if (typeId == AZ::ScriptPropertyBoolean::RTTI_Type())
        {
            typeMarshaler.Marshal(wb,typeId);

            GridMate::Marshaler<bool> boolMarshaler;
            boolMarshaler.Marshal(wb,static_cast<const AZ::ScriptPropertyBoolean*>(property)->m_value);
        }
        else if (typeId == AZ::ScriptPropertyNumber::RTTI_Type())
        {
            typeMarshaler.Marshal(wb,typeId);

            GridMate::Marshaler<double> doubleMarshaler;
            doubleMarshaler.Marshal(wb,static_cast<const AZ::ScriptPropertyNumber*>(property)->m_value);
        }
        else if (typeId == AZ::ScriptPropertyString::RTTI_Type())
        {
            typeMarshaler.Marshal(wb,typeId);

            GridMate::Marshaler<AZStd::string> stringMarshaler;
            stringMarshaler.Marshal(wb,static_cast<const AZ::ScriptPropertyString*>(property)->m_value);
        }
        else if (typeId == AZ::ScriptPropertyGenericClass::RTTI_Type())
        {
            const AZ::DynamicSerializableField& serializableField = static_cast<const AZ::ScriptPropertyGenericClass*>(property)->GetSerializableField();
            
            typeMarshaler.Marshal(wb,typeId);

            GridMate::Marshaler<AZ::DynamicSerializableField> serializableFieldMarshaler;
            serializableFieldMarshaler.Marshal(wb,serializableField);            
        }
        else if (typeId == AZ::ScriptPropertyTable::TYPEINFO_Uuid())
        {
            const AZ::ScriptPropertyTable* scriptPropertyTable = static_cast<const AZ::ScriptPropertyTable*>(property);

            typeMarshaler.Marshal(wb,typeId);

            GridMate::Marshaler<AZ::u32> mapSizeMarshaler;
            mapSizeMarshaler.Marshal(wb,static_cast<AZ::u32>(scriptPropertyTable->m_indexMapping.size()));

            GridMate::Marshaler<int> indexMarshaler;            

            // Currently only support integers as keys inside of the table.
            for (auto& mapPair : scriptPropertyTable->m_indexMapping)
            {
                indexMarshaler.Marshal(wb,mapPair.first);
                this->Marshal(wb,mapPair.second);
            }

            mapSizeMarshaler.Marshal(wb, static_cast<AZ::u32>(scriptPropertyTable->m_keyMapping.size()));

            GridMate::Marshaler<AZ::u32> hashMarshaler;

            for (auto& mapPair : scriptPropertyTable->m_keyMapping)
            {
                // For hashed values. The name of the script property is the same as the hash it should be using.
                // We still synchronize the Crc so we can unmarshal in place on the other side.
                hashMarshaler.Marshal(wb,mapPair.first);
                Marshal(wb,mapPair.second);
            }

            // EntityId's
            ScriptPropertyTableMarshalerHelper::MarshalScriptPropertyGenericMap<AZ::EntityId>((*this), wb, scriptPropertyTable);
        }
        else
        {
            typeMarshaler.Marshal(wb,AZ::ScriptPropertyNil::RTTI_Type());
        }
    }

    bool ScriptPropertyMarshaler::UnmarshalToPointer(AZ::ScriptProperty*& target, GridMate::ReadBuffer& rb) const
    {
        bool typeChanged = false;
        AZ::Uuid typeId;
        AZ::u64 id;
        AZStd::string name;

        GridMate::Marshaler<AZ::Uuid> typeMarshaler;        
        GridMate::Marshaler<AZ::u64> idMarshaler;
        GridMate::Marshaler<AZStd::string> nameMarshaler;

        nameMarshaler.Unmarshal(name,rb);
        idMarshaler.Unmarshal(id,rb);
        typeMarshaler.Unmarshal(typeId,rb);
        
        if (target == nullptr || typeId != azrtti_typeid(target))
        {
            typeChanged = true;

            AZ::ScriptProperty* actualScriptProperty = nullptr;
            if (typeId == AZ::ScriptPropertyBoolean::RTTI_Type())
            {
                actualScriptProperty = aznew AZ::ScriptPropertyBoolean();
            }
            else if (typeId == AZ::ScriptPropertyNumber::RTTI_Type())
            {
                actualScriptProperty = aznew AZ::ScriptPropertyNumber();
            }
            else if (typeId == AZ::ScriptPropertyString::RTTI_Type())
            {
                actualScriptProperty = aznew AZ::ScriptPropertyString();
            }
            else if (typeId == AZ::ScriptPropertyGenericClass::RTTI_Type())
            {
                actualScriptProperty = aznew AZ::ScriptPropertyGenericClass();                
            }
            else if (typeId == AZ::ScriptPropertyTable::RTTI_Type())
            {
                actualScriptProperty = aznew AZ::ScriptPropertyTable();
            }
            else
            {
                actualScriptProperty = aznew AZ::ScriptPropertyNil();
            }
            
            actualScriptProperty->m_name = name;            
            delete target;

            target = actualScriptProperty;            
        }

        // Update our ID
        target->m_id = id;
        
        // Method 1:
        // - Allow each ScriptProperty to unmarshal itself
        // - Currently unavailable since the ScriptProperties live in AZCore
        //   and the WriteBuffer is in GridMate            
        // actualScriptProperty->Unmarshal(rb);
        //
        // Method 2:
        // - Process all of our known marshallable types and use the appropriate marshaler

        bool valueChanged = false;

        if (typeId == AZ::ScriptPropertyBoolean::RTTI_Type())
        {
            AZ::ScriptPropertyBoolean* booleanProperty = static_cast<AZ::ScriptPropertyBoolean*>(target);
            bool oldValue = booleanProperty->m_value;

            GridMate::Marshaler<bool> boolMarshaler;
            boolMarshaler.Unmarshal(booleanProperty->m_value,rb);

            valueChanged = !(oldValue == booleanProperty->m_value);
        }
        else if (typeId == AZ::ScriptPropertyString::RTTI_Type())
        {
            AZ::ScriptPropertyString* stringProperty = static_cast<AZ::ScriptPropertyString*>(target);
            AZStd::string oldValue = stringProperty->m_value;

            GridMate::Marshaler<AZStd::string> stringMarshaler;
            stringMarshaler.Unmarshal(stringProperty->m_value,rb);

            valueChanged = !(oldValue == stringProperty->m_value);
        }
        else if (typeId == AZ::ScriptPropertyNumber::RTTI_Type())
        {
            AZ::ScriptPropertyNumber* numberProperty = static_cast<AZ::ScriptPropertyNumber*>(target);
            double oldValue = numberProperty->m_value;

            GridMate::Marshaler<double> numberMarshaler;
            numberMarshaler.Unmarshal(numberProperty->m_value,rb);

            valueChanged = !(oldValue == numberProperty->m_value);
        }
        else if (typeId == AZ::ScriptPropertyGenericClass::RTTI_Type())
        {
            AZ::ScriptPropertyGenericClass* genericProperty = static_cast<AZ::ScriptPropertyGenericClass*>(target);

            AZ::DynamicSerializableField& serializableField = genericProperty->m_value;

            AZ::DynamicSerializableField oldField;

            oldField.CopyDataFrom(serializableField);            

            GridMate::Marshaler<AZ::DynamicSerializableField> serializableFieldMarshaler;
            serializableFieldMarshaler.Unmarshal(serializableField,rb);

            // If our type hasn't changed, compare the values.
            valueChanged = !oldField.IsEqualTo(serializableField);
        }
        else if (typeId == AZ::ScriptPropertyTable::RTTI_Type())
        {
            AZ::ScriptPropertyTable* scriptPropertyTable = static_cast<AZ::ScriptPropertyTable*>(target);
            GridMate::Marshaler<AZ::u32> mapSizeMarshaler;            

            // Unmarshal all of the indexes properties
            {
                AZ::u32 mapSize = 0;
                mapSizeMarshaler.Unmarshal(mapSize, rb);

                AZStd::unordered_set<int> newIndexes;
                GridMate::Marshaler<int> indexMarshaler;

                for (AZ::u32 i=0; i < mapSize; ++i)
                {
                    int index = 0;
                    indexMarshaler.Unmarshal(index,rb);                

                    auto mapIter = scriptPropertyTable->m_indexMapping.find(index);
                
                    if (mapIter != scriptPropertyTable->m_indexMapping.end())
                    {
                        if (UnmarshalToPointer(mapIter->second,rb))
                        {
                            valueChanged = true;
                        }
                    }                
                    else
                    {
                        valueChanged = true;

                        AZ::ScriptProperty* scriptProperty = nullptr;
                        UnmarshalToPointer(scriptProperty,rb);
                        auto insertResult = scriptPropertyTable->m_indexMapping.emplace(index,scriptProperty);                    
                        mapIter = insertResult.first;
                    }

                    if (mapIter->second == nullptr || azrtti_istypeof<AZ::ScriptPropertyNil>(mapIter->second))
                    {
                        valueChanged = true;

                        delete mapIter->second;
                        scriptPropertyTable->m_indexMapping.erase(mapIter);                        
                    }
                    else
                    {
                        newIndexes.insert(index);
                    }
                }

                auto mapIter = scriptPropertyTable->m_indexMapping.begin();

                while (mapIter != scriptPropertyTable->m_indexMapping.end())
                {
                    if (newIndexes.find(mapIter->first) == newIndexes.end())
                    {
                        valueChanged = true;

                        delete mapIter->second;
                        mapIter = scriptPropertyTable->m_indexMapping.erase(mapIter);                        
                    }
                    else
                    {
                        ++mapIter;
                    }
                }
            }

            // Unmarshal all of the hashed values
            {
                AZ::u32 mapSize = 0;
                mapSizeMarshaler.Unmarshal(mapSize, rb);

                AZStd::unordered_set<AZ::u32> newHashes;
                GridMate::Marshaler<AZ::u32> hashMarshaler;

                for (AZ::u32 i=0; i < mapSize; ++i)
                {
                    AZ::u32 newHash;
                    hashMarshaler.Unmarshal(newHash, rb);

                    auto mapIter = scriptPropertyTable->m_keyMapping.find(newHash);

                    if (mapIter != scriptPropertyTable->m_keyMapping.end())
                    {
                        if (UnmarshalToPointer(mapIter->second,rb))
                        {
                            valueChanged = true;
                        }
                    }
                    else
                    {
                        valueChanged = true;

                        AZ::ScriptProperty* scriptProperty = nullptr;
                        UnmarshalToPointer(scriptProperty,rb);
                        auto emplaceResult = scriptPropertyTable->m_keyMapping.emplace(newHash,scriptProperty);
                        mapIter = emplaceResult.first;
                    }

                    if (mapIter->second == nullptr || azrtti_istypeof<AZ::ScriptPropertyNil>(mapIter->second))
                    {
                        valueChanged = true;

                        delete mapIter->second;
                        scriptPropertyTable->m_keyMapping.erase(mapIter);                        
                    }
                    else
                    {
                        newHashes.insert(newHash);
                    }
                }

                auto mapIter = scriptPropertyTable->m_keyMapping.begin();

                while (mapIter != scriptPropertyTable->m_keyMapping.end())
                {
                    if (newHashes.find(mapIter->first) == newHashes.end())
                    {
                        valueChanged = true;

                        delete mapIter->second;
                        mapIter = scriptPropertyTable->m_keyMapping.erase(mapIter);
                    }
                    else
                    {
                        ++mapIter;
                    }
                }
            }            

            // Unmarshal all of the generic properties

            // EntityId's
            if (ScriptPropertyTableMarshalerHelper::UnmarshalScriptPropertyGenericMap<AZ::EntityId>((*this), scriptPropertyTable, rb))
            {
                valueChanged = true;
            }
        }

        return typeChanged || valueChanged;
    }    

    ////////////////////////////
    // ScriptPropertyThrottler
    ////////////////////////////
    
    ScriptPropertyThrottler::ScriptPropertyThrottler()
        : m_isDirty(true)
    {
    
    }

    void ScriptPropertyThrottler::SignalDirty()
    {
        m_isDirty = true;
    }

    bool ScriptPropertyThrottler::WithinThreshold(AZ::ScriptProperty* newValue) const
    {
        return newValue == nullptr || !m_isDirty;
    }

    void ScriptPropertyThrottler::UpdateBaseline(AZ::ScriptProperty* baseline)
    {
        (void)baseline;

        m_isDirty = false;
    }
}