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
    class Executor
    {
    public:
        AZ_TYPE_INFO(Executor, "{02E0EB5F-B28E-4B95-9FF2-DEA42ECC575D}");
        AZ_CLASS_ALLOCATOR(Executor, AZ::SystemAllocator, 0);

        // will need a source handle version, with builder overrides
        // use the data system, use a different class for that, actually
        // the Interpreter

        ~Executor();

        ActivationInfo CreateActivationInfo() const;

        // make a template version with return values
        void Execute();

        const RuntimeData& GetRuntimeAssetData() const;

        ExecutionMode GetExecutionMode() const;

        const RuntimeDataOverrides& GetRuntimeDataOverrides() const;

        void Initialize(const RuntimeDataOverrides& overrides, AZStd::any&& userData = AZStd::any());

        void InitializeAndExecute(const RuntimeDataOverrides& overrides, AZStd::any&& userData = AZStd::any());

        void Stop();

    private:
        ExecutionStatePtr m_executionState;
        const RuntimeDataOverrides* m_overrides = nullptr;
    };

}
