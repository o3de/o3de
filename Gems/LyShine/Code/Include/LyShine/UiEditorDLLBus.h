/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <LyShine/UiBase.h>

class UndoStack;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that the UI Editor needs to implement
class UiEditorDLLInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorDLLInterface(){}

    //! Get the selected elements in the UiEditor
    virtual LyShine::EntityArray GetSelectedElements() = 0;

    //! Get the id of the active Canvas the UiEditor
    virtual AZ::EntityId GetActiveCanvasId() = 0;

    //! Get the active undo stack for the UI Editor
    virtual UndoStack* GetActiveUndoStack() = 0;

    //! Soft-switch to the given file.  Note that this should prompt for unsaved changes, etc.
    virtual void OpenSourceCanvasFile(QString absolutePathToFile) = 0;

public: // static member functions

    static const char* GetUniqueName() { return "UiEditorDLLInterface"; }
};

typedef AZ::EBus<UiEditorDLLInterface> UiEditorDLLBus;
