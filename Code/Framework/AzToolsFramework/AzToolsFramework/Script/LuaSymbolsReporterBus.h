/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

//AZTF-EBUS
#include <AzToolsFramework/AzToolsFrameworkEBus.h>

#include <AzToolsFramework/Script/LuaSymbolsReporter.h>
#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework
{
    namespace Script
    {
        // This is an EBus useful to scrape classes, globals and EBuses exposed to game scripting
        // e.g: Lua.
        class LuaSymbolsReporterRequests
        {
        public:
            AZ_RTTI(LuaSymbolsReporterRequests, "{3FF9A105-3159-49FF-8DC6-4948AE7B4AB8}");
            virtual ~LuaSymbolsReporterRequests() = default;
            // Put your public methods here

            virtual const AZStd::vector<LuaClassSymbol>& GetListOfClasses() = 0;
            virtual const AZStd::vector<LuaPropertySymbol>& GetListOfGlobalProperties() = 0;
            virtual const AZStd::vector<LuaMethodSymbol>& GetListOfGlobalFunctions() = 0;
            virtual const AZStd::vector<LuaEBusSymbol>& GetListOfEBuses() = 0;

        };
        
        class LuaSymbolsReporterBusTraits
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
        };

        using LuaSymbolsReporterRequestBus = AZ::EBus<LuaSymbolsReporterRequests, LuaSymbolsReporterBusTraits>;

    } // namespace Script
} // namespace AzToolsFramework

AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS_WITH_TRAITS(AzToolsFramework::Script::LuaSymbolsReporterRequests, AzToolsFramework::Script::LuaSymbolsReporterBusTraits);
