/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

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
