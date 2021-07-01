/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/any.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    class GameplayNotificationId
    {
    public:
        AZ_TYPE_INFO(GameplayNotificationId, "{C5225D36-7068-412D-A46E-DDF79CA1D7FF}");
        GameplayNotificationId() = default;
        GameplayNotificationId(const AZ::EntityId& entityChannel, AZ::Crc32 actionNameCrc, const AZ::Uuid& payloadType)
            : m_channel(entityChannel)
            , m_actionNameCrc(actionNameCrc)
            , m_payloadTypeId(payloadType)
        { }
        GameplayNotificationId(const AZ::EntityId&  entityChannel, const char* actionName, const AZ::Uuid& payloadType)
            : GameplayNotificationId(entityChannel, AZ::Crc32(actionName), payloadType) {}

        //////////////////////////////////////////////////////////////////////////
        // Deprecated constructors.  These will be removed in 1.12
        GameplayNotificationId(const AZ::EntityId& entityChannel, AZ::Crc32 actionNameCrc)
            : GameplayNotificationId(entityChannel, actionNameCrc, AZ::Uuid::CreateNull())
        {
            AZ_Warning("GameplayNotificationId", false, "You are using a deprecated constructor.  You must now create the bus id with the type you are expecting to send/receive");
        }
        GameplayNotificationId(const AZ::EntityId&  entityChannel, const char* actionName)
            : GameplayNotificationId(entityChannel, AZ::Crc32(actionName)) { }
        //////////////////////////////////////////////////////////////////////////

        bool operator==(const GameplayNotificationId& rhs) const { return m_channel == rhs.m_channel && m_actionNameCrc == rhs.m_actionNameCrc && m_payloadTypeId == rhs.m_payloadTypeId; }
        GameplayNotificationId Clone() const { return *this; }
        AZStd::string ToString() const
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            auto&& behaviorClassIterator = behaviorContext->m_typeToClassMap.find(m_payloadTypeId);
            bool typeFound = behaviorClassIterator != behaviorContext->m_typeToClassMap.end();
            AZStd::string payloadName = typeFound ? behaviorClassIterator->second->m_name : m_payloadTypeId.ToString<AZStd::string>();
            return AZStd::string::format("(channel=%llu, actionNameCrc=%u, payloadTypeId=%s)", static_cast<AZ::u64>(m_channel), static_cast<AZ::u32>(m_actionNameCrc), payloadName.c_str());
        }

        AZ::EntityId m_channel = AZ::EntityId(0);
        AZ::Crc32 m_actionNameCrc;
        AZ::Uuid m_payloadTypeId = AZ::Uuid::CreateNull();
    };


    //  //////////////////////////////////////////////////////////////////////////
    //  /// The Event Notification bus is used to alert gameplay systems that an event
    //  /// has occurred successfully or in a failure state.
    //  //////////////////////////////////////////////////////////////////////////
    class GameplayNotifications
        : public AZ::EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef GameplayNotificationId BusIdType;
        virtual ~GameplayNotifications() = default;
        virtual void OnEventBegin(const AZStd::any&) {}
        virtual void OnEventUpdating(const AZStd::any&) {}
        virtual void OnEventEnd(const AZStd::any&) {}
    };
    using GameplayNotificationBus = AZ::EBus<GameplayNotifications>;
} // namespace AZ

namespace AZStd
{
    template <>
    struct hash < AZ::GameplayNotificationId >
    {
        inline size_t operator()(const AZ::GameplayNotificationId& actionId) const
        {
            AZStd::hash<AZ::EntityId> entityIdHasher;
            size_t retVal = entityIdHasher(actionId.m_channel);
            AZStd::hash_combine(retVal, actionId.m_actionNameCrc);
            AZStd::hash_combine(retVal, actionId.m_payloadTypeId);
            return retVal;
        }
    };
}
