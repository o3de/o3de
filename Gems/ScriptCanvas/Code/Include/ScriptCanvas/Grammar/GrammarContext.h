/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptCanvas/Core/SubgraphInterface.h>

namespace ScriptCanvas
{
    namespace Grammar
    {        
        class Context
        {
        public:
            AZ_CLASS_ALLOCATOR(Context, AZ::SystemAllocator, 0);

            Context() = default;
            ~Context() = default;

            const SubgraphInterfaceSystem& GetExecutionMapSystem() const;
            SubgraphInterfaceSystem& ModExecutionMapSystem();

        private:
            SubgraphInterfaceSystem m_executionMapSystem;

            // put grammatical state globals in here, things that can be useful across several parses of graphs
        };
    } 

} 
