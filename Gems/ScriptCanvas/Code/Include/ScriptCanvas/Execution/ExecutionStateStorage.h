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
    /**
    * Defines operations for storage of the dynamic and polymorphic ExecutionState sub classes in a static size buffer. This eliminates
    * heap allocation costs associated with creating the proper ExecutionState against an asset whose type cannot be known
    * at runtime. Use with class ExecutionStateHandler to benefit from RAII semantics or you must perform
    * your own safety checks to call Destruct() before this storage is deleted if any of the Create calls were used on it.
    */
    namespace Execution
    {
        static constexpr const size_t s_StorageSize
            = AZ_SIZE_ALIGN_UP(AZStd::max(sizeof(ExecutionStateInterpretedPerActivation)
                , AZStd::max(sizeof(ExecutionStateInterpretedPerActivationOnGraphStart)
                    , AZStd::max(sizeof(ExecutionStateInterpretedPure)
                        , sizeof(ExecutionStateInterpretedPureOnGraphStart)))), 32);

        using StorageArray = AZStd::array<AZ::u8, s_StorageSize>;

        /// This struct allows forward declare usage, rather than including this file, which must pull all derived ExecutionStates into it.
        struct StateStorage
        {
            StorageArray data;
        };

        ExecutionState* CreatePerActivation(StateStorage& storage, ExecutionStateConfig& config);

        ExecutionState* CreatePerActivationOnGraphStart(StateStorage& storage, ExecutionStateConfig& config);

        ExecutionState* CreatePure(StateStorage& storage, ExecutionStateConfig& config);

        ExecutionState* CreatePureOnGraphStart(StateStorage& storage, ExecutionStateConfig& config);

        void Destruct(StateStorage& storage);

        const ExecutionState* Get(const StateStorage& storage);

        ExecutionState* Mod(StateStorage& storage);
    }
}
