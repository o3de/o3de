/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    struct BehaviorValueParameter;
}

namespace ScriptCanvas
{
    namespace Execution
    {
        struct OutInterpreted
        {
            AZ_TYPE_INFO(OutInterpreted, "{171EC052-7A51-42FB-941C-CFF0F78F9373}");
            AZ_CLASS_ALLOCATOR(OutInterpreted, AZ::SystemAllocator, 0);

            int m_lambdaRegistryIndex;
            lua_State* m_lua;

            // assumes a lambda is at the top of the stack and will pop it
            OutInterpreted(lua_State* lua);

            OutInterpreted(OutInterpreted&& source) noexcept;

            ~OutInterpreted();

            OutInterpreted& operator=(OutInterpreted&& source) noexcept;

            void operator()(AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments);

            // \note Do not use, these are here for compiler compatibility only
            OutInterpreted() = default;
            OutInterpreted(const OutInterpreted& source);
        };

        struct OutInterpretedResult
        {
            AZ_TYPE_INFO(OutInterpretedResult, "{F0FB088C-2FA2-473A-8548-CA7D0B372ABE}");
            AZ_CLASS_ALLOCATOR(OutInterpretedResult, AZ::SystemAllocator, 0);

            int m_lambdaRegistryIndex;
            lua_State* m_lua;
            
            // assumes a lambda is at the top of the stack and will pop it
            OutInterpretedResult(lua_State* lua);

            OutInterpretedResult(OutInterpretedResult&& source) noexcept;

            ~OutInterpretedResult();

            OutInterpretedResult& operator=(OutInterpretedResult&& source) noexcept;

            void operator()(AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments);

            // \note Do not use, these are here for compiler compatibility only
            OutInterpretedResult() = default;
            OutInterpretedResult(const OutInterpretedResult & source);
        };

        struct OutInterpretedUserSubgraph
        {
            AZ_TYPE_INFO(OutInterpretedUserSubgraph, "{1221F79E-0951-48F7-A0F1-1306A379D6BA}");
            AZ_CLASS_ALLOCATOR(OutInterpretedUserSubgraph, AZ::SystemAllocator, 0);

            int m_lambdaRegistryIndex;
            lua_State* m_lua;

            // assumes a lambda is at the top of the stack and will pop it
            OutInterpretedUserSubgraph(lua_State* lua);

            OutInterpretedUserSubgraph(OutInterpretedUserSubgraph&& source) noexcept;

            ~OutInterpretedUserSubgraph();

            OutInterpretedUserSubgraph& operator=(OutInterpretedUserSubgraph&& source) noexcept;

            void operator()(AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments);

            // \note Do not use, these are here for compiler compatibility only
            OutInterpretedUserSubgraph() = default;
            OutInterpretedUserSubgraph(const OutInterpretedUserSubgraph& source);
        };
    } 

} 
