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

#include <AzFramework/Input/Channels/InputChannelId.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelId::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InputChannelId>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Constructor<const char*>()
                ->Property("name", [](InputChannelId* thisPtr) { return thisPtr->GetName(); }, nullptr)
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelId::InputChannelId(const char* name)
        : m_crc32(name)
    {
        memset(m_name, 0, AZ_ARRAY_SIZE(m_name));
        azstrncpy(m_name, NAME_BUFFER_SIZE, name, MAX_NAME_LENGTH);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelId::InputChannelId(const InputChannelId& other)
        : m_crc32(other.m_crc32)
    {
        memset(m_name, 0, AZ_ARRAY_SIZE(m_name));
        azstrcpy(m_name, NAME_BUFFER_SIZE, other.m_name);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelId& InputChannelId::operator=(const InputChannelId& other)
    {
        azstrcpy(m_name, NAME_BUFFER_SIZE, other.m_name);
        m_crc32 = other.m_crc32;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* InputChannelId::GetName() const
    {
        return m_name;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZ::Crc32& InputChannelId::GetNameCrc32() const
    {
        return m_crc32;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelId::operator==(const InputChannelId& other) const
    {
        return (m_crc32 == other.m_crc32);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannelId::operator!=(const InputChannelId& other) const
    {
        return !(*this == other);
    }
} // namespace AzFramework
