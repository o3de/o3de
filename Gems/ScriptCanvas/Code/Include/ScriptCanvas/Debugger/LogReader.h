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

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Asset/ExecutionLogAsset.h>
#include <AzCore/EBus/EBus.h>
#include <Debugger/Bus.h>

#include "APIArguments.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        class LogReader
            : public LoggableEventVisitor
        {            
        public:
            AZ_CLASS_ALLOCATOR(LogReader, AZ::SystemAllocator, 0);
            
            LogReader();
            ~LogReader();
           
            // start at the beginning, if possible
            bool Start() { return false; }
            // step back one event, if possible
            bool StepBack() { return false; }
            // step forward one event, if possible
            bool StepForward() { return false; }
            
        protected:
            void Visit(ExecutionThreadEnd&);
            void Visit(ExecutionThreadBeginning&);
            void Visit(GraphActivation&);
            void Visit(GraphDeactivation&);
            void Visit(NodeStateChange&);
            void Visit(InputSignal&);
            void Visit(OutputSignal&);
            void Visit(VariableChange&);

        private:
            size_t m_index = 0;
        };
    }
}
