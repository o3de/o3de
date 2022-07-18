/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

/// SC_RUNTIME_CHECKS are only for use against machine generated code, or interactions with it.
/// It is recommended to leave them as asserts whenever possible. The primary intention of SC_RUNTIME_CHECKS_ENABLED is to allow developers
/// and users of the ScriptCanvas Runtime to quickly switch between an implementation that has been verified to execute without any errors
/// or harm to the host executable (a release or performance profiling build), or an implementation that may be riskier or under
/// active development. Care is taken to make sure that both implementations execute the logic of Scripts *exactly the same*. When checks
/// are enabled, the most catastrophic errors (to the host executable) are attempted to be avoided when they are encountered in either
/// internal systems of the ScriptCanvas runtime, or when executing Scripts themselves. Otherwise, Scripts are executed with as little
/// additional branching or diagnostics as possible, to allow for minimal overhead.
#if !defined(_RELEASE)
#define SC_RUNTIME_CHECKS_ENABLED
#endif

/// It is recommended to leave SC_RUNTIME_CHECKS implemented as a asserts, as they are to be considered a catastrophic failure that should
/// not be tolerated in a release build.
// #define SC_RUNTIME_CHECKS_AS_ERRORS 

#if defined SC_RUNTIME_CHECKS_ENABLED
    #if defined SC_RUNTIME_CHECKS_AS_ERRORS
    #define SC_RUNTIME_CHECK(expr, ...) AZ_Error("ScriptCanvas", expr, __VA_ARGS__);
    #define SC_RUNTIME_CHECK_RETURN(expr, ...) if (!(expr)) { AZ_Error("ScriptCanvas", false, __VA_ARGS__); return; }
    #else
    #define SC_RUNTIME_CHECK(expr, ...) AZ_Assert(expr, __VA_ARGS__);
    #define SC_RUNTIME_CHECK_RETURN(expr, ...) if (!(expr)) { AZ_Assert(false, __VA_ARGS__); return; }
    #endif
#else
#define SC_RUNTIME_CHECK(expr, ...) 
#define SC_RUNTIME_CHECK_RETURN(expr, ...) 
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
