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
#ifndef AZFRAMEWORK_NETWORK_DYNAMICSERIALIZABLEFIELDMARSHALER_H
#define AZFRAMEWORK_NETWORK_DYNAMICSERIALIZABLEFIELDMARSHALER_H

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/UuidMarshal.h>

namespace GridMate
{
    /**
    * Marshaler for DynamicSerializableField, contains a template param for allocating the memory buffer that it's going to use to write to.    
    */
    template<size_t BufferSize>
    class DynamicSerializableFieldMarshaler
    {
    public:
        DynamicSerializableFieldMarshaler()
            : m_serializeContext(nullptr)
        {
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        }

        // Mainly here for unit test purposes.
        DynamicSerializableFieldMarshaler(AZ::SerializeContext* context)
            : m_serializeContext(context)
        {
        }

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const AZ::DynamicSerializableField& value) const
        {
            AZ_Error("DynamicSerializableFieldMarshaler", m_serializeContext, "Unknown SerializationContext. Aborting Marshal attempt.\n");
            if (m_serializeContext)
            {
                Marshaler<AZ::u32> sizeMarshaler;                
                Marshaler<AZ::Uuid> uuidMarshaler;

                AZStd::vector<AZ::u8> memoryBuffer(BufferSize);

                // Start buffer in write mode.
                AZ::IO::ByteContainerStream<decltype(memoryBuffer)> memoryStream(&memoryBuffer);

                AZ::u32 bufferSize = 0;
                
                if (m_serializeContext->FindClassData(value.m_typeId))
                {
                    if (AZ::Utils::SaveObjectToStream(memoryStream, AZ::DataStream::StreamType::ST_BINARY, value.m_data, value.m_typeId, m_serializeContext))
                    {
                        bufferSize = static_cast<AZ::u32>(memoryStream.GetCurPos());
                    }
                }
                else
                {
                    AZ_Error("DynamicSerializableFieldMarshaler", !value.IsValid(), "Could not save object to stream because type Id %s is not registered with the serializer.\n", value.m_typeId.ToString<AZStd::string>().c_str());
                }

                sizeMarshaler.Marshal(wb, bufferSize);                    
                uuidMarshaler.Marshal(wb, value.m_typeId);                
                wb.WriteRaw(memoryBuffer.data(), bufferSize);
            }
        }
        
        AZ_FORCE_INLINE void Unmarshal(AZ::DynamicSerializableField& value, ReadBuffer& rb) const
        {
            value.DestroyData(m_serializeContext);
            
            AZ_Error("DynamicSerializableFieldMarshaler", m_serializeContext, "Unknown SerializationContext. Aborting Unmarshal attempt.\n");
            if (m_serializeContext)
            {
                Marshaler<AZ::u32> sizeMarshaler;
                AZ::u32 marshaledBufferSize = 0;
                sizeMarshaler.Unmarshal(marshaledBufferSize, rb);

                AZ_Assert(marshaledBufferSize <= BufferSize,"Trying to deserialize too much data for the allocated buffer size\n");

                // Marshal out the TypeId so I can use it on the receiving end.
                Marshaler<AZ::Uuid> uuidMarshaler;
                uuidMarshaler.Unmarshal(value.m_typeId, rb);

                if (marshaledBufferSize > 0)
                {
                    // See if there's some nice way to use this.
                    // - Can't make this a member variable, since both these methods are const.
                    AZStd::vector<AZ::u8> memoryBuffer(marshaledBufferSize + 1);

                    if (rb.ReadRaw(memoryBuffer.data(), marshaledBufferSize))
                    {
                        // Start buffer in read mode.
                        AZ::IO::ByteContainerStream<decltype(memoryBuffer)> memoryStream(&memoryBuffer);       

                        // we'll use a strict filter here, one that doesn't allow deserialization to automatically start loading assets, nor tolerates errors.
                        // this is becuase this is coming from a network interface and should always be error-free.
                        AZ::ObjectStream::FilterDescriptor filterToUse(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_STRICT);
                        value.m_data = AZ::Utils::LoadObjectFromStream(memoryStream, m_serializeContext, &value.m_typeId, filterToUse);
                    }
                }
            }
        }

    private:
        AZ::SerializeContext* m_serializeContext;
    };

    /**
     * Specialized marshaler for AZ::DynamicSerializableField
     * Mainly here to hook into the DataSet Marshaler auto detection logic, and provide a default buffer size for the actual marshaler     
     */    
    template<>
    class Marshaler<AZ::DynamicSerializableField>
        : public DynamicSerializableFieldMarshaler<1024>
    {
    public:
        
        Marshaler()
        {
        }
        
        // Mainly here for unit test purposes.
        Marshaler(AZ::SerializeContext* context)
            : DynamicSerializableFieldMarshaler(context)
        {
        }
    };
}

#endif