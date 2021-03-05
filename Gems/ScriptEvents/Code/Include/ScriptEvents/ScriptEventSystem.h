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

#include "ScriptEventsBus.h"

namespace ScriptEvents
{
    class ScriptEventsSystemComponentImpl
        : protected ScriptEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptEventsSystemComponentImpl, AZ::SystemAllocator, 0);

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
        AZStd::intrusive_ptr<Internal::ScriptEvent> RegisterScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) override;
        void RegisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition) override;
        void UnregisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition) override;
        AZStd::intrusive_ptr<Internal::ScriptEvent> GetScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version) override;
        const FundamentalTypes* GetFundamentalTypes() override;
        ////////////////////////////////////////////////////////////////////////

    private:

        // Script Event Assets
        AZStd::unordered_map<ScriptEventKey, AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>> m_scriptEvents;

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
