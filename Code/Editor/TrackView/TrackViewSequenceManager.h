/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSEQUENCEMANAGER_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSEQUENCEMANAGER_H
#pragma once


#include "TrackViewSequence.h"
#include "IDataBaseManager.h"

#include <AzCore/Component/EntityBus.h>

class CTrackViewSequenceManager
    : public IEditorNotifyListener
    , public IDataBaseManagerListener
    , public ITrackViewSequenceManager
    , public AZ::EntitySystemBus::Handler
{
public:
    CTrackViewSequenceManager();
    ~CTrackViewSequenceManager();

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    unsigned int GetCount() const { return static_cast<int>(m_sequences.size()); }

    void CreateSequence(QString name, SequenceType sequenceType);
    void DeleteSequence(CTrackViewSequence* pSequence);

    void RenameNode(CTrackViewAnimNode* pAnimNode, const char* newName) const;

    CTrackViewSequence* GetSequenceByName(QString name) const override;
    CTrackViewSequence* GetSequenceByEntityId(const AZ::EntityId& entityId) const override;
    CTrackViewSequence* GetSequenceByIndex(unsigned int index) const;
    CTrackViewSequence* GetSequenceByAnimSequence(IAnimSequence* pAnimSequence) const;

    CTrackViewAnimNodeBundle GetAllRelatedAnimNodes(AZ::EntityId entityId) const;
    CTrackViewAnimNode* GetActiveAnimNode(AZ::EntityId entityId) const;

    void AddListener(ITrackViewSequenceManagerListener* pListener) { stl::push_back_unique(m_listeners, pListener); }
    void RemoveListener(ITrackViewSequenceManagerListener* pListener) { stl::find_and_erase(m_listeners, pListener); }

    //  ITrackViewSequenceManager Overrides
    // Callback from SequenceObject
    IAnimSequence* OnCreateSequenceObject(QString name, bool isLegacySequence = true, AZ::EntityId entityId = AZ::EntityId()) override;
    void OnDeleteSequenceEntity(const AZ::EntityId& entityId) override;
    void OnCreateSequenceComponent(AZStd::intrusive_ptr<IAnimSequence>& sequence) override;
    void OnSequenceActivated(const AZ::EntityId& entityId) override;
    //~ ITrackViewSequenceManager Overrides

private:
    void AddTrackViewSequence(CTrackViewSequence* sequenceToAdd);
    void RemoveSequenceInternal(CTrackViewSequence* sequence);

    void SortSequences();
    void ResumeAllSequences();

    void OnSequenceAdded(CTrackViewSequence* pSequence);
    void OnSequenceRemoved(CTrackViewSequence* pSequence);

    void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) override;

    // AZ::EntitySystemBus
    void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;
    void OnEntityDestruction(const AZ::EntityId& entityId) override;

    std::vector<ITrackViewSequenceManagerListener*> m_listeners;
    std::vector<std::unique_ptr<CTrackViewSequence> > m_sequences;

    // Set to hold sequences that existed when undo transaction began
    std::set<CTrackViewSequence*> m_transactionSequences;

    bool m_bUnloadingLevel;

    // Used to handle object attach/detach
    std::unordered_map<CTrackViewNode*, Matrix34> m_prevTransforms;
};
#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSEQUENCEMANAGER_H
