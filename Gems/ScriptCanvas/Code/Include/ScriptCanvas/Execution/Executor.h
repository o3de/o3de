/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Execution/ExecutionState.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Internal
    {
        class ExecutorBase
        {
        public:
            AZ_TYPE_INFO(ExecutorBase, "{385E32B3-7F27-4ABE-A868-BE1482BACADB}");
            AZ_CLASS_ALLOCATOR(ExecutorBase, AZ::SystemAllocator, 0);

            ~ExecutorBase();

            bool IsExecutable() const;

        };
    }
    /*
    * Usage:
    * 1) Initialize()
    * 2) Execute()
    * 3) Stop()
    * 4) Optional (repeat steps 1-3), Stop() and Initialize() may be required to be called before subsequent calls to Execute();
    */
    class Executor final
    //     : public Internal::ExecutorBase
    {
    public:
        AZ_TYPE_INFO(Executor, "{02E0EB5F-B28E-4B95-9FF2-DEA42ECC575D}");
        AZ_CLASS_ALLOCATOR(Executor, AZ::SystemAllocator, 0);

        ~Executor();

        ActivationInfo CreateActivationInfo() const;

        void Execute();

        ExecutionMode GetExecutionMode() const;

        void Initialize(const RuntimeDataOverrides& overrides, AZStd::any&& userData = AZStd::any());

        void InitializeAndExecute(const RuntimeDataOverrides& overrides, AZStd::any&& userData = AZStd::any());

        bool IsExecutable() const;

        void StopAndClearExecutable();

        void StopAndKeepExecutable();

    protected:
        ExecutionStatePtr m_executionState;
    };

    // add a checked executor

}
