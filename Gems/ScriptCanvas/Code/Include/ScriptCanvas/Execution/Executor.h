/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/ExecutionStateHandler.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    /**
     * Convenience class for for containing an ExecutionStateHandler, and the RuntimeDataOverrides and ExecutionUserData that the
     * ExecutionStateHandler requires to run properly.
     * Like the ExecutionState and ExecutionStateHandler it provides no little or no safety checks, and host systems must take
     * care to properly initialize it.
     *
     * \see ExecutionStateHandler for documentation on Execution and Initialization methods.
     * 
     * For example usage:
     * \see Interpreter
     * \see RuntimeComponents
     */
    class Executor final
    {
        enum Version
        {
            // This is a strict, runtime-should-be-good class. Do not version it; trigger host systems to rebuild it.
            DoNotVersionThisClassButMakeHostSystemsRebuildIt = 0,
        };

    public:
        AZ_TYPE_INFO(Executor, "{0D1E4B9D-1A2C-4B9D-8364-052255BC691F}");
        AZ_CLASS_ALLOCATOR(Executor, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        void Execute();

        const RuntimeDataOverrides& GetRuntimeOverrides() const;

        void Initialize();

        void InitializeAndExecute();

        bool IsExecutable() const;

        bool IsPure() const;

        /// Set the RuntimeDataOverrides which provide a runtime Asset to run and the possible property overrides.
        void SetRuntimeOverrides(const RuntimeDataOverrides& overrideData);

        /// Set the ExecutionUserData which will be used on Execution.
        void SetUserData(const ExecutionUserData& userData);

        void StopAndClearExecutable();

        void StopAndKeepExecutable();

        /// Take the RuntimeDataOverrides which provide a runtime Asset to run and the possible property overrides.
        void TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData);

        /// Take the ExecutionUserData which will be used on Execution.
        void TakeUserData(ExecutionUserData&& userData);

    private:
        ExecutionUserData m_userData;
        ExecutionStateHandler m_execution;
        RuntimeDataOverrides m_runtimeOverrides;
    };
}
