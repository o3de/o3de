/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_UNDO_UNDOVARIABLECHANGE_H
#define CRYINCLUDE_EDITOR_UNDO_UNDOVARIABLECHANGE_H
#pragma once

#include "IUndoObject.h"

#include "Util/Variable.h"

class CAttributeItem;

//////////////////////////////////////////////////////////////////////////
//! Undo object for Variable in property control..
class CUndoVariableChange
    : public IUndoObject
{
public:
    CUndoVariableChange(IVariable* var, const char* undoDescription, const char* editorObjfullname = 0)
    {
        // Stores the current state of this object.
        assert(var != 0);
        m_undoDescription = undoDescription;
        m_var = var;
        m_undo = m_var->Clone(false);
        m_EditorObjFullName = editorObjfullname;
    }

protected:
    virtual int GetSize()
    {
        int size = sizeof(*this);
        //if (m_var)
        //size += m_var->GetSize();
        if (m_undo)
        {
            size += m_undo->GetSize();
        }
        if (m_redo)
        {
            size += m_redo->GetSize();
        }
        return size;
    }

    virtual QString GetDescription()
    {
        return m_undoDescription;
    }

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo = m_var->Clone(false);
        }
        m_var->CopyValue(m_undo);
    }

    virtual void Redo()
    {
        if (m_redo)
        {
            m_var->CopyValue(m_redo);
        }
    }

    virtual QString GetEditorObjectName()
    {
        return m_EditorObjFullName;
    };

    void SetEditorObjName(const char* fullname)
    {
        m_EditorObjFullName = fullname;
    }

private:
    QString m_undoDescription;
    QString m_EditorObjFullName;        // Add related editor object name so we can track undo by editor object
    TSmartPtr<IVariable> m_undo;
    TSmartPtr<IVariable> m_redo;
    TSmartPtr<IVariable> m_var;
};


//////////////////////////////////////////////////////////////////////////
// Confetti Start
// This undoobject class is responsible for recording varialeundo action reqiure QTUI reaction.
// This class will store a CAttributeView. Editor plugin is responsible for deal with QTUI actions.
class CUndoQTUIVariableChange
    : public CUndoVariableChange
{
public:
    CUndoQTUIVariableChange(IVariable* var, CAttributeItem* widget, const char* undoDescription, const char* editorObjfullname = 0)
        : CUndoVariableChange(var, undoDescription, editorObjfullname)
    {
        // Stores the current state of this object.
        m_uiWidget = widget;
    }

    CAttributeItem* getWidget() const
    {
        return m_uiWidget;
    }

private:
    CAttributeItem* m_uiWidget;
};
// Confetti End
////////////////////////////////////////////////////////////////////////


#endif // CRYINCLUDE_EDITOR_UNDO_UNDO_H
