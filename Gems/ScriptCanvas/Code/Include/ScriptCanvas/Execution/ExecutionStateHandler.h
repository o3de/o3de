/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    /*
    * Owns the ExecutionState for ScriptCanvas graphs and executes and stops it, if possible.
    * \note this is done WITHOUT any safety checks. For example, the the presence of a good, loaded asset is required when Execute()
    * is called. If SCRIPT_CANVAS_RUNTIME_ASSET_CHECK is defined, the class will error and return on a bad asset, otherwise it asserts and
    * for a message, but in general attempts to eliminate any branching done in the interest of safety checks.
    * All safety checks are expected be done by systems that own the Executor class. If safety checks are desired, consider
    * using the Interpreter class instead, which manages the execution stack from source file -> build system -> execution.
    *
    * Usage:
    * 1) Initialize()
    * 2) Execute()
    * 3) Stop()
    * 4) Optional (repeat steps 1-3), Stop() and Initialize() may be required to be called before subsequent calls to Execute();
    */
    class ExecutionStateHandler final
    {
    public:
        AZ_TYPE_INFO(ExecutionStateHandler, "{02E0EB5F-B28E-4B95-9FF2-DEA42ECC575D}");
        AZ_CLASS_ALLOCATOR(ExecutionStateHandler, AZ::SystemAllocator, 0);

        ~ExecutionStateHandler();

        ActivationInfo CreateActivationInfo() const;

        void Execute();

        ExecutionMode GetExecutionMode() const;

        void Initialize(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData = ExecutionUserData());

        void InitializeAndExecute(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData = ExecutionUserData());

        bool IsExecutable() const;

        bool IsPure() const;

        void StopAndClearExecutable();

        void StopAndKeepExecutable();

    protected:
        // \todo allocate static space under a union of all possible derived types, and pass it into construction call
        ExecutionStatePtr m_executionState = nullptr;
    };
}
