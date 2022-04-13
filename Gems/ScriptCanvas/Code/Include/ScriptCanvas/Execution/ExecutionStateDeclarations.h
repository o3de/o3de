/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(_RELEASE)
#define SC_RUNTIME_CHECKS_ENABLED
#endif

#if defined SC_RUNTIME_CHECKS_ENABLED
#define SC_RUNTIME_ASSERT(expr, ...) AZ_Error("ScriptCanvas", expr, __VA_ARGS__);
#define SC_RUNTIME_ASSERT_RETURN(expr, ...) if (!(expr)) { AZ_Error("ScriptCanvas", false, __VA_ARGS__); return; }
#else
#define SC_RUNTIME_ASSERT(expr, ...) AZ_Assert(expr, __VA_ARGS__);
#define SC_RUNTIME_ASSERT_RETURN(expr, ...) if (!(expr)) { AZ_Assert(false, __VA_ARGS__); return; }
#endif

#include <AzCore/std/any.h>

namespace ScriptCanvas
{
    class ExecutionState;
    using ExecutionStateConstPtr = const ExecutionState*;
    using ExecutionStatePtr = ExecutionState*;
    using ExecutionStateWeakConstPtr = const ExecutionState*;
    using ExecutionStateWeakPtr = ExecutionState*;

    class ExecutionStateInterpreted;
    using ExecutionStateInterpretedConstPtr = const ExecutionStateInterpreted*;
    using ExecutionStateInterpretedPtr = ExecutionStateInterpreted*;

    class ExecutionStateInterpretedPure;
    using ExecutionStateInterpretedPureConstPtr = const ExecutionStateInterpretedPure*;
    using ExecutionStateInterpretedPurePtr = ExecutionStateInterpretedPure*;
    
    class ExecutionStateInterpretedSingleton;
    using ExecutionStateInterpretedSingletonConstPtr = const ExecutionStateInterpretedSingleton*;
    using ExecutionStateInterpretedSingletonPtr = ExecutionStateInterpretedSingleton*;
    
    struct ExecutionStateConfig;

    using ExecutionUserData = AZStd::any;
} 
