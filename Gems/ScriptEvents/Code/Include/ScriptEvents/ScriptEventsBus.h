/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>

#include <ScriptEvents/ScriptEventFundamentalTypes.h>
#include "ScriptEventRegistration.h"

namespace ScriptEvents
{
    class ScriptEvent;
    class ScriptEventsHandler;

    namespace Internal
    {
        class ScriptEventRegistration;
    }

    //! External facing API for registering and getting ScriptEvents
    class ScriptEventRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        using MutexType = AZStd::recursive_mutex;

        virtual AZStd::intrusive_ptr<Internal::ScriptEventRegistration> RegisterScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) = 0;
        virtual void RegisterScriptEventFromDefinition([[maybe_unused]] const ScriptEvent& definition) {}
        virtual void UnregisterScriptEventFromDefinition([[maybe_unused]] const ScriptEvent& definition) {}
        virtual AZStd::intrusive_ptr<Internal::ScriptEventRegistration> GetScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) = 0;
        virtual const FundamentalTypes* GetFundamentalTypes() = 0;
    };

    using ScriptEventBus = AZ::EBus<ScriptEventRequests>;

    //! Script event general purpose notifications
    class ScriptEventNotifications : public AZ::EBusTraits 
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        using BusIdType = AZ::Data::AssetId;

        virtual void OnRegistered(const ScriptEvent&) {}
    };

    using ScriptEventNotificationBus = AZ::EBus<ScriptEventNotifications>;

    //! Used as the key into a map of ScriptEvents, it relies on the asset and version in order to support storing
    //! multiple versions of a ScriptEvent definition.
    struct ScriptEventKey
    {
        AZ::Data::AssetId m_assetId;
        AZ::u32 m_version;

        ScriptEventKey(AZ::Data::AssetId assetId, AZ::u32 version)
            : m_assetId(assetId)
            , m_version(version)
        {}

        bool operator==(const ScriptEventKey& rhs) const
        {
            return m_assetId == rhs.m_assetId && m_version == rhs.m_version;
        }
    };
}

namespace AZStd
{
    // hash specialization
    template <>
    struct hash<ScriptEvents::ScriptEventKey>
    {
        typedef AZ::Uuid    argument_type;
        typedef size_t      result_type;
        size_t operator()(const ScriptEvents::ScriptEventKey& key) const
        {
            size_t retVal = 0;
            AZStd::hash_combine(retVal, key.m_assetId);
            AZStd::hash_combine(retVal, key.m_version);
            return retVal;
        }
    };
}
