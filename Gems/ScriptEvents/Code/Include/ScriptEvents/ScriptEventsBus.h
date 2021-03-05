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

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>

#include <ScriptEvents/ScriptEvent.h>
#include <ScriptEvents/ScriptEventFundamentalTypes.h>

namespace ScriptEvents
{
    class ScriptEvent;

    class ScriptEventsHandler;

    //! External facing API for registering and getting ScriptEvents
    class ScriptEventRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual AZStd::intrusive_ptr<Internal::ScriptEvent> RegisterScriptEvent([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] AZ::u32 version) { return nullptr; }
        virtual void RegisterScriptEventFromDefinition([[maybe_unused]] const ScriptEvent& definition) {}
        virtual void UnregisterScriptEventFromDefinition([[maybe_unused]] const ScriptEvent& definition) {}
        virtual AZStd::intrusive_ptr<Internal::ScriptEvent> GetScriptEvent([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] AZ::u32 version) { return {}; }
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
