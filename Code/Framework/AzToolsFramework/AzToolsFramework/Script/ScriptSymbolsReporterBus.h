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
        struct PropertySymbol
        {
            AZ_TYPE_INFO(PropertySymbol, "{5AFB147F-50A4-4F00-9F82-D8D5BBC970D6}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            bool m_canRead;
            bool m_canWrite;

            AZStd::string ToString() const;
        };

        struct MethodSymbol
        {
            AZ_TYPE_INFO(MethodSymbol, "{7B074A36-C81D-46A0-8D2F-62E426EBE38A}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_debugArgumentInfo;

            AZStd::string ToString() const;
        };

        struct ClassSymbol
        {
            AZ_TYPE_INFO(ClassSymbol, "{5FBE5841-A8E1-44B6-BEDA-22302CF8DF5F}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZ::Uuid m_typeId;
            AZStd::vector<PropertySymbol> m_properties;
            AZStd::vector<MethodSymbol> m_methods;

            AZStd::string ToString() const;
        };

        struct EBusSender
        {
            AZ_TYPE_INFO(EBusSender, "{23EE4188-0924-49DB-BF3F-EB7AAB6D5E5C}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_debugArgumentInfo;
            AZStd::string m_category;

            AZStd::string ToString() const;
        };

        struct EBusSymbol
        {
            AZ_TYPE_INFO(EBusSymbol, "{381C5639-A916-4D2E-B825-50A3F2D93137}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            bool m_canBroadcast;
            bool m_canQueue;
            bool m_hasHandler;

            AZStd::vector<EBusSender> m_senders;

            AZStd::string ToString() const;
        };

        // This is an EBus useful to scrape classes, globals and EBuses exposed to game scripting
        // e.g: Lua.
        class SymbolsReporterRequests
        {
        public:
            AZ_RTTI(SymbolsReporterRequests, "{3ff9a105-3159-49ff-8dc6-4948ae7b4ab8}");
            virtual ~SymbolsReporterRequests() = default;
            // Put your public methods here

            virtual const AZStd::vector<ClassSymbol>& GetListOfClasses() = 0;

            virtual const AZStd::vector<PropertySymbol>& GetListOfGlobalProperties() = 0;
            virtual const AZStd::vector<MethodSymbol>& GetListOfGlobalFunctions() = 0;

            virtual const AZStd::vector<EBusSymbol>& GetListOfEBuses() = 0;

        };
        
        class SymbolsReporterBusTraits
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
        };

        using SymbolsReporterRequestBus = AZ::EBus<SymbolsReporterRequests, SymbolsReporterBusTraits>;

    } // namespace Script
} // namespace AzToolsFramework
