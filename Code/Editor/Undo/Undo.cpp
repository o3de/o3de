/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "Undo.h"

#include "Settings.h"
#include "IUndoManagerListener.h"
#include <list>

#include <QString>
#include "QtUtilWin.h"

#define UNDOREDO_BUTTON_POPUP_TEXT_WIDTH 81
#define UNDOREDO_MULTIPLE_OBJECTS_TEXT " (Multiple Objects)"


//! CSuperUndo objects groups together a block of UndoStepto to allow them to be Undo by single operation.
class CSuperUndoStep
    : public CUndoStep
{
public:
    //! Add new undo object to undo step.
    void AddUndoStep(CUndoStep* step)
    {
        m_undoSteps.push_back(step);
    }
    int GetSize() const override
    {
        int size = 0;
        for (int i = 0; i < m_undoSteps.size(); i++)
        {
            size += m_undoSteps[i]->GetSize();
        }
        return size;
    }
    bool IsEmpty() const override
    {
        return m_undoSteps.empty();
    }
    void Undo(bool bUndo) override
    {
        for (int i = static_cast<int>(m_undoSteps.size()) - 1; i >= 0; i--)
        {
            m_undoSteps[i]->Undo(bUndo);
        }
    }
    void Redo() override
    {
        for (int i = 0; i < m_undoSteps.size(); i++)
        {
            m_undoSteps[i]->Redo();
        }
    }

private:
    //! Undo steps included in this step.
    std::vector<CUndoStep*> m_undoSteps;
};

// Helper class for CUndoManager that monitors the Asset Manager and suspends undo recording while the Asset Manager
// is processing asset loading events.  The events are processed non-deterministically, so they could accidentally get captured
// within an undo recording block.
class AssetManagerUndoInterruptor
    : public AZ::Data::AssetManagerNotificationBus::Handler
{
public:
    AssetManagerUndoInterruptor()
    {
        AZ::Data::AssetManagerNotificationBus::Handler::BusConnect();
    }

    ~AssetManagerUndoInterruptor() override
    {
        AZ::Data::AssetManagerNotificationBus::Handler::BusDisconnect();
    }

    void OnAssetEventsDispatchBegin() override
    {
        GetIEditor()->GetUndoManager()->Suspend();
    }

    void OnAssetEventsDispatchEnd() override
    {
        GetIEditor()->GetUndoManager()->Resume();
    }
};


//////////////////////////////////////////////////////////////////////////
CUndoManager::CUndoManager()
{
    m_bRecording = false;
    m_bSuperRecording = false;

    m_currentUndo = nullptr;
    m_superUndo = nullptr;
    m_assetManagerUndoInterruptor = new AssetManagerUndoInterruptor();

    m_suspendCount = 0;

    m_bUndoing = false;
    m_bRedoing = false;
}

//////////////////////////////////////////////////////////////////////////
CUndoManager::~CUndoManager()
{

    m_bRecording = false;
    ClearRedoStack();
    ClearUndoStack();

    delete m_superUndo;
    delete m_currentUndo;
    delete m_assetManagerUndoInterruptor;
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Begin()
{
    //CryLog( "<Undo> Begin SuspendCount=%d",m_suspendCount );
    //if (m_bSuperRecording)
    //CLogFile::FormatLine( "<Undo> Begin (Inside SuperSuper)" );
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
    m_currentUndo = new CUndoStep;

    m_bRecording = true;
    //CLogFile::WriteLine( "<Undo> Begin OK" );
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Restore(bool bUndo)
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

// This function is used below to decide if an operation should force a save or not. This currently
// prevents selecting an entity, either from the outliner or both the old and new viewports.
static bool ShouldPersist(const QString& name)
{
    return name != "Select Object(s)" && name != "Select Entity" && name != "Box Select Entities";
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Accept(const QString& name)
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
        const bool persist = ShouldPersist(name);
        if (persist)
        {
            GetIEditor()->SetModifiedFlag();
        }

        // If accepting new undo object, must clear all redo stack.
        ClearRedoStack();

        m_currentUndo->SetName(name);
        if (m_bSuperRecording)
        {
            m_superUndo->AddUndoStep(m_currentUndo);
        }
        else
        {
            // Normal recording.
            // Keep max undo steps.
            while (m_undoStack.size() && (m_undoStack.size() >= GetIEditor()->GetEditorSettings()->undoLevels || GetDatabaseSize() > 100 * 1024 * 1024))
            {
                delete m_undoStack.front();
                m_undoStack.pop_front();
            }
            m_undoStack.push_back(m_currentUndo);
        }
        //CLogFile::FormatLine( "Undo Object Accepted (Undo:%d,Redo:%d, Size=%dKb)",m_undoStack.size(),m_redoStack.size(),GetDatabaseSize()/1024 );

        // If undo accepted, document modified.
        if (persist)
        {
            GetIEditor()->SetModifiedFlag();
        }

        if (name.compare("Select Object(s)", Qt::CaseInsensitive) == 0)
        {
            GetIEditor()->SetModifiedModule(eModifiedBrushes);
        }
        else if (name.compare("Move Selection", Qt::CaseInsensitive) == 0)
        {
            GetIEditor()->SetModifiedModule(eModifiedBrushes);
        }
        else if (name.compare("SubObject Select", Qt::CaseInsensitive) == 0)
        {
            GetIEditor()->SetModifiedModule(eModifiedBrushes);
        }
        else if (name.compare("Manipulator Drag", Qt::CaseInsensitive) == 0)
        {
            GetIEditor()->SetModifiedModule(eModifiedBrushes);
        }
    }
    else
    {
        // If no any object was recorded, Cancel undo operation.
        Cancel();
    }

    m_bRecording = false;
    m_currentUndo = nullptr;

    SignalNumUndoRedoToListeners();

    //CLogFile::WriteLine( "<Undo> Accept OK" );
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Cancel()
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
        //GetIEditor()->GetLogFile()->FormatLine( "Undo Object Canceled (Undo:%d,Redo:%d)",m_undoStack.size(),m_redoStack.size() );
    }

    delete m_currentUndo;
    m_currentUndo = nullptr;
    //CLogFile::WriteLine( "<Undo> Cancel OK" );
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Redo(int numSteps)
{
    GetIEditor()->Notify(eNotify_OnBeginUndoRedo);

    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (m_bRecording || m_bSuperRecording)
    {
        AZ_Warning("CUndoManager", false, "Cannot Redo while Recording");
        return;
    }

    m_bRedoing = true;
    BeginUndoTransaction();
    m_bRedoing = false;

    if (!m_redoStack.empty())
    {
        Suspend();
        while (numSteps-- > 0 && !m_redoStack.empty() && !m_bClearRedoStackQueued)
        {
            m_bRedoing = true;
            CUndoStep* redo = m_redoStack.back();
            redo->Redo();

            AZ_Printf("CUndoManager",
                "(Undo: %d, Redo: %d) - Redo last operation: '%s'",
                m_undoStack.size(),
                m_redoStack.size(),
                redo->GetName().toUtf8().constData());

            m_redoStack.pop_back();
            // Push undo object to redo stack.
            m_undoStack.push_back(redo);
            m_bRedoing = false;
        }
        Resume();
    }
    if (m_suspendCount == 0)
    {
        GetIEditor()->UpdateViews(eUpdateObjects);
    }

    m_bRedoing = true;
    EndUndoTransaction();
    SignalNumUndoRedoToListeners();
    m_bRedoing = false;

    GetIEditor()->Notify(eNotify_OnEndUndoRedo);

    if (m_bClearRedoStackQueued)
    {
        ClearRedoStack();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Undo(int numSteps)
{
    GetIEditor()->Notify(eNotify_OnBeginUndoRedo);

    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (m_bRecording || m_bSuperRecording)
    {
        AZ_Warning("CUndoManager", false, "Cannot Undo while Recording");
        return;
    }

    m_bUndoing = true;
    BeginUndoTransaction();
    m_bUndoing = false;

    if (!m_undoStack.empty())
    {
        Suspend();
        while (numSteps-- > 0 && !m_undoStack.empty())
        {
            m_bUndoing = true;
            CUndoStep* undo = m_undoStack.back();
            undo->Undo(true);

            AZ_Printf("CUndoManager",
                "(Undo: %d, Redo: %d) - Undo last operation: '%s'",
                m_undoStack.size(),
                m_redoStack.size(),
                undo->GetName().toUtf8().constData());

            m_undoStack.pop_back();
            // Push undo object to redo stack.
            m_redoStack.push_back(undo);
            m_bUndoing = false;
        }
        Resume();
    }
    // Update viewports.
    if (m_suspendCount == 0)
    {
        GetIEditor()->UpdateViews(eUpdateObjects);
    }

    m_bUndoing = true;
    EndUndoTransaction();
    SignalNumUndoRedoToListeners();
    m_bUndoing = false;

    GetIEditor()->Notify(eNotify_OnEndUndoRedo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::RecordUndo(IUndoObject* obj)
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
void CUndoManager::ClearRedoStack()
{
    if (m_bRedoing)
    {
        m_bClearRedoStackQueued = true;
        return;
    }
    m_bClearRedoStackQueued = false;

    for (std::list<CUndoStep*>::iterator it = m_redoStack.begin(); it != m_redoStack.end(); it++)
    {
        delete *it;
    }
    m_redoStack.clear();

    SignalNumUndoRedoToListeners();
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::ClearUndoStack()
{
    for (std::list<CUndoStep*>::iterator it = m_undoStack.begin(); it != m_undoStack.end(); it++)
    {
        delete *it;
    }
    m_undoStack.clear();

    SignalNumUndoRedoToListeners();
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::ClearUndoStack(int num)
{
    int i = num;
    while (i > 0 && !m_undoStack.empty())
    {
        delete m_undoStack.front();
        m_undoStack.pop_front();
        i--;
    }

    SignalNumUndoRedoToListeners();
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::ClearRedoStack(int num)
{
    int i = num;
    while (i > 0 && !m_redoStack.empty())
    {
        delete m_redoStack.back();
        m_redoStack.pop_back();
        i--;
    }

    SignalNumUndoRedoToListeners();
}

//////////////////////////////////////////////////////////////////////////
bool CUndoManager::IsHaveRedo() const
{
    return !m_redoStack.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CUndoManager::IsHaveUndo() const
{
    return !m_undoStack.empty();
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Suspend()
{
    m_suspendCount++;
    //CLogFile::FormatLine( "<Undo> Suspend %d",m_suspendCount );
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Resume()
{
    assert(m_suspendCount >= 0);
    if (m_suspendCount > 0)
    {
        m_suspendCount--;
    }
    //CLogFile::FormatLine( "<Undo> Resume %d",m_suspendCount );
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::SuperBegin()
{
    //CLogFile::FormatLine( "<Undo> SuperBegin (SuspendCount%d)",m_suspendCount );
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    m_bSuperRecording = true;
    m_superUndo = new CSuperUndoStep;
    //CLogFile::WriteLine( "<Undo> SuperBegin OK" );
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::SuperAccept(const QString& name)
{
    //CLogFile::WriteLine( "<Undo> SupperAccept" );
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (!m_bSuperRecording)
    {
        return;
    }

    assert(m_superUndo != 0);

    if (m_bRecording)
    {
        Accept(name);
    }

    if (!m_superUndo->IsEmpty())
    {
        m_superUndo->SetName(name);
        // Keep max undo steps.
        while (m_undoStack.size() && (m_undoStack.size() >= GetIEditor()->GetEditorSettings()->undoLevels || GetDatabaseSize() > 100 * 1024 * 1024))
        {
            delete m_undoStack.front();
            m_undoStack.pop_front();
        }
        m_undoStack.push_back(m_superUndo);
    }
    else
    {
        // If no any object was recorded, Cancel undo operation.
        SuperCancel();
    }

    //CLogFile::FormatLine( "Undo Object Accepted (Undo:%d,Redo:%d)",m_undoStack.size(),m_redoStack.size() );
    m_bSuperRecording = false;
    m_superUndo = nullptr;
    //CLogFile::WriteLine( "<Undo> SupperAccept OK" );

    SignalNumUndoRedoToListeners();
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::SuperCancel()
{
    //CLogFile::WriteLine( "<Undo> SuperCancel" );
    if (m_bUndoing || m_bRedoing) // If Undoing or redoing now, ignore this calls.
    {
        return;
    }

    if (!m_bSuperRecording)
    {
        return;
    }

    assert(m_superUndo != 0);

    if (m_bRecording)
    {
        Cancel();
    }

    Suspend();
    //! Undo all changes already made.
    m_superUndo->Undo(false); // Undo not by Undo command (no need to store Redo)
    Resume();

    m_bSuperRecording = false;
    delete m_superUndo;
    m_superUndo = nullptr;
    //CLogFile::WriteLine( "<Undo> SuperCancel OK" );
}


//////////////////////////////////////////////////////////////////////////
int CUndoManager::GetUndoStackLen() const
{
    return static_cast<int>(m_undoStack.size());
}

//////////////////////////////////////////////////////////////////////////
int CUndoManager::GetRedoStackLen() const
{
    return static_cast<int>(m_redoStack.size());
}

//////////////////////////////////////////////////////////////////////////
std::vector<QString> CUndoManager::GetUndoStackNames() const
{
    std::vector<QString> undos(m_undoStack.size());
    int i = 0;
    QString text;

    for (auto it = m_undoStack.begin(); it != m_undoStack.end(); it++)
    {
        QString objNames = (*it)->GetObjectNames();
        text = (*it)->GetName() + objNames;
        if (text.length() > UNDOREDO_BUTTON_POPUP_TEXT_WIDTH)
        {
            undos[i++] = (*it)->GetName() + UNDOREDO_MULTIPLE_OBJECTS_TEXT;
        }
        else
        {
            undos[i++] = (*it)->GetName() + (objNames.isEmpty() ? "" : " (" + objNames + ")");
        }
    }

    return undos;
}

//////////////////////////////////////////////////////////////////////////
std::vector<QString> CUndoManager::GetRedoStackNames() const
{
    std::vector<QString> redos(m_redoStack.size());
    int i = 0;
    QString text;

    for (auto it = m_redoStack.begin(); it != m_redoStack.end(); it++)
    {
        text = (*it)->GetName() + (*it)->GetObjectNames();
        if (text.length() > UNDOREDO_BUTTON_POPUP_TEXT_WIDTH)
        {
            redos[i++] = (*it)->GetName() + UNDOREDO_MULTIPLE_OBJECTS_TEXT;
        }
        else
        {
            redos[i++] = (*it)->GetName() + " (" + (*it)->GetObjectNames() + ")";
        }
    }

    return redos;
}

//////////////////////////////////////////////////////////////////////////
int CUndoManager::GetDatabaseSize()
{
    int size = 0;
    {
        for (std::list<CUndoStep*>::iterator it = m_undoStack.begin(); it != m_undoStack.end(); it++)
        {
            size += (*it)->GetSize();
        }
    }
    {
        for (std::list<CUndoStep*>::iterator it = m_redoStack.begin(); it != m_redoStack.end(); it++)
        {
            size += (*it)->GetSize();
        }
    }
    return size;
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::Flush()
{
    m_bRecording = false;
    ClearRedoStack();
    ClearUndoStack();

    delete m_superUndo;
    delete m_currentUndo;
    m_superUndo = nullptr;
    m_currentUndo = nullptr;

    SignalUndoFlushedToListeners();
}

//////////////////////////////////////////////////////////////////////////
CUndoStep* CUndoManager::GetNextUndo()
{
    if (!m_undoStack.empty())
    {
        return m_undoStack.back();
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUndoStep* CUndoManager::GetNextRedo()
{
    if (!m_redoStack.empty())
    {
        return m_redoStack.back();
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CUndoManager::SetMaxUndoStep(int steps)
{
    GetIEditor()->GetEditorSettings()->undoLevels = steps;
};

//////////////////////////////////////////////////////////////////////////
int CUndoManager::GetMaxUndoStep() const
{
    return GetIEditor()->GetEditorSettings()->undoLevels;
}

void CUndoManager::AddListener(IUndoManagerListener* pListener)
{
    stl::push_back_unique(m_listeners, pListener);
}

void CUndoManager::RemoveListener(IUndoManagerListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}

void CUndoManager::BeginUndoTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->BeginUndoTransaction();
    }
}

void CUndoManager::EndUndoTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->EndUndoTransaction();
    }
}

void CUndoManager::BeginRestoreTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->BeginRestoreTransaction();
    }
}

void CUndoManager::EndRestoreTransaction()
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->EndRestoreTransaction();
    }
}

void CUndoManager::SignalNumUndoRedoToListeners()
{
    for (IUndoManagerListener* listener : m_listeners)
    {
        listener->SignalNumUndoRedo(static_cast<unsigned int>(m_undoStack.size()), static_cast<unsigned int>(m_redoStack.size()));
    }
}

void CUndoManager::SignalUndoFlushedToListeners()
{
    for (IUndoManagerListener* listener : m_listeners)
    {
        listener->UndoStackFlushed();
    }
}

bool CUndoManager::IsUndoRecording() const
{
    return (m_bRecording || m_bSuperRecording) && m_suspendCount == 0;
}

bool CUndoManager::IsUndoSuspended() const
{
    return m_suspendCount != 0;
}

CScopedSuspendUndo::CScopedSuspendUndo()
{
    GetIEditor()->SuspendUndo();
}

CScopedSuspendUndo::~CScopedSuspendUndo()
{
    GetIEditor()->ResumeUndo();
}
