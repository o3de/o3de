/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <ScriptCanvas/Core/NodeableOut.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

struct lua_State;

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    struct RuntimeData;

    namespace Execution
    {
        void ActivateInterpreted();

        AZ::BehaviorValueParameter BehaviorValueParameterFromTypeIdString(const char* string, AZ::BehaviorContext& behaviorContext);

        AZ::Uuid CreateIdFromStringFast(const char* string);

        AZStd::string CreateStringFastFromId(const AZ::Uuid& uuid);

        int CallExecutionOut(lua_State* lua);

        int InterpretedSafeCall(lua_State* lua, int argCount, int returnValueCount);

        void InterpretedUnloadData(RuntimeData& runtimeData);

        void InitializeInterpretedStatics(RuntimeData& runtimeData);

        int InitializeNodeableOutKeys(lua_State* lua);

        int OverrideNodeableMetatable(lua_State* lua);

        void PushActivationArgs(lua_State* lua, AZ::BehaviorValueParameter* arguments, size_t numArguments);

        void RegisterAPI(lua_State* lua);

        // Lua: (ebus handler) userdata, (out name) string, (out implementation) function
        int SetExecutionOut(lua_State* lua);

        // Lua: (ebus handler) userdata, (out name) string, (out implementation) function
        int SetExecutionOutResult(lua_State* lua);

        // Lua: executionState, outKey, function
        int SetExecutionOutUserSubgraph(lua_State* lua);
            
        void SetInterpretedExecutionMode(BuildConfiguration configuration);

        void SetInterpretedExecutionModeDebug();

        void SetInterpretedExecutionModePerformance();

        void SetInterpretedExecutionModeRelease();

        void StackPush(lua_State* lua, AZ::BehaviorContext* context, AZ::BehaviorValueParameter& param);

        bool StackRead(lua_State* lua, AZ::BehaviorContext* context, int index, AZ::BehaviorValueParameter& param, AZ::StackVariableAllocator* allocator);

        // Lua: executionState, dependentAssets, dependentAssetsIndex
        // leaves dependentAssets[dependentAssetsIndex], and all the construction args at the top of the stack
        int UnpackDependencyConstructionArgs(lua_State* lua);

        // Lua: executionState, dependentAssets, dependentAssetsIndex
        // leaves  all the construction args at the top of the stack
        int UnpackDependencyConstructionArgsLeaf(lua_State* lua);
    } 

} 
