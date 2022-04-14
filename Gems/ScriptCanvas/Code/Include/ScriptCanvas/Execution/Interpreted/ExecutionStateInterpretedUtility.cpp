/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedUtility.h>

namespace ExecutionStateInterpretedUtilityCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    void InitializeFromLuaStackFunctions(AZ::BehaviorContext* behaviorContext, AZStd::vector<DebugDataSource>& symbols)
    {
        for (auto& debugDataSource : symbols)
        {
            if (debugDataSource.m_sourceType != DebugDataSourceType::Internal
                || debugDataSource.m_slotDatumType.IsValid())
            {
                AZ::BehaviorClass* behaviorClassUnused{};
                AZ::BehaviorParameter param;
                param.m_typeId = debugDataSource.m_slotDatumType.GetAZType();
                debugDataSource.m_fromStack = FromLuaStack(behaviorContext, &param, behaviorClassUnused);
                SC_RUNTIME_CHECK(debugDataSource.m_fromStack, "LuaLoadFromStack function not found")
            }            
        }
    }

    void InitializeFromLuaStackFunctions(AZ::BehaviorContext* behaviorContext, AZStd::vector<Grammar::DebugExecution>& symbols)
    {
        for (auto& debugExecution : symbols)
        {
            InitializeFromLuaStackFunctions(behaviorContext, debugExecution.m_data);
        }
    }
}

namespace ScriptCanvas
{
    namespace Execution
    {
        void InitializeFromLuaStackFunctions(Grammar::DebugSymbolMap& debugMap)
        {
            AZ::ScriptContext* scriptContext{};
            AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            SC_RUNTIME_CHECK_RETURN(scriptContext, "Must have a default script context");
            auto behaviorContext = scriptContext->GetBoundContext();

            ExecutionStateInterpretedUtilityCpp::InitializeFromLuaStackFunctions(behaviorContext, debugMap.m_ins);
            ExecutionStateInterpretedUtilityCpp::InitializeFromLuaStackFunctions(behaviorContext, debugMap.m_outs);
            ExecutionStateInterpretedUtilityCpp::InitializeFromLuaStackFunctions(behaviorContext, debugMap.m_returns);
            ExecutionStateInterpretedUtilityCpp::InitializeFromLuaStackFunctions(behaviorContext, debugMap.m_variables);
        }

        bool IsLuaValueType(Data::eType etype)
        {
            switch (etype)
            {
            case ScriptCanvas::Data::eType::Boolean:
            case ScriptCanvas::Data::eType::EntityID:
            case ScriptCanvas::Data::eType::NamedEntityID:
            case ScriptCanvas::Data::eType::Number:
            case ScriptCanvas::Data::eType::String:
                return true;
            
            case ScriptCanvas::Data::eType::AABB:
            case ScriptCanvas::Data::eType::BehaviorContextObject:
            case ScriptCanvas::Data::eType::CRC:
            case ScriptCanvas::Data::eType::Color:
            case ScriptCanvas::Data::eType::Matrix3x3:
            case ScriptCanvas::Data::eType::Matrix4x4:
            case ScriptCanvas::Data::eType::OBB:
            case ScriptCanvas::Data::eType::Plane:
            case ScriptCanvas::Data::eType::Quaternion:
            case ScriptCanvas::Data::eType::Transform:
            case ScriptCanvas::Data::eType::Vector2:
            case ScriptCanvas::Data::eType::Vector3:
            case ScriptCanvas::Data::eType::Vector4:
                return false;

            case ScriptCanvas::Data::eType::Invalid:
            default:
                SC_RUNTIME_CHECK(false, "Invalid type in ScriptCanvas runtime");
                return false;
            }
        }

        AZ::Outcome<void, AZStd::string> PushValue(lua_State* lua, const Datum& datum)
        {
            auto etype = datum.GetType().GetType();
            switch (etype)
            {
            case ScriptCanvas::Data::eType::Boolean:
                AZ::ScriptValue<Data::BooleanType>::StackPush(lua, *datum.GetAs<Data::BooleanType>()); 
                break;

            case ScriptCanvas::Data::eType::EntityID:
                AZ::ScriptValue<Data::EntityIDType>::StackPush(lua, *datum.GetAs<Data::EntityIDType>()); 
                break;

            case ScriptCanvas::Data::eType::NamedEntityID:
                AZ::ScriptValue<Data::EntityIDType>::StackPush(lua, *datum.GetAs<Data::NamedEntityIDType>());
                break;

            case ScriptCanvas::Data::eType::Number:
                AZ::ScriptValue<Data::NumberType>::StackPush(lua, *datum.GetAs<Data::NumberType>());
                break;

            case ScriptCanvas::Data::eType::String:
                {
                    auto stringPtr = datum.GetAs<Data::StringType>();
                    if (!stringPtr || stringPtr->empty())
                    {
                        AZ::ScriptValue<const char*>::StackPush(lua, "");
                    }
                    else
                    {
                        AZ::ScriptValue<const char*>::StackPush(lua, stringPtr->data());
                    }
                }
                break;

            case ScriptCanvas::Data::eType::AABB:
            case ScriptCanvas::Data::eType::BehaviorContextObject:
            case ScriptCanvas::Data::eType::CRC:
            case ScriptCanvas::Data::eType::Color:
            case ScriptCanvas::Data::eType::Matrix3x3:
            case ScriptCanvas::Data::eType::Matrix4x4:
            case ScriptCanvas::Data::eType::OBB:
            case ScriptCanvas::Data::eType::Plane:
            case ScriptCanvas::Data::eType::Quaternion:
            case ScriptCanvas::Data::eType::Transform:
            case ScriptCanvas::Data::eType::Vector2:
            case ScriptCanvas::Data::eType::Vector3:
            case ScriptCanvas::Data::eType::Vector4:
                AZ::Internal::LuaScriptValueStackPush
                    ( lua
                    , const_cast<void*>(datum.GetAsDanger())
                    , (datum.GetType().GetAZType())
                    , AZ::ObjectToLua::ByValue);
                break;

            case ScriptCanvas::Data::eType::Invalid:
            default:
                SC_RUNTIME_CHECK(false, "Invalid type in ScriptCanvas");
                return AZ::Failure(AZStd::string("Invalid type in ScriptCanvas"));
            }

            return AZ::Success();
        }
    } 

} 
