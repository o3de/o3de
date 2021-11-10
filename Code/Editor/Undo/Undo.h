/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITORCORE_UNDO_UNDO_H
#define CRYINCLUDE_EDITORCORE_UNDO_UNDO_H
#pragma once

#include "IUndoManagerListener.h"
#include "IUndoObject.h"
#include <AzCore/Asset/AssetManager.h>
#include <CryCommon/StlUtils.h>

struct IUndoObject;
class CSuperUndoStep;
class AssetManagerUndoInterruptor;

//! CUndo is a collection of IUndoObjects instances that forms single undo step.
class CUndoStep
{
public:
    CUndoStep() {};
    virtual ~CUndoStep() { ClearObjects(); }

    //! Set undo object name.
    void SetName(const QString& name) { m_name = name; };
    //! Get undo object name.
    const QString& GetName() { return m_name; };

    //! Add new undo object to undo step.
    void AddUndoObject(IUndoObject* o)
    {
        m_undoObjects.push_back(o);
    }
    void ClearObjects()
    {
        int i;
        // Release all undo objects.
        std::vector<IUndoObject*> undoObjects = m_undoObjects;
        m_undoObjects.clear();

        for (i = 0; i < undoObjects.size(); i++)
        {
            undoObjects[i]->Release();
        }
    };
    virtual int GetSize() const
    {
        int size = 0;
        for (int i = 0; i < m_undoObjects.size(); i++)
        {
            size += m_undoObjects[i]->GetSize();
        }
        return size;
    }
    // Confetti: Get the size of m_undoObjects
    virtual int GetCount() const
    {
        return static_cast<int>(m_undoObjects.size());
    }
    virtual bool IsEmpty() const { return m_undoObjects.empty(); };
    virtual void Undo(bool bUndo)
    {
        for (int i = static_cast<int>(m_undoObjects.size() - 1); i >= 0; i--)
        {
            m_undoObjects[i]->Undo(bUndo);
        }
    }
    virtual void Redo()
    {
        for (int i = 0; i < m_undoObjects.size(); i++)
        {
            m_undoObjects[i]->Redo();
        }
    }

    // get undo object at index i
    IUndoObject* GetUndoObject(int i = 0)
    {
        if (i < m_undoObjects.size())
        {
            return m_undoObjects[i];
        }
        return nullptr;
    }

    const QString GetObjectNames()
    {
        QString objNamesStr("");
        std::vector<QString> objNames;

        bool bFirst = true;
        for (int i = 0; i < m_undoObjects.size(); ++i)
        {
            if (!m_undoObjects[i])
            {
                continue;
            }

            if (m_undoObjects[i]->GetObjectName() == nullptr)
            {
                continue;
            }

            if (stl::find(objNames, m_undoObjects[i]->GetObjectName()))
            {
                continue;
            }

            if (bFirst == false)
            {
                objNamesStr += ",";
            }
            else
            {
                bFirst = false;
            }

            objNamesStr += m_undoObjects[i]->GetObjectName();

            objNames.push_back(m_undoObjects[i]->GetObjectName());
        }

        return objNamesStr;
    };

private: // ------------------------------------------------------

    friend class CUndoManager;
    QString m_name;
    // Undo objects registered for this step.
    std::vector<IUndoObject*> m_undoObjects;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/*!
 *  CUndoManager is keeping and operating on CUndo class instances.
 */
class EDITOR_CORE_API CUndoManager
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CUndoManager();
    ~CUndoManager();

    //! Begin operation requiring undo.
    //! Undo manager enters holding state.
    void Begin();
    //! Restore all undo objects registered since last Begin call.
    //! @param bUndo if true all Undo object registered up to this point will be undone.
    void Restore(bool bUndo);
    //! Accept changes and registers an undo object with the undo manager.
    //! This will allow the user to undo the operation.
    void Accept(const QString& name);
    //! Cancel changes and restore undo objects.
    void Cancel();

    //! Normally this is NOT needed but in special cases this can be useful.
    //! This allows to group a set of Begin()/Accept() sequences to be undone in one operation.
    void SuperBegin();
    //! When a SuperBegin() used, this method is used to Accept.
    //! This leaves the undo database in its modified state and registers the IUndoObjects with the undo system.
    //! This will allow the user to undo the operation.
    void SuperAccept(const QString& name);
    //! Cancel changes and restore undo objects.
    void SuperCancel();

    //! Temporarily suspends recording of undo.
    void Suspend();
    //! Resume recording if was suspended.
    void Resume();

    // Undo last operation.
    void Undo(int numSteps = 1);

    // Undo a specific operation
    void Reset(const QString& name);

    //! Redo last undo.
    void Redo(int numSteps = 1);

    //! Check if undo information is recording now.
    bool IsUndoRecording() const;

    //////////////////////////////////////////////////////////////////////////
    bool IsUndoSuspended() const;

    //! Put new undo object, must be called between Begin and Accept/Cancel methods.
    void RecordUndo(IUndoObject* obj);

    bool IsHaveUndo() const;
    bool IsHaveRedo() const;

    void SetMaxUndoStep(int steps);
    int GetMaxUndoStep() const;

    //! Returns length of undo stack.
    int GetUndoStackLen() const;
    //! Returns length of redo stack.
    int GetRedoStackLen() const;

    //! Retreves array of undo objects names.
    std::vector<QString> GetUndoStackNames() const;
    //! Retreves array of redo objects names.
    std::vector<QString> GetRedoStackNames() const;

    //! Get size of current Undo and redo database size.
    int GetDatabaseSize();

    //! Completly flush all Undo and redo buffers.
    //! Must be done on level reloads or global Fetch operation.
    void Flush();

    //! Get Next Undo Item and Next Redo Item -- Vera, Confetti
    CUndoStep* GetNextUndo();
    CUndoStep* GetNextRedo();


    void ClearRedoStack();
    void ClearUndoStack();

    void ClearUndoStack(int num);
    void ClearRedoStack(int num);

    void AddListener(IUndoManagerListener* pListener);
    void RemoveListener(IUndoManagerListener* pListener);

    int GetSuspendCount() { return m_suspendCount; }

private: // ---------------------------------------------------------------

    void BeginUndoTransaction();
    void EndUndoTransaction();
    void BeginRestoreTransaction();
    void EndRestoreTransaction();
    void SignalNumUndoRedoToListeners();
    void SignalUndoFlushedToListeners();

    bool                                            m_bRecording;
    bool                                            m_bSuperRecording;
    int                                             m_suspendCount;

    bool                                            m_bUndoing;
    bool                                            m_bRedoing;

    bool                                            m_bClearRedoStackQueued;

    CUndoStep*                             m_currentUndo;
    //! Undo step object created by SuperBegin.
    CSuperUndoStep*                    m_superUndo;

    AssetManagerUndoInterruptor* m_assetManagerUndoInterruptor;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING

    std::list<CUndoStep*>      m_undoStack;
    std::list<CUndoStep*>      m_redoStack;

    std::vector<IUndoManagerListener*> m_listeners;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};


class CScopedSuspendUndo
{
public:
    CScopedSuspendUndo() { GetIEditor()->SuspendUndo(); }
    ~CScopedSuspendUndo() { GetIEditor()->ResumeUndo(); }
};

#endif // CRYINCLUDE_EDITORCORE_UNDO_UNDO_H
