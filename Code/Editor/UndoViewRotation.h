/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Undo for Python function (PySetCurrentViewRotation)


#ifndef CRYINCLUDE_EDITOR_UNDOVIEWROTATION_H
#define CRYINCLUDE_EDITOR_UNDOVIEWROTATION_H
#pragma once
#include "Undo/IUndoObject.h"


class CUndoViewRotation
    : public IUndoObject
{
public:
    CUndoViewRotation(const QString& pUndoDescription = "Set Current View Rotation");

protected:
    int GetSize();
    QString GetDescription();
    void Undo(bool bUndo);
    void Redo();

private:
    Ang3 m_undo;
    Ang3 m_redo;
    QString m_undoDescription;
};

#endif // CRYINCLUDE_EDITOR_UNDOVIEWROTATION_H
