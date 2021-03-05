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

#include <Atom/RHI/ScopeProducer.h>

namespace AZ
{
    namespace RHI
    {
        class ScopeProducerEmpty final
            : public ScopeProducer
        {
        public:
            AZ_CLASS_ALLOCATOR(ScopeProducerEmpty, SystemAllocator, 0);

            ScopeProducerEmpty(const ScopeId& scopeId)
                : ScopeProducer(scopeId)
            {}

        private:
            void SetupFrameGraphDependencies(FrameGraphInterface) override {}
            void CompileResources(const FrameGraphCompileContext&) override {}
            void BuildCommandList(const FrameGraphExecuteContext&) override {}
        };
    }
}
