/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <MCore/Source/Array.h>
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
            AZStd::unordered_set<AZ::TypeId> mNodeHistoryTypes; /**< The array of type node type IDs to capture. Empty array means everything. */
            AZStd::unordered_set<AZ::TypeId> mNodeHistoryTypesToIgnore; /**< The array of type node type IDs to NOT capture. Empty array means nothing to ignore. */
            uint32 mFPS; /**< The rate at which to sample (default=15). */
            uint32 mNumPreAllocTransformKeys; /**< Pre-allocate space for this amount of transformation keys per node per actor instance (default=32). */
            uint32 mInitialAnimGraphAnimBytes; /**< The number of bytes to allocate to store the anim graph recording (default=2*1024*1024, which is 2mb). This is only used when actually recording anim graph internal state animation. */
            bool mRecordTransforms; /**< Record transformations? (default=true). */
            bool mRecordAnimGraphStates; /**< Record the anim graph internal state? (default=false). */
            bool mRecordNodeHistory; /**< Record the node history? (default=false). */
            bool mHistoryStatesOnly; /**< Record only states in the node history? (default=false, and only used when mRecordNodeHistory is true). */
            bool mRecordScale; /**< Record scale changes when recording transforms? */
            bool mRecordEvents; /**< Record events (default=false). */
            bool mRecordMorphs; /**< Record morph target weight animation (default=true). */
            bool mInterpolate; /**< Interpolate playback? (default=false) */

            RecordSettings()
            {
                mFPS                        = 60;
                mNumPreAllocTransformKeys   = 32;
                mInitialAnimGraphAnimBytes  = 1 * 1024 * 1024;  // 1 megabyte
                mRecordTransforms           = true;
                mRecordNodeHistory          = false;
                mHistoryStatesOnly          = false;
                mRecordAnimGraphStates      = false;
                mRecordEvents               = false;
                mRecordScale                = true;
                mRecordMorphs               = true;
                mInterpolate                = false;
            }

            static void Reflect(AZ::ReflectContext* context);
        };


        struct EMFX_API EventHistoryItem
        {
            EventInfo       mEventInfo;
            uint32          mEventIndex;            /**< The index to use in combination with GetEventManager().GetEvent(index). */
            uint32          mTrackIndex;
            AnimGraphNodeId mEmitterNodeId;
            uint32          mAnimGraphID;
            AZ::Color       mColor;
            float           mStartTime;
            float           mEndTime;
            bool            mIsTickEvent;

            EventHistoryItem()
            {
                mEventIndex         = MCORE_INVALIDINDEX32;
                mTrackIndex         = MCORE_INVALIDINDEX32;
                mEmitterNodeId      = AnimGraphNodeId();
                mAnimGraphID        = MCORE_INVALIDINDEX32;

                const AZ::u32 col = MCore::GenerateColor();
                mColor = AZ::Color(
                    MCore::ExtractRed(col)/255.0f,
                    MCore::ExtractGreen(col)/255.0f,
                    MCore::ExtractBlue(col)/255.0f,
                    1.0f);
            }
        };

        struct EMFX_API NodeHistoryItem
        {
            AZStd::string           mName;
            AZStd::string           mMotionFileName;
            float                   mStartTime;             // time the motion starts being active
            float                   mEndTime;               // time the motion stops being active
            KeyTrackLinearDynamic<float, float> mGlobalWeights; // the global weights at given time values
            KeyTrackLinearDynamic<float, float> mLocalWeights;  // the local weights at given time values
            KeyTrackLinearDynamic<float, float> mPlayTimes;     // normalized time values (current time in the node/motion)
            uint32                  mMotionID;              // the ID of the Motion object used
            uint32                  mTrackIndex;            // the track index
            uint32                  mCachedKey;             // a cached key
            AnimGraphNodeId         mNodeId;                // animgraph node Id
            AnimGraphInstance*      mAnimGraphInstance;     // the anim graph instance this node was recorded from
            AZ::Color               mColor;                 // the node viz color
            AZ::Color               mTypeColor;             // the node type color
            uint32                  mAnimGraphID;           // the animgraph ID
            AZ::TypeId              mNodeType;              // the node type (Uuid)
            uint32                  mCategoryID;            // the category ID
            bool                    mIsFinalized;           // is this a finalized item?

            NodeHistoryItem()
            {
                mStartTime      = 0.0f;
                mEndTime        = 0.0f;
                mMotionID       = MCORE_INVALIDINDEX32;
                mTrackIndex     = MCORE_INVALIDINDEX32;
                mCachedKey      = MCORE_INVALIDINDEX32;
                mNodeId         = AnimGraphNodeId();
                mAnimGraphInstance = nullptr;
                mAnimGraphID    = MCORE_INVALIDINDEX32;
                mNodeType       = AZ::TypeId::CreateNull();
                mCategoryID     = MCORE_INVALIDINDEX32;
                mColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
                mTypeColor.Set(1.0f, 0.0f, 0.0f, 1.0f);
                mIsFinalized    = false;
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
            NodeHistoryItem*    mNodeHistoryItem;
            uint32              mTrackIndex;
            float               mValue;
            float               mKeyTrackSampleTime;

            friend bool operator< (const ExtractedNodeHistoryItem& a, const ExtractedNodeHistoryItem& b)    { return (a.mValue > b.mValue); }
            friend bool operator==(const ExtractedNodeHistoryItem& a, const ExtractedNodeHistoryItem& b)    { return (a.mValue == b.mValue); }
        };

        struct EMFX_API TransformTracks final
        {
            AZ_TYPE_INFO(EMotionFX::Recorder::TransformTracks, "{1ED907A9-EBA5-4317-86BD-4A05B145E903}");

            static void Reflect(AZ::ReflectContext* context);

            KeyTrackLinearDynamic<AZ::Vector3, AZ::Vector3>                                 mPositions;
            KeyTrackLinearDynamic<AZ::Quaternion, AZ::Quaternion>                           mRotations;

            #ifndef EMFX_SCALE_DISABLED
            KeyTrackLinearDynamic<AZ::Vector3, AZ::Vector3>                                 mScales;
            #endif
        };


        struct EMFX_API AnimGraphAnimObjectInfo
        {
            uint32              mFrameByteOffset;
            AnimGraphObject*   mObject;
        };


        struct EMFX_API AnimGraphAnimFrame
        {
            float                                       mTimeValue;
            uint32                                      mByteOffset;
            uint32                                      mNumBytes;
            MCore::Array<AnimGraphAnimObjectInfo>      mObjectInfos;
            MCore::Array<MCore::Attribute*>             mParameterValues;

            AnimGraphAnimFrame()
            {
                mTimeValue  = 0.0f;
                mByteOffset = 0;
                mNumBytes   = 0;
            }

            ~AnimGraphAnimFrame()
            {
                const uint32 numParams = mParameterValues.GetLength();
                for (uint32 i = 0; i < numParams; ++i)
                {
                    delete mParameterValues[i];
                }
            }
        };

        struct EMFX_API AnimGraphInstanceData
        {
            AnimGraphInstance* mAnimGraphInstance;
            uint32              mNumFrames;
            uint32              mDataBufferSize;
            uint8*              mDataBuffer;
            MCore::Array<AnimGraphAnimFrame>   mFrames;

            AnimGraphInstanceData()
            {
                mAnimGraphInstance = nullptr;
                mNumFrames          = 0;
                mDataBufferSize     = 0;
                mDataBuffer         = nullptr;
            }

            ~AnimGraphInstanceData()
            {
                MCore::Free(mDataBuffer);
            }
        };

        struct EMFX_API ActorInstanceData final
        {
            AZ_TYPE_INFO(EMotionFX::Recorder::ActorInstanceData, "{955A7EF9-5DC6-4548-BB72-10C974CF3886}");
            AZ_CLASS_ALLOCATOR_DECL

            ActorInstance*                          mActorInstance;         // the actor instance this data is about
            AnimGraphInstanceData*                  mAnimGraphData;         // the anim graph instance data
            AZStd::vector<TransformTracks>          m_transformTracks;       // the transformation tracks, one for each node
            MCore::Array<NodeHistoryItem*>          mNodeHistoryItems;      // node history items
            MCore::Array<EventHistoryItem*>         mEventHistoryItems;     // event history item
            TransformTracks                         mActorLocalTransform;   // the actor instance's local transformation
            MCore::Array< KeyTrackLinearDynamic<float, float> > mMorphTracks;   // morph animation data

            ActorInstanceData()
            {
                mNodeHistoryItems.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
                mEventHistoryItems.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
                mMorphTracks.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
                mNodeHistoryItems.Reserve(64);
                mEventHistoryItems.Reserve(1024);
                mMorphTracks.Reserve(32);
                mAnimGraphData = nullptr;
                mActorInstance = nullptr;
            }

            ~ActorInstanceData()
            {
                // clear the node history items
                const uint32 numMotionItems = mNodeHistoryItems.GetLength();
                for (uint32 i = 0; i < numMotionItems; ++i)
                {
                    delete mNodeHistoryItems[i];
                }
                mNodeHistoryItems.Clear();

                // clear the event history items
                const uint32 numEventItems = mEventHistoryItems.GetLength();
                for (uint32 i = 0; i < numEventItems; ++i)
                {
                    delete mEventHistoryItems[i];
                }
                mEventHistoryItems.Clear();

                delete mAnimGraphData;
            }

            static void Reflect(AZ::ReflectContext* context);
        };

        Recorder();
        ~Recorder() override;

        static void Reflect(AZ::ReflectContext* context);

        static Recorder* Create();

        void Reserve(uint32 numTransformKeys);
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

        MCORE_INLINE float GetRecordTime() const                                                { return mRecordTime; }
        MCORE_INLINE float GetCurrentPlayTime() const                                           { return mCurrentPlayTime; }
        MCORE_INLINE bool GetIsRecording() const                                                { return mIsRecording; }
        MCORE_INLINE bool GetIsInPlayMode() const                                               { return mIsInPlayMode; }
        MCORE_INLINE bool GetIsInAutoPlayMode() const                                           { return mAutoPlay; }
        bool GetHasRecorded(ActorInstance* actorInstance) const;
        MCORE_INLINE const RecordSettings& GetRecordSettings() const                            { return mRecordSettings; }
        const AZ::Uuid& GetSessionUuid() const                                                  { return m_sessionUuid; }
        const AZStd::vector<float>& GetTimeDeltas()                                             { return m_timeDeltas; }

        MCORE_INLINE size_t GetNumActorInstanceDatas() const                                    { return m_actorInstanceDatas.size(); }
        MCORE_INLINE ActorInstanceData& GetActorInstanceData(uint32 index)                      { return *m_actorInstanceDatas[index]; }
        MCORE_INLINE const ActorInstanceData& GetActorInstanceData(uint32 index) const          { return *m_actorInstanceDatas[index]; }
        uint32 FindActorInstanceDataIndex(ActorInstance* actorInstance) const;

        uint32 CalcMaxNodeHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const;
        uint32 CalcMaxNodeHistoryTrackIndex() const;
        uint32 CalcMaxEventHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const;
        AZ::u32 CalcMaxNumActiveMotions(const ActorInstanceData& actorInstanceData) const;
        AZ::u32 CalcMaxNumActiveMotions() const;

        void ExtractNodeHistoryItems(const ActorInstanceData& actorInstanceData, float timeValue, bool sort, EValueType valueType, MCore::Array<ExtractedNodeHistoryItem>* outItems, MCore::Array<uint32>* outMap);

        void StartPlayBack();
        void StopPlayBack();
        void SetCurrentPlayTime(float timeInSeconds);
        void SetAutoPlay(bool enabled);
        void Rewind();

        void Lock();
        void Unlock();

    private:
        RecordSettings                          mRecordSettings;
        AZStd::vector<ActorInstanceData*>       m_actorInstanceDatas;
        AZStd::vector<float>                    m_timeDeltas; // The value of the time deltas whenever a key is made
        MCore::Array<AnimGraphObject*>          mObjects;
        AZStd::vector<AnimGraphNode*>           mActiveNodes;       /**< A temp array to store active animgraph nodes in. */
        MCore::Mutex                            mLock;
        AZ::TypeId                              m_sessionUuid;
        float                                   mRecordTime;
        float                                   mLastRecordTime;
        float                                   mCurrentPlayTime;
        bool                                    mIsRecording;
        bool                                    mIsInPlayMode;
        bool                                    mAutoPlay;

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
        bool AssureAnimGraphBufferSize(AnimGraphInstanceData& animGraphInstanceData, uint32 numBytes);
        NodeHistoryItem* FindNodeHistoryItem(const ActorInstanceData& actorInstanceData, AnimGraphNode* node, float recordTime) const;
        uint32 FindFreeNodeHistoryItemTrack(const ActorInstanceData& actorInstanceData, NodeHistoryItem* item) const;
        void FinalizeAllNodeHistoryItems();
        EventHistoryItem* FindEventHistoryItem(const ActorInstanceData& actorInstanceData, const EventInfo& eventInfo, float recordTime);
        uint32 FindFreeEventHistoryItemTrack(const ActorInstanceData& actorInstanceData, EventHistoryItem* item) const;
        uint32 FindAnimGraphDataFrameNumber(float timeValue) const;
    };
}   // namespace EMotionFX
