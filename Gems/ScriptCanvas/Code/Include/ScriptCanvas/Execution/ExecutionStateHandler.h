/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Execution/ExecutionStateStorage.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    struct ActivationInfo;

    /**
    * ExecutionStateHandler provides RAII semantics and an interface for the ExecutionStateStorage for ScriptCanvas graph. It and executes
    * and stops the runtime graph, if possible.
    * \note this is done WITHOUT any safety checks. For example, the the presence of a good, loaded asset is required when Execute()
    * is called. If SC_RUNTIME_CHECKS_ENABLED is defined, the class will error on a bad asset, and early return if possible.
    * In this class general attempts to eliminate any branching done in the interest of safety checks.
    * All safety checks are expected be done by systems that own the Executor class. If safety checks are desired, consider
    * using the Interpreter class instead, which manages the execution stack from source file -> build system -> execution.
    *
    * Usage:
    * 1) Initialize()
    * 2) Execute()
    * 3) <stop function>()
    * 4) Optional (repeat steps 1-3), <stop function>() and Initialize() may be required to be called before subsequent calls to Execute();
    */
    class ExecutionStateHandler final
    {
    public:
        AZ_TYPE_INFO(ExecutionStateHandler, "{02E0EB5F-B28E-4B95-9FF2-DEA42ECC575D}");
        AZ_CLASS_ALLOCATOR(ExecutionStateHandler, AZ::SystemAllocator, 0);

        /// Clears the Executable on destruction if required.
        ~ExecutionStateHandler();

        ActivationInfo CreateActivationInfo() const;

        /** Executes if possible, fails an SC_RUNTIME_CHECK if not. */
        void Execute();

        ExecutionMode GetExecutionMode() const;

        /** Initializes the runtime with the inputs, but does NOT execute. */
        void Initialize(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData = ExecutionUserData());

        /** Provides Initialize with the provided inputs and immediately executes. */
        void InitializeAndExecute(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData = ExecutionUserData());

        /** Returns true iff the object has been initialized with valid, executable data */
        bool IsExecutable() const;

        /** Returns true iff the user can reasonably call Execute() serially without calling a <stop function>() in between. */
        bool IsPure() const;

        /** Stops Execution if possible, and destroys the runtime. The user must call Initialize() before Executing again. */
        void StopAndClearExecutable();

        /** Stops Execution if possible, and keeps the runtime, allowing the user to immediately call Execute() again. */
        void StopAndKeepExecutable();

    protected:
        Execution::StateStorage m_executionStateStorage;
        ExecutionState* m_executionState = nullptr;
    };
}
