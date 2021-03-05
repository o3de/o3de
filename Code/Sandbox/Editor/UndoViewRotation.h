/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
