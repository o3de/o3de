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

#include "LuaSymbolsReporterSystemComponent.h"

namespace AzToolsFramework
{
    namespace Script
    {
        AZStd::string LuaPropertySymbol::ToString() const
        {
            return AZStd::string::format("%s [%s/%s]",
                                         m_name.c_str(),
                                         m_canRead ? "R" : "_",
                                         m_canWrite ? "W" : "_");
        }

        void LuaPropertySymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<LuaPropertySymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&LuaPropertySymbol::m_name))
                    ->Property("canRead", BehaviorValueProperty(&LuaPropertySymbol::m_canRead))
                    ->Property("canWrite", BehaviorValueProperty(&LuaPropertySymbol::m_canWrite))
                    ->Method("ToString", &LuaPropertySymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string LuaMethodSymbol::ToString() const
        {
            return AZStd::string::format("%s(%s)", m_name.c_str(), m_debugArgumentInfo.c_str());
        }

        void LuaMethodSymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<LuaMethodSymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&LuaMethodSymbol::m_name))
                    ->Property("debugArgumentInfo", BehaviorValueProperty(&LuaMethodSymbol::m_debugArgumentInfo))
                    ->Method("ToString", &LuaMethodSymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string LuaClassSymbol::ToString() const
        {
            return AZStd::string::format("%s [%s]", m_name.c_str(), m_typeId.ToString<AZStd::string>().c_str());
        }

        void LuaClassSymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<LuaClassSymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&LuaClassSymbol::m_name))
                    ->Property("typeId", BehaviorValueProperty(&LuaClassSymbol::m_typeId))
                    ->Property("properties", BehaviorValueProperty(&LuaClassSymbol::m_properties))
                    ->Property("methods", BehaviorValueProperty(&LuaClassSymbol::m_methods))
                    ->Method("ToString", &LuaClassSymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string LuaEBusSender::ToString() const
        {
            return AZStd::string::format("%s(%s) - [%s]", m_name.c_str(), m_debugArgumentInfo.c_str(), m_category.c_str());
        }

        void LuaEBusSender::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<LuaEBusSender>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&LuaEBusSender::m_name))
                    ->Property("debugArgumentInfo", BehaviorValueProperty(&LuaEBusSender::m_debugArgumentInfo))
                    ->Property("category", BehaviorValueProperty(&LuaEBusSender::m_category))
                    ->Method("ToString", &LuaEBusSender::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        AZStd::string LuaEBusSymbol::ToString() const
        {
            auto boolToStr = +[](bool val) { return val ? "true" : "false"; };
            return AZStd::string::format("%s: canBroadcast(%s), canQueue(%s), hasHandler(%s)",
                                         m_name.c_str(),
                                         boolToStr(m_canBroadcast), boolToStr(m_canQueue), boolToStr(m_hasHandler));
        }

        void LuaEBusSymbol::Reflect(AZ::ReflectContext* context)
        {
            auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<LuaEBusSymbol>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("name", BehaviorValueProperty(&LuaEBusSymbol::m_name))
                    ->Property("canBroadcast", BehaviorValueProperty(&LuaEBusSymbol::m_canBroadcast))
                    ->Property("canQueue", BehaviorValueProperty(&LuaEBusSymbol::m_canQueue))
                    ->Property("hasHandler", BehaviorValueProperty(&LuaEBusSymbol::m_hasHandler))
                    ->Property("senders", BehaviorValueProperty(&LuaEBusSymbol::m_senders))
                    ->Method("ToString", &LuaEBusSymbol::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ;
            }
        }

        //! This local class helps us keeping private the sensitive data in LuaSymbolsReporterSystemComponent
        //! Used inside the function pointers for several AZ::SciptContextDebug::Enumerate* functions.
        class IntrusiveHelper
        {
        public:
            static AZStd::vector<LuaClassSymbol>& GetClassSymbols(LuaSymbolsReporterSystemComponent& symbolsReporter) { return symbolsReporter.m_cachedClassSymbols; }
            static AZStd::unordered_map<AZ::Uuid, size_t>& GetClassUuidToIndexMap(LuaSymbolsReporterSystemComponent& symbolsReporter) { return symbolsReporter.m_classUuidToIndexMap; }
            static AZStd::vector<LuaPropertySymbol>& GetGlobalPropertySymbols(LuaSymbolsReporterSystemComponent& symbolsReporter) { return symbolsReporter.m_cachedGlobalPropertySymbols; }
            static AZStd::vector<LuaMethodSymbol>& GetGlobalFunctionSymbols(LuaSymbolsReporterSystemComponent& symbolsReporter) { return symbolsReporter.m_cachedGlobalFunctionSymbols; }
            static AZStd::vector<LuaEBusSymbol>& GetEBusSymbols(LuaSymbolsReporterSystemComponent& symbolsReporter) { return symbolsReporter.m_cachedEbusSymbols; }
            static AZStd::unordered_map<AZStd::string, size_t>& GetEBusNameToIndexMap(LuaSymbolsReporterSystemComponent& symbolsReporter) { return symbolsReporter.m_ebusNameToIndexMap; }
        };

        void LuaSymbolsReporterSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            LuaPropertySymbol::Reflect(context);
            LuaMethodSymbol::Reflect(context);
            LuaClassSymbol::Reflect(context);
            LuaEBusSender::Reflect(context);
            LuaEBusSymbol::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LuaSymbolsReporterSystemComponent, AZ::Component>()
                    ->Version(0);

                serializeContext->RegisterGenericType<AZStd::vector<LuaPropertySymbol>>();
                serializeContext->RegisterGenericType<AZStd::vector<LuaMethodSymbol>>();
                serializeContext->RegisterGenericType<AZStd::vector<LuaClassSymbol>>();
                serializeContext->RegisterGenericType<AZStd::vector<LuaEBusSender>>();
                serializeContext->RegisterGenericType<AZStd::vector<LuaEBusSymbol>>();
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<LuaSymbolsReporterRequestBus>("LuaSymbolsReporterBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "script")
                    ->Event("GetListOfClasses", &LuaSymbolsReporterRequests::GetListOfClasses)
                    ->Event("GetListOfGlobalProperties", &LuaSymbolsReporterRequests::GetListOfGlobalProperties)
                    ->Event("GetListOfGlobalFunctions", &LuaSymbolsReporterRequests::GetListOfGlobalFunctions)
                    ->Event("GetListOfEBuses", &LuaSymbolsReporterRequests::GetListOfEBuses)
                    ;
            }
        }

        void LuaSymbolsReporterSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("LuaSymbolsReporterSystemService"));
        }

        void LuaSymbolsReporterSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("LuaSymbolsReporterSystemService"));
        }

        void LuaSymbolsReporterSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("ScriptService"));
        }

        void LuaSymbolsReporterSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            // No dependent services.
        }

        void LuaSymbolsReporterSystemComponent::Activate()
        {
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
            LuaSymbolsReporterRequestBus::Handler::BusConnect();
        }

        void LuaSymbolsReporterSystemComponent::Deactivate()
        {
            LuaSymbolsReporterRequestBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        }

        AZ::ScriptContext* LuaSymbolsReporterSystemComponent::InitScriptContext()
        {
            if (m_scriptContext)
            {
                return m_scriptContext;
            }

            AZ::ScriptSystemRequestBus::BroadcastResult(m_scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            return m_scriptContext;
        }

        void LuaSymbolsReporterSystemComponent::LoadGlobalSymbols()
        {
            auto scriptContext = InitScriptContext();
            if (!scriptContext)
            {
                AZ_Error(LogName, false, "Invalid scriptContext");
                return;
            }

            scriptContext->EnableDebug();

            auto debugContext = scriptContext->GetDebugContext();
            if (!debugContext)
            {
                AZ_Error(LogName, false, "Invalid debugContext from scriptContext");
                return;
            }

            auto enumMethodFunc = +[]([[maybe_unused]] const AZ::Uuid* classTypeId, const char* methodName, const char* debugArgumentInfo, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);
                auto& methodSymbols = IntrusiveHelper::GetGlobalFunctionSymbols(mySelf);
                methodSymbols.push_back({});
                auto& methodSymbol = methodSymbols.back();
                methodSymbol.m_name = methodName;
                if (debugArgumentInfo)
                {
                    methodSymbol.m_debugArgumentInfo = debugArgumentInfo;
                }
                return true;
            };

            auto enumPropertyFunc = +[]([[maybe_unused]] const AZ::Uuid* classTypeId, const char* propertyName, bool canRead, bool canWrite, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);
                auto& propertySymbols = IntrusiveHelper::GetGlobalPropertySymbols(mySelf);
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
        /// LuaSymbolsReporterRequestBus::Handler
        const AZStd::vector<LuaClassSymbol>& LuaSymbolsReporterSystemComponent::GetListOfClasses()
        {
            if (!m_cachedClassSymbols.empty())
            {
                return m_cachedClassSymbols;
            }

            auto scriptContext = InitScriptContext();
            if (!scriptContext)
            {
                return m_cachedClassSymbols;
            }

            scriptContext->EnableDebug();

            auto debugContext = scriptContext->GetDebugContext();
            if (!debugContext)
            {
                return m_cachedClassSymbols;
            }

            auto enumClassFunc = +[](const char* className, const AZ::Uuid& classTypeId, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);

                auto& classSymbols =  IntrusiveHelper::GetClassSymbols(mySelf);
                classSymbols.push_back({});
                auto& classSymbol = classSymbols.back();
                classSymbol.m_name = className;
                classSymbol.m_typeId = classTypeId;

                auto& uuidToClassMap = IntrusiveHelper::GetClassUuidToIndexMap(mySelf);
                uuidToClassMap.emplace(classTypeId, classSymbols.size() - 1);

                return true;
            };

            auto enumMethodFunc = +[](const AZ::Uuid* classTypeId, const char* methodName, const char* debugArgumentInfo, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);
                auto& classUuidToIndexMap = IntrusiveHelper::GetClassUuidToIndexMap(mySelf);
                auto itor = classUuidToIndexMap.find(*classTypeId);
                if (itor == classUuidToIndexMap.end())
                {
                    AZ_Error(LogName, false, "Can not add method [%s] because class uuid [%s] is not registered", methodName, classTypeId->ToString<AZStd::string>().c_str());
                    return false;
                }

                auto classIndex = itor->second;
                auto& classSymbols = IntrusiveHelper::GetClassSymbols(mySelf);
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

            auto enumPropertyFunc = +[](const AZ::Uuid* classTypeId, const char* propertyName, bool canRead, bool canWrite, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);
                auto& classUuidToIndexMap = IntrusiveHelper::GetClassUuidToIndexMap(mySelf);
                auto itor = classUuidToIndexMap.find(*classTypeId);
                if (itor == classUuidToIndexMap.end())
                {
                    AZ_Error(LogName, false, "Can not add property [%s] because class uuid [%s] is not registered", propertyName, classTypeId->ToString<AZStd::string>().c_str());
                    return false;
                }

                auto classIndex = itor->second;
                auto& classSymbols = IntrusiveHelper::GetClassSymbols(mySelf);
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

            return m_cachedClassSymbols;
        }

        const AZStd::vector<LuaPropertySymbol>& LuaSymbolsReporterSystemComponent::GetListOfGlobalProperties()
        {
            if (!m_cachedGlobalPropertySymbols.empty())
            {
                return m_cachedGlobalPropertySymbols;
            }

            LoadGlobalSymbols();

            return m_cachedGlobalPropertySymbols;
        }

        const AZStd::vector<LuaMethodSymbol>& LuaSymbolsReporterSystemComponent::GetListOfGlobalFunctions()
        {
            if (!m_cachedGlobalFunctionSymbols.empty())
            {
                return m_cachedGlobalFunctionSymbols;
            }

            LoadGlobalSymbols();

            return m_cachedGlobalFunctionSymbols;
        }

        const AZStd::vector<LuaEBusSymbol>& LuaSymbolsReporterSystemComponent::GetListOfEBuses()
        {
            if (!m_cachedEbusSymbols.empty())
            {
                return m_cachedEbusSymbols;
            }

            auto scriptContext = InitScriptContext();
            if (!scriptContext)
            {
                return m_cachedEbusSymbols;
            }

            scriptContext->EnableDebug();

            auto debugContext = scriptContext->GetDebugContext();
            if (!debugContext)
            {
                return m_cachedEbusSymbols;
            }

            auto enumEBusFunc = +[](const AZStd::string& ebusName, bool canBroadcast, bool canQueue, bool hasHandler, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);

                auto& ebusSymbols = IntrusiveHelper::GetEBusSymbols(mySelf);
                ebusSymbols.push_back({});
                auto& ebusSymbol = ebusSymbols.back();
                ebusSymbol.m_name = ebusName;
                ebusSymbol.m_canBroadcast = canBroadcast;
                ebusSymbol.m_canQueue = canQueue;
                ebusSymbol.m_hasHandler = hasHandler;

                auto& nameToIndexMap = IntrusiveHelper::GetEBusNameToIndexMap(mySelf);
                nameToIndexMap.emplace(ebusName, ebusSymbols.size() - 1);

                return true;
            };

            auto enumEBusSenderFunc = +[](const AZStd::string& ebusName, const AZStd::string& senderName, const AZStd::string& debugArgumentInfo, const AZStd::string& category, void* userData) -> bool
            {
                auto& mySelf = *reinterpret_cast<LuaSymbolsReporterSystemComponent*>(userData);
                auto& nameToIndexMap = IntrusiveHelper::GetEBusNameToIndexMap(mySelf);
                auto itor = nameToIndexMap.find(ebusName);
                if (itor == nameToIndexMap.end())
                {
                    AZ_Error(LogName, false, "Can not add ebus sender [%s] because ebus [%s] is not registered", senderName.c_str(), ebusName.c_str());
                    return false;
                }

                auto ebusIndex = itor->second;
                auto& ebusSymbols = IntrusiveHelper::GetEBusSymbols(mySelf);
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

            return m_cachedEbusSymbols;
        }
        ///////////////////////////////////////////////////////////////////////////

    } //namespace Script
} // namespace AzToolsFramework
