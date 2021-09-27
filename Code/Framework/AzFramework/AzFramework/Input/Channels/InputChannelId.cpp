/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    const char* InputChannelId::GetName() const
    {
        return m_name.c_str();
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
