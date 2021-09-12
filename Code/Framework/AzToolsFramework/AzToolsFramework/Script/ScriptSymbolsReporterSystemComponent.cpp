/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/ScriptContextDebug.h>

#include "ScriptSymbolsReporterSystemComponent.h"

namespace AzToolsFramework
{
    namespace Script
    {
        AZStd::string PropertySymbol::ToString() const
        {
            return AZStd::string::format("%s [%s/%s]", m_name.c_str(), m_canRead ? "R" : "_", m_canWrite ? "W" : "_");
        }

        void PropertySymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<PropertySymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&PropertySymbol::m_name))
                    ->Property("canRead", BehaviorValueProperty(&PropertySymbol::m_canRead))
                    ->Property("canWrite", BehaviorValueProperty(&PropertySymbol::m_canWrite))
                    ->Method("ToString", &PropertySymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string MethodSymbol::ToString() const
        {
            return AZStd::string::format("%s(%s)", m_name.c_str(), m_debugArgumentInfo.c_str());
        }

        void MethodSymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<MethodSymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&MethodSymbol::m_name))
                    ->Property("debugArgumentInfo", BehaviorValueProperty(&MethodSymbol::m_debugArgumentInfo))
                    ->Method("ToString", &MethodSymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string ClassSymbol::ToString() const
        {
            return AZStd::string::format("%s [%s]", m_name.c_str(), m_typeId.ToString<AZStd::string>().c_str());
        }

        void ClassSymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<ClassSymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&ClassSymbol::m_name))
                    ->Property("typeId", BehaviorValueProperty(&ClassSymbol::m_typeId))
                    ->Property("properties", BehaviorValueProperty(&ClassSymbol::m_properties))
                    ->Property("methods", BehaviorValueProperty(&ClassSymbol::m_methods))
                    ->Method("ToString", &ClassSymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string EBusSender::ToString() const
        {
            return AZStd::string::format("%s(%s) - [%s]", m_name.c_str(), m_debugArgumentInfo.c_str(), m_category.c_str());
        }

        void EBusSender::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<EBusSender>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&EBusSender::m_name))
                    ->Property("debugArgumentInfo", BehaviorValueProperty(&EBusSender::m_debugArgumentInfo))
                    ->Property("category", BehaviorValueProperty(&EBusSender::m_category))
                    ->Method("ToString", &EBusSender::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string EBusSymbol::ToString() const
        {
            return AZStd::string::format("%s: canBroadcast(%s), canQueue(%s), hasHandler(%s)", m_name.c_str(),
                m_canBroadcast ? "true" : "false", m_canQueue ? "true" : "false", m_hasHandler ? "true" : "false");
        }

        void EBusSymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<EBusSymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&EBusSymbol::m_name))
                    ->Property("canBroadcast", BehaviorValueProperty(&EBusSymbol::m_canBroadcast))
                    ->Property("canQueue", BehaviorValueProperty(&EBusSymbol::m_canQueue))
                    ->Property("hasHandler", BehaviorValueProperty(&EBusSymbol::m_hasHandler))
                    ->Property("senders", BehaviorValueProperty(&EBusSymbol::m_senders))
                    ->Method("ToString", &EBusSymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        void SymbolsReporterSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            PropertySymbol::Reflect(context);
            MethodSymbol::Reflect(context);
            ClassSymbol::Reflect(context);
            EBusSender::Reflect(context);
            EBusSymbol::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SymbolsReporterSystemComponent, AZ::Component>()
                    ->Version(0);

                serializeContext->RegisterGenericType<AZStd::vector<PropertySymbol>>();
                serializeContext->RegisterGenericType<AZStd::vector<MethodSymbol>>();
                serializeContext->RegisterGenericType<AZStd::vector<ClassSymbol>>();
                serializeContext->RegisterGenericType<AZStd::vector<EBusSender>>();
                serializeContext->RegisterGenericType<AZStd::vector<EBusSymbol>>();
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<SymbolsReporterRequestBus>("SymbolsReporterBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Event("GetListOfClasses", &SymbolsReporterRequests::GetListOfClasses)
                    ->Event("GetListOfGlobalProperties", &SymbolsReporterRequests::GetListOfGlobalProperties)
                    ->Event("GetListOfGlobalFunctions", &SymbolsReporterRequests::GetListOfGlobalFunctions)
                    ->Event("GetListOfEBuses", &SymbolsReporterRequests::GetListOfEBuses)
                    ;
            }
        }

        SymbolsReporterSystemComponent::SymbolsReporterSystemComponent() = default;

        SymbolsReporterSystemComponent::~SymbolsReporterSystemComponent() = default;

        void SymbolsReporterSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SymbolsReporterSystemService"));
        }

        void SymbolsReporterSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SymbolsReporterSystemService"));
        }

        void SymbolsReporterSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {

        }

        void SymbolsReporterSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {

        }

        void SymbolsReporterSystemComponent::Activate()
        {
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
            SymbolsReporterRequestBus::Handler::BusConnect();
        }

        void SymbolsReporterSystemComponent::Deactivate()
        {
            SymbolsReporterRequestBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        }

        AZ::ScriptContext* SymbolsReporterSystemComponent::InitScriptContext()
        {
            if (m_scriptContext)
            {
                return m_scriptContext;
            }

            AZ::ScriptSystemRequestBus::BroadcastResult(m_scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            return m_scriptContext;
        }

        void SymbolsReporterSystemComponent::LoadGlobalSymbols()
        {
            auto scriptContext = InitScriptContext();
            if (!scriptContext)
            {
                return;
            }

            scriptContext->EnableDebug();

            auto debugContext = scriptContext->GetDebugContext();
            if (!debugContext)
            {
                return;
            }

            auto enumMethodFunc = +[]([[maybe_unused]] const AZ::Uuid* classTypeId, [[maybe_unused]] const char* methodName, [[maybe_unused]] const char* debugArgumentInfo, [[maybe_unused]] void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);
                auto& methodSymbols = mySelf.GetGlobalFunctionSymbols();
                methodSymbols.push_back({});
                auto& methodSymbol = methodSymbols.back();
                methodSymbol.m_name = methodName;
                if (debugArgumentInfo)
                {
                    methodSymbol.m_debugArgumentInfo = debugArgumentInfo;
                }
                return true;
            };

            auto enumPropertyFunc = +[]([[maybe_unused]] const AZ::Uuid* classTypeId, [[maybe_unused]] const char* propertyName, [[maybe_unused]] bool canRead, [[maybe_unused]] bool canWrite, [[maybe_unused]] void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);
                auto& propertySymbols = mySelf.GetGlobalPropertySymbols();
                propertySymbols.push_back({});
                auto& propertySymbol = propertySymbols.back();
                propertySymbol.m_name = propertyName;
                propertySymbol.m_canRead = canRead;
                propertySymbol.m_canWrite = canWrite;

                return true;
            };

            debugContext->EnumRegisteredGlobals(enumMethodFunc, enumPropertyFunc, this);

            scriptContext->DisableDebug();
        }

        ///////////////////////////////////////////////////////////////////////////
        /// SymbolsReporterRequestBus::Handler
        const AZStd::vector<ClassSymbol>& SymbolsReporterSystemComponent::GetListOfClasses()
        {
            if (!m_classSymbols.empty())
            {
                return m_classSymbols;
            }

            auto scriptContext = InitScriptContext();
            if (!scriptContext)
            {
                return m_classSymbols;
            }

            scriptContext->EnableDebug();

            auto debugContext = scriptContext->GetDebugContext();
            if (!debugContext)
            {
                return m_classSymbols;
            }

            auto enumClassFunc = +[](const char* className, [[maybe_unused]]const AZ::Uuid& classTypeId, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);

                auto& classSymbols =  mySelf.GetClassSymbols();
                classSymbols.push_back({});
                auto& classSymbol = classSymbols.back();
                classSymbol.m_name = className;
                classSymbol.m_typeId = classTypeId;

                auto& uuidToClassMap = mySelf.GetClassUuidToIndexMap();
                uuidToClassMap.emplace(classTypeId, classSymbols.size() - 1);

                return true;
            };

            auto enumMethodFunc = +[]([[maybe_unused]] const AZ::Uuid* classTypeId, [[maybe_unused]] const char* methodName, [[maybe_unused]] const char* debugArgumentInfo, [[maybe_unused]] void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);
                auto& classUuidToIndexMap = mySelf.GetClassUuidToIndexMap();
                auto itor = classUuidToIndexMap.find(*classTypeId);
                if (itor == classUuidToIndexMap.end())
                {
                    AZ_Error(LogName, false, "Can not add method [%s] because class uuid [%s] is not registered", methodName, classTypeId->ToString<AZStd::string>().c_str());
                    return false;
                }

                auto classIndex = itor->second;
                auto& classSymbols = mySelf.GetClassSymbols();
                auto& classSymbol = classSymbols[classIndex];
                classSymbol.m_methods.push_back({});
                auto& methodSymbol = classSymbol.m_methods.back();
                methodSymbol.m_name = methodName;
                if (debugArgumentInfo)
                {
                    methodSymbol.m_debugArgumentInfo = debugArgumentInfo;
                }
                return true;
            };

            auto enumPropertyFunc = +[]([[maybe_unused]] const AZ::Uuid* classTypeId, [[maybe_unused]] const char* propertyName, [[maybe_unused]] bool canRead, [[maybe_unused]] bool canWrite, [[maybe_unused]] void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);
                auto& classUuidToIndexMap = mySelf.GetClassUuidToIndexMap();
                auto itor = classUuidToIndexMap.find(*classTypeId);
                if (itor == classUuidToIndexMap.end())
                {
                    AZ_Error(LogName, false, "Can not add property [%s] because class uuid [%s] is not registered", propertyName, classTypeId->ToString<AZStd::string>().c_str());
                    return false;
                }

                auto classIndex = itor->second;
                auto& classSymbols = mySelf.GetClassSymbols();
                auto& classSymbol = classSymbols[classIndex];
                classSymbol.m_properties.push_back({});
                auto& propertySymbol = classSymbol.m_properties.back();
                propertySymbol.m_name = propertyName;
                propertySymbol.m_canRead = canRead;
                propertySymbol.m_canWrite = canWrite;

                return true;
            };

            debugContext->EnumRegisteredClasses(enumClassFunc, enumMethodFunc, enumPropertyFunc, this);

            scriptContext->DisableDebug();

            return m_classSymbols;
        }

        const AZStd::vector<PropertySymbol>& SymbolsReporterSystemComponent::GetListOfGlobalProperties()
        {
            if (!m_globalPropertySymbols.empty())
            {
                return m_globalPropertySymbols;
            }

            LoadGlobalSymbols();

            return m_globalPropertySymbols;
        }

        const AZStd::vector<MethodSymbol>& SymbolsReporterSystemComponent::GetListOfGlobalFunctions()
        {
            if (!m_globalFunctionSymbols.empty())
            {
                return m_globalFunctionSymbols;
            }

            LoadGlobalSymbols();

            return m_globalFunctionSymbols;
        }

        const AZStd::vector<EBusSymbol>& SymbolsReporterSystemComponent::GetListOfEBuses()
        {
            if (!m_ebusSymbols.empty())
            {
                return m_ebusSymbols;
            }

            auto scriptContext = InitScriptContext();
            if (!scriptContext)
            {
                return m_ebusSymbols;
            }

            scriptContext->EnableDebug();

            auto debugContext = scriptContext->GetDebugContext();
            if (!debugContext)
            {
                return m_ebusSymbols;
            }

            auto enumEBusFunc = +[](const AZStd::string& ebusName, bool canBroadcast, bool canQueue, bool hasHandler, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);

                auto& ebusSymbols = mySelf.GetEBusSymbols();
                ebusSymbols.push_back({});
                auto& ebusSymbol = ebusSymbols.back();
                ebusSymbol.m_name = ebusName;
                ebusSymbol.m_canBroadcast = canBroadcast;
                ebusSymbol.m_canQueue = canQueue;
                ebusSymbol.m_hasHandler = hasHandler;

                auto& nameToIndexMap = mySelf.GetEBusNameToIndexMap();
                nameToIndexMap.emplace(ebusName, ebusSymbols.size() - 1);

                return true;
            };

            auto enumEBusSenderFunc = +[](const AZStd::string& ebusName, const AZStd::string& senderName, const AZStd::string& debugArgumentInfo, const AZStd::string& category, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<SymbolsReporterSystemComponent*>(userData);
                auto& nameToIndexMap = mySelf.GetEBusNameToIndexMap();
                auto itor = nameToIndexMap.find(ebusName);
                if (itor == nameToIndexMap.end())
                {
                    AZ_Error(LogName, false, "Can not add ebus sender [%s] because ebus [%s] is not registered", senderName.c_str(), ebusName.c_str());
                    return false;
                }

                auto ebusIndex = itor->second;
                auto& ebusSymbols = mySelf.GetEBusSymbols();
                auto& ebusSymbol = ebusSymbols[ebusIndex];

                ebusSymbol.m_senders.push_back({});
                auto& ebusSender = ebusSymbol.m_senders.back();
                ebusSender.m_name = senderName;
                ebusSender.m_debugArgumentInfo = debugArgumentInfo;
                ebusSender.m_category = category;
                return true;
            };

            debugContext->EnumRegisteredEBuses(enumEBusFunc, enumEBusSenderFunc, this);

            scriptContext->DisableDebug();

            return m_ebusSymbols;
        }
        ///////////////////////////////////////////////////////////////////////////

    } //namespace Script
} // namespace AzToolsFramework
