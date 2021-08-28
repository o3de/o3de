/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiAnimUndoManager.h"
#include "UiAnimUndoObject.h"
#include "Undo/IUndoManagerListener.h"
#include <CryCommon/StlUtils.h>

// UI Editor
#include "EditorCommon.h"
#include <QUndoCommand>

UiAnimUndoManager* UiAnimUndoManager::s_instance = nullptr;

//! UiAnimUndoStep is a collection of UiAnimUndoObjects instances that forms a single undo step.
class UiAnimUndoStep
    : public QUndoCommand
{
public:
    UiAnimUndoStep()
        : m_hasDoneUndo(false) {};
    virtual ~UiAnimUndoStep() { ClearObjects(); }

    //! Set undo object name.
    void SetName(const AZStd::string& name) { m_name = name; };
    //! Get undo object name.
    const AZStd::string& GetName() { return m_name; };

    //! Add new undo object to undo step.
    void AddUndoObject(UiAnimUndoObject* o)
    {
        m_undoObjects.push_back(o);
    }
    void ClearObjects()
    {
        int i;
        // Release all undo objects.
        for (i = 0; i < m_undoObjects.size(); i++)
        {
            m_undoObjects[i]->Release();
        }
        m_undoObjects.clear();
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
    virtual bool IsEmpty() const { return m_undoObjects.empty(); };
    virtual void Undo(bool bUndo)
    {
        for (int i = aznumeric_cast<int>(m_undoObjects.size()) - 1; i >= 0; i--)
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

    // Add to Qt undo stack
    void Push(UndoStack* undoStack)
    {
        setText(m_name.c_str());
        undoStack->push(this);
    }

private: // ------------------------------------------------------

    // these are called from the Qt undo system
    void undo() override
    {
        UiAnimUndoManager::Get()->UndoStep(this);
        m_hasDoneUndo = true;
    }

    void redo() override
    {
        // Qt automatically calls redo when a command is added to the Qt undo stack
        // We are emulating the CUndo behavior so we ingore that first redo
        if (m_hasDoneUndo)
        {
            UiAnimUndoManager::Get()->RedoStep(this);
        }
    }

    bool m_hasDoneUndo;

    friend class UiAnimUndoManager;
    AZStd::string m_name;
    // Undo objects registered for this step.
    std::vector<UiAnimUndoObject*> m_undoObjects;
};

//////////////////////////////////////////////////////////////////////////
UiAnimUndoManager::UiAnimUndoManager()
{
    m_bRecording = false;

    m_currentUndo = 0;

    m_suspendCount = 0;

    m_bUndoing = false;
    m_bRedoing = false;

    s_instance = this;
}

//////////////////////////////////////////////////////////////////////////
UiAnimUndoManager::~UiAnimUndoManager()
{
    m_bRecording = false;

    delete m_currentUndo;

    s_instance = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Begin()
{
    //CryLog( "<Undo> Begin SuspendCount=%d",m_suspendCount );
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    //  assert( m_bRecording == false );

    if (m_bRecording)
    {
        //CLogFile::WriteLine( "<Undo> Begin (already recording)" );
        // Not cancel, just combine.
        return;
    }

    // Begin Creates a new undo object.
    m_currentUndo = new UiAnimUndoStep;

    m_bRecording = true;
    //CLogFile::WriteLine( "<Undo> Begin OK" );
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Restore(bool bUndo)
{
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (m_currentUndo)
    {
        BeginRestoreTransaction();
        Suspend();
        if (bUndo && m_currentUndo)
        {
            m_currentUndo->Undo(false); // Undo not by Undo command (no need to store Redo)
        }
        Resume();
        if (m_currentUndo)
        {
            m_currentUndo->ClearObjects();
        }
        EndRestoreTransaction();
    }
    //CryLog( "Restore Undo" );
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Accept(const AZStd::string& name)
{
    //CryLog( "<Undo> Accept, Suspend Count=%d",m_suspendCount );
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (!m_bRecording)
    {
        //CLogFile::WriteLine( "<Undo> Accept (Not recording)" );
        return;
    }

    if (!m_currentUndo->IsEmpty())
    {
        m_currentUndo->SetName(name);
        {
            // push this undo stap on the Ui Editor undo stack
            m_currentUndo->Push(m_uiUndoStack);
        }
    }
    else
    {
        // If no any object was recorded, Cancel undo operation.
        Cancel();
    }

    m_bRecording = false;
    m_currentUndo = 0;

    //CLogFile::WriteLine( "<Undo> Accept OK" );
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Cancel()
{
    //CryLog( "<Undo> Cancel" );
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (!m_bRecording)
    {
        return;
    }

    assert(m_currentUndo != 0);

    m_bRecording = false;

    if (!m_currentUndo->IsEmpty())
    {
        // Restore all objects to the state they was at Begin call and throws out all undo objects.
        Restore(true);
    }

    delete m_currentUndo;
    m_currentUndo = 0;
    //CLogFile::WriteLine( "<Undo> Cancel OK" );
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Redo()
{
    // this is called when using the Redo menu/toolbar actions in the
    // UI Animation Editor. Just use the UI Editor redo
    m_uiUndoStack->redo();
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Undo()
{
    // this is called when using the undo menu/toolbar actions in the
    // UI Animation Editor. Just use the UI Editor undo
    m_uiUndoStack->undo();
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::RedoStep(UiAnimUndoStep* step)
{
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (m_bRecording)
    {
        //      GetIEditor()->GetLogFile()->FormatLine("Cannot Redo while Recording");
        return;
    }

    m_bRedoing = true;
    BeginUndoTransaction();
    m_bRedoing = false;

    Suspend();
    m_bRedoing = true;
    step->Redo();
    m_bRedoing = false;

    Resume();

    //  if (m_suspendCount == 0)
    //      GetIEditor()->UpdateViews(eUpdateObjects);

    m_bRedoing = true;
    EndUndoTransaction();
    m_bRedoing = false;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::UndoStep(UiAnimUndoStep* step)
{
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (m_bRecording)
    {
        //      GetIEditor()->GetLogFile()->FormatLine("Cannot Undo while Recording");
        return;
    }

    m_bUndoing = true;
    BeginUndoTransaction();
    m_bUndoing = false;

    Suspend();
    m_bUndoing = true;
    step->Undo(true);
    m_bUndoing = false;
    Resume();

    // Update viewports.
    //    if (m_suspendCount == 0)
    //        GetIEditor()->UpdateViews(eUpdateObjects);

    m_bUndoing = true;
    EndUndoTransaction();
    m_bUndoing = false;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::RecordUndo(UiAnimUndoObject* obj)
{
    //CryLog( "<Undo> RecordUndo Name=%s",obj->GetDescription() );

    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        //CLogFile::WriteLine( "<Undo> RecordUndo (Undoing or Redoing)" );
        obj->Release();
        return;
    }

    if (m_bRecording && (m_suspendCount == 0))
    {
        assert(m_currentUndo != 0);
        m_currentUndo->AddUndoObject(obj);
        //CLogFile::FormatLine( "Undo Object Added: %s",obj->GetDescription() );
    }
    else
    {
        //CLogFile::WriteLine( "<Undo> RecordUndo (Not Recording)" );
        // Ignore this object.
        obj->Release();
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Suspend()
{
    m_suspendCount++;
    //CLogFile::FormatLine( "<Undo> Suspend %d",m_suspendCount );
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Resume()
{
    assert(m_suspendCount >= 0);
    if (m_suspendCount > 0)
    {
        m_suspendCount--;
    }
    //CLogFile::FormatLine( "<Undo> Resume %d",m_suspendCount );
}

//////////////////////////////////////////////////////////////////////////
void UiAnimUndoManager::Flush()
{
    m_bRecording = false;

    delete m_currentUndo;
    m_currentUndo = 0;
}

void UiAnimUndoManager::AddListener(IUndoManagerListener* pListener)
{
    stl::push_back_unique(m_listeners, pListener);
}

void UiAnimUndoManager::RemoveListener(IUndoManagerListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}

void UiAnimUndoManager::BeginUndoTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->BeginUndoTransaction();
    }
}

void UiAnimUndoManager::EndUndoTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->EndUndoTransaction();
    }
}

void UiAnimUndoManager::BeginRestoreTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->BeginRestoreTransaction();
    }
}

void UiAnimUndoManager::EndRestoreTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->EndRestoreTransaction();
    }
}

bool UiAnimUndoManager::IsUndoRecording() const
{
    return (m_bRecording) && m_suspendCount == 0;
}

bool UiAnimUndoManager::IsUndoSuspended() const
{
    return m_suspendCount != 0;
}

void UiAnimUndoManager::SetActiveUndoStack(UndoStack* undoStack)
{
    m_uiUndoStack = undoStack;
}

UndoStack* UiAnimUndoManager::GetActiveUndoStack() const
{
    return m_uiUndoStack;
}

UiAnimUndoManager* UiAnimUndoManager::Get()
{
    return s_instance;
}
