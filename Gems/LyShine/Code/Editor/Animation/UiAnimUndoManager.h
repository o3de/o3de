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

#pragma once

#include <AzCore/std/string/string.h>

struct UiAnimUndoObject;
class UiAnimUndoStep;
struct IUndoManagerListener;
class UndoStack;

/*!
 *  UiAnimUndoManager is keeping and operating on UiAnimUndo class instances.
 */
class UiAnimUndoManager
{
public:
    UiAnimUndoManager();
    ~UiAnimUndoManager();

    //! Begin operation requiring undo.
    //! Undo manager enters holding state.
    void Begin();
    //! Restore all undo objects registered since last Begin call.
    //! @param bUndo if true all Undo object registered up to this point will be undone.
    void Restore(bool bUndo = true);
    //! Accept changes and registers an undo object with the undo manager.
    //! This will allow the user to undo the operation.
    void Accept(const AZStd::string& name);
    //! Cancel changes and restore undo objects.
    void Cancel();

    //! Temporarily suspends recording of undo.
    void Suspend();
    //! Resume recording if was suspended.
    void Resume();

    // Undo last operation.
    void Undo();

    //! Redo last undo.
    void Redo();

    void RedoStep(UiAnimUndoStep* step);
    void UndoStep(UiAnimUndoStep* step);

    //! Check if undo information is recording now.
    bool IsUndoRecording() const;

    //////////////////////////////////////////////////////////////////////////
    bool IsUndoSuspended() const;

    //! Put new undo object, must be called between Begin and Accept/Cancel methods.
    void RecordUndo(UiAnimUndoObject* obj);

    //! Completly flush all Undo and redo buffers.
    //! Must be done on level reloads or global Fetch operation.
    void Flush();

    void AddListener(IUndoManagerListener* pListener);
    void RemoveListener(IUndoManagerListener* pListener);

    void SetActiveUndoStack(UndoStack* undoStack);
    UndoStack* GetActiveUndoStack() const;

    // Get the active UiAnimUndoManager (if any). The active undo manager
    static UiAnimUndoManager* Get();

private: // ---------------------------------------------------------------


    void BeginUndoTransaction();
    void EndUndoTransaction();
    void BeginRestoreTransaction();
    void EndRestoreTransaction();

    static UiAnimUndoManager*                       s_instance;
    UndoStack*                                      m_uiUndoStack;

    bool                                            m_bRecording;
    int                                             m_suspendCount;

    bool                                            m_bUndoing;
    bool                                            m_bRedoing;

    UiAnimUndoStep*                                m_currentUndo;

    AZStd::vector<IUndoManagerListener*> m_listeners;
};
