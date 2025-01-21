/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewAnimNode.h"

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

// CryCommon
#include <CryCommon/Maestro/Bus/EditorSequenceComponentBus.h>
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>
#include <CryCommon/MathConversion.h>

// Editor
#include "AnimationContext.h"
#include "Clipboard.h"
#include "CommentNodeAnimator.h"
#include "DirectorNodeAnimator.h"
#include "ViewManager.h"
#include "TrackView/TrackViewDialog.h"
#include "TrackView/TrackViewSequence.h"
#include "TrackView/TrackViewNodeFactories.h"

// AzCore
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/set.h>


// static class data
const AZ::Uuid CTrackViewAnimNode::s_nullUuid = AZ::Uuid::CreateNull();

namespace
{
    void CreateDefaultTracksForEntityNode(CTrackViewAnimNode* node, const AZStd::vector<AnimParamType>& tracks)
    {
        AZ_Assert(node->GetType() == AnimNodeType::AzEntity, "Expected AzEntity node for creating default tracks");

        // add a Transform Component anim node if needed, then go through and look for Position,
        // Rotation and Scale default tracks and adds them by hard-coded Virtual Property name. This is not a scalable way to do this,
        // but fits into the legacy Track View entity property system. This will all be re-written in a new TrackView for components in the
        // future.
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, node->GetAzEntityId());
        if (entity)
        {
            AZ::Component* transformComponent = entity->FindComponent(AzToolsFramework::Components::TransformComponent::TYPEINFO_Uuid());
            if (transformComponent)
            {
                // find a transform Component Node if it exists, otherwise create one.
                CTrackViewAnimNode* transformComponentNode = nullptr;

                for (int i = node->GetChildCount(); --i >= 0;)
                {
                    if (node->GetChild(i)->GetNodeType() == eTVNT_AnimNode)
                    {
                        CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(node->GetChild(i));
                        AZ::ComponentId componentId = childAnimNode->GetComponentId();
                        AZ::Uuid componentTypeId;
                        AzFramework::ApplicationRequests::Bus::BroadcastResult(
                            componentTypeId,
                            &AzFramework::ApplicationRequests::Bus::Events::GetComponentTypeId,
                            entity->GetId(),
                            componentId);
                        if (componentTypeId == AzToolsFramework::Components::TransformComponent::TYPEINFO_Uuid())
                        {
                            transformComponentNode = childAnimNode;
                            break;
                        }
                    }
                }

                if (!transformComponentNode)
                {
                    // no existing Transform Component node found - create one.
                    transformComponentNode = node->AddComponent(transformComponent, false);
                }

                if (transformComponentNode)
                {
                    for (size_t i = 0; i < tracks.size(); ++i)
                    {
                        // This is not ideal - we hard-code the VirtualProperty names for "Position", "Rotation",
                        // and "Scale" here, which creates an implicitly name dependency, but these are unlikely to change.
                        CAnimParamType paramType = tracks[i];
                        CAnimParamType transformPropertyParamType;
                        bool createTransformTrack = false;

                        if (paramType.GetType() == AnimParamType::Position)
                        {
                            transformPropertyParamType = AZStd::string("Position");
                            createTransformTrack = true;
                        }
                        else if (paramType.GetType() == AnimParamType::Rotation)
                        {
                            transformPropertyParamType = AZStd::string("Rotation");
                            createTransformTrack = true;
                        }
                        else if (paramType.GetType() == AnimParamType::Scale)
                        {
                            transformPropertyParamType = AZStd::string("Scale");
                            createTransformTrack = true;
                        }

                        if (createTransformTrack)
                        {
                            // this sets the type to one of AnimParamType::Position, AnimParamType::Rotation or AnimParamType::Scale
                            // but maintains the name
                            transformPropertyParamType = paramType.GetType();
                            transformComponentNode->CreateTrack(transformPropertyParamType);
                        }
                    }
                }
            }
        }
    }
} // namespace

void CTrackViewAnimNodeBundle::AppendAnimNode(CTrackViewAnimNode* node)
{
    stl::push_back_unique(m_animNodes, node);
}

void CTrackViewAnimNodeBundle::AppendAnimNodeBundle(const CTrackViewAnimNodeBundle& bundle)
{
    for (auto* node : bundle.m_animNodes)
    {
        AppendAnimNode(node);
    }
}

void CTrackViewAnimNodeBundle::ExpandAll(bool bAlsoExpandParentNodes)
{
    AZStd::set<CTrackViewNode*> nodesToExpand(m_animNodes.begin(), m_animNodes.end());

    if (bAlsoExpandParentNodes)
    {
        for (const auto* node : nodesToExpand)
        {
            for (CTrackViewNode* pParent = node->GetParentNode(); pParent; pParent = pParent->GetParentNode())
            {
                nodesToExpand.insert(pParent);
            }
        }
    }

    for (auto* node : nodesToExpand)
    {
        node->SetExpanded(true);
    }
}

void CTrackViewAnimNodeBundle::CollapseAll()
{
    for (auto* node : m_animNodes)
    {
        node->SetExpanded(false);
    }
}

const bool CTrackViewAnimNodeBundle::DoesContain(const CTrackViewNode* pTargetNode)
{
    return AZStd::find(m_animNodes.begin(), m_animNodes.end(), pTargetNode);
}

void CTrackViewAnimNodeBundle::Clear()
{
    m_animNodes.clear();
}

CTrackViewAnimNode::CTrackViewAnimNode(IAnimSequence* pSequence, IAnimNode* animNode, CTrackViewNode* pParentNode)
    : CTrackViewNode(pParentNode)
    , m_animSequence(pSequence)
    , m_animNode(animNode)
    , m_pNodeAnimator(nullptr)
{
    if (animNode)
    {
        // Search for child nodes
        const int nodeCount = pSequence->GetNodeCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            IAnimNode* node = pSequence->GetNode(i);
            IAnimNode* pNodeParentNode = node->GetParent();

            // If our node is the parent, then the current node is a child of it
            if (animNode == pNodeParentNode)
            {
                CTrackViewAnimNodeFactory animNodeFactory;
                CTrackViewAnimNode* pNewTVAnimNode = animNodeFactory.BuildAnimNode(pSequence, node, this);
                m_childNodes.push_back(AZStd::unique_ptr<CTrackViewNode>(pNewTVAnimNode));
            }
        }

        // Copy tracks from animNode
        const int trackCount = animNode->GetTrackCount();
        for (int i = 0; i < trackCount; ++i)
        {
            IAnimTrack* pTrack = animNode->GetTrackByIndex(i);

            CTrackViewTrackFactory trackFactory;
            CTrackViewTrack* pNewTVTrack = trackFactory.BuildTrack(pTrack, this, this);
            m_childNodes.push_back(AZStd::unique_ptr<CTrackViewNode>(pNewTVTrack));
        }

        // Set owner to update entity CryMovie entity IDs and remove it again
        SetNodeEntityId(GetNodeEntityId());
    }

    SortNodes();

    switch (GetType())
    {
    case AnimNodeType::Comment:
        m_pNodeAnimator.reset(new CCommentNodeAnimator(this));
        break;
    case AnimNodeType::Layer:
        AZ_Assert(false, "Animated Cry Layers are unsupported");
        return;
    case AnimNodeType::Director:
        m_pNodeAnimator.reset(new CDirectorNodeAnimator(this));
        break;
    }

    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

    if (IsBoundToAzEntity())
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetAzEntityId());
    }
}

CTrackViewAnimNode::~CTrackViewAnimNode()
{
    UnRegisterEditorObjectListeners();

    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

    if (IsBoundToAzEntity())
    {
        AZ::EntityId entityId = GetAzEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        AZ::EntityBus::Handler::BusDisconnect(entityId);
    }
}

void CTrackViewAnimNode::BindToEditorObjects()
{
    if (!IsActive())
    {
        return;
    }

    CTrackViewSequenceNotificationContext context(GetSequence());

    CTrackViewAnimNode* director = GetDirector();
    const bool bBelongsToActiveDirector = director ? director->IsActiveDirector() : true;

    if (bBelongsToActiveDirector)
    {
        bool ownerChanged = false;
        if (m_pNodeAnimator)
        {
            m_pNodeAnimator->Bind(this);
        }

        if (m_animNode)
        {
            m_animNode->SetNodeOwner(this);
            ownerChanged = true;
        }

        if (const AZ::EntityId entityId = GetNodeEntityId();
            entityId.IsValid())
        {
            const auto entity = AzToolsFramework::GetEntityById(entityId);
            if (entity)
            {
                RegisterEditorObjectListeners(entityId);
                SetNodeEntityId(entityId);
            }
        }

        if (ownerChanged)
        {
            GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
        }

        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            CTrackViewNode* pChildNode = (*iter).get();
            if (pChildNode->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
                pChildAnimNode->BindToEditorObjects();
            }
        }
    }
}

void CTrackViewAnimNode::UnBindFromEditorObjects()
{
    CTrackViewSequenceNotificationContext context(GetSequence());

    UnRegisterEditorObjectListeners();

    if (m_animNode)
    {
        // 'Owner' is the TrackViewNode, as opposed to the EditorEntityNode (as 'owner' is used in animSequence, or the pEntity 
        // returned from FindAnimNodeOwner() - confusing, isn't it?
        m_animNode->SetNodeOwner(nullptr);
    }

    if (m_pNodeAnimator)
    {
        m_pNodeAnimator->UnBind(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
            pChildAnimNode->UnBindFromEditorObjects();
        }
    }
}

bool CTrackViewAnimNode::IsBoundToEditorObjects() const
{
    if (m_animNode)
    {
        if (m_animNode->GetType() == AnimNodeType::AzEntity)
        {
            // check if bound to comoponent entity
            return m_animNode->GetAzEntityId().IsValid();
        }
        else
        {
            // check if bound to legacy entity
            return (m_animNode->GetNodeOwner() != nullptr);
        }
    }

    return false;
}

void CTrackViewAnimNode::SyncToConsole(SAnimContext& animContext)
{
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
            pChildAnimNode->SyncToConsole(animContext);
        }
    }
}

CTrackViewAnimNode* CTrackViewAnimNode::CreateSubNode(
    const QString& originalName, const AnimNodeType animNodeType, const AZ::EntityId owner, AZ::Uuid componentTypeId,
    AZ::ComponentId componentId)
{
    const bool isGroupNode = IsGroupNode();
    AZ_Assert(isGroupNode, "Expected CreateSubNode to be called on a group capible node.");
    if (!isGroupNode)
    {
        return nullptr;
    }

    const auto originalNameStr = originalName.toUtf8();

    // Find the director or sequence.
    CTrackViewAnimNode* director = (GetType() == AnimNodeType::Director) ? this : GetDirector();
    director = director ? director : GetSequence();
    AZ_Assert(director, "Expected a valid director or sequence to be found.");
    if (!director)
    {
        return nullptr;
    }

    // If this is an AzEntity, make sure there is an associated entity id
    if (animNodeType == AnimNodeType::AzEntity)
    {
        if (!owner.IsValid())
        {
            IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
            if (movieSystem)
            {
                movieSystem->LogUserNotificationMsg(
                    AZStd::string::format(
                        "Failed to add '%s' to sequence '%s', could not find associated entity. "
                        "Please try adding the entity associated with '%s'.",
                        originalNameStr.constData(), director->GetName().c_str(), originalNameStr.constData()));
            }

            return nullptr;
        }
    }

    QString name = originalName;

    // Check if the node's director or sequence already contains a node with this name, unless it's a component, for which we allow duplicate names since
    // Components are children of unique AZEntities in Track View. If a different Entity component with the same name exists, create a new unique name;
    if (animNodeType != AnimNodeType::Component)
    {
        CTrackViewAnimNode* director2 = (GetType() == AnimNodeType::Director) ? this : GetDirector();
        director2 = director2 ? director2 : GetSequence();

        AZ_Assert(director2, "Expected a valid director or sequence to be found.");
        if (!director2)
        {
            return nullptr;
        }

        bool alreadyExists = false;

        // If this is a valid AzEntity, we may generate a unique name if a dupe name is found
        // from a different entity.
        if (owner.IsValid())
        {
            // Check for a duplicates
            CTrackViewAnimNodeBundle azEntityNodesFound = director2->GetAnimNodesByType(AnimNodeType::AzEntity);
            for (unsigned int x = 0; x < azEntityNodesFound.GetCount(); x++)
            {
                if (azEntityNodesFound.GetNode(x)->GetAzEntityId() == owner)
                {
                    alreadyExists = true;
                    break;
                }
            }
        }
        else
        {
            // Search by name for other non AzEntity 
            alreadyExists = director2->GetAnimNodesByName(name.toUtf8().data()).GetCount() > 0;
        }

        // Show an error if this node is a duplicate
        if (alreadyExists)
        {
            IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
            if (movieSystem)
            {
                movieSystem->LogUserNotificationMsg(
                    AZStd::string::format("'%s' already exists in sequence '%s', skipping...",
                        originalNameStr.constData(), director2->GetName().c_str()));
            }

            return nullptr;
        }

        // Ensure a unique name, disallowed duplicates are already resolved by here.
        name = GetAvailableNodeNameStartingWith(name);
    }

    const auto nameStr = name.toUtf8();

    // Create CryMovie and TrackView node
    IAnimNode* newAnimNode = m_animSequence->CreateNode(animNodeType);
    if (!newAnimNode)
    {
        IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
        if (movieSystem)
        {
            movieSystem->LogUserNotificationMsg(
                AZStd::string::format("Failed to add '%s' to sequence '%s'.", nameStr.constData(), director->GetName().c_str()));
        }
        return nullptr;
    }

    newAnimNode->SetName(nameStr.constData());
    newAnimNode->CreateDefaultTracks();
    newAnimNode->SetParent(m_animNode.get());
    newAnimNode->SetComponent(componentId, componentTypeId);

    CTrackViewAnimNodeFactory animNodeFactory;
    CTrackViewAnimNode* newNode = animNodeFactory.BuildAnimNode(m_animSequence, newAnimNode, this);

    // Make sure that camera and entity nodes get created with an owner
    AZ_Assert((animNodeType != AnimNodeType::Entity), "Entity node should have valid owner.");

    newNode->SetNodeEntityId(owner);
    newAnimNode->SetNodeOwner(newNode);

    newNode->BindToEditorObjects();

    AddNode(newNode);

    // Add node to sequence, let AZ Undo take care of undo/redo
    m_animSequence->AddNode(newNode->m_animNode.get());

    return newNode;
}

// Helper function to remove a child node
void CTrackViewAnimNode::RemoveChildNode(CTrackViewAnimNode* child)
{
    AZ_Assert(child, "Attempting to remove null node");

    auto parent = static_cast<CTrackViewAnimNode*>(child->m_pParentNode);
    AZ_Assert(parent, "Parent node for child %p is null", child);

    child->UnBindFromEditorObjects();

    for (auto iter = parent->m_childNodes.begin(); iter != parent->m_childNodes.end(); ++iter)
    {
        AZStd::unique_ptr<CTrackViewNode>& currentNode = *iter;

        if (currentNode.get() == child)
        {
            parent->m_childNodes.erase(iter);
            break;
        }
    }
}

void CTrackViewAnimNode::RemoveSubNode(CTrackViewAnimNode* pSubNode)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");

    const bool bIsGroupNode = IsGroupNode();
    AZ_Assert(bIsGroupNode, "Attempting to remove sub-node from not a group node");
    if (!bIsGroupNode)
    {
        return;
    }

    // remove animation node children
    for (int i = pSubNode->GetChildCount(); --i >= 0;)
    {
        CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(pSubNode->GetChild(i));
        if (childAnimNode->GetNodeType() == eTVNT_AnimNode)
        {
            RemoveSubNode(childAnimNode);
        }
    }

    // Remove node from sequence entity, let AZ Undo take care of undo/redo
    m_animSequence->RemoveNode(pSubNode->m_animNode.get(), /*removeChildRelationships=*/ false);
    pSubNode->GetSequence()->OnNodeChanged(pSubNode, ITrackViewSequenceListener::eNodeChangeType_Removed);
    RemoveChildNode(pSubNode);
}

CTrackViewTrack* CTrackViewAnimNode::CreateTrack(const CAnimParamType& paramType)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");

    if (GetTrackForParameter(paramType) && !(GetParamFlags(paramType) & IAnimNode::eSupportedParamFlags_MultipleTracks))
    {
        return nullptr;
    }

    // Create CryMovie track
    IAnimTrack* pNewAnimTrack = m_animNode->CreateTrack(paramType);
    if (!pNewAnimTrack)
    {
        return nullptr;
    }

    // Create Track View Track
    CTrackViewTrackFactory trackFactory;
    CTrackViewTrack* pNewTrack = trackFactory.BuildTrack(pNewAnimTrack, this, this);

    AddNode(pNewTrack);

    MarkAsModified();

    const AnimParamType animParamType = paramType.GetType();
    SetPosRotScaleTracksDefaultValues(
        animParamType == AnimParamType::Position,
        animParamType == AnimParamType::Rotation,
        animParamType == AnimParamType::Scale
    );

    return pNewTrack;
}

void CTrackViewAnimNode::RemoveTrack(CTrackViewTrack* track)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");
    const bool isSubTrack = track->IsSubTrack();
    AZ_Assert(!isSubTrack, "Attempting to remove a sub-track");

    if (!isSubTrack)
    {
        CTrackViewSequence* sequence = track->GetSequence();
        if (nullptr != sequence)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Remove Track");
            CTrackViewAnimNode* parentNode = track->GetAnimNode();
            AZStd::unique_ptr<CTrackViewNode> foundTrack;

            if (nullptr != parentNode)
            {
                for (auto iter = parentNode->m_childNodes.begin(); iter != parentNode->m_childNodes.end(); ++iter)
                {
                    AZStd::unique_ptr<CTrackViewNode>& currentNode = *iter;
                    if (currentNode.get() == track)
                    {
                        // Hang onto a reference until after OnNodeChanged is called.
                        currentNode.swap(foundTrack);

                        // Remove from parent node and sequence
                        parentNode->m_childNodes.erase(iter);
                        parentNode->m_animNode->RemoveTrack(track->GetAnimTrack());
                        break;
                    }
                }

                m_pParentNode->GetSequence()->OnNodeChanged(track, ITrackViewSequenceListener::eNodeChangeType_Removed);

                // Release the track now that OnNodeChanged is complete.
                if (nullptr != foundTrack.get())
                {
                    foundTrack.release();
                }

            }
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());            
        }
    }
}

bool CTrackViewAnimNode::SnapTimeToPrevKey(float& time) const
{
    const float startTime = time;
    float closestTrackTime = std::numeric_limits<float>::min();
    bool bFoundPrevKey = false;

    for (size_t i = 0; i < m_childNodes.size(); ++i)
    {
        const CTrackViewNode* node = m_childNodes[i].get();

        float closestNodeTime = startTime;
        if (node->SnapTimeToPrevKey(closestNodeTime))
        {
            closestTrackTime = AZStd::max(closestNodeTime, closestTrackTime);
            bFoundPrevKey = true;
        }
    }

    if (bFoundPrevKey)
    {
        time = closestTrackTime;
    }

    return bFoundPrevKey;
}

bool CTrackViewAnimNode::SnapTimeToNextKey(float& time) const
{
    const float startTime = time;
    float closestTrackTime = std::numeric_limits<float>::max();
    bool bFoundNextKey = false;

    for (size_t i = 0; i < m_childNodes.size(); ++i)
    {
        const CTrackViewNode* node = m_childNodes[i].get();

        float closestNodeTime = startTime;
        if (node->SnapTimeToNextKey(closestNodeTime))
        {
            closestTrackTime = AZStd::min(closestNodeTime, closestTrackTime);
            bFoundNextKey = true;
        }
    }

    if (bFoundNextKey)
    {
        time = closestTrackTime;
    }

    return bFoundNextKey;
}

void CTrackViewAnimNode::SetExpanded(bool expanded)
{
    if (GetExpanded() != expanded)
    {
        CTrackViewSequence* sequence = GetSequence();
        AZ_Assert(nullptr != sequence, "Every node should have a sequence.");
        if (nullptr != sequence)
        {
            AZ_Assert(m_animNode, "Expected m_animNode to be valid.");
            if (m_animNode)
            {
                m_animNode->SetExpanded(expanded);
            }

            if (expanded)
            {
                sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Expanded);
            }
            else
            {
                sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Collapsed);
            }
        }
    }
}

bool CTrackViewAnimNode::GetExpanded() const
{
    bool result = true;

    AZ_Assert(m_animNode, "Expected m_animNode to be valid.");
    if (m_animNode)
    {
        result = m_animNode->GetExpanded();
    }

    return result;
}

CTrackViewKeyBundle CTrackViewAnimNode::GetSelectedKeys()
{
    CTrackViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
    }

    return bundle;
}

CTrackViewKeyBundle CTrackViewAnimNode::GetAllKeys()
{
    CTrackViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetAllKeys());
    }

    return bundle;
}

CTrackViewKeyBundle CTrackViewAnimNode::GetKeysInTimeRange(const float t0, const float t1)
{
    CTrackViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetKeysInTimeRange(t0, t1));
    }

    return bundle;
}

CTrackViewTrackBundle CTrackViewAnimNode::GetAllTracks()
{
    return GetTracks(false, CAnimParamType());
}

CTrackViewTrackBundle CTrackViewAnimNode::GetSelectedTracks()
{
    return GetTracks(true, CAnimParamType());
}

CTrackViewTrackBundle CTrackViewAnimNode::GetTracksByParam(const CAnimParamType& paramType) const
{
    return GetTracks(false, paramType);
}

CTrackViewTrackBundle CTrackViewAnimNode::GetTracks(const bool bOnlySelected, const CAnimParamType& paramType) const
{
    CTrackViewTrackBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* node = (*iter).get();

        if (node->GetNodeType() == eTVNT_Track)
        {
            CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(node);

            if (paramType != AnimParamType::Invalid && pTrack->GetParameterType() != paramType)
            {
                continue;
            }

            if (!bOnlySelected || pTrack->IsSelected())
            {
                bundle.AppendTrack(pTrack);
            }

            const unsigned int subTrackCount = pTrack->GetChildCount();
            for (unsigned int subTrackIndex = 0; subTrackIndex < subTrackCount; ++subTrackIndex)
            {
                CTrackViewTrack* pSubTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(subTrackIndex));
                if (!bOnlySelected || pSubTrack->IsSelected())
                {
                    bundle.AppendTrack(pSubTrack);
                }
            }
        }
        else if (node->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* animNode = static_cast<CTrackViewAnimNode*>(node);
            bundle.AppendTrackBundle(animNode->GetTracks(bOnlySelected, paramType));
        }
    }

    return bundle;
}

AnimNodeType CTrackViewAnimNode::GetType() const
{
    return m_animNode ? m_animNode->GetType() : AnimNodeType::Invalid;
}

EAnimNodeFlags CTrackViewAnimNode::GetFlags() const
{
    return m_animNode ? (EAnimNodeFlags)m_animNode->GetFlags() : (EAnimNodeFlags)0;
}

bool CTrackViewAnimNode::AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const
{
    return m_animNode ? m_animNode->AreFlagsSetOnNodeOrAnyParent(flagsToCheck) : false;
}

void CTrackViewAnimNode::SetAsActiveDirector()
{
    if (GetType() == AnimNodeType::Director)
    {
        m_animSequence->SetActiveDirector(m_animNode.get());

        GetSequence()->UnBindFromEditorObjects();
        GetSequence()->BindToEditorObjects();

        GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_SetAsActiveDirector);
    }
}

bool CTrackViewAnimNode::IsActiveDirector() const
{
    return m_animNode == m_animSequence->GetActiveDirector();
}

bool CTrackViewAnimNode::IsParamValid(const CAnimParamType& param) const
{
    return m_animNode ? m_animNode->IsParamValid(param) : false;
}

CTrackViewTrack* CTrackViewAnimNode::GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const
{
    uint32 currentIndex = 0;

    if (GetType() == AnimNodeType::AzEntity)
    {
        // For AzEntity, search for track on all child components - returns first track match found (note components searched in reverse)
        for (int i = GetChildCount(); --i >= 0;)
        {
            if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* componentNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
                if (componentNode->GetType() == AnimNodeType::Component)
                {
                    if (CTrackViewTrack* track = componentNode->GetTrackForParameter(paramType, index))
                    {
                        return track;
                    }
                }
            }
        }
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* node = (*iter).get();

        if (node->GetNodeType() == eTVNT_Track)
        {
            CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(node);

            if (pTrack->GetParameterType() == paramType)
            {
                if (currentIndex == index)
                {
                    return pTrack;
                }
                else
                {
                    ++currentIndex;
                }
            }

            if (pTrack->IsCompoundTrack())
            {
                unsigned int numChildTracks = pTrack->GetChildCount();
                for (unsigned int i = 0; i < numChildTracks; ++i)
                {
                    CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
                    if (pChildTrack->GetParameterType() == paramType)
                    {
                        if (currentIndex == index)
                        {
                            return pChildTrack;
                        }
                        else
                        {
                            ++currentIndex;
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

void CTrackViewAnimNode::Render(const SAnimContext& ac)
{
    if (m_pNodeAnimator && IsActive())
    {
        m_pNodeAnimator->Render(this, ac);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            pChildAnimNode->Render(ac);
        }
    }
}

void CTrackViewAnimNode::Animate(const SAnimContext& animContext)
{
    if (m_pNodeAnimator && IsActive())
    {
        m_pNodeAnimator->Animate(this, animContext);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            pChildAnimNode->Animate(animContext);
        }
    }
}

bool CTrackViewAnimNode::SetName(const char* pName)
{
    // Check if the node's director already contains a node with this name
    CTrackViewAnimNode* director = GetDirector();
    director = director ? director : GetSequence();

    CTrackViewAnimNodeBundle nodes = director->GetAnimNodesByName(pName);
    const uint numNodes = nodes.GetCount();
    for (uint i = 0; i < numNodes; ++i)
    {
        if (nodes.GetNode(i) != this)
        {
            return false;
        }
    }

    AZStd::string oldName = GetName();
    m_animNode->SetName(pName);

    CTrackViewSequence* sequence = GetSequence();
    AZ_Assert(sequence, "Nodes should never have a null sequence.");

    sequence->OnNodeRenamed(this, oldName.c_str());

    return true;
}

bool CTrackViewAnimNode::CanBeRenamed() const
{
    return (GetFlags() & eAnimNodeFlags_CanChangeName) != 0;
}

void CTrackViewAnimNode::SetNodeEntityId(const AZ::EntityId entityId)
{
    bool entityPointerChanged = (entityId != m_nodeEntityId);

    m_nodeEntityId = entityId;

    if (entityId.IsValid())
    {
        if (m_animNode->GetType() == AnimNodeType::AzEntity)
        {
            // We're connecting to a new AZ::Entity
            AZ::EntityId sequenceComponentEntityId(m_animSequence->GetSequenceEntityId());

            // Notify the SequenceComponent that we're binding an entity to the sequence
            bool wasInvoked = false;
            Maestro::EditorSequenceComponentRequestBus::EventResult(
                wasInvoked,
                sequenceComponentEntityId, &Maestro::EditorSequenceComponentRequestBus::Events::AddEntityToAnimate, entityId);

            AZ_Trace(
                "CTrackViewAnimNode::SetNodeEntityId", "AddEntityToAnimate %s sequenceComponentEntityId %s was invoked %s", entityId.ToString().c_str(),
                sequenceComponentEntityId.ToString().c_str(),
                wasInvoked ? "true" : "false");

            if (entityId != m_animNode->GetAzEntityId())
            {
                if (m_animNode->GetAzEntityId().IsValid())
                {
                    // disconnect from bus with previous entity ID before we reset it
                    AZ::EntityBus::Handler::BusDisconnect(m_animNode->GetAzEntityId());
                    AZ::TransformNotificationBus::Handler::BusDisconnect(m_animNode->GetAzEntityId());
                }

                m_animNode->SetAzEntityId(entityId);
            }

            // connect to EntityBus for OnEntityActivated() notifications to sync components on the entity
            if (!AZ::EntityBus::Handler::BusIsConnectedId(m_animNode->GetAzEntityId()))
            {
                AZ::EntityBus::Handler::BusConnect(m_animNode->GetAzEntityId());
            }

            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_animNode->GetAzEntityId()))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_animNode->GetAzEntityId());
            }
        }

        if (entityPointerChanged)
        {
            SetPosRotScaleTracksDefaultValues();
        }

        bool selected = AzToolsFramework::IsSelected(entityId);

        OnSelectionChanged(selected);
    }
}

AZ::EntityId CTrackViewAnimNode::GetNodeEntityId(const bool bSearch)
{
    if (m_animNode)
    {
        if (m_nodeEntityId.IsValid())
        {
            return m_nodeEntityId;
        }

        if (bSearch)
        {
            return GetAzEntityId();
        }
    }

    return AZ::EntityId();
}

CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAllAnimNodes()
{
    CTrackViewAnimNodeBundle bundle;

    if (GetNodeType() == eTVNT_AnimNode)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllAnimNodes());
        }
    }

    return bundle;
}

CTrackViewAnimNodeBundle CTrackViewAnimNode::GetSelectedAnimNodes()
{
    CTrackViewAnimNodeBundle bundle;

    if ((GetNodeType() == eTVNT_AnimNode || GetNodeType() == eTVNT_Sequence) && IsSelected())
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetSelectedAnimNodes());
        }
    }

    return bundle;
}

CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAllOwnedNodes(const AZ::EntityId entityId)
{
    CTrackViewAnimNodeBundle bundle;

    if (GetNodeType() == eTVNT_AnimNode && GetAzEntityId() == entityId)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllOwnedNodes(entityId));
        }
    }

    return bundle;
}

CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByType(AnimNodeType animNodeType)
{
    CTrackViewAnimNodeBundle bundle;

    if (GetNodeType() == eTVNT_AnimNode && GetType() == animNodeType)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByType(animNodeType));
        }
    }

    return bundle;
}

CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByName(const char* pName)
{
    CTrackViewAnimNodeBundle bundle;

    QString nodeName = QString::fromUtf8(GetName().c_str());
    if (GetNodeType() == eTVNT_AnimNode && QString::compare(pName, nodeName, Qt::CaseInsensitive) == 0)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByName(pName));
        }
    }

    return bundle;
}

AZStd::string CTrackViewAnimNode::GetParamName(const CAnimParamType& paramType) const
{
    return m_animNode->GetParamName(paramType);
}

bool CTrackViewAnimNode::IsGroupNode() const
{
    AnimNodeType nodeType = GetType();

    // AZEntities are really just containers for components, so considered a 'Group' node
    return nodeType == AnimNodeType::Director || nodeType == AnimNodeType::Group || nodeType == AnimNodeType::AzEntity;
}

QString CTrackViewAnimNode::GetAvailableNodeNameStartingWith(const QString& name) const
{
    QString newName = name;
    unsigned int index = 2;

    while (const_cast<CTrackViewAnimNode*>(this)->GetAnimNodesByName(newName.toUtf8().data()).GetCount() > 0)
    {
        newName = QStringLiteral("%1%2").arg(name).arg(index);
        ++index;
    }

    return newName;
}

CTrackViewAnimNodeBundle CTrackViewAnimNode::AddSelectedEntities(const AZStd::vector<AnimParamType>& tracks)
{
    AZ_Assert(IsGroupNode(), "Expected to added selected entities to a group node.");

    CTrackViewAnimNodeBundle addedNodes;

    AzToolsFramework::EntityIdList entityIds;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        entityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    // Add selected nodes.
    for (const AZ::EntityId& entityId : entityIds)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        if (entity == nullptr)
        {
            continue;
        }

        // Check if object already assigned to some AnimNode.
        if (CTrackViewAnimNode* existingNode = GetIEditor()->GetSequenceManager()->GetActiveAnimNode(entityId))
        {
            // If it has the same director than the current node, reject it
            if (existingNode->GetDirector() == GetDirector())
            {
                IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
                if (movieSystem)
                {
                    movieSystem->LogUserNotificationMsg(AZStd::string::format(
                        "'%s' was already added to '%s', skipping...", entity->GetName().c_str(), GetDirector()->GetName().c_str()));
                }

                continue;
            }
        }

        if (CTrackViewAnimNode* animNode = CreateSubNode(entity->GetName().c_str(), AnimNodeType::AzEntity, entityId))
        {
            CUndo undo("Add Default Tracks");

            CreateDefaultTracksForEntityNode(animNode, tracks);

            addedNodes.AppendAnimNode(animNode);
        }
    }

    return addedNodes;
}

void CTrackViewAnimNode::AddCurrentLayer()
{
    AZ_Assert(IsGroupNode(), "Attempting to add current layer to not a group node");

    static const QString name = "Main";

    CreateSubNode(name, AnimNodeType::Entity);
}

unsigned int CTrackViewAnimNode::GetParamCount() const
{
    return m_animNode ? m_animNode->GetParamCount() : 0;
}

CAnimParamType CTrackViewAnimNode::GetParamType(unsigned int index) const
{
    unsigned int paramCount = GetParamCount();
    if (!m_animNode || index >= paramCount)
    {
        return AnimParamType::Invalid;
    }

    return m_animNode->GetParamType(index);
}

IAnimNode::ESupportedParamFlags CTrackViewAnimNode::GetParamFlags(const CAnimParamType& paramType) const
{
    if (m_animNode)
    {
        return m_animNode->GetParamFlags(paramType);
    }

    return IAnimNode::ESupportedParamFlags(0);
}

AnimValueType CTrackViewAnimNode::GetParamValueType(const CAnimParamType& paramType) const
{
    if (m_animNode)
    {
        return m_animNode->GetParamValueType(paramType);
    }

    return AnimValueType::Unknown;
}

void CTrackViewAnimNode::UpdateDynamicParams()
{
    if (m_animNode)
    {
        m_animNode->UpdateDynamicParams();
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            pChildAnimNode->UpdateDynamicParams();
        }
    }
}

void CTrackViewAnimNode::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    XmlNodeRef childNode = xmlNode->createNode("Node");
    childNode->setAttr("name", GetName().c_str());
    childNode->setAttr("type", static_cast<int>(GetType()));

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        pChildNode->CopyKeysToClipboard(childNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);
    }

    if (childNode->getChildCount() > 0)
    {
        xmlNode->addChild(childNode);
    }
}

void CTrackViewAnimNode::CopyNodesToClipboard(const bool bOnlySelected, QWidget* context)
{
    XmlNodeRef animNodesRoot = XmlHelpers::CreateXmlNode("CopyAnimNodesRoot");

    CopyNodesToClipboardRec(this, animNodesRoot, bOnlySelected);

    CClipboard clipboard(context);
    clipboard.Put(animNodesRoot, "Track view entity nodes");
}

void CTrackViewAnimNode::CopyNodesToClipboardRec(CTrackViewAnimNode* pCurrentAnimNode, XmlNodeRef& xmlNode, const bool bOnlySelected)
{
    if (pCurrentAnimNode->m_animNode && (!bOnlySelected || pCurrentAnimNode->IsSelected()))
    {
        XmlNodeRef childXmlNode = xmlNode->newChild("Node");
        pCurrentAnimNode->m_animNode->Serialize(childXmlNode, false, true);
    }

    for (auto iter = pCurrentAnimNode->m_childNodes.begin(); iter != pCurrentAnimNode->m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);

            // If selected and group node, force copying of children
            const bool bSelectedAndGroupNode = pCurrentAnimNode->IsSelected() && pCurrentAnimNode->IsGroupNode();
            CopyNodesToClipboardRec(pChildAnimNode, xmlNode, !bSelectedAndGroupNode && bOnlySelected);
        }
    }
}

void CTrackViewAnimNode::PasteTracksFrom(XmlNodeRef& xmlNodeWithTracks)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");

    // we clear our own tracks first because calling SerializeAnims() will clear out m_animNode's tracks below
    CTrackViewTrackBundle allTracksBundle = GetAllTracks();
    for (int i = allTracksBundle.GetCount(); --i >= 0;)
    {
        RemoveTrack(allTracksBundle.GetTrack(i));
    }

    // serialize all the tracks from xmlNode - note this will first delete all existing tracks on m_animNode
    m_animNode->SerializeAnims(xmlNodeWithTracks, true, true);

    // create TrackView tracks
    const int trackCount = m_animNode->GetTrackCount();
    for (int i = 0; i < trackCount; ++i)
    {
        IAnimTrack* pTrack = m_animNode->GetTrackByIndex(i);

        CTrackViewTrackFactory trackFactory;
        CTrackViewTrack* newTrackNode = trackFactory.BuildTrack(pTrack, this, this);

        AddNode(newTrackNode);

        MarkAsModified();
    }
}

bool CTrackViewAnimNode::PasteNodesFromClipboard(QWidget* context)
{
    AZ_Assert(CUndo::IsRecording(), "Undo is not recording");

    CClipboard clipboard(context);
    if (clipboard.IsEmpty())
    {
        return false;
    }

    XmlNodeRef animNodesRoot = clipboard.Get();
    if (animNodesRoot == nullptr || strcmp(animNodesRoot->getTag(), "CopyAnimNodesRoot") != 0)
    {
        return false;
    }

    const bool bLightAnimationSetActive = GetSequence()->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet;

    AZStd::map<int, IAnimNode*> copiedIdToNodeMap;
    const unsigned int numNodes = animNodesRoot->getChildCount();
    for (unsigned int i = 0; i < numNodes; ++i)
    {
        XmlNodeRef xmlNode = animNodesRoot->getChild(i);

        // skip non-light nodes in light animation sets
        int type;
        if (!xmlNode->getAttr("Type", type) || (bLightAnimationSetActive && (AnimNodeType)type != AnimNodeType::Light))
        {
            continue;
        }

        PasteNodeFromClipboard(copiedIdToNodeMap, xmlNode);
    }

    return true;
}

void CTrackViewAnimNode::PasteNodeFromClipboard(AZStd::map<int, IAnimNode*>& copiedIdToNodeMap, XmlNodeRef xmlNode)
{
    QString name;

    if (!xmlNode->getAttr("Name", name))
    {
        return;
    }

    // can only paste nodes into a groupNode (i.e. accepts children)
    const bool bIsGroupNode = IsGroupNode();
    AZ_Assert(IsGroupNode(), "Attempting to paste nodes to not a group node");
    if (!bIsGroupNode)
    {
        return;
    }

    AnimNodeType nodeType = AnimNodeType::Invalid;
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        movieSystem->SerializeNodeType(nodeType, xmlNode, /*bLoading=*/ true, IAnimSequence::kSequenceVersion, m_animSequence->GetFlags());
    }
    
    if (nodeType == AnimNodeType::Component)
    {
        // When pasting Component Nodes, the parent Component Entity Node would have already added all its Components as part of its OnEntityActivated() sync.
        // Here we need to go copy any Components Tracks as well. For pasting, Components matched by ComponentId, which assumes the pasted Track View Component Node
        // refers to the copied Component Entity referenced in the level

        // Find the pasted parent Component Entity Node
        int parentId = 0;
        xmlNode->getAttr("ParentNode", parentId);
        if (copiedIdToNodeMap.find(parentId) != copiedIdToNodeMap.end())
        {
            CTrackViewAnimNode* componentEntityNode = FindNodeByAnimNode(copiedIdToNodeMap[parentId]);
            if (componentEntityNode)
            {
                // Find the copied Component Id on the pasted Component Entity Node, if it exists
                AZ::ComponentId componentId = AZ::InvalidComponentId;
                xmlNode->getAttr("ComponentId", componentId);

                for (int i = componentEntityNode->GetChildCount(); --i >= 0;)
                {
                    CTrackViewNode* childNode = componentEntityNode->GetChild(i);
                    if (childNode->GetNodeType() == eTVNT_AnimNode)
                    {
                        CTrackViewAnimNode* componentNode = static_cast<CTrackViewAnimNode*>(childNode);

                        if (componentNode->GetComponentId() == componentId)
                        {
                            componentNode->PasteTracksFrom(xmlNode);
                            break;
                        }
                    }
                }
            }
        }   
    }
    else
    {
        // Pasting a non-Component Node - create and add nodes to CryMovie and TrackView

        // Check if the node's director or sequence already contains a node with this name
        CTrackViewAnimNode* director = GetDirector();
        director = director ? director : GetSequence();
        if (director->GetAnimNodesByName(name.toUtf8().data()).GetCount() > 0)
        {
            return;
        }

        IAnimNode* newAnimNode = m_animSequence->CreateNode(xmlNode);
        if (!newAnimNode)
        {
            return;
        }

        // add new node to mapping of copied Id's to pasted nodes
        int id;
        xmlNode->getAttr("Id", id);
        copiedIdToNodeMap[id] = newAnimNode;

        // search for the parent Node among the pasted nodes - if not found, parent to the group node doing the pasting
        IAnimNode* parentAnimNode = m_animNode.get();
        int parentId = 0;
        if (xmlNode->getAttr("ParentNode", parentId))
        {
            if (copiedIdToNodeMap.find(parentId) != copiedIdToNodeMap.end())
            {
                parentAnimNode = copiedIdToNodeMap[parentId];
            }
        }
        newAnimNode->SetParent(parentAnimNode);

        // Find the TrackViewNode corresponding to the parentNode
        CTrackViewAnimNode* parentNode = FindNodeByAnimNode(parentAnimNode);
        if (!parentNode)
        {
            parentNode = this;
        }

        CTrackViewAnimNodeFactory animNodeFactory;
        CTrackViewAnimNode* newNode = animNodeFactory.BuildAnimNode(m_animSequence, newAnimNode, parentNode);

        parentNode->AddNode(newNode);

        // Add node to sequence, let AZ Undo take care of undo/redo
        m_animSequence->AddNode(newNode->m_animNode.get());
    }

    // Make sure there are no duplicate track Ids
    AZStd::vector<unsigned int> usedTrackIds;

    int nodeCount = m_animSequence->GetNodeCount();
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        IAnimNode* animNode = m_animSequence->GetNode(nodeIndex);
        AZ_Assert(animNode, "Expected valid animNode");

        int trackCount = animNode->GetTrackCount();
        for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
        {
            IAnimTrack* track = animNode->GetTrackByIndex(trackIndex);
            AZ_Assert(track, "Expected valid track");

            // If the Track Id is already used, generate a new one
            if (AZStd::find(usedTrackIds.begin(), usedTrackIds.end(), track->GetId()) != usedTrackIds.end())
            {
                track->SetId(m_animSequence->GetUniqueTrackIdAndGenerateNext());
            }

            usedTrackIds.push_back(track->GetId());

            int subTrackCount = track->GetSubTrackCount();
            for (int subTrackIndex = 0; subTrackIndex < subTrackCount; subTrackIndex++)
            {
                IAnimTrack* subTrack = track->GetSubTrack(subTrackIndex);
                AZ_Assert(subTrack, "Expected valid subtrack.");

                // If the Track Id is already used, generate a new one
                if (AZStd::find(usedTrackIds.begin(), usedTrackIds.end(), subTrack->GetId()) != usedTrackIds.end())
                {
                    subTrack->SetId(m_animSequence->GetUniqueTrackIdAndGenerateNext());
                }

                usedTrackIds.push_back(subTrack->GetId());
            }
        }
    }
}

CTrackViewAnimNode* CTrackViewAnimNode::FindNodeByAnimNode(const IAnimNode* animNode)
{
    // Depth-first search for TrackViewAnimNode associated with the given animNode. Returns the first match found.
    CTrackViewAnimNode* retNode = nullptr;

    for (const AZStd::unique_ptr<CTrackViewNode>& childNode : m_childNodes)
    {
        if (childNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(childNode.get());

            // recurse to search children of group nodes
            if (childNode->IsGroupNode())
            {
                retNode = childAnimNode->FindNodeByAnimNode(animNode);
                if (retNode)
                {
                    break;
                }
            }

            if (childAnimNode->GetAnimNode() == animNode)
            {
                retNode = childAnimNode;
                break;
            }
        }
    }
    return retNode;
}

bool CTrackViewAnimNode::IsValidReparentingTo(CTrackViewAnimNode* pNewParent)
{
    if (pNewParent == GetParentNode() || !pNewParent->IsGroupNode() || pNewParent->GetType() == AnimNodeType::AzEntity)
    {
        return false;
    }

    // Check if the new parent already contains a node with this name
    CTrackViewAnimNodeBundle foundNodes = pNewParent->GetAnimNodesByName(GetName().c_str());
    if (foundNodes.GetCount() > 1 || (foundNodes.GetCount() == 1 && foundNodes.GetNode(0) != this))
    {
        return false;
    }

    // Check if another node already owns this entity in the new parent's tree
    if (const AZ::EntityId owner = GetNodeEntityId();
        owner.IsValid())
    {
        CTrackViewAnimNodeBundle ownedNodes = pNewParent->GetAllOwnedNodes(owner);
        if (ownedNodes.GetCount() > 0 && ownedNodes.GetNode(0) != this)
        {
            return false;
        }
    }

    return true;
}

void CTrackViewAnimNode::SetParentsInChildren(CTrackViewAnimNode* currentNode)
{
    const uint numChildren = currentNode->GetChildCount();

    for (uint childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(currentNode->GetChild(childIndex));

        if (childAnimNode->GetNodeType() != eTVNT_Track)
        {
            childAnimNode->m_animNode->SetParent(currentNode->m_animNode.get());

            if (childAnimNode->GetChildCount() > 0 && childAnimNode->GetNodeType() != eTVNT_AnimNode)
            {
                SetParentsInChildren(childAnimNode);
            }
        }
    }
}
void CTrackViewAnimNode::SetNewParent(CTrackViewAnimNode* newParent)
{
    if (newParent == GetParentNode())
    {
        return;
    }

    AZ_Assert(IsValidReparentingTo(newParent), "Node cannot be moved to new parent %p", newParent);

    CTrackViewSequence* sequence = newParent->GetSequence();
    AZ_Assert(sequence, "Expected valid sequence.");

    AzToolsFramework::ScopedUndoBatch undoBatch("Set New Track View Anim Node Parent");

    UnBindFromEditorObjects();

    // Remove from the old parent's children and hang on to a ref.
    AZStd::unique_ptr<CTrackViewNode> storedTrackViewNode;
    CTrackViewAnimNode* lastParent = static_cast<CTrackViewAnimNode*>(m_pParentNode);
    if (nullptr != lastParent)
    {
        for (auto iter = lastParent->m_childNodes.begin(); iter != lastParent->m_childNodes.end(); ++iter)
        {
            AZStd::unique_ptr<CTrackViewNode>& currentNode = *iter;

            if (currentNode.get() == this)
            {
                currentNode.swap(storedTrackViewNode);
                lastParent->m_childNodes.erase(iter);
                break;
            }
        }
    }
    AZ_Assert(nullptr != storedTrackViewNode.get(), "Existing Parent of node not found");

    sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Removed);

    // Set new parent
    m_pParentNode = newParent;
    m_animNode->SetParent(newParent->m_animNode.get());
    SetParentsInChildren(this);

    // Add node to the new parent's children.
    static_cast<CTrackViewAnimNode*>(m_pParentNode)->AddNode(storedTrackViewNode.release());

    BindToEditorObjects();

    undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());    
}

bool CTrackViewAnimNode::CanBeEnabled() const
{
    bool canBeEnabled = true;
    // If this node was disabled because the component was disabled,
    // do not allow it to be re-enabled until that is resolved.
    if (m_animNode)
    {
        canBeEnabled = !(m_animNode->GetFlags() & eAnimNodeFlags_DisabledForComponent);
    }
    return canBeEnabled;
}

void CTrackViewAnimNode::SetDisabled(bool disabled)
{
    {
        CTrackViewSequence* sequence = GetSequence();
        AZ_Assert(sequence, "Expected valid sequence.");
        AZ_Assert(m_animNode, "Expected valid m_animNode.");

        if (disabled)
        {
            m_animNode->SetFlags(m_animNode->GetFlags() | eAnimNodeFlags_Disabled);
            sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Disabled);

            // Call OnReset to disable the effects of the node.
            m_animNode->OnReset();
        }
        else
        {
            m_animNode->SetFlags(m_animNode->GetFlags() & ~eAnimNodeFlags_Disabled);
            sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Enabled);
        }
    }
    MarkAsModified();
}

bool CTrackViewAnimNode::IsDisabled() const
{
    return m_animNode ? m_animNode->GetFlags() & eAnimNodeFlags_Disabled : false;
}

bool CTrackViewAnimNode::IsActive()
{
    CTrackViewSequence* pSequence = GetSequence();
    const bool bInActiveSequence = pSequence ? GetSequence()->IsBoundToEditorObjects() : false;

    CTrackViewAnimNode* director = GetDirector();
    const bool bMemberOfActiveDirector = director ? GetDirector()->IsActiveDirector() : true;

    return bInActiveSequence && bMemberOfActiveDirector;
}

void CTrackViewAnimNode::OnSelectionChanged(const bool selected)
{
    if (m_animNode)
    {
        [[maybe_unused]] const AnimNodeType animNodeType = GetType();
        AZ_Assert(animNodeType == AnimNodeType::AzEntity, "Expected AzEntity for selection changed");

        const EAnimNodeFlags flags = (EAnimNodeFlags)m_animNode->GetFlags();
        m_animNode->SetFlags(selected ? (flags | eAnimNodeFlags_EntitySelected) : (flags & ~eAnimNodeFlags_EntitySelected));
    }
}

void CTrackViewAnimNode::OnSelected()
{
    OnSelectionChanged(true);
}

void CTrackViewAnimNode::OnDeselected()
{
    OnSelectionChanged(false);
}

void CTrackViewAnimNode::SetPosRotScaleTracksDefaultValues(bool positionAllowed, bool rotationAllowed, bool scaleAllowed)
{
    AZ::EntityId entityId;
    bool entityIsBoundToEditorObjects = false;

    if (m_animNode)
    {
        if (m_animNode->GetType() == AnimNodeType::Component)
        {
            // get entity from the parent Component Entity
           CTrackViewNode* parentNode = GetParentNode();
           if (parentNode && parentNode->GetNodeType() == eTVNT_AnimNode)
           {
               CTrackViewAnimNode* parentAnimNode = static_cast<CTrackViewAnimNode*>(parentNode);
               if (parentAnimNode)
               {
                   entityId = parentAnimNode->GetNodeEntityId(false);
                   entityIsBoundToEditorObjects = parentAnimNode->IsBoundToEditorObjects();
               }
           }
        }
        else
        {
            // not a component - get the entity on this node directly
            entityId = GetNodeEntityId(false);
            entityIsBoundToEditorObjects = IsBoundToEditorObjects();
        }

        if (entityId.IsValid() && entityIsBoundToEditorObjects)
        {
            const float time = GetSequence()->GetTime();
            if (positionAllowed)
            {
                AZ::Vector3 position = AZ::Vector3::CreateZero();
                AZ::TransformBus::EventResult(position, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
                m_animNode->SetPos(time, position);
            }
            if (rotationAllowed)
            {
                AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
                AZ::TransformBus::EventResult(rotation, entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
                m_animNode->SetRotate(time, rotation);
            }
            if (scaleAllowed)
            {
                float scale = 1.0f;
                AZ::TransformBus::EventResult(scale, entityId, &AZ::TransformBus::Events::GetWorldUniformScale);
                m_animNode->SetScale(time, AZ::Vector3(scale, scale, scale));
            }
        }
    }
}

bool CTrackViewAnimNode::CheckTrackAnimated(const CAnimParamType& paramType) const
{
    if (!m_animNode)
    {
        return false;
    }

    CTrackViewTrack* pTrack = GetTrackForParameter(paramType);
    return pTrack && pTrack->GetKeyCount() > 0;
}

void CTrackViewAnimNode::OnNodeVisibilityChanged([[maybe_unused]] IAnimNode* node, const bool bHidden)
{
    if (m_nodeEntityId.IsValid())
    {
        AzToolsFramework::SetEntityVisibility(m_nodeEntityId, !bHidden);

        // Need to do this to force recreation of gizmos
        const bool showSelected =
            AzToolsFramework::IsEntityVisible(m_nodeEntityId) && AzToolsFramework::IsSelected(m_nodeEntityId);
        if (showSelected)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities,
                AzToolsFramework::EntityIdList{m_nodeEntityId});
        }
    }
}

void CTrackViewAnimNode::OnNodeReset([[maybe_unused]] IAnimNode* node)
{
}

void CTrackViewAnimNode::SetComponent(AZ::ComponentId componentId, const AZ::Uuid& componentTypeId)
{
    if (m_animNode)
    {
        m_animNode->SetComponent(componentId, componentTypeId);
    }
}

AZ::ComponentId CTrackViewAnimNode::GetComponentId() const
{
    return m_animNode ? m_animNode->GetComponentId() : AZ::InvalidComponentId;
}

void CTrackViewAnimNode::MarkAsModified()
{
    GetSequence()->MarkAsModified();
}

bool CTrackViewAnimNode::ContainsComponentWithId(AZ::ComponentId componentId) const
{
    bool retFound = false;

    if (GetType() == AnimNodeType::AzEntity)
    {
        // search for a matching componentId on all children
        for (unsigned int i = 0; i < GetChildCount(); i++)
        {
            CTrackViewNode* childNode = GetChild(i);
            if (childNode->GetNodeType() == eTVNT_AnimNode)
            {
                if (static_cast<CTrackViewAnimNode*>(childNode)->GetComponentId() == componentId)
                {
                    retFound = true;
                    break;
                }
            }
        }
    }

    return retFound;
}

void CTrackViewAnimNode::OnStartPlayInEditor()
{
    if (m_animSequence->GetSequenceEntityId().IsValid())
    {
        AZ::EntityId remappedId;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, m_animSequence->GetSequenceEntityId(), remappedId);

        if (remappedId.IsValid())
        {
            // stash and remap the AZ::EntityId of the SequenceComponent entity to restore it when we switch back to Edit mode
            m_stashedAnimSequenceEditorAzEntityId = m_animSequence->GetSequenceEntityId();
            m_animSequence->SetSequenceEntityId(remappedId);
        }
    }

    if (m_animNode && m_animNode->GetAzEntityId().IsValid())
    {
        AZ::EntityId remappedId;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, m_animNode->GetAzEntityId(), remappedId);

        if (remappedId.IsValid())
        {
            // stash the AZ::EntityId of the SequenceComponent entity to restore it when we switch back to Edit mode
            m_stashedAnimNodeEditorAzEntityId = m_animNode->GetAzEntityId();
            m_animNode->SetAzEntityId(remappedId);
        }
    }

    if (m_animNode)
    {
        m_animNode->OnStartPlayInEditor();
    }
}

void CTrackViewAnimNode::OnStopPlayInEditor()
{
    // restore sequenceComponent entity Ids back to their original Editor Ids
    if (m_animSequence && m_stashedAnimSequenceEditorAzEntityId.IsValid())
    {
        m_animSequence->SetSequenceEntityId(m_stashedAnimSequenceEditorAzEntityId);

        // invalidate the stashed Id now that we've restored it
        m_stashedAnimSequenceEditorAzEntityId.SetInvalid();
    }

    if (m_animNode && m_stashedAnimNodeEditorAzEntityId.IsValid())
    {
        m_animNode->SetAzEntityId(m_stashedAnimNodeEditorAzEntityId);

        // invalidate the stashed Id now that we've restored it
        m_stashedAnimNodeEditorAzEntityId.SetInvalid();
    }

    if (m_animNode)
    {
        m_animNode->OnStopPlayInEditor();
    }
}

void CTrackViewAnimNode::OnEntityActivated(const AZ::EntityId& activatedEntityId)
{
    if (GetAzEntityId() != activatedEntityId)
    {
        // This can happen when we're exiting Game/Sim Mode and entity Id's are remapped. Do nothing in such a circumstance.
        return;
    }

    CTrackViewDialog* dialog = CTrackViewDialog::GetCurrentInstance();
    if ((dialog && dialog->IsDoingUndoOperation()) || GetAzEntityId() != activatedEntityId)
    {
        // Do not respond during Undo. We'll get called when we connect to the AZ::EntityBus in SetEntityNode(),
        // which would result in adding component nodes twice during Undo.

        // Also do not respond to entity activation notifications for entities not associated with this animNode,
        // although this should never happen
        return;
    }

    // Ensure the components on the Entity match the components on the Entity Node in Track View.
    //
    // Note this gets called as soon as we connect to AZ::EntityBus - so in effect SetNodeEntity() on an AZ::Entity results
    // in all of it's component nodes being added.
    //
    // If the component exists in Track View but not in the entity, we remove it from Track View.

    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, activatedEntityId);

    // check if all Track View components are (still) on the entity. If not, remove it from TrackView
    for (int i = GetChildCount(); --i >= 0;)
    {
        if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
            if (childAnimNode->GetComponentId() != AZ::InvalidComponentId && !(entity->FindComponent(childAnimNode->GetComponentId())))
            {
                // Check to see if the component is still on the entity, but just disabled. Don't remove it in that case.
                AZ::Entity::ComponentArrayType disabledComponents;
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(
                    entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

                bool isDisabled = false;
                for (auto disabledComponent : disabledComponents)
                {
                    if (disabledComponent->GetId() == childAnimNode->GetComponentId())
                    {
                        isDisabled = true;
                        break;
                    }
                }

                // Check to see if the component is still on the entity, but just pending. Don't remove it in that case.
                AZ::Entity::ComponentArrayType pendingComponents;
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(
                    entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);

                bool isPending = false;
                for (auto pendingComponent : pendingComponents)
                {
                    if (pendingComponent->GetId() == childAnimNode->GetComponentId())
                    {
                        isPending = true;
                        break;
                    }
                }

                if (!isDisabled && !isPending)
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Remove Track View Component Node");
                    RemoveSubNode(childAnimNode);
                    CTrackViewSequence* sequence = GetSequence();
                    AZ_Assert(sequence != nullptr, "Sequence should not be null");
                    undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
                }
                else
                {
                    // don't remove this node, but do disable it.
                    if (childAnimNode->m_animNode)
                    {
                        int flags = childAnimNode->m_animNode->GetFlags();
                        flags |= eAnimNodeFlags_DisabledForComponent;
                        childAnimNode->m_animNode->SetFlags(flags);
                        childAnimNode->SetDisabled(true);
                    }
                }
            }
            else
            {
                // re-enable the node if it was disabled because of a missing component
                if (childAnimNode->m_animNode)
                {
                    int flags = childAnimNode->m_animNode->GetFlags();
                    if (flags & eAnimNodeFlags_DisabledForComponent)
                    {
                        flags &= ~eAnimNodeFlags_DisabledForComponent;
                        childAnimNode->m_animNode->SetFlags(flags);
                        childAnimNode->SetDisabled(false);
                    }
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // check that all animatable components on the Entity are in Track View

    AZStd::vector<AZ::ComponentId> animatableComponentIds;

    // Get all components animated through the behavior context
    Maestro::EditorSequenceComponentRequestBus::Event(GetSequence()->GetSequenceComponentEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::GetAnimatableComponents,
                                                          animatableComponentIds, activatedEntityId);

    for (const AZ::ComponentId& componentId : animatableComponentIds)
    {
        bool componentFound = false;
        for (int i = GetChildCount(); --i >= 0;)
        {
            if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
                if (childAnimNode->GetComponentId() == componentId)
                {
                    componentFound = true;
                    break;
                }
            }
        }
        if (!componentFound)
        {
            bool disabled = false;
            const AZ::Component* component = entity->FindComponent(componentId);

            // If not found in enabled components, check disabled and pending components
            if (!component)
            {
                // Disable the node when it is created because the component is not enabled.
                disabled = true;

                // Check in disabled components
                AZ::Entity::ComponentArrayType disabledComponents;
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

                for (AZ::Component* currentComponent : disabledComponents)
                {
                    if (currentComponent->GetId() == componentId)
                    {
                        component = currentComponent;
                        break;
                    }
                }

                // Check in pending components
                if (!component)
                {
                    AZ::Entity::ComponentArrayType pendingComponents;
                    AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
                    for (AZ::Component* currentComponent : pendingComponents)
                    {
                        if (currentComponent->GetId() == componentId)
                        {
                            component = currentComponent;
                            break;
                        }
                    }
                }
            }

            if (component)
            {
                AddComponent(component, disabled);
            }
        }
    }

    // Refresh the sequence because things may have been enabled/disabled.
    GetSequence()->ForceAnimation();
}

void CTrackViewAnimNode::OnEntityRemoved()
{
    // This is called by CTrackViewSequenceManager for both legacy and AZ Entities. When we deprecate legacy entities,
    // we could (should) probably handle this via ComponentApplicationEventBus::Events::OnEntityRemoved

    if (IsBoundToAzEntity())
    {
        AZ::EntityId entityId = GetAzEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        AZ::EntityBus::Handler::BusDisconnect(entityId);
    }

    m_nodeEntityId = AZ::EntityId(); // invalidate cached node entity id

    // notify the change. This leads to Track View updating it's UI to account for the entity removal
    GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
}

CTrackViewAnimNode* CTrackViewAnimNode::AddComponent(const AZ::Component* component, bool disabled)
{
    CTrackViewAnimNode* retNewComponentNode = nullptr;
    AZStd::string componentName;
    AZ::Uuid      componentTypeId(AZ::Uuid::CreateNull());

    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        componentTypeId, &AzFramework::ApplicationRequests::Bus::Events::GetComponentTypeId, GetAzEntityId(), component->GetId());

    AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
        componentName, &AzToolsFramework::EntityCompositionRequests::GetComponentName, component);

    if (!componentName.empty() && !componentTypeId.IsNull())
    {
        CTrackViewSequence* sequence = GetSequence();
        AZ_Assert(sequence, "Expected valid sequence.");

        AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Component");
        retNewComponentNode = CreateSubNode(
            componentName.c_str(), AnimNodeType::Component, AZ::EntityId(), componentTypeId, component->GetId());
        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
    }
    else
    {
        AZ_Warning("TrackView", false, "Could not determine component name or type for adding component - skipping...");
    }

    if (retNewComponentNode)
    {
        retNewComponentNode->SetDisabled(disabled);
    }

    return retNewComponentNode;
}

bool CTrackViewAnimNode::IsTransformAnimParamTypeDelegated(const AnimParamType animParamType) const
{
    const bool delegated = (GetIEditor()->GetAnimation()->IsRecording() && AzToolsFramework::IsSelected(m_nodeEntityId) &&
                             GetTrackForParameter(animParamType)) ||
        CheckTrackAnimated(animParamType);
    return delegated;
}

void CTrackViewAnimNode::OnEntityDestruction([[maybe_unused]] const AZ::EntityId& entityId)
{
    UnRegisterEditorObjectListeners();

    SetNodeEntityId(AZ::EntityId());
}

AZ::Transform CTrackViewAnimNode::GetEntityWorldTM(const AZ::EntityId entityId)
{
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

    AZ::Transform worldTM = AZ::Transform::Identity();
    if (entity != nullptr)
    {
        AZ::TransformInterface* transformInterface = entity->GetTransform();
        if (transformInterface != nullptr)
        {
            worldTM = transformInterface->GetWorldTM();
        }
    }

    return worldTM;
}

void CTrackViewAnimNode::UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM)
{
    // Update the Position, Rotation and Scale tracks.
    AZStd::vector<AnimParamType> animParamTypes{ AnimParamType::Position, AnimParamType::Rotation, AnimParamType::Scale };
    for (AnimParamType animParamType : animParamTypes)
    {
        CTrackViewTrack* track = GetTrackForParameter(animParamType);
        if (track != nullptr)
        {
            track->UpdateKeyDataAfterParentChanged(oldParentWorldTM, newParentWorldTM);
        }
    }

    // Refresh after key data changed or parent changed.
    CTrackViewSequence* sequence = GetSequence();
    if (sequence != nullptr)
    {
        sequence->OnKeysChanged();
    }
}

void CTrackViewAnimNode::OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent)
{
    // If the change is from no parent to parent, or the other way around,
    // update the key data, because that action is like going from world space to
    // relative to a new parent.
    if (!oldParent.IsValid() || !newParent.IsValid())
    {
        // Get the world transforms, Identity if there was no parent
        AZ::Transform oldParentWorldTM = GetEntityWorldTM(oldParent);
        AZ::Transform newParentWorldTM = GetEntityWorldTM(newParent);

        UpdateKeyDataAfterParentChanged(oldParentWorldTM, newParentWorldTM);
    }

    // Refresh after key data changed or parent changed.
    CTrackViewSequence* sequence = GetSequence();
    if (sequence != nullptr)
    {
        sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
    }
}

void CTrackViewAnimNode::OnParentTransformWillChange(AZ::Transform oldTransform, AZ::Transform newTransform)
{
    // Only used in circumstances where modified keys are required, but OnParentChanged
    // message will not be received for some reason, e.g. node being cloned in memory
    UpdateKeyDataAfterParentChanged(oldTransform, newTransform);

    CTrackViewSequence* sequence = GetSequence();
    if (sequence != nullptr)
    {
        sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
    }
}

void CTrackViewAnimNode::RegisterEditorObjectListeners(const AZ::EntityId entityId)
{
    EntitySelectionEvents::Bus::Handler::BusConnect(entityId);
}

void CTrackViewAnimNode::UnRegisterEditorObjectListeners()
{
    EntitySelectionEvents::Bus::Handler::BusDisconnect();
}
