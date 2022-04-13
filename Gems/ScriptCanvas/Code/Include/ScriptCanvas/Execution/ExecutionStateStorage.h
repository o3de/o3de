/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedPure.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedPerActivation.h>

namespace ScriptCanvas
{
    /*
    * Holds a dynamic execution state in a static allocation to elimination heap allocation costs
    * associated with creating the proper execution state against and asset whose type cannot be known
    * at runtime. Use with RAII class ExecutionStateHandler or you must perform your own saftey checks.
    */
    class ExecutionStateStorage final
    {
        static constexpr const size_t s_StorageSize
            = AZ_SIZE_ALIGN_UP(AZStd::max(sizeof(ExecutionStateInterpretedPerActivation)
            , AZStd::max(sizeof(ExecutionStateInterpretedPerActivationOnGraphStart)
            , AZStd::max(sizeof(ExecutionStateInterpretedPure)
            , sizeof(ExecutionStateInterpretedPureOnGraphStart)))), 32);

        using Storage = AZStd::array<AZ::u8, s_StorageSize>;

    public:
        AZ_TYPE_INFO(ExecutionStateStorage, "{AD40E734-739A-4784-9842-79D251499B91}");
        AZ_CLASS_ALLOCATOR(ExecutionStateStorage, AZ::SystemAllocator, 0);

        static void CreatePerActivation(ExecutionStateStorage& storage, ExecutionStateConfig& config);

        static void CreatePerActivationOnGraphStart(ExecutionStateStorage& storage, ExecutionStateConfig& config);

        static void CreatePure(ExecutionStateStorage& storage, ExecutionStateConfig& config);

        static void CreatePureOnGraphStart(ExecutionStateStorage& storage, ExecutionStateConfig& config);

        void Destroy();

        const ExecutionState* Get() const;

        ExecutionState* Mod();
        
    private:
        Storage m_storage;
    };
}
