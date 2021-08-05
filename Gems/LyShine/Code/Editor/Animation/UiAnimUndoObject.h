/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

//! UiAnimUndoObject is a interface of general Undo object that makes up
//! part of a UiAnimUndoStep
struct UiAnimUndoObject
{
    UiAnimUndoObject() {}

    // Virtual destructor.
    virtual ~UiAnimUndoObject() {};

    //! Called to delete undo object.
    virtual void Release() { delete this; };
    //! Return size of this Undo object.
    virtual int GetSize() = 0;
    //! Return description of this Undo object.
    virtual const char* GetDescription() = 0;

    //! Undo this object.
    //! @param bUndo If true this operation called in response to Undo operation.
    virtual void Undo(bool bUndo = true) = 0;

    //! Redo undone changes on object.
    virtual void Redo() = 0;

    // Returns the name of undo object
    virtual const char* GetObjectName() { return 0; };

    virtual bool IsChanged([[maybe_unused]] unsigned int& compareValue) const { return false; }
};
