/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/Memory/SystemAllocator.h>

struct lua_State;

namespace AZ
{
    struct BehaviorArgument;
}

namespace ScriptCanvas
{
    namespace Execution
    {
        struct OutInterpreted
        {
            AZ_TYPE_INFO(OutInterpreted, "{171EC052-7A51-42FB-941C-CFF0F78F9373}");
            AZ_CLASS_ALLOCATOR(OutInterpreted, AZ::SystemAllocator);

            int m_lambdaRegistryIndex;
            lua_State* m_lua;

            // assumes a lambda is at the top of the stack and will pop it
            OutInterpreted(lua_State* lua);

            OutInterpreted(OutInterpreted&& source) noexcept;

            ~OutInterpreted();

            OutInterpreted& operator=(OutInterpreted&& source) noexcept;

            void operator()(AZ::BehaviorArgument* resultBVP, AZ::BehaviorArgument* argsBVPs, int numArguments);

            // \note Do not use, these are here for compiler compatibility only
            OutInterpreted() = default;
            OutInterpreted(const OutInterpreted& source);
        };

        struct OutInterpretedResult
        {
            AZ_TYPE_INFO(OutInterpretedResult, "{F0FB088C-2FA2-473A-8548-CA7D0B372ABE}");
            AZ_CLASS_ALLOCATOR(OutInterpretedResult, AZ::SystemAllocator);

            int m_lambdaRegistryIndex;
            lua_State* m_lua;
            
            // assumes a lambda is at the top of the stack and will pop it
            OutInterpretedResult(lua_State* lua);

            OutInterpretedResult(OutInterpretedResult&& source) noexcept;

            ~OutInterpretedResult();

            OutInterpretedResult& operator=(OutInterpretedResult&& source) noexcept;

            void operator()(AZ::BehaviorArgument* resultBVP, AZ::BehaviorArgument* argsBVPs, int numArguments);

            // \note Do not use, these are here for compiler compatibility only
            OutInterpretedResult() = default;
            OutInterpretedResult(const OutInterpretedResult & source);
        };
    } 

} 
