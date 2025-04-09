/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"
#include "UndoDropDown.h"

// Editor
#include "Undo/Undo.h"          // for CUndoManager

/////////////////////////////////////////////////////////////////////////////
// Turns listener callbacks into signals

UndoStackStateAdapter::UndoStackStateAdapter(QObject* parent /* = nullptr */)
    : QObject(parent)
{
    GetIEditor()->GetUndoManager()->AddListener(this);
}

UndoStackStateAdapter::~UndoStackStateAdapter()
{
    GetIEditor()->GetUndoManager()->RemoveListener(this);
}
