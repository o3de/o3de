/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    const Crc32 Serialize::Attributes::EnumValueKey(AZ_CRC_CE("EnumValueKey"));
    const Crc32 Serialize::Attributes::EnumUnderlyingType(AZ_CRC_CE("EnumUnderlyingType"));

    SerializeContext::EnumBuilder::EnumBuilder(SerializeContext* context, const typename SerializeContext::UuidToClassMap::iterator& classMapIter)
        : m_context(context)
        , m_classData(classMapIter)
    {
        if (!context->IsRemovingReflection())
        {
            m_currentAttributes = &classMapIter->second.m_attributes;
        }
    }

    auto SerializeContext::EnumBuilder::operator->() -> EnumBuilder*
    {
        return this;
    }

    auto SerializeContext::EnumBuilder::Version(unsigned int version, VersionConverter converter) -> EnumBuilder*
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        AZ_Assert(version != Serialize::VersionClassDeprecated, "You cannot use %u as the version, it is reserved by the system!", version);
        m_classData->second.m_version = version;
        m_classData->second.m_converter = converter;
        return this;
    }

    auto SerializeContext::EnumBuilder::Serializer(Serialize::IDataSerializerPtr serializer) -> EnumBuilder*
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }

        AZ_Assert(m_classData->second.m_elements.empty(),
            "Enum %s has a custom serializer and can not have additional fields. Enum can either have a custom serializer or values.",
            m_classData->second.m_name);

        m_classData->second.m_serializer = AZStd::move(serializer);
        return this;
    }

    auto SerializeContext::EnumBuilder::EventHandler(IEventHandler* eventHandler) -> EnumBuilder*
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_eventHandler = eventHandler;
        return this;
    }

    auto SerializeContext::EnumBuilder::DataContainer(IDataContainer* dataContainer) -> EnumBuilder*
    {
        if (m_context->IsRemovingReflection())
        {
            return this;
        }
        m_classData->second.m_container = dataContainer;
        return this;
    }

    auto SerializeContext::EnumBuilder::PersistentId(ClassPersistentId persistentId) -> EnumBuilder*
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_persistentId = persistentId;
        return this;
    }
}
