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

#include <AzToolsFramework/Script/ScriptSymbolsReporterBus.h>

namespace AzToolsFramework
{
    namespace Script
    {
        /// System component for LuaSymbolsReporter editor
        class SymbolsReporterSystemComponent
            : public AZ::Component
            , public SymbolsReporterRequestBus::Handler
            , private AzToolsFramework::EditorEvents::Bus::Handler
        {
        public:
            AZ_COMPONENT(SymbolsReporterSystemComponent, "{db8d95ba-fecf-4d81-a45c-8c05e706e2ac}");
            static void Reflect(AZ::ReflectContext* context);

            static constexpr char LogName[] = "ScriptSymbolsReporter";

            SymbolsReporterSystemComponent();
            ~SymbolsReporterSystemComponent();

            ///////////////////////////////////////////////////////////////////////////
            /// LuaSymbolsReporterRequestBus::Handler
            const AZStd::vector<ClassSymbol>& GetListOfClasses() override;
            const AZStd::vector<PropertySymbol>& GetListOfGlobalProperties() override;
            const AZStd::vector<MethodSymbol>& GetListOfGlobalFunctions() override;
            const AZStd::vector<EBusSymbol>& GetListOfEBuses() override;
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

            AZStd::vector<ClassSymbol> m_classSymbols;
            // The key is a class uuid, the value is the index in @m_classSymbols
            AZStd::unordered_map<AZ::Uuid, size_t> m_classUuidToIndexMap;

            AZStd::vector<PropertySymbol> m_globalPropertySymbols;
            AZStd::vector<MethodSymbol> m_globalFunctionSymbols;

            AZStd::vector<EBusSymbol> m_ebusSymbols;
            // The key is the ebus name, the value is the index in @m_ebusSymbols
            AZStd::unordered_map<AZStd::string, size_t> m_ebusNameToIndexMap;
            
        };
    } // namespace Script
} // namespace AzToolsFramework
