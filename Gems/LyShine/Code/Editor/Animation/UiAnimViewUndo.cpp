/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiAnimViewUndo.h"
#include "UiAnimViewSequenceManager.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewEventNode.h"
#include "AnimationContext.h"

#include "UiEditorAnimationBus.h"

//////////////////////////////////////////////////////////////////////////
CUndoSequenceSettings::CUndoSequenceSettings(CUiAnimViewSequence* pSequence)
    : m_pSequence(pSequence)
    , m_oldTimeRange(pSequence->GetTimeRange())
    , m_oldFlags(pSequence->GetFlags())
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceSettings::Undo([[maybe_unused]] bool bUndo)
{
    m_newTimeRange = m_pSequence->GetTimeRange();
    m_newFlags = m_pSequence->GetFlags();

    m_pSequence->SetTimeRange(m_oldTimeRange);
    m_pSequence->SetFlags(m_oldFlags);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceSettings::Redo()
{
    m_pSequence->SetTimeRange(m_newTimeRange);
    m_pSequence->SetFlags(m_newFlags);
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimKeySelection::CUndoAnimKeySelection(CUiAnimViewSequence* pSequence)
    : m_pSequence(pSequence)
{
    // Stores the current state of this sequence.
    assert(pSequence);

    // Save sequence name and key selection states
    m_undoKeyStates = SaveKeyStates(pSequence);
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimKeySelection::CUndoAnimKeySelection(CUiAnimViewTrack* pTrack)
    : m_pSequence(pTrack->GetSequence())
{
    // This is called from CUndoTrackObject which will save key states itself
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimKeySelection::Undo(bool bUndo)
{
    {
        CUiAnimViewSequenceNoNotificationContext context(m_pSequence);

        // Save key selection states for redo if necessary
        if (bUndo)
        {
            m_redoKeyStates = SaveKeyStates(m_pSequence);
        }

        // Undo key selection state
        RestoreKeyStates(m_pSequence, m_undoKeyStates);
    }

    if (bUndo)
    {
        m_pSequence->OnKeySelectionChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimKeySelection::Redo()
{
    // Redo key selection state
    RestoreKeyStates(m_pSequence, m_redoKeyStates);
}

//////////////////////////////////////////////////////////////////////////
std::vector<bool> CUndoAnimKeySelection::SaveKeyStates(CUiAnimViewSequence* pSequence) const
{
    CUiAnimViewKeyBundle keys = pSequence->GetAllKeys();
    const unsigned int numkeys = keys.GetKeyCount();

    std::vector<bool> selectionState;
    selectionState.reserve(numkeys);

    for (unsigned int i = 0; i < numkeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = keys.GetKey(i);
        selectionState.push_back(keyHandle.IsSelected());
    }

    return selectionState;
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimKeySelection::RestoreKeyStates(CUiAnimViewSequence* pSequence, const std::vector<bool> keyStates)
{
    CUiAnimViewKeyBundle keys = pSequence->GetAllKeys();
    const unsigned int numkeys = keys.GetKeyCount();

    assert(numkeys <= keyStates.size());

    CUiAnimViewSequenceNotificationContext context(pSequence);
    for (unsigned int i = 0; i < numkeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = keys.GetKey(i);
        keyHandle.Select(keyStates[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUndoAnimKeySelection::IsSelectionChanged() const
{
    const std::vector<bool> currentKeyState = SaveKeyStates(m_pSequence);
    return m_undoKeyStates != currentKeyState;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUndoTrackObject::CUndoTrackObject(CUiAnimViewTrack* pTrack, bool bStoreKeySelection)
    : CUndoAnimKeySelection(pTrack)
    , m_pTrack(pTrack)
    , m_bStoreKeySelection(bStoreKeySelection)
{
    assert(pTrack);

    if (m_bStoreKeySelection)
    {
        m_undoKeyStates = SaveKeyStates(m_pSequence);
    }

    // Stores the current state of this track.
    assert(pTrack != 0);

    // Store undo info.
    m_undo = pTrack->GetMemento();
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackObject::Undo(bool bUndo)
{
    assert(m_pSequence);

    {
        CUiAnimViewSequenceNoNotificationContext context(m_pSequence);

        if (bUndo)
        {
            m_redo = m_pTrack->GetMemento();

            if (m_bStoreKeySelection)
            {
                // Save key selection states for redo if necessary
                m_redoKeyStates = SaveKeyStates(m_pSequence);
            }
        }

        // Undo track state.
        m_pTrack->RestoreFromMemento(m_undo);

        if (m_bStoreKeySelection)
        {
            // Undo key selection state
            RestoreKeyStates(m_pSequence, m_undoKeyStates);
        }
    }

    if (bUndo)
    {
        m_pSequence->OnKeysChanged();
    }
    else
    {
        m_pSequence->ForceAnimation();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackObject::Redo()
{
    assert(m_pSequence);

    // Redo track state.
    m_pTrack->RestoreFromMemento(m_redo);

    if (m_bStoreKeySelection)
    {
        // Redo key selection state
        RestoreKeyStates(m_pSequence, m_redoKeyStates);
    }

    m_pSequence->OnKeysChanged();
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoSequenceTransaction::CAbstractUndoSequenceTransaction(CUiAnimViewSequence* pSequence)
    : m_pSequence(pSequence)
{
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoSequenceTransaction::AddSequence()
{
    CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();

    IUiAnimationSystem* pUiAnimationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(pUiAnimationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    // Add sequence back to UI Animation system
    pUiAnimationSystem->AddSequence(m_pSequence->m_pAnimSequence.get());

    // Release ownership and add node back to UI Animation system
    m_pStoredUiAnimViewSequence.release();
    pSequenceManager->m_sequences.push_back(std::unique_ptr<CUiAnimViewSequence>(m_pSequence));

    pSequenceManager->OnSequenceAdded(m_pSequence);
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoSequenceTransaction::RemoveSequence(bool bAquireOwnership)
{
    CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();

    for (auto iter = pSequenceManager->m_sequences.begin(); iter != pSequenceManager->m_sequences.end(); ++iter)
    {
        std::unique_ptr<CUiAnimViewSequence>& currentSequence = *iter;

        if (currentSequence.get() == m_pSequence)
        {
            if (bAquireOwnership)
            {
                // Acquire ownership of sequence
                currentSequence.swap(m_pStoredUiAnimViewSequence);
                assert(m_pStoredUiAnimViewSequence.get());
            }

            // Remove from UI Animation system and UiAnimView
            pSequenceManager->m_sequences.erase(iter);
            IUiAnimationSystem* pUiAnimationSystem = nullptr;
            UiEditorAnimationBus::BroadcastResult(pUiAnimationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);
            pUiAnimationSystem->RemoveSequence(m_pSequence->m_pAnimSequence.get());

            break;
        }
    }

    pSequenceManager->OnSequenceRemoved(m_pSequence);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceAdd::Undo(bool bUndo)
{
    RemoveSequence(bUndo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceAdd::Redo()
{
    AddSequence();
}

//////////////////////////////////////////////////////////////////////////
CUndoSequenceRemove::CUndoSequenceRemove(CUiAnimViewSequence* pRemovedSequence)
    : CAbstractUndoSequenceTransaction(pRemovedSequence)
{
    RemoveSequence(true);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceRemove::Undo([[maybe_unused]] bool bUndo)
{
    AddSequence();
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceRemove::Redo()
{
    RemoveSequence(true);
}

//////////////////////////////////////////////////////////////////////////
CUndoSequenceChange::CUndoSequenceChange(CUiAnimViewSequence* oldSequence, CUiAnimViewSequence* newSequence)
    : m_oldSequence(oldSequence)
    , m_newSequence(newSequence)
{
}


//////////////////////////////////////////////////////////////////////////
void CUndoSequenceChange::ChangeSequence(CUiAnimViewSequence* sequence)
{
    CUiAnimationContext* animContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(animContext, &UiEditorAnimationBus::Events::GetAnimationContext);

    if (animContext)
    {
        animContext->SetSequence(sequence, false, false);
    }
    else
    {
        AZ_Assert(false, "Active UI animation sequence failed to be changed.");
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceChange::Undo(bool undo)
{
    AZ_UNUSED(undo);
    ChangeSequence(m_oldSequence);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceChange::Redo()
{
    ChangeSequence(m_newSequence);
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoAnimNodeTransaction::CAbstractUndoAnimNodeTransaction(CUiAnimViewAnimNode* pNode)
    : m_pParentNode(static_cast<CUiAnimViewAnimNode*>(pNode->GetParentNode()))
    , m_pNode(pNode)
{
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoAnimNodeTransaction::AddNode()
{
    // Add node back to sequence
    m_pParentNode->m_pAnimSequence->AddNode(m_pNode->m_pAnimNode.get());

    // Release ownership and add node back to parent node
    m_pStoredUiAnimViewNode.release();
    m_pParentNode->AddNode(m_pNode);

    m_pNode->BindToEditorObjects();
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoAnimNodeTransaction::RemoveNode(bool bAquireOwnership)
{
    m_pNode->UnBindFromEditorObjects();

    for (auto iter = m_pParentNode->m_childNodes.begin(); iter != m_pParentNode->m_childNodes.end(); ++iter)
    {
        std::unique_ptr<CUiAnimViewNode>& currentNode = *iter;

        if (currentNode.get() == m_pNode)
        {
            if (bAquireOwnership)
            {
                // Acquire ownership of node
                currentNode.swap(m_pStoredUiAnimViewNode);
                assert(m_pStoredUiAnimViewNode.get());
            }

            // Remove from parent node and sequence
            m_pParentNode->m_childNodes.erase(iter);
            m_pParentNode->m_pAnimSequence->RemoveNode(m_pNode->m_pAnimNode.get());

            break;
        }
    }

    m_pNode->GetSequence()->OnNodeChanged(m_pNode, IUiAnimViewSequenceListener::eNodeChangeType_Removed);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeAdd::Undo(bool bUndo)
{
    RemoveNode(bUndo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeAdd::Redo()
{
    AddNode();
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeRemove::CUndoAnimNodeRemove(CUiAnimViewAnimNode* pRemovedNode)
    : CAbstractUndoAnimNodeTransaction(pRemovedNode)
{
    RemoveNode(true);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRemove::Undo([[maybe_unused]] bool bUndo)
{
    AddNode();
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRemove::Redo()
{
    RemoveNode(true);
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoTrackTransaction::CAbstractUndoTrackTransaction(CUiAnimViewTrack* pTrack)
    : m_pParentNode(pTrack->GetAnimNode())
    , m_pTrack(pTrack)
{
    assert(!pTrack->IsSubTrack());
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoTrackTransaction::AddTrack()
{
    // Add node back to sequence
    m_pParentNode->m_pAnimNode->AddTrack(m_pTrack->m_pAnimTrack.get());

    // Add track back to parent node and release ownership
    CUiAnimViewNode* pTrack = m_pStoredUiAnimViewTrack.release();
    m_pParentNode->AddNode(pTrack);
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoTrackTransaction::RemoveTrack(bool bAquireOwnership)
{
    for (auto iter = m_pParentNode->m_childNodes.begin(); iter != m_pParentNode->m_childNodes.end(); ++iter)
    {
        std::unique_ptr<CUiAnimViewNode>& currentNode = *iter;

        if (currentNode.get() == m_pTrack)
        {
            if (bAquireOwnership)
            {
                // Acquire ownership of track
                currentNode.swap(m_pStoredUiAnimViewTrack);
            }

            // Remove from parent node and sequence
            m_pParentNode->m_childNodes.erase(iter);
            m_pParentNode->m_pAnimNode->RemoveTrack(m_pTrack->m_pAnimTrack.get());

            break;
        }
    }

    m_pParentNode->GetSequence()->OnNodeChanged(m_pStoredUiAnimViewTrack.get(), IUiAnimViewSequenceListener::eNodeChangeType_Removed);
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackAdd::Undo(bool bUndo)
{
    RemoveTrack(bUndo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackAdd::Redo()
{
    AddTrack();
}

//////////////////////////////////////////////////////////////////////////
CUndoTrackRemove::CUndoTrackRemove(CUiAnimViewTrack* pRemovedTrack)
    : CAbstractUndoTrackTransaction(pRemovedTrack)
{
    RemoveTrack(true);
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackRemove::Undo([[maybe_unused]] bool bUndo)
{
    AddTrack();
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackRemove::Redo()
{
    RemoveTrack(true);
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeReparent::CUndoAnimNodeReparent(CUiAnimViewAnimNode* pAnimNode, CUiAnimViewAnimNode* pNewParent)
    : CAbstractUndoAnimNodeTransaction(pAnimNode)
    , m_pNewParent(pNewParent)
    , m_pOldParent(m_pParentNode)
{
#if !defined(NDEBUG)
    CUiAnimViewSequence* pSequence =
#endif
        pAnimNode->GetSequence();
    assert(pSequence == m_pNewParent->GetSequence() && pSequence == m_pOldParent->GetSequence());

    Reparent(pNewParent);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeReparent::Undo([[maybe_unused]] bool bUndo)
{
    Reparent(m_pOldParent);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeReparent::Redo()
{
    Reparent(m_pNewParent);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeReparent::Reparent(CUiAnimViewAnimNode* pNewParent)
{
    RemoveNode(true);
    m_pParentNode = pNewParent;
    m_pNode->m_pAnimNode->SetParent(pNewParent->m_pAnimNode.get());
    AddParentsInChildren(m_pNode);
    AddNode();

    // This undo object must never have ownership of the node
    assert(m_pStoredUiAnimViewNode.get() == nullptr);
}

void CUndoAnimNodeReparent::AddParentsInChildren(CUiAnimViewAnimNode* pCurrentNode)
{
    const uint numChildren = pCurrentNode->GetChildCount();

    for (uint childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pCurrentNode->GetChild(childIndex));

        if (pChildAnimNode->GetNodeType() != eUiAVNT_Track)
        {
            pChildAnimNode->m_pAnimNode->SetParent(pCurrentNode->m_pAnimNode.get());

            if (pChildAnimNode->GetChildCount() > 0 && pChildAnimNode->GetNodeType() != eUiAVNT_AnimNode)
            {
                AddParentsInChildren(pChildAnimNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeRename::CUndoAnimNodeRename(CUiAnimViewAnimNode* pNode, const AZStd::string& oldName)
    : m_pNode(pNode)
    , m_newName(pNode->GetName())
    , m_oldName(oldName)
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRename::Undo([[maybe_unused]] bool bUndo)
{
    m_pNode->SetName(m_oldName.c_str());
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRename::Redo()
{
    m_pNode->SetName(m_newName.c_str());
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoTrackEventTransaction::CAbstractUndoTrackEventTransaction(CUiAnimViewSequence* pSequence, const QString& eventName)
    : m_pSequence(pSequence)
    , m_eventName(eventName)
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventAdd::Undo(bool bUndo)
{
    AZ_UNUSED(bUndo);
    m_pSequence->RemoveTrackEvent(m_eventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventAdd::Redo()
{
    m_pSequence->AddTrackEvent(m_eventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
CUndoTrackEventRemove::CUndoTrackEventRemove(CUiAnimViewSequence* pSequence, const QString& eventName)
    : CAbstractUndoTrackEventTransaction(pSequence, eventName)
    , m_changedKeys(CUiAnimViewEventNode::GetTrackEventKeys(pSequence, eventName.toUtf8().data()))
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventRemove::Undo(bool bUndo)
{
    AZ_UNUSED(bUndo);
    const AZStd::string rawName = m_eventName.toUtf8().data();

    m_pSequence->AddTrackEvent(rawName.c_str());

    const uint numKeys = m_changedKeys.GetKeyCount();
    for (uint keyIndex = 0; keyIndex < numKeys; ++keyIndex)
    {
        CUiAnimViewKeyHandle keyHandle = m_changedKeys.GetKey(keyIndex);
        IEventKey eventKey;

        keyHandle.GetKey(&eventKey);
        // re-set the eventKey with the m_eventName
        eventKey.event = rawName;
        keyHandle.SetKey(&eventKey);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventRemove::Redo()
{
    m_pSequence->RemoveTrackEvent(m_eventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
CUndoTrackEventRename::CUndoTrackEventRename(CUiAnimViewSequence* pSequence, const QString& eventName, const QString& newEventName)
    : CAbstractUndoTrackEventTransaction(pSequence, eventName)
    , m_newEventName(newEventName)
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventRename::Undo(bool bUndo)
{
    AZ_UNUSED(bUndo);
    m_pSequence->RenameTrackEvent(m_newEventName.toUtf8().data(), m_eventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventRename::Redo()
{
    m_pSequence->RenameTrackEvent(m_eventName.toUtf8().data(), m_newEventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoTrackEventMove::MoveUp()
{
    m_pSequence->MoveUpTrackEvent(m_eventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoTrackEventMove::MoveDown()
{
    m_pSequence->MoveDownTrackEvent(m_eventName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventMoveUp::Undo(bool bUndo)
{
    AZ_UNUSED(bUndo);
    MoveDown();
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventMoveUp::Redo()
{
    MoveUp();
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventMoveDown::Undo(bool bUndo)
{
    AZ_UNUSED(bUndo);
    MoveUp();
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackEventMoveDown::Redo()
{
    MoveDown();
}
