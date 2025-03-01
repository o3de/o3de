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
     } //namespace Script
 } // namespace AzToolsFramework
 