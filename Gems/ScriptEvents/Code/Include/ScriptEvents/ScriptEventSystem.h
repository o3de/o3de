/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ScriptEventsBus.h"
#include "ScriptEvent.h"

namespace ScriptEvents
{
    class ScriptEventsSystemComponentImpl
        : protected ScriptEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptEventsSystemComponentImpl, AZ::SystemAllocator);

        ScriptEventsSystemComponentImpl()
        {
            ScriptEventBus::Handler::BusConnect();
        }

        ~ScriptEventsSystemComponentImpl()
        {
            ScriptEventBus::Handler::BusDisconnect();
        }

        virtual void RegisterAssetHandler() = 0;
        virtual void UnregisterAssetHandler() = 0;

        ////////////////////////////////////////////////////////////////////////
        // ScriptEvents::ScriptEventBus::Handler
        AZStd::intrusive_ptr<Internal::ScriptEventRegistration> RegisterScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) override;
        void RegisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition) override;
        void UnregisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition) override;
        AZStd::intrusive_ptr<Internal::ScriptEventRegistration> GetScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) override;
        const FundamentalTypes* GetFundamentalTypes() override;
        AZ::Outcome<ScriptEvents::ScriptEvent, AZStd::string> LoadDefinitionSource(const AZ::IO::Path& path) override;
        AZ::Outcome<void, AZStd::string> SaveDefinitionSourceFile
            ( const ScriptEvents::ScriptEvent& events
            , const AZ::IO::Path& path) override;
        ////////////////////////////////////////////////////////////////////////

    private:

        // Script Event Assets
        AZStd::unordered_map<ScriptEventKey, AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration>> m_scriptEvents;

        FundamentalTypes m_fundamentalTypes;

    };

    class ScriptEventModuleConfigurationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ScriptEventsSystemComponentImpl* GetSystemComponentImpl() = 0;
    };
    using ScriptEventModuleConfigurationRequestBus = AZ::EBus< ScriptEventModuleConfigurationRequests>;

}
