/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewSequenceManager.h"

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

// CryCommon
#include <CryCommon/Maestro/Bus/EditorSequenceComponentBus.h>
#include <CryCommon/Maestro/Types/SequenceType.h>

// AzCore
#include <AzCore/std/sort.h>


// Editor
#include "AnimationContext.h"
#include "GameEngine.h"


CTrackViewSequenceManager::CTrackViewSequenceManager()
{
    GetIEditor()->RegisterNotifyListener(this);

    AZ::EntitySystemBus::Handler::BusConnect();
}

CTrackViewSequenceManager::~CTrackViewSequenceManager()
{
    AZ::EntitySystemBus::Handler::BusDisconnect();

    GetIEditor()->UnregisterNotifyListener(this);
}

void CTrackViewSequenceManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        ResumeAllSequences();
        break;
    case eNotify_OnCloseScene:
    // Fall through
    case eNotify_OnBeginLoad:
        m_bUnloadingLevel = true;
        break;
    case eNotify_OnEndNewScene:
    // Fall through
    case eNotify_OnEndSceneOpen:
    // Fall through
    case eNotify_OnEndLoad:
    // Fall through
    case eNotify_OnLayerImportEnd:
        m_bUnloadingLevel = false;
        SortSequences();
        break;
    }
}

CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByName(QString name) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* sequence = (*iter).get();

        if (QString::fromUtf8(sequence->GetName().c_str()) == name)
        {
            return sequence;
        }
    }

    return nullptr;
}

CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByEntityId(const AZ::EntityId& entityId) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* sequence = (*iter).get();

        if (sequence->GetSequenceComponentEntityId() == entityId)
        {
            return sequence;
        }
    }

    return nullptr;
}

CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByAnimSequence(IAnimSequence* pAnimSequence) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* sequence = (*iter).get();

        if (sequence->m_pAnimSequence == pAnimSequence)
        {
            return sequence;
        }
    }

    return nullptr;
}

CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByIndex(unsigned int index) const
{
    if (index >= m_sequences.size())
    {
        return nullptr;
    }

    return m_sequences[index].get();
}

void CTrackViewSequenceManager::CreateSequence(QString name, [[maybe_unused]] SequenceType sequenceType)
{
    CGameEngine* pGameEngine = GetIEditor()->GetGameEngine();
    if (!pGameEngine || !pGameEngine->IsLevelLoaded())
    {
        return;
    }

    CTrackViewSequence* pExistingSequence = GetSequenceByName(name);
    if (pExistingSequence)
    {
        return;
    }

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Create TrackView Sequence");
    }

    // create AZ::Entity at the current center of the viewport, but don't select it

    // Store the current selection for selection restore after the sequence component is created
    AzToolsFramework::EntityIdList selectedEntities;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

    AZ::EntityId newEntityId;   // initialized with InvalidEntityId
    AzToolsFramework::EditorRequests::Bus::BroadcastResult(
        newEntityId, &AzToolsFramework::EditorRequests::Bus::Events::CreateNewEntity, AZ::EntityId());
    if (newEntityId.IsValid())
    {
        // set the entity name
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);
        if (entity)
        {
            entity->SetName(static_cast<const char*>(name.toUtf8().data()));
        }

        // add the SequenceComponent. The SequenceComponent's Init() method will call OnCreateSequenceObject() which will actually create
        // the sequence and connect it
        // #TODO LY-21846: Use "SequenceService" to find component, rather than specific component-type.
        AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, AzToolsFramework::EntityIdList{ newEntityId }, AZ::ComponentTypeList{ AZ::TypeId("{C02DC0E2-D0F3-488B-B9EE-98E28077EC56}") });

        // restore the Editor selection
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities, selectedEntities);

        if (undoBatch)
        {
            undoBatch->MarkEntityDirty(newEntityId);
        }
    }
    else if (undoBatch)
    {
        undoBatch.reset();
    }
}

IAnimSequence* CTrackViewSequenceManager::OnCreateSequenceObject(QString name, bool isLegacySequence, AZ::EntityId entityId)
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

    // Drop legacy sequences on the floor, they are no longer supported.
    if (isLegacySequence && movieSystem)
    {
        movieSystem->LogUserNotificationMsg(AZStd::string::format("Legacy Sequences are no longer supported. Skipping '%s'.", name.toUtf8().data()));
        return nullptr;
    }

    if (movieSystem)
    {
        IAnimSequence* sequence = movieSystem->CreateSequence(name.toUtf8().data(), /*bload =*/ false, /*id =*/ 0U, SequenceType::SequenceComponent, entityId);
        AZ_Assert(sequence, "Failed to create sequence");
        AddTrackViewSequence(new CTrackViewSequence(sequence));

        return sequence;
    }
    else
    {
        return nullptr;
    }
}

void CTrackViewSequenceManager::OnSequenceActivated(const AZ::EntityId& entityId)
{
    if (!entityId.IsValid())
    {
        AZ_Assert(entityId.IsValid(), "Expected valid EntityId.");
        return;
    }

    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    if (pAnimationContext != nullptr)
    {
        pAnimationContext->OnSequenceActivated(entityId);
    }
}

void CTrackViewSequenceManager::OnSequenceDeactivated(const AZ::EntityId& entityId)
{
    if (!entityId.IsValid())
    {
        AZ_Assert(entityId.IsValid(), "Expected valid EntityId.");
        return;
    }

    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    if (pAnimationContext != nullptr)
    {
        pAnimationContext->OnSequenceDeactivated(entityId);
    }
}

void CTrackViewSequenceManager::OnCreateSequenceComponent(AZStd::intrusive_ptr<IAnimSequence>& sequence)
{
    if (!sequence.get())
    {
        AZ_Assert(false, "Expected a valid sequence pointer.");
        return;
    }

    // Fix up the internal pointers in the sequence to match the deserialized structure
    sequence->InitPostLoad();

    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

    // Add the sequence to the movie system
    if (movieSystem)
    {
        movieSystem->AddSequence(sequence.get());
    }

    // Create the TrackView Sequence
    CTrackViewSequence* newTrackViewSequence = new CTrackViewSequence(sequence);

    AddTrackViewSequence(newTrackViewSequence);
}

void CTrackViewSequenceManager::AddTrackViewSequence(CTrackViewSequence* sequenceToAdd)
{
    if (!sequenceToAdd)
    {
        AZ_Assert(false, "Expected a valid sequence pointer.");
        return;
    }

    m_sequences.push_back(AZStd::unique_ptr<CTrackViewSequence>(sequenceToAdd));
    SortSequences();
    OnSequenceAdded(sequenceToAdd);
}

void CTrackViewSequenceManager::DeleteSequence(CTrackViewSequence* sequence)
{
    if (!sequence)
    {
        AZ_Assert(false, "Expected a valid sequence pointer.");
        return;
    }

    // Find sequence
    const int numSequences = static_cast<int>(m_sequences.size());
    for (int sequenceIndex = 0; sequenceIndex < numSequences; ++sequenceIndex)
    {
        if (m_sequences[sequenceIndex].get() != sequence)
        {
            continue;
        }

        // Sequence found, now find entity and sequence component
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = sequence->m_pAnimSequence->GetSequenceEntityId();
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        if (!entity)
        {
            AZ_Error("TrackViewSequenceManager", false, "DeleteSequence('%s'): Invalid entity.", sequence->GetName().c_str());
            return;
        }
        const AZ::Uuid editorSequenceComponentTypeId(EditorSequenceComponentTypeId);
        AZ::Component* sequenceComponent = entity->FindComponent(editorSequenceComponentTypeId);
        if (!sequenceComponent)
        {
            AZ_Error("TrackViewSequenceManager", false, "DeleteSequence('%s'): Invalid sequence component.", sequence->GetName().c_str());
            return;
        }

        AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
        if (!AzToolsFramework::UndoRedoOperationInProgress())
        {
            undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Delete TrackView Sequence");
        }

        // Delete Sequence Component (and entity if there's no other components left on the entity except for the Transform Component)
        AZ::ComponentTypeList requiredComponents;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(requiredComponents, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetRequiredComponentTypes);
        const int numComponentToDeleteEntity = static_cast<int>(requiredComponents.size() + 1);

        AZ::Entity::ComponentArrayType entityComponents = entity->GetComponents();
        if (entityComponents.size() == numComponentToDeleteEntity)
        {
            // if the entity only has required components + 1 (the found sequenceComponent), delete the Entity. No need to start undo here
            // AzToolsFramework::ToolsApplicationRequests::DeleteEntities will take care of that
            AzToolsFramework::EntityIdList entitiesToDelete;
            entitiesToDelete.push_back(entityId);

            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::DeleteEntities, entitiesToDelete);
        }
        else
        {
            // just remove the sequence component from the entity
            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::RemoveComponents, AZ::Entity::ComponentArrayType{ sequenceComponent });
        }

        // Do not mark deleted entityId as dirty, sequence was deleted, we can stop searching.
        return;
    }
}

void CTrackViewSequenceManager::RenameNode(CTrackViewAnimNode* animNode, const char* newName) const
{
    if (!animNode || !newName || !(*newName))
    {
        AZ_Assert(animNode != nullptr, "Expected a valid AnimNode pointer.");
        AZ_Assert(newName != nullptr && (*newName) != 0, "Expected a valid new name C-string.");
        return;
    }
    CTrackViewSequence* sequence = animNode->GetSequence();
    if (!sequence)
    {
        return; // Assert raised in GetSequence() above.
    }

    AZ::EntityId entityId;
    if (animNode->IsBoundToEditorObjects())
    {
        if (animNode->GetNodeType() == eTVNT_Sequence)
        {
            if (CTrackViewSequence* sequenceNode = static_cast<CTrackViewSequence*>(animNode))
            {
                entityId = sequenceNode->GetSequenceComponentEntityId();
            }
        }
        else if (animNode->GetNodeType() == eTVNT_AnimNode)
        {
            entityId = animNode->GetNodeEntityId();
        }
    }

    if (entityId.IsValid())
    {
        AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
        if (!AzToolsFramework::UndoRedoOperationInProgress())
        {
            undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Modify Entity Name");
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        entity->SetName(newName);

        if (undoBatch)
        {
            undoBatch->MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
    else
    {
        AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
        if (!AzToolsFramework::UndoRedoOperationInProgress())
        {
            undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Rename TrackView Node");
        }
        animNode->SetName(newName);

        if (undoBatch)
        {
            undoBatch->MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
}

void CTrackViewSequenceManager::RemoveSequenceInternal(CTrackViewSequence* sequence)
{
    AZStd::unique_ptr<CTrackViewSequence> storedTrackViewSequence;
    
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        AZStd::unique_ptr<CTrackViewSequence>& currentSequence = *iter;

        if (currentSequence.get() == sequence)
        {
            // Hang onto this until we finish this function.
            currentSequence.swap(storedTrackViewSequence);

            // Remove from CryMovie and TrackView
            m_sequences.erase(iter);

            IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
            if (movieSystem)
            {
                movieSystem->RemoveSequence(sequence->m_pAnimSequence.get());
            }

            break;
        }
    }

    OnSequenceRemoved(sequence);
}

void CTrackViewSequenceManager::OnDeleteSequenceEntity(const AZ::EntityId& entityId)
{
    CTrackViewSequence* sequence = GetSequenceByEntityId(entityId);
    if (!sequence || !entityId.IsValid())
    {
        AZ_Assert(sequence, "Sequence is null.");
        AZ_Assert(entityId.IsValid(), "Expected valid EntityId.");
        return;
    }

    const bool bUndoWasSuspended = !AzToolsFramework::UndoRedoOperationInProgress() && GetIEditor()->IsUndoSuspended();

    if (bUndoWasSuspended)
    {
        GetIEditor()->ResumeUndo();
    }

    RemoveSequenceInternal(sequence);

    if (bUndoWasSuspended)
    {
        GetIEditor()->SuspendUndo();
    }
}

void CTrackViewSequenceManager::SortSequences()
{
    AZStd::stable_sort(m_sequences.begin(), m_sequences.end(),
        [](const AZStd::unique_ptr<CTrackViewSequence>& a, const AZStd::unique_ptr<CTrackViewSequence>& b) -> bool
        {
            QString aName = QString::fromUtf8(a.get()->GetName().c_str());
            QString bName = QString::fromUtf8(b.get()->GetName().c_str());
            return aName < bName;
        });
}

void CTrackViewSequenceManager::ResumeAllSequences()
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* sequence = (*iter).get();
        if (sequence)
        {
            sequence->Resume();
        }
    }
}

void CTrackViewSequenceManager::OnSequenceAdded(CTrackViewSequence* sequence)
{
    if (!sequence)
    {
        AZ_Assert(false, "Expected valid TrackViewSequence pointer.");
        return;
    }

    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnSequenceAdded(sequence);
    }
}

void CTrackViewSequenceManager::OnSequenceRemoved(CTrackViewSequence* sequence)
{
    if (!sequence)
    {
        AZ_Assert(false, "Expected valid TrackViewSequence pointer.");
        return;
    }

    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnSequenceRemoved(sequence);
    }
}

CTrackViewAnimNodeBundle CTrackViewSequenceManager::GetAllRelatedAnimNodes(const AZ::EntityId entityId) const
{
    CTrackViewAnimNodeBundle nodeBundle;

    if (!entityId.IsValid())
    {
        AZ_Assert(false, "Expected valid EntityId.")
        return nodeBundle;
    }

    const uint sequenceCount = GetCount();

    for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
    {
        if (CTrackViewSequence* sequence = GetSequenceByIndex(sequenceIndex))
        {
            nodeBundle.AppendAnimNodeBundle(sequence->GetAllOwnedNodes(entityId));
        }
    }

    return nodeBundle;
}

CTrackViewAnimNode* CTrackViewSequenceManager::GetActiveAnimNode(const AZ::EntityId entityId) const
{
    CTrackViewAnimNodeBundle nodeBundle = GetAllRelatedAnimNodes(entityId);

    const uint nodeCount = nodeBundle.GetCount();
    for (uint nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
    {
        if (CTrackViewAnimNode* animNode = nodeBundle.GetNode(nodeIndex))
        {
            if (animNode->IsActive())
            {
                return animNode;
            }
        }
    }

    return nullptr;
}

void CTrackViewSequenceManager::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
{
    if (!entityId.IsValid() || name.empty())
    {
        AZ_Assert(name.empty(), "Invalid new name.");
        AZ_Assert(entityId.IsValid(), "Expected valid EntityId.");
        return;
    }

    CTrackViewAnimNodeBundle bundle;

    // entity or component entity sequence object
    bundle = GetAllRelatedAnimNodes(entityId);

    // GetAllRelatedAnimNodes only accounts for entities in the sequences, not the sequence entities themselves. We additionally check
    // for sequence entities that have object as their entity object for renaming
    const uint sequenceCount = GetCount();
    for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
    {
        if (CTrackViewSequence* sequence = GetSequenceByIndex(sequenceIndex))
        {
            if (entityId == sequence->GetSequenceComponentEntityId())
            {
                bundle.AppendAnimNode(sequence);
            }
        }
    }

    const uint numAffectedNodes = bundle.GetCount();
    for (uint i = 0; i < numAffectedNodes; ++i)
    {
        if (CTrackViewAnimNode* animNode = bundle.GetNode(i))
        {
            animNode->SetName(name.c_str());
        }
    }

    if (numAffectedNodes > 0)
    {
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }
}

void CTrackViewSequenceManager::OnEntityDestruction(const AZ::EntityId& entityId)
{
    if (!entityId.IsValid())
    {
        AZ_Assert(false, "Expected valid EntityId.");
        return;
    }

    // we handle pre-delete instead of delete because GetAllRelatedAnimNodes() uses the ObjectManager to find node owners
    CTrackViewAnimNodeBundle bundle = GetAllRelatedAnimNodes(entityId);

    const uint numAffectedAnimNodes = bundle.GetCount();
    for (uint i = 0; i < numAffectedAnimNodes; ++i)
    {
        if (CTrackViewAnimNode* animNode = bundle.GetNode(i))
        {
            animNode->OnEntityRemoved();
        }
    }

    if (numAffectedAnimNodes > 0)
    {
        // Only reload track view if the object being deleted has related anim nodes.
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }
}
