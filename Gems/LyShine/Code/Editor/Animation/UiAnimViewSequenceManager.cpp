/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiEditorAnimationBus.h"
#include <LyShine/UiEditorDLLBus.h>
#include "UiAnimViewSequenceManager.h"
#include "UiAnimViewUndo.h"
#include "AnimationContext.h"
#include "GameEngine.h"
#include <LyShine/Bus/UiCanvasBus.h>

CUiAnimViewSequenceManager* CUiAnimViewSequenceManager::s_instance = nullptr;

////////////////////////////////////////////////////////////////////////////
CUiAnimViewSequenceManager::CUiAnimViewSequenceManager()
    : m_nextSequenceId(0)
    , m_animationSystem(nullptr)
{
    GetIEditor()->RegisterNotifyListener(this);
    UiEditorAnimationBus::Handler::BusConnect();

    // get the undo stack from the UI editor, if no editor is active this will be null
    UndoStack* undoStack = nullptr;
    UiEditorDLLBus::BroadcastResult(undoStack, &UiEditorDLLBus::Events::GetActiveUndoStack);
    m_undoManager.SetActiveUndoStack(undoStack);

    s_instance = this;

    m_animationContext = new CUiAnimationContext;
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewSequenceManager::~CUiAnimViewSequenceManager()
{
    GetIEditor()->UnregisterNotifyListener(this);
    UiEditorAnimationBus::Handler::BusDisconnect();

    delete m_animationContext;

    s_instance = nullptr;
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::OnEditorNotifyEvent([[maybe_unused]] EEditorNotifyEvent event)
{
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence* CUiAnimViewSequenceManager::GetSequenceByName(QString name) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CUiAnimViewSequence* pSequence = (*iter).get();

        if (QString::fromUtf8(pSequence->GetName().c_str()) == name)
        {
            return pSequence;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence* CUiAnimViewSequenceManager::GetSequenceByAnimSequence(IUiAnimSequence* pAnimSequence) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CUiAnimViewSequence* pSequence = (*iter).get();

        if (pSequence->m_pAnimSequence == pAnimSequence)
        {
            return pSequence;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence* CUiAnimViewSequenceManager::GetSequenceByIndex(unsigned int index) const
{
    if (index >= m_sequences.size())
    {
        return nullptr;
    }

    return m_sequences[index].get();
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::CreateSequence(QString name)
{
    CUiAnimViewSequence* pExistingSequence = GetSequenceByName(name);
    if (pExistingSequence)
    {
        return;
    }

    UiAnimUndo undo("Create Animation Sequence");

    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    IUiAnimSequence* pNewUiAnimationSequence = animationSystem->CreateSequence(name.toUtf8().data(), false, ++m_nextSequenceId);
    CUiAnimViewSequence* pNewSequence = new CUiAnimViewSequence(pNewUiAnimationSequence);

    m_sequences.push_back(std::unique_ptr<CUiAnimViewSequence>(pNewSequence));
    UiAnimUndo::Record(new CUndoSequenceAdd(pNewSequence));

    SortSequences();
    OnSequenceAdded(pNewSequence);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::DeleteSequence(CUiAnimViewSequence* pSequence)
{
    assert(pSequence);

    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    uint32 animSequenceId = 0;
    IUiAnimSequence* animSequence = nullptr;

    if (pSequence)
    {
        animSequenceId = pSequence->GetSequenceId();
        animSequence = animationSystem->FindSequenceById(animSequenceId);

        // this is to solve an issue of mismatched calls to the sequence from the undo system
        pSequence->BeginUndoTransaction();

        UiAnimUndo undo("Delete Animation Sequence");
        UiAnimUndo::Record(new CUndoSequenceRemove(pSequence));

        SortSequences();
        OnSequenceRemoved(pSequence);
    }

    if (animSequence)
    {
        //        animSequence->SetOwner(nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::SortSequences()
{
    std::stable_sort(m_sequences.begin(), m_sequences.end(),
        [](const std::unique_ptr<CUiAnimViewSequence>& a, const std::unique_ptr<CUiAnimViewSequence>& b) -> bool
        {
            QString aName = QString::fromUtf8(a.get()->GetName().c_str());
            QString bName = QString::fromUtf8(b.get()->GetName().c_str());
            return aName < bName;
        });
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::OnSequenceAdded(CUiAnimViewSequence* pSequence)
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnSequenceAdded(pSequence);
    }

    UiAnimUndoManager::Get()->AddListener(pSequence);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::OnSequenceRemoved(CUiAnimViewSequence* pSequence)
{
    UiAnimUndoManager::Get()->RemoveListener(pSequence);

    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnSequenceRemoved(pSequence);
    }
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewSequenceManager* CUiAnimViewSequenceManager::GetSequenceManager()
{
    return s_instance;
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::Create()
{
    new CUiAnimViewSequenceManager;
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::Destroy()
{
    delete s_instance;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimationContext* CUiAnimViewSequenceManager::GetAnimationContext()
{
    return m_animationContext;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimationSystem* CUiAnimViewSequenceManager::GetAnimationSystem()
{
    return m_animationSystem;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence* CUiAnimViewSequenceManager::GetCurrentSequence()
{
    return m_animationContext->GetSequence();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::ActiveCanvasChanged()
{
    // Get the animation system from the active canvas, this must be updated if the UI Editor
    // switches to a different canvas
    AZ::EntityId canvasId;
    UiEditorDLLBus::BroadcastResult(canvasId, &UiEditorDLLBus::Events::GetActiveCanvasId);

    // Note: the canvasId may be invalid if the UI Editor window has closed in which case
    // the GetAnimationSystem event does nothing
    m_animationSystem = nullptr;
    UiCanvasBus::EventResult(m_animationSystem, canvasId, &UiCanvasBus::Events::GetAnimationSystem);

    // get the undo stack from the UI editor, if no editor is active this will be null
    UndoStack* undoStack = nullptr;
    UiEditorDLLBus::BroadcastResult(undoStack, &UiEditorDLLBus::Events::GetActiveUndoStack);
    UiAnimUndoManager::Get()->SetActiveUndoStack(undoStack);

    m_animationContext->ActiveCanvasChanged();

    DeleteAllSequences();
    CreateSequencesFromAnimationSystem();

    // tell listeners in UI animation system the active canvas has changed
    UiEditorAnimListenerBus::Broadcast(&UiEditorAnimListenerBus::Events::OnActiveCanvasChanged);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::DeleteAllSequences()
{
    // This is called when the active canvas changes
    while (m_sequences.size() > 0)
    {
        auto iterator = m_sequences.begin();
        CUiAnimViewSequence* pSequence = iterator->get();
        assert(pSequence);

        // UI_ANIMATION_REVISIT,some clients of OnSequenceRemoved expect the pSequence to be still valid
        // so call before the erase
        if (pSequence)
        {
            OnSequenceRemoved(pSequence);
        }

        m_sequences.erase(iterator);
    }

    // UI_ANIMATION_REVISIT,some clients of OnSequenceRemoved expect the pSequence to be removed
    // from the list already so call again
    OnSequenceRemoved(nullptr);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequenceManager::CreateSequencesFromAnimationSystem()
{
    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    if (!animationSystem)
    {
        return;
    }

    int numSequences = animationSystem->GetNumSequences();
    for (int sequenceIndex = 0; sequenceIndex < numSequences; ++sequenceIndex)
    {
        IUiAnimSequence* sequence = animationSystem->GetSequence(sequenceIndex);

        CUiAnimViewSequence* pNewSequence = new CUiAnimViewSequence(sequence);
        m_sequences.push_back(std::unique_ptr<CUiAnimViewSequence>(pNewSequence));

        pNewSequence->Load();

        SortSequences();
        OnSequenceAdded(pNewSequence);
    }
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewSequenceManager::GetAllRelatedAnimNodes(const AZ::Entity* pEntityObject) const
{
    CUiAnimViewAnimNodeBundle nodeBundle;

    const uint sequenceCount = GetCount();

    for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
    {
        CUiAnimViewSequence* pSequence = GetSequenceByIndex(sequenceIndex);
        nodeBundle.AppendAnimNodeBundle(pSequence->GetAllOwnedNodes(pEntityObject));
    }

    return nodeBundle;
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNode* CUiAnimViewSequenceManager::GetActiveAnimNode(const AZ::Entity* pEntityObject) const
{
    CUiAnimViewAnimNodeBundle nodeBundle = GetAllRelatedAnimNodes(pEntityObject);

    const uint nodeCount = nodeBundle.GetCount();
    for (uint nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
    {
        CUiAnimViewAnimNode* pAnimNode = nodeBundle.GetNode(nodeIndex);
        if (pAnimNode->IsActive())
        {
            return pAnimNode;
        }
    }

    return nullptr;
}
