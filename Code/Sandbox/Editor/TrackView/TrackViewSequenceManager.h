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

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    unsigned int GetCount() const { return m_sequences.size(); }

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

    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);

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
