/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Execution/ExecutionStateStorage.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        ExecutionState* CreatePerActivation(StateStorage& storage, ExecutionStateConfig& config)
        {
            new (&storage.data) ExecutionStateInterpretedPerActivation(config);
            return reinterpret_cast<ExecutionState*>(&storage.data);
        }

        ExecutionState* CreatePerActivationOnGraphStart(StateStorage& storage, ExecutionStateConfig& config)
        {
            new (&storage.data) ExecutionStateInterpretedPerActivationOnGraphStart(config);
            return reinterpret_cast<ExecutionState*>(&storage.data);
        }

        ExecutionState* CreatePure(StateStorage& storage, ExecutionStateConfig& config)
        {
            new (&storage.data) ExecutionStateInterpretedPure(config);
            return reinterpret_cast<ExecutionState*>(&storage.data);
        }

        ExecutionState* CreatePureOnGraphStart(StateStorage& storage, ExecutionStateConfig& config)
        {
            new (&storage.data) ExecutionStateInterpretedPureOnGraphStart(config);
            return reinterpret_cast<ExecutionState*>(&storage.data);
        }

        void Destruct(StateStorage& storage)
        {
            Mod(storage)->~ExecutionState();
        }

        const ExecutionState* Get(const StateStorage& storage)
        {
            return reinterpret_cast<const ExecutionState*>(&storage.data);
        }

        ExecutionState* Mod(StateStorage& storage)
        {
            return reinterpret_cast<ExecutionState*>(&storage.data);
        }
    }
}
