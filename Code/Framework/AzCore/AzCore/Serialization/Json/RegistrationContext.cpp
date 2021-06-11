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

#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    JsonRegistrationContext::~JsonRegistrationContext()
    {
        AZ_Assert(m_jsonSerializers.empty(), "JsonRegistrationContext is being destroyed without unreflecting all serializers. Check your reflection functions.");
        AZ_Assert(m_handledTypesMap.empty(), "JsonRegistrationContext is being destroyed without unreflecting all serializers. Check your reflection functions.");
    }

    JsonRegistrationContext::SerializerBuilder* JsonRegistrationContext::SerializerBuilder::operator->()
    {
        return this;
    }

    JsonRegistrationContext::SerializerBuilder::SerializerBuilder(JsonRegistrationContext* context, SerializerMap::const_iterator serializerMapIter)
        : m_context(context)
        , m_serializerIter(serializerMapIter)
    {}

    JsonRegistrationContext::SerializerBuilder* JsonRegistrationContext::SerializerBuilder::HandlesTypeId(
        const Uuid& uuid, bool overwriteExisting)
    {
        if (!m_context->IsRemovingReflection())
        {
            auto serializer = m_serializerIter->second.get();
            if (uuid.IsNull())
            {
                AZ_Assert(false, "Could not register Json serializer %s. Its Uuid is null.", serializer->RTTI_GetTypeName());
                return this;
            }

            if (!overwriteExisting)
            {
                auto emplaceResult = m_context->m_handledTypesMap.try_emplace(uuid, serializer);
                AZ_Assert(
                    emplaceResult.second,
                    "Couldn't register Json serializer %s. Another serializer (%s) has already been registered for the same Uuid (%s).",
                    serializer->RTTI_GetTypeName(), emplaceResult.first->second->RTTI_GetTypeName(),
                    uuid.ToString<AZStd::string>().c_str());
            }
            else
            {
                m_context->m_handledTypesMap.insert_or_assign(uuid, serializer);
            }
        }
        else
        {
            m_context->m_handledTypesMap.erase(uuid);
        }

        return this;
    }

    const JsonRegistrationContext::HandledTypesMap& JsonRegistrationContext::GetRegisteredSerializers() const
    {
        return m_handledTypesMap;
    }

    BaseJsonSerializer* JsonRegistrationContext::GetSerializerForType(const Uuid& typeId) const
    {        
        auto serializer = m_handledTypesMap.find(typeId);
        if (serializer != m_handledTypesMap.end())
        {
            return serializer->second;
        }

        return nullptr;
    }

    BaseJsonSerializer* JsonRegistrationContext::GetSerializerForSerializerType(const Uuid& typeId) const
    {
        auto serializerIter = m_jsonSerializers.find(typeId);
        return serializerIter != m_jsonSerializers.end() ? serializerIter->second.get() : nullptr;
    }
} // namespace AZ
