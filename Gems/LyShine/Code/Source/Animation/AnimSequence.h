/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Implementation of IAnimSequence interface.

#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include "TrackEventTrack.h"

class CUiAnimSequence
    : public IUiAnimSequence
{
public:
    AZ_CLASS_ALLOCATOR(CUiAnimSequence, AZ::SystemAllocator)
    AZ_RTTI(CUiAnimSequence, "{AA5AB4ED-CB98-4166-953E-0FE1EF7AC61F}", IUiAnimSequence);

    CUiAnimSequence();  // required for serialization
    CUiAnimSequence(IUiAnimationSystem* pUiAnimationSystem, uint32 id);
    ~CUiAnimSequence();

    //////////////////////////////////////////////////////////////////////////
    // for intrusive_ptr support
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    // Animation system.
    IUiAnimationSystem* GetUiAnimationSystem() const override { return m_pUiAnimationSystem; };

    void SetName(const char* name) override;
    const char* GetName() const override;
    uint32 GetId() const override { return m_id; }

    float GetTime() const { return m_time; }

    void SetOwner(IUiAnimSequenceOwner* pOwner) override { m_pOwner = pOwner; }
    IUiAnimSequenceOwner* GetOwner() const override { return m_pOwner; }

    void SetActiveDirector(IUiAnimNode* pDirectorNode) override;
    IUiAnimNode* GetActiveDirector() const override;

    void SetFlags(int flags) override;
    int GetFlags() const override;
    int GetCutSceneFlags(const bool localFlags = false) const override;

    void SetParentSequence(IUiAnimSequence* pParentSequence) override;
    const IUiAnimSequence* GetParentSequence() const override;
    bool IsAncestorOf(const IUiAnimSequence* pSequence) const override;

    void SetTimeRange(Range timeRange) override;
    Range GetTimeRange() override { return m_timeRange; };

    void AdjustKeysToTimeRange(const Range& timeRange) override;

    //! Return number of animation nodes in sequence.
    int GetNodeCount() const override;
    //! Get specified animation node.
    IUiAnimNode* GetNode(int index) const override;

    IUiAnimNode* FindNodeByName(const char* sNodeName, const IUiAnimNode* pParentDirector) override;
    IUiAnimNode* FindNodeById(int nNodeId);
    void ReorderNode(IUiAnimNode* node, IUiAnimNode* pPivotNode, bool next) override;

    void Reset(bool bSeekToStart) override;
    void ResetHard() override;
    void Pause() override;
    void Resume() override;
    bool IsPaused() const override;

    virtual void OnStart();
    virtual void OnStop();
    void OnLoop() override;

    //! Add animation node to sequence.
    bool AddNode(IUiAnimNode* node) override;
    IUiAnimNode* CreateNode(EUiAnimNodeType nodeType) override;
    IUiAnimNode* CreateNode(XmlNodeRef node) override;
    void RemoveNode(IUiAnimNode* node) override;
    //! Add scene node to sequence.
    void RemoveAll() override;

    void Activate() override;
    bool IsActivated() const override { return m_bActive; }
    void Deactivate() override;

    void PrecacheData(float startTime) override;
    void PrecacheStatic(const float startTime);
    void PrecacheDynamic(float time);

    void StillUpdate() override;
    void Animate(const SUiAnimContext& ec) override;
    void Render() override;

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true, uint32 overrideId = 0, bool bResetLightAnimSet = false) override;
    void InitPostLoad(IUiAnimationSystem* pUiAnimationSystem, bool remapIds, LyShine::EntityIdMap* entityIdMap) override;

    void CopyNodes(XmlNodeRef& xmlNode, IUiAnimNode** pSelectedNodes, uint32 count) override;
    void PasteNodes(const XmlNodeRef& xmlNode, IUiAnimNode* pParent) override;

    //! Add/remove track events in sequence
    bool AddTrackEvent(const char* szEvent) override;
    bool RemoveTrackEvent(const char* szEvent) override;
    bool RenameTrackEvent(const char* szEvent, const char* szNewEvent) override;
    bool MoveUpTrackEvent(const char* szEvent) override;
    bool MoveDownTrackEvent(const char* szEvent) override;
    void ClearTrackEvents() override;

    //! Get the track events in the sequence
    int GetTrackEventsCount() const override;
    char const* GetTrackEvent(int iIndex) const override;
    IUiAnimStringTable* GetTrackEventStringTable() override { return m_pEventStrings.get(); }

    //! Call to trigger a track event
    void TriggerTrackEvent(const char* event, const char* param = nullptr) override;

    //! Track event listener
    void AddTrackEventListener(IUiTrackEventListener* pListener) override;
    void RemoveTrackEventListener(IUiTrackEventListener* pListener) override;

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    void ComputeTimeRange();
    void CopyNodeChildren(XmlNodeRef& xmlNode, IUiAnimNode* pAnimNode);
    void NotifyTrackEvent(IUiTrackEventListener::ETrackEventReason reason,
        const char* event, const char* param = nullptr);

    // Create a new animation node.
    IUiAnimNode* CreateNodeInternal(EUiAnimNodeType nodeType, uint32 nNodeId = -1);

    bool AddNodeNeedToRender(IUiAnimNode* pNode);
    void RemoveNodeNeedToRender(IUiAnimNode* pNode);

    int m_refCount;

    typedef AZStd::vector<AZStd::intrusive_ptr<IUiAnimNode>> AnimNodes;
    AnimNodes m_nodes;
    AnimNodes m_nodesNeedToRender;

    uint32 m_id;
    AZStd::string m_name;
    mutable AZStd::string m_fullNameHolder;
    Range m_timeRange;
    UiTrackEvents m_events;

    AZStd::intrusive_ptr<IUiAnimStringTable> m_pEventStrings;

    // Listeners
    typedef AZStd::list<IUiTrackEventListener*> TUiTrackEventListeners;
    TUiTrackEventListeners m_listeners;

    int m_flags;

    bool m_precached;
    bool m_bResetting;

    IUiAnimSequence* m_pParentSequence;

    IUiAnimationSystem* m_pUiAnimationSystem;
    bool m_bPaused;
    bool m_bActive;

    uint32 m_nextGenId;

    IUiAnimSequenceOwner* m_pOwner;

    IUiAnimNode* m_pActiveDirector;

    float m_time;
};
