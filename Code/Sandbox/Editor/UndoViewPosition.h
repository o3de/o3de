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
