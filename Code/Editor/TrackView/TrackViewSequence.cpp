/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "Entity/EditorEntityHelpers.h"
#include "TrackViewSequence.h"

// Qt
#include <QMessageBox>

// AzCore
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/sort.h>

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

// CryCommon
#include <CryCommon/Maestro/Types/AnimValueType.h>
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Bus/EditorSequenceComponentBus.h>
#include <CryCommon/MathConversion.h>

// Editor
#include "AnimationContext.h"
#include "Clipboard.h"
#include "TrackViewSequenceManager.h"
#include "TrackViewNodeFactories.h"


CTrackViewSequence::CTrackViewSequence(IAnimSequence* pSequence)
    : CTrackViewAnimNode(pSequence, nullptr, nullptr)
    , m_pAnimSequence(pSequence)
{
    AZ_Assert(m_pAnimSequence, "Expected valid m_pAnimSequence");
}

CTrackViewSequence::CTrackViewSequence(AZStd::intrusive_ptr<IAnimSequence>& sequence)
    : CTrackViewAnimNode(sequence.get(), nullptr, nullptr)
    , m_pAnimSequence(sequence)
{
}

CTrackViewSequence::~CTrackViewSequence()
{
    GetIEditor()->GetSequenceManager()->RemoveListener(this);
    GetIEditor()->GetUndoManager()->RemoveListener(this);       // For safety. Should be done by OnRemoveSequence callback

    // For safety, disconnect to any buses we may have been listening on for record mode
    if (m_pAnimSequence)
    {
        // disconnect from all EBuses for notification of changes for all AZ::Entities in our sequence
        for (int i = m_pAnimSequence->GetNodeCount(); --i >= 0;)
        {
            IAnimNode* animNode = m_pAnimSequence->GetNode(i);
            if (animNode->GetType() == AnimNodeType::AzEntity)
            {
                Maestro::EditorSequenceComponentRequestBus::Event(m_pAnimSequence->GetSequenceEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::RemoveEntityToAnimate, animNode->GetAzEntityId());
                ConnectToBusesForRecording(animNode->GetAzEntityId(), false);
            }
        }
    }
}

void CTrackViewSequence::Load()
{
    m_childNodes.clear();

    const int nodeCount = m_pAnimSequence->GetNodeCount();
    for (int i = 0; i < nodeCount; ++i)
    {
        IAnimNode* node = m_pAnimSequence->GetNode(i);

        // Only add top level nodes to sequence
        if (!node->GetParent())
        {
            CTrackViewAnimNodeFactory animNodeFactory;
            CTrackViewAnimNode* pNewTVAnimNode = animNodeFactory.BuildAnimNode(m_pAnimSequence.get(), node, this);
            m_childNodes.push_back(AZStd::unique_ptr<CTrackViewNode>(pNewTVAnimNode));
        }
    }

    SortNodes();
}

void CTrackViewSequence::BindToEditorObjects()
{
    m_bBoundToEditorObjects = true;
    CTrackViewAnimNode::BindToEditorObjects();
}

void CTrackViewSequence::UnBindFromEditorObjects()
{
    m_bBoundToEditorObjects = false;
    CTrackViewAnimNode::UnBindFromEditorObjects();
}

bool CTrackViewSequence::IsBoundToEditorObjects() const
{
    return m_bBoundToEditorObjects;
}

CTrackViewKeyHandle CTrackViewSequence::FindSingleSelectedKey()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return CTrackViewKeyHandle();
    }

    CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

    if (selectedKeys.GetKeyCount() != 1)
    {
        return CTrackViewKeyHandle();
    }

    return selectedKeys.GetKey(0);
}

void CTrackViewSequence::OnEntityComponentPropertyChanged(AZ::ComponentId changedComponentId)
{
    // find the component node for this changeComponentId if it exists
    for (int i = m_pAnimSequence->GetNodeCount(); --i >= 0;)
    {
        IAnimNode* animNode = m_pAnimSequence->GetNode(i);
        if (animNode && animNode->GetComponentId() == changedComponentId)
        {
            // we have a component animNode for this changedComponentId. Process the component change
            RecordTrackChangesForNode(static_cast<CTrackViewAnimNode*>(animNode->GetNodeOwner()));
        }
    }
}

CTrackViewTrack* CTrackViewSequence::FindTrackById(unsigned int trackId)
{
    CTrackViewTrack* result = nullptr;
    CTrackViewTrackBundle allTracks = GetAllTracks();

    int allTracksCount = allTracks.GetCount();
    for (int trackIndex = 0; trackIndex < allTracksCount; trackIndex++)
    {
        CTrackViewTrack* track = allTracks.GetTrack(trackIndex);
        AZ_Assert(track, "Expected valid track.");
        if (track->GetId() == trackId)
        {
            result = track;
            break;
        }
    }

    return result;
}

AZStd::vector<bool> CTrackViewSequence::SaveKeyStates() const
{
    // const hack because GetAllKeys();
    CTrackViewSequence* nonConstSequence = const_cast<CTrackViewSequence*>(this);
    CTrackViewKeyBundle keys = nonConstSequence->GetAllKeys();
    const unsigned int numkeys = keys.GetKeyCount();

    AZStd::vector<bool> selectionState;
    selectionState.reserve(numkeys);

    for (unsigned int i = 0; i < numkeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = keys.GetKey(i);
        selectionState.push_back(keyHandle.IsSelected());
    }

    return selectionState;
}

void CTrackViewSequence::RestoreKeyStates(const AZStd::vector<bool>& keyStates)
{
    CTrackViewKeyBundle keys = GetAllKeys();
    const unsigned int numkeys = keys.GetKeyCount();

    if (keyStates.size() >= numkeys)
    {
        CTrackViewSequenceNotificationContext context(this);
        for (unsigned int i = 0; i < numkeys; ++i)
        {
            CTrackViewKeyHandle keyHandle = keys.GetKey(i);
            keyHandle.Select(keyStates[i]);
        }
    }
}

void CTrackViewSequence::ConnectToBusesForRecording(const AZ::EntityId& entityId, bool enableConnection)
{
    // we connect to PropertyEditorEntityChangeNotificationBus for all other changes
    if (enableConnection)
    {
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(entityId);
    }
    else
    {
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(entityId);
    }
}

int CTrackViewSequence::RecordTrackChangesForNode(CTrackViewAnimNode* componentNode)
{
    int retNumKeysSet = 0;

    if (componentNode)
    {
        retNumKeysSet = componentNode->SetKeysForChangedTrackValues(GetIEditor()->GetAnimation()->GetTime());
        if (retNumKeysSet)
        {
            OnKeysChanged();    // change notification for updating TrackView UI
        }
    }

    return retNumKeysSet;
}

void CTrackViewSequence::SetRecording(bool enableRecording)
{
    if (m_pAnimSequence)
    {
        // connect (or disconnect) to EBuses for notification of changes for all AZ::Entities in our sequence
        for (int i = m_pAnimSequence->GetNodeCount(); --i >= 0;)
        {
            IAnimNode* animNode = m_pAnimSequence->GetNode(i);
            if (animNode->GetType() == AnimNodeType::AzEntity)
            {
                ConnectToBusesForRecording(animNode->GetAzEntityId(), enableRecording);
            }
        }
    }
}

bool CTrackViewSequence::IsAncestorOf(CTrackViewSequence* pSequence) const
{
    return m_pAnimSequence->IsAncestorOf(pSequence->m_pAnimSequence.get());
}

void CTrackViewSequence::BeginCutScene(const bool bResetFx) const
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

    IMovieUser* pMovieUser = movieSystem ? movieSystem->GetUser() : nullptr;

    if (pMovieUser)
    {
        pMovieUser->BeginCutScene(m_pAnimSequence.get(), m_pAnimSequence->GetCutSceneFlags(false), bResetFx);
    }
}

void CTrackViewSequence::EndCutScene() const
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

    IMovieUser* pMovieUser = movieSystem ? movieSystem->GetUser() : nullptr;

    if (pMovieUser)
    {
        pMovieUser->EndCutScene(m_pAnimSequence.get(), m_pAnimSequence->GetCutSceneFlags(true));
    }
}

void CTrackViewSequence::Render(const SAnimContext& animContext)
{
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
            pChildAnimNode->Render(animContext);
        }
    }

    m_pAnimSequence->Render();
}

void CTrackViewSequence::Animate(const SAnimContext& animContext)
{
    if (!m_pAnimSequence->IsActivated())
    {
        return;
    }

    m_time = animContext.time;

    m_pAnimSequence->Animate(animContext);

    CTrackViewSequenceNoNotificationContext context(this);
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
            pChildAnimNode->Animate(animContext);
        }
    }
}

void CTrackViewSequence::AddListener(ITrackViewSequenceListener* pListener)
{
    stl::push_back_unique(m_sequenceListeners, pListener);
}

void CTrackViewSequence::RemoveListener(ITrackViewSequenceListener* pListener)
{
    stl::find_and_erase(m_sequenceListeners, pListener);
}

void CTrackViewSequence::OnNodeSelectionChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bNodeSelectionChanged = true;
    }
    else
    {
        CTrackViewSequenceNoNotificationContext context(this);
        for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
        {
            (*iter)->OnNodeSelectionChanged(this);
        }
    }
}

void CTrackViewSequence::ForceAnimation()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bForceAnimation = true;
    }
    else
    {
        if (IsActive())
        {
            GetIEditor()->GetAnimation()->ForceAnimation();
        }
    }
}

void CTrackViewSequence::OnKeySelectionChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bKeySelectionChanged = true;
    }
    else
    {
        CTrackViewSequenceNoNotificationContext context(this);
        for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
        {
            (*iter)->OnKeySelectionChanged(this);
        }
    }
}

void CTrackViewSequence::OnKeysChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bKeysChanged = true;
    }
    else
    {
        CTrackViewSequenceNoNotificationContext context(this);
        for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
        {
            (*iter)->OnKeysChanged(this);
        }

        if (IsActive())
        {
            GetIEditor()->GetAnimation()->ForceAnimation();
        }
    }
}

void CTrackViewSequence::OnKeyAdded(CTrackViewKeyHandle& addedKeyHandle)
{
    if (m_bNoNotifications)
    {
        return;
    }

    CTrackViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnKeyAdded(addedKeyHandle);
    }
}

void CTrackViewSequence::OnNodeChanged(CTrackViewNode* node, ITrackViewSequenceListener::ENodeChangeType type)
{
    if (node && node->GetNodeType() == eTVNT_AnimNode)
    {
        // Deselect the node before deleting to give listeners a chance to update things like UI state.
        if (type == ITrackViewSequenceListener::eNodeChangeType_Removed)
        {
            CTrackViewSequenceNotificationContext context(this);

            // Make sure to deselect any keys
            CTrackViewKeyBundle keys = node->GetAllKeys();
            for (unsigned int key = 0; key < keys.GetKeyCount(); key++)
            {
                CTrackViewKeyHandle keyHandle = keys.GetKey(key);
                if (keyHandle.IsSelected())
                {
                    keyHandle.Select(false);
                    m_bKeySelectionChanged = true;
                }
            }

            // Cancel notification if nothing changed.
            if (!m_bKeySelectionChanged)
            {
                context.Cancel();
            }

            // deselect the node
            if (node->IsSelected())
            {
                node->SetSelected(false);
            }
        }

        CTrackViewAnimNode* pAnimNode = static_cast<CTrackViewAnimNode*>(node);
        if (pAnimNode->IsActive())
        {
            switch (type)
            {
            case ITrackViewSequenceListener::eNodeChangeType_Added:
                {
                    ForceAnimation();
                    // if we're in record mode and this is an AzEntity node, add the node to the buses we listen to for notification of changes
                    if (pAnimNode->GetType() == AnimNodeType::AzEntity && GetIEditor()->GetAnimation()->IsRecordMode())
                    {
                        ConnectToBusesForRecording(pAnimNode->GetAzEntityId(), true);
                    }
                }
                break;
            case ITrackViewSequenceListener::eNodeChangeType_Removed:
                {
                    ForceAnimation();
                    // if we're in record mode and this is an AzEntity node, remove the node to the buses we listen to for notification of changes
                    if (pAnimNode->GetType() == AnimNodeType::AzEntity && GetIEditor()->GetAnimation()->IsRecordMode())
                    {
                        ConnectToBusesForRecording(pAnimNode->GetAzEntityId(), false);
                    }
                }
                break;
            }
        }

        switch (type)
        {
        case ITrackViewSequenceListener::eNodeChangeType_Enabled:
        // Fall through
        case ITrackViewSequenceListener::eNodeChangeType_Hidden:
        // Fall through
        case ITrackViewSequenceListener::eNodeChangeType_SetAsActiveDirector:
        // Fall through
        case ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged:
            ForceAnimation();
            break;
        }
    }

    // Mark Layer with Sequence Object as dirty for non-internal or non-UI changes
    if (type != ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged &&
        type != ITrackViewSequenceListener::eNodeChangeType_Selected &&
        type != ITrackViewSequenceListener::eNodeChangeType_Deselected &&
        type != ITrackViewSequenceListener::eNodeChangeType_Collapsed &&
        type != ITrackViewSequenceListener::eNodeChangeType_Expanded)
    {
        MarkAsModified();
    }

    if (m_bNoNotifications)
    {
        return;
    }

    CTrackViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnNodeChanged(node, type);
    }
}

void CTrackViewSequence::OnNodeRenamed(CTrackViewNode* node, const char* pOldName)
{
    // Marks Layer with Sequence Object as dirty
    MarkAsModified();

    if (m_bNoNotifications)
    {
        return;
    }

    CTrackViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnNodeRenamed(node, pOldName);
    }
}

void CTrackViewSequence::OnSequenceSettingsChanged()
{
    MarkAsModified();

    if (m_bNoNotifications)
    {
        return;
    }

    CTrackViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnSequenceSettingsChanged(this);
    }
}

void CTrackViewSequence::MarkAsModified()
{
    if (m_pAnimSequence)
    {
        Maestro::EditorSequenceComponentRequestBus::Event(
            m_pAnimSequence->GetSequenceEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::MarkEntityAsDirty);
    }
}

void CTrackViewSequence::QueueNotifications()
{
    m_bQueueNotifications = true;
    ++m_selectionRecursionLevel;
}

void CTrackViewSequence::DequeueNotifications()
{
    AZ_Assert(m_selectionRecursionLevel > 0, "QueueNotifications should be called before DequeueNotifications()");
    --m_selectionRecursionLevel;
    if (m_selectionRecursionLevel == 0)
    {
        m_bQueueNotifications = false;
    }
}

void CTrackViewSequence::SubmitPendingNotifications(bool force)
{
    if (force)
    {
        m_selectionRecursionLevel = 1;
    }

    AZ_Assert(m_selectionRecursionLevel > 0, "Dangling SubmitPendingNotifications()");
    if (m_selectionRecursionLevel > 0)
    {
        --m_selectionRecursionLevel;
    }

    if (m_selectionRecursionLevel == 0)
    {
        m_bQueueNotifications = false;

        if (m_bNodeSelectionChanged)
        {
            OnNodeSelectionChanged();
        }

        if (m_bKeysChanged)
        {
            OnKeysChanged();
        }

        if (m_bKeySelectionChanged)
        {
            OnKeySelectionChanged();
        }

        if (m_bForceAnimation)
        {
            ForceAnimation();
        }

        m_bForceAnimation = false;
        m_bKeysChanged = false;
        m_bNodeSelectionChanged = false;
        m_bKeySelectionChanged = false;
    }
}

void CTrackViewSequence::OnSequenceRemoved(CTrackViewSequence* removedSequence)
{
    if (removedSequence == this)
    {
        // submit any queued notifications before removing
        if (m_bQueueNotifications)
        {
            SubmitPendingNotifications(true);
        }

        // remove ourselves as listeners from the undo manager
        GetIEditor()->GetUndoManager()->RemoveListener(this);
    }
}

void CTrackViewSequence::OnSequenceAdded(CTrackViewSequence* addedSequence)
{
    if (addedSequence == this)
    {
        GetIEditor()->GetUndoManager()->AddListener(this);
    }
}

void CTrackViewSequence::DeleteSelectedNodes()
{
    if (IsSelected())
    {
        GetIEditor()->GetSequenceManager()->DeleteSequence(this);
        return;
    }

    // Don't notify in the above IsSelected() case,
    // because 'this' will become deleted and invalid.
    CTrackViewSequenceNotificationContext context(this);

    CTrackViewAnimNodeBundle selectedNodes = GetSelectedAnimNodes();
    const unsigned int numSelectedNodes = selectedNodes.GetCount();

    // Call RemoveEntityToAnimate on any nodes that are able to be removed right here. If we wait to do it inside
    // of RemoveSubNode() it will fail because the EditorSequenceComponentRequestBus will be disconnected
    // by the Deactivate / Activate of the sequence entity.
    if (nullptr != m_pAnimSequence)
    {
        AZ::EntityId sequenceEntityId = m_pAnimSequence->GetSequenceEntityId();
        if (sequenceEntityId.IsValid())
        {
            for (unsigned int i = 0; i < numSelectedNodes; ++i)
            {
                AZ::EntityId removedNodeId = selectedNodes.GetNode(i)->GetAzEntityId();
                if (removedNodeId.IsValid())
                {
                    Maestro::EditorSequenceComponentRequestBus::Event(
                        m_pAnimSequence->GetSequenceEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::RemoveEntityToAnimate,
                        removedNodeId);
                }
            }
        }
    }

    // Deactivate the sequence entity while we are potentially removing things from it. 
    // We need to allow the full removal operation (node and children) to complete before 
    // OnActivate happens on the Sequence again. If we don't deactivate the sequence entity
    // OnActivate will get called by the entity system as components are removed.
    // In some cases this will erroneously cause some components to be added 
    // back to the sequence that were just deleted.
    bool sequenceEntityWasActive = false;
    AZ::Entity* sequenceEntity = nullptr;
    if (GetSequenceComponentEntityId().IsValid())
    {
        AZ::ComponentApplicationBus::BroadcastResult(sequenceEntity, &AZ::ComponentApplicationBus::Events::FindEntity, GetSequenceComponentEntityId());
        if (sequenceEntity != nullptr)
        {
            if (sequenceEntity->GetState() == AZ::Entity::State::Active)
            {
                sequenceEntityWasActive = true;
                sequenceEntity->Deactivate();
            }
        }
    }

    CTrackViewTrackBundle selectedTracks = GetSelectedTracks();
    const unsigned int numSelectedTracks = selectedTracks.GetCount();

    for (int i = numSelectedTracks - 1; i >= 0; i--)
    {
        CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);

        // Ignore sub tracks
        if (!pTrack->IsSubTrack())
        {
            pTrack->GetAnimNode()->RemoveTrack(pTrack);
        }
    }

    // GetSelectedAnimNodes() will add parent nodes first and then children to the selected
    // node bundle list. So iterating backwards here causes child nodes to be deleted first,
    // and then parents. If parent nodes get deleted first, node->GetParentNode() will return
    // a bad pointer if it happens to be one of the nodes that was deleted.
    for (int i = numSelectedNodes - 1; i >= 0; i--)
    {
        CTrackViewAnimNode* node = selectedNodes.GetNode(i);
        CTrackViewAnimNode* pParentNode = static_cast<CTrackViewAnimNode*>(node->GetParentNode());
        pParentNode->RemoveSubNode(node);
    }

    if (sequenceEntityWasActive && sequenceEntity != nullptr)
    {
        sequenceEntity->Activate();
    }
}

void CTrackViewSequence::SelectSelectedNodesInViewport()
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");

    CTrackViewAnimNodeBundle selectedNodes = GetSelectedAnimNodes();
    const unsigned int numSelectedNodes = selectedNodes.GetCount();

    AZStd::vector<AZ::EntityId> entitiesToBeSelected;
    for (unsigned int i = 0; i < numSelectedNodes; ++i)
    {
        CTrackViewAnimNode* node = selectedNodes.GetNode(i);
        ETrackViewNodeType nodeType = node->GetNodeType();

        if (nodeType == eTVNT_Sequence)
        {
            CTrackViewSequence* seqNode = static_cast<CTrackViewSequence*>(node);
            entitiesToBeSelected.push_back(seqNode->GetSequenceComponentEntityId());
        }
        else
        {
            // TrackView AnimNode
            entitiesToBeSelected.push_back(node->GetAzEntityId());
        }
    }

    // remove duplicate entities
    AZStd::sort(entitiesToBeSelected.begin(), entitiesToBeSelected.end());
    entitiesToBeSelected.erase(
        AZStd::unique(entitiesToBeSelected.begin(), entitiesToBeSelected.end()), entitiesToBeSelected.end());

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entitiesToBeSelected);
}

bool CTrackViewSequence::SetName(const char* name)
{
    // Check if there is already a sequence with that name
    const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
    if (pSequenceManager->GetSequenceByName(name))
    {
        return false;
    }

    AZStd::string oldName = GetName();
    if (name != oldName)
    {
        m_pAnimSequence->SetName(name);
        MarkAsModified();

        AzToolsFramework::ScopedUndoBatch undoBatch("Rename Sequence");
        GetSequence()->OnNodeRenamed(this, oldName.c_str());
        undoBatch.MarkEntityDirty(m_pAnimSequence->GetSequenceEntityId());
    }

    return true;
}

void CTrackViewSequence::DeleteSelectedKeys()
{
    CTrackViewSequenceNotificationContext context(this);
    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();
    for (int k = (int)selectedKeys.GetKeyCount() - 1; k >= 0; --k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(k);
        skey.Delete();
    }

    // The selected keys are deleted, so notify the selection was just changed.
    OnKeySelectionChanged();
}

void CTrackViewSequence::CopyKeysToClipboard(const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    XmlNodeRef copyNode = XmlHelpers::CreateXmlNode("CopyKeysNode");
    CopyKeysToClipboard(copyNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);

    CClipboard clip(nullptr);
    clip.Put(copyNode, "Track view keys");
}

void CTrackViewSequence::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        pChildNode->CopyKeysToClipboard(xmlNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);
    }
}

void CTrackViewSequence::PasteKeysFromClipboard(CTrackViewAnimNode* pTargetNode, CTrackViewTrack* pTargetTrack, const float timeOffset)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");

    CClipboard clipboard(nullptr);
    XmlNodeRef clipboardContent = clipboard.Get();
    if (clipboardContent)
    {
        AZStd::vector<TMatchedTrackLocation> matchedLocations = GetMatchedPasteLocations(clipboardContent, pTargetNode, pTargetTrack);

        for (auto iter = matchedLocations.begin(); iter != matchedLocations.end(); ++iter)
        {
            const TMatchedTrackLocation& location = *iter;
            CTrackViewTrack* pTrack = location.first;
            const XmlNodeRef& trackNode = location.second;
            pTrack->PasteKeys(trackNode, timeOffset);
        }

        OnKeysChanged();
    }
}

AZStd::vector<CTrackViewSequence::TMatchedTrackLocation>
CTrackViewSequence::GetMatchedPasteLocations(XmlNodeRef clipboardContent, CTrackViewAnimNode* pTargetNode, CTrackViewTrack* pTargetTrack)
{
    AZStd::vector<TMatchedTrackLocation> matchedLocations;

    bool bPastingSingleNode = false;
    XmlNodeRef singleNode;
    bool bPastingSingleTrack = false;
    XmlNodeRef singleTrack;

    // Check if the XML tree only contains one node and if so if that node only contains one track
    for (XmlNodeRef currentNode = clipboardContent; currentNode->getChildCount() > 0; currentNode = currentNode->getChild(0))
    {
        bool bAllChildsAreTracks = true;
        const unsigned int numChilds = currentNode->getChildCount();
        for (unsigned int i = 0; i < numChilds; ++i)
        {
            XmlNodeRef childNode = currentNode->getChild(i);
            if (strcmp(currentNode->getChild(0)->getTag(), "Track") != 0)
            {
                bAllChildsAreTracks = false;
                break;
            }
        }

        if (bAllChildsAreTracks)
        {
            bPastingSingleNode = true;
            singleNode = currentNode;

            if (currentNode->getChildCount() == 1)
            {
                bPastingSingleTrack = true;
                singleTrack = currentNode->getChild(0);
            }
        }
        else if (currentNode->getChildCount() != 1)
        {
            break;
        }
    }

    if (bPastingSingleTrack && pTargetNode && pTargetTrack)
    {
        // We have a target node & track, so try to match the value type
        int valueType = 0;
        if (singleTrack->getAttr("valueType", valueType))
        {
            if (pTargetTrack->GetValueType() == static_cast<AnimValueType>(valueType))
            {
                matchedLocations.push_back(TMatchedTrackLocation(pTargetTrack, singleTrack));
                return matchedLocations;
            }
        }
    }

    if (bPastingSingleNode && pTargetNode)
    {
        // Set of tracks that were already matched
        AZStd::vector<CTrackViewTrack*> matchedTracks;

        // We have a single node to paste and have been given a target node
        // so try to match the tracks by param type
        const unsigned int numTracks = singleNode->getChildCount();
        for (unsigned int i = 0; i < numTracks; ++i)
        {
            XmlNodeRef trackNode = singleNode->getChild(i);

            // Try to match the track
            auto matchingTracks = GetMatchingTracks(pTargetNode, trackNode);
            for (auto iter = matchingTracks.begin(); iter != matchingTracks.end(); ++iter)
            {
                CTrackViewTrack* pMatchedTrack = *iter;
                // Pick the first track that was matched *and* was not already matched
                if (!AZStd::find(matchedTracks.begin(), matchedTracks.end(), pMatchedTrack))
                {
                    stl::push_back_unique(matchedTracks, pMatchedTrack);
                    matchedLocations.push_back(TMatchedTrackLocation(pMatchedTrack, trackNode));
                    break;
                }
            }
        }

        // Return if matching succeeded
        if (matchedLocations.size() > 0)
        {
            return matchedLocations;
        }
    }

    if (!bPastingSingleNode)
    {
        // Ok, we're pasting keys from multiple nodes, haven't been given any target
        // or matching the targets failed. Ignore given target pointers and start
        // a recursive match at the sequence root.
        GetMatchedPasteLocationsRec(matchedLocations, this, clipboardContent);
    }

    return matchedLocations;
}

AZStd::deque<CTrackViewTrack*> CTrackViewSequence::GetMatchingTracks(CTrackViewAnimNode* pAnimNode, XmlNodeRef trackNode)
{
    AZStd::deque<CTrackViewTrack*> matchingTracks;

    const AZStd::string trackName = trackNode->getAttr("name");

    CAnimParamType animParamType;
    animParamType.LoadFromXml(trackNode);

    int valueType;
    if (!trackNode->getAttr("valueType", valueType))
    {
        return matchingTracks;
    }

    CTrackViewTrackBundle tracks = pAnimNode->GetTracksByParam(animParamType);
    const unsigned int trackCount = tracks.GetCount();

    if (trackCount > 0)
    {
        // Search for a track with the given name and value type
        for (unsigned int i = 0; i < trackCount; ++i)
        {
            CTrackViewTrack* pTrack = tracks.GetTrack(i);

            if (pTrack->GetValueType() == static_cast<AnimValueType>(valueType))
            {
                if (pTrack->GetName() == trackName)
                {
                    matchingTracks.push_back(pTrack);
                }
            }
        }

        // Then with lower precedence add the tracks that only match the value
        for (unsigned int i = 0; i < trackCount; ++i)
        {
            CTrackViewTrack* pTrack = tracks.GetTrack(i);

            if (pTrack->GetValueType() == static_cast<AnimValueType>(valueType))
            {
                stl::push_back_unique(matchingTracks, pTrack);
            }
        }
    }

    return matchingTracks;
}

void CTrackViewSequence::GetMatchedPasteLocationsRec(AZStd::vector<TMatchedTrackLocation>& locations, CTrackViewNode* pCurrentNode, XmlNodeRef clipboardNode)
{
    if (pCurrentNode->GetNodeType() == eTVNT_Sequence)
    {
        if (strcmp(clipboardNode->getTag(), "CopyKeysNode") != 0)
        {
            return;
        }
    }

    const unsigned int numChildNodes = clipboardNode->getChildCount();
    for (unsigned int nodeIndex = 0; nodeIndex < numChildNodes; ++nodeIndex)
    {
        XmlNodeRef xmlChildNode = clipboardNode->getChild(nodeIndex);
        const AZStd::string tagName = xmlChildNode->getTag();

        if (tagName == "Node")
        {
            const AZStd::string nodeName = xmlChildNode->getAttr("name");

            int nodeType = static_cast<int>(AnimNodeType::Invalid);
            xmlChildNode->getAttr("type", nodeType);

            const unsigned int childCount = pCurrentNode->GetChildCount();
            for (unsigned int i = 0; i < childCount; ++i)
            {
                CTrackViewNode* pChildNode = pCurrentNode->GetChild(i);

                if (pChildNode->GetNodeType() == eTVNT_AnimNode)
                {
                    CTrackViewAnimNode* pAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
                    if (pAnimNode->GetName() == nodeName && pAnimNode->GetType() == static_cast<AnimNodeType>(nodeType))
                    {
                        GetMatchedPasteLocationsRec(locations, pChildNode, xmlChildNode);
                    }
                }
            }
        }
        else if (tagName == "Track")
        {
            const AZStd::string trackName = xmlChildNode->getAttr("name");

            CAnimParamType trackParamType;
            trackParamType.Serialize(xmlChildNode, true);

            int trackParamValue = static_cast<int>(AnimValueType::Unknown);
            xmlChildNode->getAttr("valueType", trackParamValue);

            const unsigned int childCount = pCurrentNode->GetChildCount();
            for (unsigned int i = 0; i < childCount; ++i)
            {
                CTrackViewNode* node = pCurrentNode->GetChild(i);

                if (node->GetNodeType() == eTVNT_Track)
                {
                    CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(node);
                    if (pTrack->GetName() == trackName && pTrack->GetParameterType() == trackParamType)
                    {
                        locations.push_back(TMatchedTrackLocation(pTrack, xmlChildNode));
                    }
                }
            }
        }
    }
}

void CTrackViewSequence::AdjustKeysToTimeRange(Range newTimeRange)
{
    // Set new time range
    Range oldTimeRange = GetTimeRange();
    float offset = newTimeRange.start - oldTimeRange.start;
    // Calculate scale ratio.
    float scale = newTimeRange.Length() / oldTimeRange.Length();
    SetTimeRange(newTimeRange);

    CTrackViewKeyBundle keyBundle = GetAllKeys();
    const unsigned int numKeys = keyBundle.GetKeyCount();

    // Do not notify listeners until all the times are set, otherwise the
    // keys will be sorted and the indices inside the CTrackViewKeyHandle
    // will become invalid.
    bool notifyListeners = false;

    for (unsigned int i = 0; i < numKeys; ++i)
    {
        CTrackViewKeyHandle keyHandle = keyBundle.GetKey(i);
        float scaled = (keyHandle.GetTime() - oldTimeRange.start) * scale;
        keyHandle.SetTime(offset + scaled + oldTimeRange.start, notifyListeners);
    }

    // notifyListeners was disabled in the above SetTime() calls so
    // notify all the keys changes now.
    OnKeysChanged();

    MarkAsModified();
}

void CTrackViewSequence::SetTimeRange(Range timeRange)
{
    m_pAnimSequence->SetTimeRange(timeRange);
    OnSequenceSettingsChanged();
}

Range CTrackViewSequence::GetTimeRange() const
{
    return m_pAnimSequence->GetTimeRange();
}

void CTrackViewSequence::SetFlags(IAnimSequence::EAnimSequenceFlags flags)
{
    m_pAnimSequence->SetFlags(flags);
    OnSequenceSettingsChanged();
}

IAnimSequence::EAnimSequenceFlags CTrackViewSequence::GetFlags() const
{
    return (IAnimSequence::EAnimSequenceFlags)m_pAnimSequence->GetFlags();
}

void CTrackViewSequence::DeselectAllKeys()
{
    CTrackViewSequenceNotificationContext context(this);

    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();
    for (unsigned int i = 0; i < selectedKeys.GetKeyCount(); ++i)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(i);
        keyHandle.Select(false);
    }
}

void CTrackViewSequence::OffsetSelectedKeys(const float timeOffset)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");
    CTrackViewSequenceNotificationContext context(this);

    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();

    // Set notifyListeners to false and wait until all keys
    // have been updated, otherwise the indexes in CTrackViewKeyHandle
    // may become invalid after sorted with a new time.
    bool notifyListeners = false;

    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(k);
        skey.Offset(timeOffset, notifyListeners);
    }

    if (selectedKeys.GetKeyCount() > 0)
    {
        OnKeysChanged();
    }
}

float CTrackViewSequence::ClipTimeOffsetForOffsetting(const float timeOffset)
{
    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();

    float newTimeOffset = timeOffset;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(k);
        const float keyTime = skey.GetTime();
        float newKeyTime = keyTime + timeOffset;

        Range extendedTimeRange(0.0f, GetTimeRange().end);
        extendedTimeRange.ClipValue(newKeyTime);

        float offset = newKeyTime - keyTime;
        if (fabs(offset) < fabs(newTimeOffset))
        {
            newTimeOffset = offset;
        }
    }

    return newTimeOffset;
}

float CTrackViewSequence::ClipTimeOffsetForScaling(float timeOffset)
{
    if (timeOffset <= 0)
    {
        return timeOffset;
    }

    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();

    float newTimeOffset = timeOffset;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(k);
        float keyTime = skey.GetTime();
        float newKeyTime = keyTime * timeOffset;
        GetTimeRange().ClipValue(newKeyTime);
        float offset = newKeyTime / keyTime;
        if (offset < newTimeOffset)
        {
            newTimeOffset = offset;
        }
    }

    return newTimeOffset;
}

void CTrackViewSequence::ScaleSelectedKeys(const float timeOffset)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");
    CTrackViewSequenceNotificationContext context(this);

    if (timeOffset <= 0)
    {
        return;
    }

    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();

    const CTrackViewTrack* pTrack = nullptr;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(k);
        if (pTrack != skey.GetTrack())
        {
            pTrack = skey.GetTrack();
        }

        float keyt = skey.GetTime() * timeOffset;
        skey.SetTime(keyt);
    }
}

float CTrackViewSequence::ClipTimeOffsetForSliding(const float timeOffset)
{
    CTrackViewKeyBundle keys = GetSelectedKeys();

    AZStd::set<CTrackViewTrack*> tracks;
    AZStd::set<CTrackViewTrack*>::const_iterator pTrackIter;

    Range timeRange = GetTimeRange();

    // Get the first key in the timeline among selected and
    // also gather tracks.
    float time0 = timeRange.end;
    for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = keys.GetKey(k);
        tracks.insert(skey.GetTrack());
        float keyTime = skey.GetTime();
        if (keyTime < time0)
        {
            time0 = keyTime;
        }
    }

    // If 'bAll' is true, slide all tracks.
    // (Otherwise, slide only selected tracks.)
    bool bAll = Qt::AltModifier & QApplication::queryKeyboardModifiers();
    if (bAll)
    {
        keys = GetKeysInTimeRange(time0, timeRange.end);
        // Gather tracks again.
        tracks.clear();
        for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
        {
            CTrackViewKeyHandle skey = keys.GetKey(k);
            tracks.insert(skey.GetTrack());
        }
    }

    float newTimeOffset = timeOffset;
    for (pTrackIter = tracks.begin(); pTrackIter != tracks.end(); ++pTrackIter)
    {
        CTrackViewTrack* pTrack = *pTrackIter;
        for (unsigned int i = 0; i < pTrack->GetKeyCount(); ++i)
        {
            const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

            const float keyTime = keyHandle.GetTime();
            if (keyTime >= time0)
            {
                float newKeyTime = keyTime + timeOffset;
                timeRange.ClipValue(newKeyTime);
                float offset = newKeyTime - keyTime;
                if (fabs(offset) < fabs(newTimeOffset))
                {
                    newTimeOffset = offset;
                }
            }
        }
    }

    return newTimeOffset;
}

void CTrackViewSequence::SlideKeys(float timeOffset)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");
    CTrackViewSequenceNotificationContext context(this);

    CTrackViewKeyBundle keys = GetSelectedKeys();

    AZStd::set<CTrackViewTrack*> tracks;
    AZStd::set<CTrackViewTrack*>::const_iterator pTrackIter;
    Range timeRange = GetTimeRange();

    // Get the first key in the timeline among selected and
    // also gather tracks.
    float time0 = timeRange.end;
    for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = keys.GetKey(k);
        tracks.insert(skey.GetTrack());
        float keyTime = skey.GetTime();
        if (keyTime < time0)
        {
            time0 = keyTime;
        }
    }

    // If 'bAll' is true, slide all tracks.
    // (Otherwise, slide only selected tracks.)
    bool bAll = Qt::AltModifier & QApplication::queryKeyboardModifiers();
    if (bAll)
    {
        keys = GetKeysInTimeRange(time0, timeRange.end);
        // Gather tracks again.
        tracks.clear();
        for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
        {
            CTrackViewKeyHandle skey = keys.GetKey(k);
            tracks.insert(skey.GetTrack());
        }
    }

    for (pTrackIter = tracks.begin(); pTrackIter != tracks.end(); ++pTrackIter)
    {
        CTrackViewTrack* pTrack = *pTrackIter;
        pTrack->SlideKeys(time0, timeOffset);
    }
}

void CTrackViewSequence::CloneSelectedKeys()
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");
    CTrackViewSequenceNotificationContext context(this);

    CTrackViewKeyBundle selectedKeys = GetSelectedKeys();

    const CTrackViewTrack* pTrack = nullptr;
    // In case of multiple cloning, indices cannot be used as a solid pointer to the original.
    // So use the time of keys as an identifier, instead.
    AZStd::vector<float> selectedKeyTimes;
    for (size_t k = 0; k < selectedKeys.GetKeyCount(); ++k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(static_cast<unsigned int>(k));
        if (pTrack != skey.GetTrack())
        {
            pTrack = skey.GetTrack();
        }

        selectedKeyTimes.push_back(skey.GetTime());
    }

    // Now, do the actual cloning.
    for (size_t k = 0; k < selectedKeyTimes.size(); ++k)
    {
        CTrackViewKeyHandle skey = selectedKeys.GetKey(static_cast<unsigned int>(k));
        skey = skey.GetTrack()->GetKeyByTime(selectedKeyTimes[k]);

        AZ_Assert(skey.IsValid(), "Key is not valid");
        if (!skey.IsValid())
        {
            continue;
        }

        CTrackViewKeyHandle newKey = skey.Clone();

        // Select new key.
        newKey.Select(true);
        // Deselect cloned key.
        skey.Select(false);
    }
}

void CTrackViewSequence::BeginUndoTransaction()
{
    QueueNotifications();
}

void CTrackViewSequence::EndUndoTransaction()
{
    // if the sequence was added during a redo, it will add itself as an UndoManagerListener in the process and we'll
    // get an EndUndoTransaction without a corresponding BeginUndoTransaction() call - only SubmitPendingNotifications()
    // if we're queued
    if (m_bQueueNotifications)
    {
        SubmitPendingNotifications();
    }
}

void CTrackViewSequence::BeginRestoreTransaction()
{
    QueueNotifications();
}

void CTrackViewSequence::EndRestoreTransaction()
{
    // if the sequence was added during a restore, it will add itself as an UndoManagerListener in the process and we'll
    // get an EndUndoTransaction without a corresponding BeginUndoTransaction() call - only SubmitPendingNotifications()
    // if we're queued
    if (m_bQueueNotifications)
    {
        SubmitPendingNotifications();
    }
}


bool CTrackViewSequence::IsActiveSequence() const
{
    return GetIEditor()->GetAnimation()->GetSequence() == this;
}

const float CTrackViewSequence::GetTime() const
{
    return m_time;
}

CTrackViewSequence* CTrackViewSequence::LookUpSequenceByEntityId(const AZ::EntityId& sequenceId)
{
    CTrackViewSequence* sequence = nullptr;

    IEditor* editor = nullptr;
    AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::Bus::Events::GetEditor);
    if (editor)
    {
        ITrackViewSequenceManager* sequenceManager = editor->GetSequenceManager();
        if (sequenceManager)
        {
            sequence = sequenceManager->GetSequenceByEntityId(sequenceId);
        }
    }

    return sequence;
}
