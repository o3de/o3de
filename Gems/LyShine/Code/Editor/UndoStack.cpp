/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include <QUndoGroup>

UndoStack::UndoStack(QUndoGroup* group)
    : QUndoStack(group)
    , m_isExecuting(false)
{
}

void UndoStack::SetIsExecuting(bool b)
{
    m_isExecuting = b;
}

bool UndoStack::GetIsExecuting() const
{
    return m_isExecuting;
}

#include <moc_UndoStack.cpp>
