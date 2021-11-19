/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework
{
    namespace Script
    {
        struct LuaPropertySymbol
        {
            AZ_TYPE_INFO(LuaPropertySymbol, "{5AFB147F-50A4-4F00-9F82-D8D5BBC970D6}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            bool m_canRead;
            bool m_canWrite;

            AZStd::string ToString() const;
        };

        struct LuaMethodSymbol
        {
            AZ_TYPE_INFO(LuaMethodSymbol, "{7B074A36-C81D-46A0-8D2F-62E426EBE38A}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_debugArgumentInfo;

            AZStd::string ToString() const;
        };

        struct LuaClassSymbol
        {
            AZ_TYPE_INFO(LuaClassSymbol, "{5FBE5841-A8E1-44B6-BEDA-22302CF8DF5F}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZ::Uuid m_typeId;
            AZStd::vector<LuaPropertySymbol> m_properties;
            AZStd::vector<LuaMethodSymbol> m_methods;

            AZStd::string ToString() const;
        };

        struct LuaEBusSender
        {
            AZ_TYPE_INFO(LuaEBusSender, "{23EE4188-0924-49DB-BF3F-EB7AAB6D5E5C}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_debugArgumentInfo;
            AZStd::string m_category;

            AZStd::string ToString() const;
        };

        struct LuaEBusSymbol
        {
            AZ_TYPE_INFO(LuaEBusSymbol, "{381C5639-A916-4D2E-B825-50A3F2D93137}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            bool m_canBroadcast;
            bool m_canQueue;
            bool m_hasHandler;

            AZStd::vector<LuaEBusSender> m_senders;

            AZStd::string ToString() const;
        };

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
