/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzFramework/Input/User/LocalUserId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Crc.h>

namespace StartingPointInput
{
    class InputEventNotificationId
    {
    public:
        AZ_TYPE_INFO(InputEventNotificationId, "{9E0F0801-348B-4FF9-AF9B-858D59404968}");
        InputEventNotificationId() = default;
        InputEventNotificationId(const AzFramework::LocalUserId& localUserId, const AZ::Crc32& actionNameCrc)
            : m_localUserId(localUserId)
            , m_actionNameCrc(actionNameCrc)
        { }
        InputEventNotificationId(const AzFramework::LocalUserId& localUserId, const char* actionName)
            : InputEventNotificationId(localUserId, AZ::Crc32(actionName)) { }
        InputEventNotificationId(const AZ::Crc32& actionNameCrc)
            : InputEventNotificationId(AzFramework::LocalUserIdAny, actionNameCrc)
        {
        }
        InputEventNotificationId(const char* actionName) : InputEventNotificationId(AZ::Crc32(actionName)) {}
        bool operator==(const InputEventNotificationId& rhs) const { return m_localUserId == rhs.m_localUserId && m_actionNameCrc == rhs.m_actionNameCrc; }
        InputEventNotificationId Clone() const { return *this; }
        AZStd::string ToString() const { return AZStd::string::format("%s, %u", AzFramework::LocalUserIdToString(m_localUserId).c_str(), static_cast<AZ::u32>(m_actionNameCrc)); }

        AzFramework::LocalUserId m_localUserId;
        AZ::Crc32 m_actionNameCrc;
    };

    //////////////////////////////////////////////////////////////////////////
    /// The Input Event Notification bus is used to alert systems that an input event
    /// has been processed
    //////////////////////////////////////////////////////////////////////////
    class InputEventNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef InputEventNotificationId BusIdType;
        virtual ~InputEventNotifications() = default;

        virtual void OnPressed(float /*Value*/) {}
        virtual void OnHeld(float /*Value*/) {}
        virtual void OnReleased(float /*Value*/) {}
    };
    using InputEventNotificationBus = AZ::EBus<InputEventNotifications>;
} // namespace StartingPointInput


namespace AZStd
{
    template <>
    struct hash <StartingPointInput::InputEventNotificationId>
    {
        inline size_t operator()(const StartingPointInput::InputEventNotificationId& actionId) const
        {
            size_t retVal = 0;
            AZStd::hash_combine(retVal, actionId.m_localUserId, actionId.m_actionNameCrc);
            return retVal;
        }
    };
}
