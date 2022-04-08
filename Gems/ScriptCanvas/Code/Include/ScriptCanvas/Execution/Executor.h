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
     * Convenience class for for containing an ExecutionStateHandler, and the RuntimeDataOverrides and Userdata that it will use.
     * Like the ExecutionState and ExecutionStateHandler it provides no little or not safety checks, and host systems must take
     * care to properly initialize it.
     *       
     * For example usage:
     * \see Interpreter
     * \see RuntimeComponents
     */
    class Executor final
    {
        enum Version
        {
            // This is a strict, runtime-should-be-good class. Do not version it, trigger host systems to rebuild it.
            DoNotVersionThisClassButMakeHostSystemsRebuildIt = 0,
        };

    public:
        AZ_TYPE_INFO(Executor, "{0D1E4B9D-1A2C-4B9D-8364-052255BC691F}");
        AZ_CLASS_ALLOCATOR(Executor, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        void Execute();

        void Initialize();

        void InitializeAndExecute();

        bool IsExecutable() const;

        bool IsPure() const;

        void SetRuntimeOverrides(const RuntimeDataOverrides& overrideData);

        void SetUserData(const ExecutionUserData& userData);

        void StopAndClearExecutable();

        void StopAndKeepExecutable();

        void TakeRuntimeDataOverrides(RuntimeDataOverrides&& overrideData);

        void TakeUserData(ExecutionUserData&& userData);

    private:
        ExecutionUserData m_userData;
        ExecutionStateHandler m_execution;
        RuntimeDataOverrides m_runtimeOverrides;
    };
}
