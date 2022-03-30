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
    /*
    * Usage:
    * 1) Initialize
    * 2) Execute
    * 3) Stop()
    * 4) Optional (repeat steps 1-3), Stop() and Initialize() MUST be called before subsequent calls to Execute();
    */
    class Executor
    {
    public:
        AZ_TYPE_INFO(Executor, "{02E0EB5F-B28E-4B95-9FF2-DEA42ECC575D}");
        AZ_CLASS_ALLOCATOR(Executor, AZ::SystemAllocator, 0);

        ~Executor();

        ActivationInfo CreateActivationInfo() const;

        // \todo consider making a template version with return values, similar to execution out
        // or perhaps safety checked versions with an array / table of any. something parsable
        // or consider just having users make ebuses that the graphs will handle
        // and wrapping the whole thing in a single class
        // interpreter + ebus, and calling it EZ SC Hook or something like that
        void Execute();

        ExecutionMode GetExecutionMode() const;

        void Initialize(const RuntimeDataOverrides& overrides, AZStd::any&& userData = AZStd::any());

        void InitializeAndExecute(const RuntimeDataOverrides& overrides, AZStd::any&& userData = AZStd::any());

        void Stop();

    private:
        ExecutionStatePtr m_executionState;
    };

}
