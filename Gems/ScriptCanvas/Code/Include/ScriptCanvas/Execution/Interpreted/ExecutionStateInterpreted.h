/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpreted, AZ::SystemAllocator);

        ExecutionStateInterpreted(ExecutionStateConfig& config);

        ExecutionMode GetExecutionMode() const override;

    protected:
        lua_State* m_luaState;

        lua_State* LoadLuaScript();
        
    private:
        const AZ::Data::Asset<AZ::ScriptAsset>& m_interpretedAsset;
    };
} 
