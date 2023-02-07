/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/DynamicSerializableField.h>

#include <AzCore/Component/ComponentApplicationBus.h>   // to find the default serialize context

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    DynamicSerializableField::DynamicSerializableField()
        : m_data(nullptr)
        , m_typeId(Uuid::CreateNull())        
    {
    }
    //-------------------------------------------------------------------------
    DynamicSerializableField::DynamicSerializableField(const DynamicSerializableField& serializableField)
    {
        m_data = serializableField.CloneData();
        m_typeId = serializableField.m_typeId;
    }
    //-------------------------------------------------------------------------
    DynamicSerializableField::~DynamicSerializableField()
    {
        // Should we call DestroyData() here?
    }
    //-------------------------------------------------------------------------
    bool DynamicSerializableField::IsValid() const
    {
        return m_data && !m_typeId.IsNull();
    }
    //-------------------------------------------------------------------------
    void DynamicSerializableField::DestroyData(SerializeContext* useContext)
    {
        if (IsValid())
        {
            // Right now we rely on the fact that destructor info has to be reflected in the default
            // SerializeContext held by the ComponentApplication.
            // If we ever need to have multiple contexts, we will have to add a way to refer to the context
            // that created our data and use that to query for the proper destructor info.
            if (!useContext)
            {
                ComponentApplicationBus::BroadcastResult(useContext, &ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("DynamicSerializableField", useContext, "Can't find valid serialize context. Dynamic data cannot be deleted without it!");
            }
            if (useContext)
            {
                const SerializeContext::ClassData* classData = useContext->FindClassData(m_typeId);
                if (classData)
                {
                    if (classData->m_factory)
                    {
                        classData->m_factory->Destroy(m_data);
                        m_data = nullptr;
                        m_typeId = Uuid::CreateNull();                        
                        return;
                    }
                    else
                    {
                        AZ_Error("DynamicSerializableField", false, "Class data for type Uuid %s does not contain a valid destructor. Dynamic data cannot be deleted without it!", m_typeId.ToString<AZStd::string>().c_str());
                    }
                }
                else
                {
                    AZ_Error("DynamicSerializableField", false, "Can't find class data for type Uuid %s. Dynamic data cannot be deleted without it!", m_typeId.ToString<AZStd::string>().c_str());
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void* DynamicSerializableField::CloneData(SerializeContext* useContext) const
    {
        if (IsValid())
        {
            if (!useContext)
            {
                ComponentApplicationBus::BroadcastResult(useContext, &ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("DynamicSerializableField", useContext, "Can't find valid serialize context. Dynamic data cannot be deleted without it!");
            }
            if (useContext)
            {
                void* clonedData = useContext->CloneObject(m_data, m_typeId);
                return clonedData;
            }
        }
        return nullptr;
    }
    //-------------------------------------------------------------------------
    void DynamicSerializableField::CopyDataFrom(const DynamicSerializableField& other, SerializeContext* useContext)
    {
        DestroyData();
        m_typeId    = other.m_typeId;        
        m_data      = other.CloneData(useContext);
    }
    //-------------------------------------------------------------------------
    bool DynamicSerializableField::IsEqualTo(const DynamicSerializableField& other, SerializeContext* useContext) const
    {
        if (other.m_typeId != m_typeId)
        {
            return false;
        }

        bool isEqual = false;

        if (!useContext)
        {
            ComponentApplicationBus::BroadcastResult(useContext, &ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Error("DynamicSerializableField", useContext, "Can't find valid serialize context. Dynamic data cannot be compared without it!");
        }        

        if (useContext)
        {
            const SerializeContext::ClassData* classData = useContext->FindClassData(m_typeId);
            if (classData)
            {
                if (classData->m_serializer)
                {
                    isEqual = classData->m_serializer->CompareValueData(m_data, other.m_data);
                }
                else
                {
                    AZStd::vector<AZ::u8> myData;
                    AZ::IO::ByteContainerStream<decltype(myData)> myDataStream(&myData);

                    Utils::SaveObjectToStream(myDataStream, AZ::ObjectStream::ST_BINARY, m_data, m_typeId);

                    AZStd::vector<AZ::u8> otherData;
                    AZ::IO::ByteContainerStream<decltype(otherData)> otherDataStream(&otherData);

                    Utils::SaveObjectToStream(otherDataStream, AZ::ObjectStream::ST_BINARY, other.m_data, other.m_typeId);
                    isEqual = (myData.size() == otherData.size()) && (memcmp(myData.data(), otherData.data(), myData.size()) == 0);
                }
            }
            else
            {
                isEqual = m_data == nullptr && other.m_data == nullptr;
                AZ_Error("DynamicSerializableField", m_data == nullptr, "Trying to compare unknown data types(%i).", m_typeId);
            }
        }

        return isEqual;
    }
    
}   // namespace AZ
