/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Undo for Python function (PySetCurrentViewPosition)

#ifndef CRYINCLUDE_EDITOR_UNDOVIEWPOSITION_H
#define CRYINCLUDE_EDITOR_UNDOVIEWPOSITION_H
#pragma once

#include "Undo/IUndoObject.h"

class CUndoViewPosition
    : public IUndoObject
{
public:
    CUndoViewPosition(const QString& pUndoDescription = "Set Current View Position");

protected:
    int GetSize();
    QString GetDescription();
    void Undo(bool bUndo);
    void Redo();

private:
    Vec3 m_undo;
    Vec3 m_redo;
    QString m_undoDescription;
};

#endif // CRYINCLUDE_EDITOR_UNDOVIEWPOSITION_H
