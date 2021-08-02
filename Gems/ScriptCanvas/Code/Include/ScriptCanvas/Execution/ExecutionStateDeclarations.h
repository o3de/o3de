/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
