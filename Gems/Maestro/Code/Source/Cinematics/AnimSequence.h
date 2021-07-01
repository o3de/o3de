/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Implementation of IAnimSequence interface.

#ifndef CRYINCLUDE_CRYMOVIE_ANIMSEQUENCE_H
#define CRYINCLUDE_CRYMOVIE_ANIMSEQUENCE_H

#pragma once

#include "IMovieSystem.h"

#include "TrackEventTrack.h"

class CAnimSequence
    : public IAnimSequence
{
public:
    AZ_CLASS_ALLOCATOR(CAnimSequence, AZ::SystemAllocator, 0)
    AZ_RTTI(CAnimSequence, "{5127191A-0E7C-4C6F-9AF2-E5544F07BF22}", IAnimSequence);

    CAnimSequence(IMovieSystem* pMovieSystem, uint32 id, SequenceType = kSequenceTypeDefault);
    CAnimSequence();
    ~CAnimSequence();

    //////////////////////////////////////////////////////////////////////////
    // for intrusive_ptr support
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    // Movie system.
    IMovieSystem* GetMovieSystem() const { return m_pMovieSystem; };

    void SetName(const char* name);
    const char* GetName() const;
    uint32 GetId() const { return m_id; }
    void ResetId() override;

    float GetTime() const { return m_time; }

    void SetLegacySequenceObject(IAnimLegacySequenceObject* legacySequenceObject) override { m_legacySequenceObject = legacySequenceObject; }
    virtual IAnimLegacySequenceObject* GetLegacySequenceObject() const override { return m_legacySequenceObject; }
    void SetSequenceEntityId(const AZ::EntityId& sequenceEntityId) override;
    const AZ::EntityId& GetSequenceEntityId() const override { return m_sequenceEntityId; }

    virtual void SetActiveDirector(IAnimNode* pDirectorNode);
    virtual IAnimNode* GetActiveDirector() const;

    virtual void SetFlags(int flags);
    virtual int GetFlags() const;
    virtual int GetCutSceneFlags(const bool localFlags = false) const;

    virtual void SetParentSequence(IAnimSequence* pParentSequence);
    virtual const IAnimSequence* GetParentSequence() const;
    virtual bool IsAncestorOf(const IAnimSequence* pSequence) const;

    void SetTimeRange(Range timeRange);
    Range GetTimeRange() { return m_timeRange; };

    void AdjustKeysToTimeRange(const Range& timeRange);

    //! Return number of animation nodes in sequence.
    int GetNodeCount() const;
    //! Get specified animation node.
    IAnimNode* GetNode(int index) const;

    IAnimNode* FindNodeByName(const char* sNodeName, const IAnimNode* pParentDirector);
    IAnimNode* FindNodeById(int nNodeId);
    virtual void ReorderNode(IAnimNode* node, IAnimNode* pPivotNode, bool next);

    void Reset(bool bSeekToStart);
    void ResetHard();
    void Pause();
    void Resume();
    bool IsPaused() const;

    virtual void OnStart();
    virtual void OnStop();
    void OnLoop() override;

    void TimeChanged(float newTime) override;

    //! Add animation node to sequence.
    bool AddNode(IAnimNode* node);
    IAnimNode* CreateNode(AnimNodeType nodeType);
    IAnimNode* CreateNode(XmlNodeRef node);
    void RemoveNode(IAnimNode* node, bool removeChildRelationships=true) override;
    //! Add scene node to sequence.
    void RemoveAll();

    virtual void Activate();
    virtual bool IsActivated() const { return m_bActive; }
    virtual void Deactivate();

    virtual void PrecacheData(float startTime);
    void PrecacheStatic(const float startTime);
    void PrecacheDynamic(float time);

    void StillUpdate();
    void Animate(const SAnimContext& ec);
    void Render();

    void InitPostLoad() override;

    void CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count);
    void PasteNodes(const XmlNodeRef& xmlNode, IAnimNode* pParent);

    //! Add/remove track events in sequence
    virtual bool AddTrackEvent(const char* szEvent);
    virtual bool RemoveTrackEvent(const char* szEvent);
    virtual bool RenameTrackEvent(const char* szEvent, const char* szNewEvent);
    virtual bool MoveUpTrackEvent(const char* szEvent);
    virtual bool MoveDownTrackEvent(const char* szEvent);
    virtual void ClearTrackEvents();

    //! Get the track events in the sequence
    virtual int GetTrackEventsCount() const;
    virtual char const* GetTrackEvent(int iIndex) const;
    virtual IAnimStringTable* GetTrackEventStringTable() { return m_pEventStrings.get(); }

    //! Call to trigger a track event
    virtual void TriggerTrackEvent(const char* event, const char* param = NULL);

    //! Track event listener
    virtual void AddTrackEventListener(ITrackEventListener* pListener);
    virtual void RemoveTrackEventListener(ITrackEventListener* pListener);

    SequenceType GetSequenceType() const override { return m_sequenceType; }

    void SetExpanded(bool expanded) override
    {
        m_expanded = expanded;
    }

    bool GetExpanded() const override
    {
        return m_expanded;
    }

    unsigned int GetUniqueTrackIdAndGenerateNext() override
    {
        AZ_Assert(m_nextTrackId < UINT_MAX, "Max unique track ids used.");
        return m_nextTrackId++;
    }

    static void Reflect(AZ::ReflectContext* context);

private:
    void ComputeTimeRange();
    void CopyNodeChildren(XmlNodeRef& xmlNode, IAnimNode* pAnimNode);
    void NotifyTrackEvent(ITrackEventListener::ETrackEventReason reason,
        const char* event, const char* param = NULL);

    // Create a new animation node.
    IAnimNode* CreateNodeInternal(AnimNodeType nodeType, uint32 nNodeId = -1);

    bool AddNodeNeedToRender(IAnimNode* pNode);
    void RemoveNodeNeedToRender(IAnimNode* pNode);

    void SetId(uint32 newId);

    int m_refCount;

    typedef AZStd::vector< AZStd::intrusive_ptr<IAnimNode> > AnimNodes;
    AnimNodes m_nodes;
    AnimNodes m_nodesNeedToRender;

    uint32 m_id;
    AZStd::string m_name;
    mutable string m_fullNameHolder;
    Range m_timeRange;
    TrackEvents m_events;

    AZStd::intrusive_ptr<IAnimStringTable> m_pEventStrings;

    // Listeners
    typedef std::list<ITrackEventListener*> TTrackEventListeners;
    TTrackEventListeners m_listeners;

    int m_flags;

    bool m_precached;
    bool m_bResetting;

    IAnimSequence* m_pParentSequence;

    //
    IMovieSystem* m_pMovieSystem;
    bool m_bPaused;
    bool m_bActive;

    uint32 m_nextGenId;

    IAnimLegacySequenceObject* m_legacySequenceObject;   // legacy sequence objects are connected by pointer

    // NOTE: for Legacy components this contains the Sequence Id so that we have a single way to find an existing sequence
    AZ::EntityId        m_sequenceEntityId;  // SequenceComponent entities are connected by Id

    IAnimNode* m_activeDirector;
    int m_activeDirectorNodeId;

    float m_time;

    SequenceType m_sequenceType;       // indicates if this sequence is connected to a legacy sequence entity or Sequence Component

    bool m_expanded;

    unsigned int m_nextTrackId = 1;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMSEQUENCE_H
