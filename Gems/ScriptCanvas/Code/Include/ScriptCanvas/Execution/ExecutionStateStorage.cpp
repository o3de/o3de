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
    ExecutionStateStorage::~ExecutionStateStorage()
    {
        Mod()->~ExecutionState();
    }

    void ExecutionStateStorage::CreatePerActivation(ExecutionStateStorage& storage, ExecutionStateConfig& config)
    {
        new (&storage.m_storage) ExecutionStateInterpretedPerActivation(config);
    }

    void ExecutionStateStorage::CreatePerActivationOnGraphStart(ExecutionStateStorage& storage, ExecutionStateConfig& config)
    {
        new (&storage.m_storage) ExecutionStateInterpretedPerActivationOnGraphStart(config);
    }

    void ExecutionStateStorage::CreatePure(ExecutionStateStorage& storage, ExecutionStateConfig& config)
    {
        new (&storage.m_storage) ExecutionStateInterpretedPure(config);
    }

    void ExecutionStateStorage::CreatePureOnGraphStart(ExecutionStateStorage& storage, ExecutionStateConfig& config)
    {
        new (&storage.m_storage) ExecutionStateInterpretedPureOnGraphStart(config);
    }

    const ExecutionState* ExecutionStateStorage::Get() const
    {
        return reinterpret_cast<const ExecutionState*>(&m_storage);
    }

    ExecutionState* ExecutionStateStorage::Mod()
    {
        return reinterpret_cast<ExecutionState*>(&m_storage);
    }
}
