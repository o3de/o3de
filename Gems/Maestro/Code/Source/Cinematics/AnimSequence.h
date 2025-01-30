/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IMovieSystem.h>
#include "TrackEventTrack.h"
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <limits>

namespace Maestro
{

    class CAnimSequence : public IAnimSequence
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimSequence, AZ::SystemAllocator)
        AZ_RTTI(CAnimSequence, "{5127191A-0E7C-4C6F-9AF2-E5544F07BF22}", IAnimSequence);

        CAnimSequence(uint32 id, SequenceType = kSequenceTypeDefault);
        CAnimSequence();
        ~CAnimSequence();

        //////////////////////////////////////////////////////////////////////////
        // for intrusive_ptr support
        void add_ref() override;
        void release() override;
        //////////////////////////////////////////////////////////////////////////

        // Movie system.
        IMovieSystem* GetMovieSystem() const
        {
            return m_pMovieSystem;
        }

        void SetName(const char* name) override;
        const char* GetName() const override;
        uint32 GetId() const override
        {
            return m_id;
        }

        void ResetId() override;

        float GetTime() const
        {
            return m_time;
        }

        void SetLegacySequenceObject(IAnimLegacySequenceObject* legacySequenceObject) override
        {
            m_legacySequenceObject = legacySequenceObject;
        }

        IAnimLegacySequenceObject* GetLegacySequenceObject() const override
        {
            return m_legacySequenceObject;
        }

        void SetSequenceEntityId(const AZ::EntityId& sequenceEntityId) override;

        const AZ::EntityId& GetSequenceEntityId() const override
        {
            return m_sequenceEntityId;
        }

        void SetActiveDirector(IAnimNode* pDirectorNode) override;
        IAnimNode* GetActiveDirector() const override;

        void SetFlags(int flags) override;
        int GetFlags() const override;
        int GetCutSceneFlags(const bool localFlags = false) const override;

        void SetParentSequence(IAnimSequence* pParentSequence) override;
        const IAnimSequence* GetParentSequence() const override;
        bool IsAncestorOf(const IAnimSequence* pSequence) const override;

        void SetTimeRange(Range timeRange) override;
        Range GetTimeRange() override
        {
            return m_timeRange;
        }

        void AdjustKeysToTimeRange(const Range& timeRange) override;

        //! Return number of animation nodes in sequence.
        int GetNodeCount() const override;
        //! Get specified animation node.
        IAnimNode* GetNode(int index) const override;

        IAnimNode* FindNodeByName(const char* sNodeName, const IAnimNode* pParentDirector) override;
        IAnimNode* FindNodeById(int nNodeId);
        void ReorderNode(IAnimNode* node, IAnimNode* pPivotNode, bool next) override;

        void Reset(bool bSeekToStart) override;
        void ResetHard() override;
        void Pause() override;
        void Resume() override;
        bool IsPaused() const override;

        virtual void OnStart();
        virtual void OnStop();
        void OnLoop() override;

        void TimeChanged(float newTime) override;

        //! Add animation node to sequence.
        bool AddNode(IAnimNode* node) override;
        IAnimNode* CreateNode(AnimNodeType nodeType) override;
        IAnimNode* CreateNode(XmlNodeRef node) override;
        void RemoveNode(IAnimNode* node, bool removeChildRelationships = true) override;
        //! Add scene node to sequence.
        void RemoveAll() override;

        void Activate() override;
        bool IsActivated() const override
        {
            return m_bActive;
        }
        void Deactivate() override;

        void PrecacheData(float startTime) override;
        void PrecacheStatic(const float startTime);
        void PrecacheDynamic(float time);

        void StillUpdate() override;
        void Animate(const SAnimContext& ec) override;
        void Render() override;

        void InitPostLoad() override;

        void CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count) override;
        void PasteNodes(const XmlNodeRef& xmlNode, IAnimNode* pParent) override;

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
        IAnimStringTable* GetTrackEventStringTable() override
        {
            return m_pEventStrings.get();
        }

        //! Call to trigger a track event
        void TriggerTrackEvent(const char* event, const char* param = nullptr) override;

        //! Track event listener
        void AddTrackEventListener(ITrackEventListener* pListener) override;
        void RemoveTrackEventListener(ITrackEventListener* pListener) override;

        SequenceType GetSequenceType() const override
        {
            return m_sequenceType;
        }

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
            AZ_Assert(m_nextTrackId < std::numeric_limits<AZ::u32>::max(), "Max unique track ids used.");
            return m_nextTrackId++;
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void ComputeTimeRange();
        void CopyNodeChildren(XmlNodeRef& xmlNode, IAnimNode* pAnimNode);
        void NotifyTrackEvent(ITrackEventListener::ETrackEventReason reason, const char* event, const char* param = nullptr);

        // Create a new animation node.
        IAnimNode* CreateNodeInternal(AnimNodeType nodeType, uint32 nNodeId = -1);

        bool AddNodeNeedToRender(IAnimNode* pNode);
        void RemoveNodeNeedToRender(IAnimNode* pNode);

        void SetId(uint32 newId);

        int m_refCount;

        typedef AZStd::vector<AZStd::intrusive_ptr<IAnimNode>> AnimNodes;
        AnimNodes m_nodes;
        AnimNodes m_nodesNeedToRender;

        uint32 m_id;
        AZStd::string m_name;
        mutable AZStd::string m_fullNameHolder;
        Range m_timeRange;
        TrackEvents m_events;

        AZStd::intrusive_ptr<IAnimStringTable> m_pEventStrings;

        // Listeners
        typedef AZStd::list<ITrackEventListener*> TTrackEventListeners;
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

        IAnimLegacySequenceObject* m_legacySequenceObject; // legacy sequence objects are connected by pointer

        // NOTE: for Legacy components this contains the Sequence Id so that we have a single way to find an existing sequence
        AZ::EntityId m_sequenceEntityId; // SequenceComponent entities are connected by Id

        IAnimNode* m_activeDirector;
        int m_activeDirectorNodeId;

        float m_time;

        SequenceType m_sequenceType; // indicates if this sequence is connected to a legacy sequence entity or Sequence Component

        bool m_expanded;

        unsigned int m_nextTrackId = 1;
    };

} // namespace Maestro
