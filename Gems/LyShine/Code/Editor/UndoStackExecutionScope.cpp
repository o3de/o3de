/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

UndoStackExecutionScope::UndoStackExecutionScope(UndoStack* stack)
    : m_stack(stack)
{
    m_stack->SetIsExecuting(true);
}

UndoStackExecutionScope::~UndoStackExecutionScope()
{
    m_stack->SetIsExecuting(false);
}
