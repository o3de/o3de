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
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace ScriptCanvas
{
    class ExecutionState;
    using ExecutionStateConstPtr = AZStd::shared_ptr<const ExecutionState>;
    using ExecutionStatePtr = AZStd::shared_ptr<ExecutionState>;
    using ExecutionStateWeakConstPtr = const ExecutionState*;
    using ExecutionStateWeakPtr = ExecutionState*;

    class ExecutionStateInterpreted;
    using ExecutionStateInterpretedConstPtr = AZStd::shared_ptr<const ExecutionStateInterpreted>;
    using ExecutionStateInterpretedPtr = AZStd::shared_ptr<ExecutionStateInterpreted>;

    class ExecutionStateInterpretedPure;
    using ExecutionStateInterpretedPureConstPtr = AZStd::shared_ptr<const ExecutionStateInterpretedPure>;
    using ExecutionStateInterpretedPurePtr = AZStd::shared_ptr<ExecutionStateInterpretedPure>;
    
    class ExecutionStateInterpretedSingleton;
    using ExecutionStateInterpretedSingletonConstPtr = AZStd::shared_ptr<const ExecutionStateInterpretedSingleton>;
    using ExecutionStateInterpretedSingletonPtr = AZStd::shared_ptr<ExecutionStateInterpretedSingleton>;
    
    struct ExecutionStateConfig;

} 
