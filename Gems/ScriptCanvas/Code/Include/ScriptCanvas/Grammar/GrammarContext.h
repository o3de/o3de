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
