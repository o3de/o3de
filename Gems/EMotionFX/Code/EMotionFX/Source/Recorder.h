/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Color.h>
#include "BaseObject.h"
#include <MCore/Source/Attribute.h>
#include "MCore/Source/Color.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/File.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/MultiThreadManager.h>
#include <EMotionFX/Source/ActorInstanceBus.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventInfo.h>
#include <EMotionFX/Source/KeyTrackLinearDynamic.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class AnimGraphObject;
    class AnimGraphInstance;
    class AnimGraphObject;
    class AnimGraphMotionNode;
    class AnimGraphNode;
    class AnimGraph;
    class Motion;

    class EMFX_API Recorder
        : public BaseObject
        , private EMotionFX::ActorInstanceNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_TYPE_INFO(EMotionFX::Recorder, "{4A398AA0-D7CD-48F5-937A-BECF57887DDD}")

        /**
         * The recording settings.
         */
        struct EMFX_API RecordSettings
        {
            AZ_CLASS_ALLOCATOR_DECL
            AZ_TYPE_INFO(EMotionFX::Recorder::RecordSettings, "{A3F9D69E-3543-4B9D-B6F1-C0D5BAB1EDB1}");

            AZStd::vector<ActorInstance*> m_actorInstances; /**< The actor instances to record, or specify none to record all (default=record all). */
            AZStd::unordered_set<AZ::TypeId> m_nodeHistoryTypes; /**< The array of type node type IDs to capture. Empty array means everything. */
            AZStd::unordered_set<AZ::TypeId> m_nodeHistoryTypesToIgnore; /**< The array of type node type IDs to NOT capture. Empty array means nothing to ignore. */
            uint32 m_fps; /**< The rate at which to sample (default=15). */
            uint32 m_numPreAllocTransformKeys; /**< Pre-allocate space for this amount of transformation keys per node per actor instance (default=32). */
            size_t m_initialAnimGraphAnimBytes; /**< The number of bytes to allocate to store the anim graph recording (default=2*1024*1024, which is 2mb). This is only used when actually recording anim graph internal state animation. */
            bool m_recordTransforms; /**< Record transformations? (default=true). */
            bool m_recordAnimGraphStates; /**< Record the anim graph internal state? (default=false). */
            bool m_recordNodeHistory; /**< Record the node history? (default=false). */
            bool m_historyStatesOnly; /**< Record only states in the node history? (default=false, and only used when m_recordNodeHistory is true). */
            bool m_recordScale; /**< Record scale changes when recording transforms? */
            bool m_recordEvents; /**< Record events (default=false). */
            bool m_recordMorphs; /**< Record morph target weight animation (default=true). */
            bool m_interpolate; /**< Interpolate playback? (default=false) */

            RecordSettings()
            {
                m_fps                        = 60;
                m_numPreAllocTransformKeys   = 32;
                m_initialAnimGraphAnimBytes  = 1 * 1024 * 1024;  // 1 megabyte
                m_recordTransforms           = true;
                m_recordNodeHistory          = false;
                m_historyStatesOnly          = false;
                m_recordAnimGraphStates      = false;
                m_recordEvents               = false;
                m_recordScale                = true;
                m_recordMorphs               = true;
                m_interpolate                = false;
            }

            static void Reflect(AZ::ReflectContext* context);
        };


        struct EMFX_API EventHistoryItem
        {
            EventInfo       m_eventInfo;
            size_t          m_eventIndex;            /**< The index to use in combination with GetEventManager().GetEvent(index). */
            size_t          m_trackIndex;
            AnimGraphNodeId m_emitterNodeId;
            uint32          m_animGraphId;
            AZ::Color       m_color;
            float           m_startTime;
            float           m_endTime;
            bool            m_isTickEvent;

            EventHistoryItem()
            {
                m_eventIndex         = InvalidIndex;
                m_trackIndex         = InvalidIndex;
                m_emitterNodeId      = AnimGraphNodeId();
                m_animGraphId        = MCORE_INVALIDINDEX32;

                const AZ::u32 col = MCore::GenerateColor();
                m_color = AZ::Color(
                    MCore::ExtractRed(col)/255.0f,
                    MCore::ExtractGreen(col)/255.0f,
                    MCore::ExtractBlue(col)/255.0f,
                    1.0f);
            }
        };

        struct EMFX_API NodeHistoryItem
        {
            AZStd::string           m_name;
            AZStd::string           m_motionFileName;
            float                   m_startTime;             // time the motion starts being active
            float                   m_endTime;               // time the motion stops being active
            KeyTrackLinearDynamic<float, float> m_globalWeights; // the global weights at given time values
            KeyTrackLinearDynamic<float, float> m_localWeights;  // the local weights at given time values
            KeyTrackLinearDynamic<float, float> m_playTimes;     // normalized time values (current time in the node/motion)
            uint32                  m_motionId;              // the ID of the Motion object used
            size_t                  m_trackIndex;            // the track index
            size_t                  m_cachedKey;             // a cached key
            AnimGraphNodeId         m_nodeId;                // animgraph node Id
            AnimGraphInstance*      m_animGraphInstance;     // the anim graph instance this node was recorded from
            AZ::Color               m_color;                 // the node viz color
            AZ::Color               m_typeColor;             // the node type color
            uint32                  m_animGraphId;           // the animgraph ID
            AZ::TypeId              m_nodeType;              // the node type (Uuid)
            uint32                  m_categoryId;            // the category ID
            bool                    m_isFinalized;           // is this a finalized item?

            NodeHistoryItem()
            {
                m_startTime      = 0.0f;
                m_endTime        = 0.0f;
                m_motionId       = MCORE_INVALIDINDEX32;
                m_trackIndex     = InvalidIndex;
                m_cachedKey      = InvalidIndex;
                m_nodeId         = AnimGraphNodeId();
                m_animGraphInstance = nullptr;
                m_animGraphId    = MCORE_INVALIDINDEX32;
                m_nodeType       = AZ::TypeId::CreateNull();
                m_categoryId     = MCORE_INVALIDINDEX32;
                m_color.Set(1.0f, 0.0f, 0.0f, 1.0f);
                m_typeColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
                m_isFinalized    = false;
            }
        };

        enum EValueType
        {
            VALUETYPE_GLOBALWEIGHT  = 0,
            VALUETYPE_LOCALWEIGHT   = 1,
            VALUETYPE_PLAYTIME      = 2
        };

        struct EMFX_API ExtractedNodeHistoryItem
        {
            NodeHistoryItem*    m_nodeHistoryItem = nullptr;
            size_t              m_trackIndex = 0;
            float               m_value = 0.0f;
            float               m_keyTrackSampleTime = 0.0f;

            friend bool operator< (const ExtractedNodeHistoryItem& a, const ExtractedNodeHistoryItem& b)    { return (a.m_value > b.m_value); }
            friend bool operator==(const ExtractedNodeHistoryItem& a, const ExtractedNodeHistoryItem& b)    { return (a.m_value == b.m_value); }
        };

        struct EMFX_API TransformTracks final
        {
            AZ_TYPE_INFO(EMotionFX::Recorder::TransformTracks, "{1ED907A9-EBA5-4317-86BD-4A05B145E903}");

            static void Reflect(AZ::ReflectContext* context);

            KeyTrackLinearDynamic<AZ::Vector3, AZ::Vector3>                                 m_positions;
            KeyTrackLinearDynamic<AZ::Quaternion, AZ::Quaternion>                           m_rotations;

            #ifndef EMFX_SCALE_DISABLED
            KeyTrackLinearDynamic<AZ::Vector3, AZ::Vector3>                                 m_scales;
            #endif
        };


        struct EMFX_API AnimGraphAnimObjectInfo
        {
            size_t              m_frameByteOffset;
            AnimGraphObject*   m_object;
        };


        struct EMFX_API AnimGraphAnimFrame
        {
            float                                       m_timeValue = 0.0f;
            size_t                                      m_byteOffset = 0;
            size_t                                      m_numBytes = 0;
            AZStd::vector<AnimGraphAnimObjectInfo>      m_objectInfos{};
            AZStd::vector<AZStd::unique_ptr<MCore::Attribute>>             m_parameterValues{};
        };

        struct EMFX_API AnimGraphInstanceData
        {
            AnimGraphInstance* m_animGraphInstance = nullptr;
            size_t              m_numFrames = 0;
            size_t              m_dataBufferSize = 0;
            uint8*              m_dataBuffer = nullptr;
            AZStd::vector<AnimGraphAnimFrame>   m_frames{};

            AnimGraphInstanceData() = default;
            AnimGraphInstanceData(const AnimGraphInstanceData&) = delete;
            AnimGraphInstanceData(AnimGraphInstanceData&& rhs)
            {
                if (&rhs == this)
                {
                    return;
                }
                m_animGraphInstance = rhs.m_animGraphInstance;
                m_numFrames = rhs.m_numFrames;
                m_dataBufferSize = rhs.m_dataBufferSize;
                m_dataBuffer = rhs.m_dataBuffer;
                m_frames = AZStd::move(rhs.m_frames);
                rhs.m_animGraphInstance = nullptr;
                rhs.m_numFrames = 0;
                rhs.m_dataBufferSize = 0;
                rhs.m_dataBuffer = nullptr;
                rhs.m_frames = {};
            }

            AnimGraphInstanceData& operator=(const AnimGraphInstanceData&) = delete;
            AnimGraphInstanceData& operator=(AnimGraphInstanceData&& rhs)
            {
                if (&rhs == this)
                {
                    return *this;
                }
                m_animGraphInstance = rhs.m_animGraphInstance;
                m_numFrames = rhs.m_numFrames;
                m_dataBufferSize = rhs.m_dataBufferSize;
                m_dataBuffer = rhs.m_dataBuffer;
                m_frames = AZStd::move(rhs.m_frames);
                rhs.m_animGraphInstance = nullptr;
                rhs.m_numFrames = 0;
                rhs.m_dataBufferSize = 0;
                rhs.m_dataBuffer = nullptr;
                rhs.m_frames = {};
                return *this;
            }

            ~AnimGraphInstanceData()
            {
                MCore::Free(m_dataBuffer);
            }
        };

        struct EMFX_API ActorInstanceData final
        {
            AZ_TYPE_INFO(EMotionFX::Recorder::ActorInstanceData, "{955A7EF9-5DC6-4548-BB72-10C974CF3886}");
            AZ_CLASS_ALLOCATOR_DECL

            ActorInstance*                          m_actorInstance;         // the actor instance this data is about
            AnimGraphInstanceData*                  m_animGraphData;         // the anim graph instance data
            AZStd::vector<TransformTracks>          m_transformTracks;       // the transformation tracks, one for each node
            AZStd::vector<NodeHistoryItem*>          m_nodeHistoryItems;      // node history items
            AZStd::vector<EventHistoryItem*>         m_eventHistoryItems;     // event history item
            TransformTracks                         m_actorLocalTransform;   // the actor instance's local transformation
            AZStd::vector< KeyTrackLinearDynamic<float, float> > m_morphTracks;   // morph animation data

            ActorInstanceData()
            {
                m_nodeHistoryItems.reserve(64);
                m_eventHistoryItems.reserve(1024);
                m_morphTracks.reserve(32);
                m_animGraphData = nullptr;
                m_actorInstance = nullptr;
            }

            ~ActorInstanceData()
            {
                // clear the node history items
                for (NodeHistoryItem* nodeHistoryItem : m_nodeHistoryItems)
                {
                    delete nodeHistoryItem;
                }
                m_nodeHistoryItems.clear();

                // clear the event history items
                for (auto & eventHistoryItem : m_eventHistoryItems)
                {
                    delete eventHistoryItem;
                }
                m_eventHistoryItems.clear();

                delete m_animGraphData;
            }

            static void Reflect(AZ::ReflectContext* context);
        };

        Recorder();
        ~Recorder() override;

        static void Reflect(AZ::ReflectContext* context);

        static Recorder* Create();

        bool HasRecording() const;
        void Clear();
        void StartRecording(const RecordSettings& settings);
        void UpdatePlayMode(float timeDelta);
        void Update(float timeDelta);
        void StopRecording(bool lock = true);
        void OptimizeRecording();
        bool SaveToFile(const char* outFile);
        static Recorder* LoadFromFile(const char* filename);

        void RemoveActorInstanceFromRecording(ActorInstance* actorInstance);
        void RemoveAnimGraphFromRecording(AnimGraph* animGraph);

        // ActorInstanceNotificationBus overrides
        void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

        void SampleAndApplyTransforms(float timeInSeconds, ActorInstance* actorInstance) const;
        void SampleAndApplyMainTransform(float timeInSeconds, ActorInstance* actorInstance) const;
        void SampleAndApplyAnimGraphs(float timeInSeconds) const;
        void SampleAndApplyMorphs(float timeInSeconds, ActorInstance* actorInstance) const;

        MCORE_INLINE float GetRecordTime() const                                                { return m_recordTime; }
        MCORE_INLINE float GetCurrentPlayTime() const                                           { return m_currentPlayTime; }
        MCORE_INLINE bool GetIsRecording() const                                                { return m_isRecording; }
        MCORE_INLINE bool GetIsInPlayMode() const                                               { return m_isInPlayMode; }
        MCORE_INLINE bool GetIsInAutoPlayMode() const                                           { return m_autoPlay; }
        bool GetHasRecorded(ActorInstance* actorInstance) const;
        MCORE_INLINE const RecordSettings& GetRecordSettings() const                            { return m_recordSettings; }
        const AZ::Uuid& GetSessionUuid() const                                                  { return m_sessionUuid; }
        const AZStd::vector<float>& GetTimeDeltas()                                             { return m_timeDeltas; }

        MCORE_INLINE size_t GetNumActorInstanceDatas() const                                    { return m_actorInstanceDatas.size(); }
        MCORE_INLINE ActorInstanceData& GetActorInstanceData(size_t index)                      { return *m_actorInstanceDatas[index]; }
        MCORE_INLINE const ActorInstanceData& GetActorInstanceData(size_t index) const          { return *m_actorInstanceDatas[index]; }
        size_t FindActorInstanceDataIndex(ActorInstance* actorInstance) const;

        size_t CalcMaxNodeHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const;
        size_t CalcMaxNodeHistoryTrackIndex() const;
        size_t CalcMaxEventHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const;
        size_t CalcMaxNumActiveMotions(const ActorInstanceData& actorInstanceData) const;
        size_t CalcMaxNumActiveMotions() const;

        void ExtractNodeHistoryItems(const ActorInstanceData& actorInstanceData, float timeValue, bool sort, EValueType valueType, AZStd::vector<ExtractedNodeHistoryItem>* outItems, AZStd::vector<size_t>* outMap) const;

        void StartPlayBack();
        void StopPlayBack();
        void SetCurrentPlayTime(float timeInSeconds);
        void SetAutoPlay(bool enabled);
        void Rewind();

        void Lock();
        void Unlock();

    private:
        RecordSettings                          m_recordSettings;
        AZStd::vector<ActorInstanceData*>       m_actorInstanceDatas;
        AZStd::vector<float>                    m_timeDeltas; // The value of the time deltas whenever a key is made
        AZStd::vector<AnimGraphObject*>          m_objects;
        AZStd::vector<AnimGraphNode*>           m_activeNodes;       /**< A temp array to store active animgraph nodes in. */
        MCore::Mutex                            m_lock;
        AZ::TypeId                              m_sessionUuid;
        float                                   m_recordTime;
        float                                   m_lastRecordTime;
        float                                   m_currentPlayTime;
        bool                                    m_isRecording;
        bool                                    m_isInPlayMode;
        bool                                    m_autoPlay;

        void PrepareForRecording();
        void RecordMorphs();
        void RecordCurrentFrame(float timeDelta);
        void RecordCurrentTransforms();
        bool RecordCurrentAnimGraphStates();
        void RecordMainLocalTransforms();
        void RecordEvents();
        void UpdateNodeHistoryItems();
        void ShrinkTransformTracks();
        void AddTransformKey(TransformTracks& track, const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale);
        void SampleAndApplyTransforms(float timeInSeconds, size_t actorInstanceIndex) const;
        void SampleAndApplyMainTransform(float timeInSeconds, size_t actorInstanceIndex) const;
        void SampleAndApplyAnimGraphStates(float timeInSeconds, const AnimGraphInstanceData& animGraphInstanceData) const;
        bool SaveUniqueData(AnimGraphInstance* animGraphInstance, AnimGraphObject* object, AnimGraphInstanceData& animGraphInstanceData);

        // Resizes the AnimGraphInstanceData's buffer to be big enough to hold
        // /param numBytes bytes. Returns true if the buffer is big enough
        // after the operation, false otherwise. False indicates there's not
        // enough memory to accommodate the request
        bool AssureAnimGraphBufferSize(AnimGraphInstanceData& animGraphInstanceData, size_t numBytes);
        NodeHistoryItem* FindNodeHistoryItem(const ActorInstanceData& actorInstanceData, const AnimGraphNode* node, float recordTime) const;
        size_t FindFreeNodeHistoryItemTrack(const ActorInstanceData& actorInstanceData, NodeHistoryItem* item) const;
        void FinalizeAllNodeHistoryItems();
        EventHistoryItem* FindEventHistoryItem(const ActorInstanceData& actorInstanceData, const EventInfo& eventInfo, float recordTime);
        size_t FindFreeEventHistoryItemTrack(const ActorInstanceData& actorInstanceData, EventHistoryItem* item) const;
        size_t FindAnimGraphDataFrameNumber(float timeValue) const;
    };
}   // namespace EMotionFX
