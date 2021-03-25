/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/lua/lua.h>

#include "Asset/RuntimeAsset.h"
#include "Execution/ExecutionState.h"

struct lua_State;

namespace ScriptCanvas
{
    class ExecutionStateInterpreted
        : public ExecutionState
    {
    public:
        AZ_RTTI(ExecutionStateInterpreted, "{824E3CF1-5403-4AF7-AC5F-B69699FFF669}", ExecutionState);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpreted, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        ExecutionStateInterpreted(const ExecutionStateConfig& config);

        void ClearLuaRegistryIndex();

        const Grammar::DebugExecution* GetDebugSymbolIn(size_t index) const;

        const Grammar::DebugExecution* GetDebugSymbolIn(size_t index, const AZ::Data::AssetId& id) const;

        const Grammar::DebugExecution* GetDebugSymbolOut(size_t index) const;

        const Grammar::DebugExecution* GetDebugSymbolOut(size_t index, const AZ::Data::AssetId& id) const;

        const Grammar::DebugExecution* GetDebugSymbolReturn(size_t index) const;

        const Grammar::DebugExecution* GetDebugSymbolReturn(size_t index, const AZ::Data::AssetId& id) const;

        const Grammar::DebugDataSource* GetDebugSymbolVariableChange(size_t index) const;

        const Grammar::DebugDataSource* GetDebugSymbolVariableChange(size_t index, const AZ::Data::AssetId& id) const;

        ExecutionMode GetExecutionMode() const override;

        int GetLuaRegistryIndex() const;

        void ReferenceExecutionState();

        void ReleaseExecutionState();

        void ReleaseExecutionStateUnchecked();

    protected:
        lua_State* m_luaState;

        lua_State* LoadLuaScript();
        
    private:
        AZ::Data::Asset<AZ::ScriptAsset> m_interpretedAsset;
        int m_luaRegistryIndex = LUA_NOREF;
    };
} 
