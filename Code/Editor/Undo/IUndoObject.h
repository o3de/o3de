/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Interface for implementation of IUndo objects.


#ifndef CRYINCLUDE_EDITOR_UNDO_IUNDOOBJECT_H
#define CRYINCLUDE_EDITOR_UNDO_IUNDOOBJECT_H
#pragma once

#include <QString>

//! IUndoObject is a interface of general Undo object.
struct IUndoObject
{
    // Virtual destructor.
    virtual ~IUndoObject() {};

    //! Called to delete undo object.
    virtual void Release() { delete this; };
    //! Return size of this Undo object.
    virtual int GetSize() = 0;
    //! Return description of this Undo object.
    virtual QString GetDescription() = 0;

    //! Undo this object.
    //! @param bUndo If true this operation called in response to Undo operation.
    virtual void Undo(bool bUndo = true) = 0;

    //! Redo undone changes on object.
    virtual void Redo() = 0;

    // Returns the name of undo object
    virtual QString GetObjectName(){ return QString(); };

    // Returns the name of related editor object.
    // Ex: For a undo action which would modify value for var "Emitter Strength" of emitter "Level.example",
    // this function will return emitter name "Level.example" - Vera, Confetti
    virtual QString GetEditorObjectName() { return QString(); };

    virtual bool IsChanged([[maybe_unused]] unsigned int& compareValue) const { return false; }
};

#endif // CRYINCLUDE_EDITOR_UNDO_IUNDOOBJECT_H
