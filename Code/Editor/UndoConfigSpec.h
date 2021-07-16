/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Undo for Python function (PySetConfigSpec)


#ifndef CRYINCLUDE_EDITOR_UNDOCONFIGSPEC_H
#define CRYINCLUDE_EDITOR_UNDOCONFIGSPEC_H
#pragma once

#include "Undo/IUndoObject.h"

class CUndoConficSpec
    : public IUndoObject
{
public:
    CUndoConficSpec(const QString& pUndoDescription = "Set Config Spec");

protected:
    int GetSize();
    QString GetDescription();
    void Undo(bool bUndo);
    void Redo();

private:
    int m_undo;
    int m_redo;
    QString m_undoDescription;
};

#endif // CRYINCLUDE_EDITOR_UNDOCONFIGSPEC_H
