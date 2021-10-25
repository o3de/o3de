/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Script/ScriptContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <AzToolsFramework/Script/LuaSymbolsReporterBus.h>

namespace AzToolsFramework
{
    namespace Script
    {
        /// System component for LuaSymbolsReporterRequestBus
        class LuaSymbolsReporterSystemComponent
            : public AZ::Component
            , public LuaSymbolsReporterRequestBus::Handler
            , private AzToolsFramework::EditorEvents::Bus::Handler
        {
        public:
            AZ_COMPONENT(LuaSymbolsReporterSystemComponent, "{DB8D95BA-FECF-4D81-A45C-8C05E706E2AC}");
            static void Reflect(AZ::ReflectContext* context);

            static constexpr char LogName[] = "LuaSymbolsReporter";

            LuaSymbolsReporterSystemComponent() = default;
            ~LuaSymbolsReporterSystemComponent() = default;

            ///////////////////////////////////////////////////////////////////////////
            /// LuaSymbolsReporterRequestBus::Handler
            const AZStd::vector<LuaClassSymbol>& GetListOfClasses() override;
            const AZStd::vector<LuaPropertySymbol>& GetListOfGlobalProperties() override;
            const AZStd::vector<LuaMethodSymbol>& GetListOfGlobalFunctions() override;
            const AZStd::vector<LuaEBusSymbol>& GetListOfEBuses() override;
            ///////////////////////////////////////////////////////////////////////////

        private:
            friend class IntrusiveHelper;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            // AZ::Component
            void Activate() override;
            void Deactivate() override;

            AZ::ScriptContext* InitScriptContext();
            void LoadGlobalSymbols();

            AZ::ScriptContext* m_scriptContext = nullptr;

            AZStd::vector<LuaClassSymbol> m_cachedClassSymbols;
            // The key is a class uuid, the value is the index in @m_cachedClassSymbols
            AZStd::unordered_map<AZ::Uuid, size_t> m_classUuidToIndexMap;

            AZStd::vector<LuaPropertySymbol> m_cachedGlobalPropertySymbols;
            AZStd::vector<LuaMethodSymbol> m_cachedGlobalFunctionSymbols;

            AZStd::vector<LuaEBusSymbol> m_cachedEbusSymbols;

            // The key is the ebus name, the value is the index in @m_cachedEbusSymbols
            AZStd::unordered_map<AZStd::string, size_t> m_ebusNameToIndexMap;
            
        };
    } // namespace Script
} // namespace AzToolsFramework
