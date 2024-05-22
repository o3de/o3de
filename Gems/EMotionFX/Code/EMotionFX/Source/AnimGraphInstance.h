/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/Outcome/Outcome.h>
#include <EMotionFX/Source/AnimGraphEventBuffer.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphSnapshot.h>
#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/Attribute.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Random.h>


namespace AZ
{
    class Vector2;
}

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class AnimGraph;
    class MotionInstance;
    class Pose;
    class MotionSet;
    class AnimGraphNode;
    class AnimGraphStateTransition;
    class AnimGraphInstanceEventHandler;
    class AnimGraphObjectData;
    class AnimGraphNodeData;

    /**
     * The anim graph instance class.
     */
    class EMFX_API AnimGraphInstance
        : public MCore::RefCounted
    {
    public:
        AZ_RTTI(AnimGraphInstance, "{2CC86AA2-AFC0-434B-A317-B102FD02E76D}")
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OBJECTFLAGS_OUTPUT_READY                = 1 << 0,
            OBJECTFLAGS_UPDATE_READY                = 1 << 1,
            OBJECTFLAGS_TOPDOWNUPDATE_READY         = 1 << 2,
            OBJECTFLAGS_POSTUPDATE_READY            = 1 << 3,
            OBJECTFLAGS_SYNCED                      = 1 << 4,
            OBJECTFLAGS_RESYNC                      = 1 << 5,
            OBJECTFLAGS_SYNCINDEX_CHANGED           = 1 << 6,
            OBJECTFLAGS_PLAYMODE_BACKWARD           = 1 << 7,
            OBJECTFLAGS_IS_SYNCLEADER               = 1 << 8
        };

        struct EMFX_API InitSettings
        {
            bool    m_preInitMotionInstances;

            InitSettings()
            {
                m_preInitMotionInstances = false;
            }
        };

        static AnimGraphInstance* Create(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings = nullptr);

        void Output(Pose* outputPose);

        void Start();
        void Stop();

        MCORE_INLINE ActorInstance* GetActorInstance() const            { return m_actorInstance; }
        MCORE_INLINE AnimGraph* GetAnimGraph() const                  { return m_animGraph; }
        MCORE_INLINE MotionSet* GetMotionSet() const                    { return m_motionSet; }

        void SetParentAnimGraphInstance(AnimGraphInstance* parentAnimGraphInstance);
        MCORE_INLINE AnimGraphInstance* GetParentAnimGraphInstance() const { return m_parentAnimGraphInstance; }
        void RemoveChildAnimGraphInstance(AnimGraphInstance* childAnimGraphInstance);

        bool GetParameterValueAsFloat(const char* paramName, float* outValue);
        bool GetParameterValueAsBool(const char* paramName, bool* outValue);
        bool GetParameterValueAsInt(const char* paramName, int32* outValue);
        bool GetVector2ParameterValue(const char* paramName, AZ::Vector2* outValue);
        bool GetVector3ParameterValue(const char* paramName, AZ::Vector3* outValue);
        bool GetVector4ParameterValue(const char* paramName, AZ::Vector4* outValue);
        bool GetRotationParameterValue(const char* paramName, AZ::Quaternion* outRotation);

        bool GetParameterValueAsFloat(size_t paramIndex, float* outValue);
        bool GetParameterValueAsBool(size_t paramIndex, bool* outValue);
        bool GetParameterValueAsInt(size_t paramIndex, int32* outValue);
        bool GetVector2ParameterValue(size_t paramIndex, AZ::Vector2* outValue);
        bool GetVector3ParameterValue(size_t paramIndex, AZ::Vector3* outValue);
        bool GetVector4ParameterValue(size_t paramIndex, AZ::Vector4* outValue);
        bool GetRotationParameterValue(size_t paramIndex, AZ::Quaternion* outRotation);

        void SetMotionSet(MotionSet* motionSet);

        void CreateParameterValues();
        void AddMissingParameterValues();   // add the missing parameters that the anim graph has to this anim graph instance
        void ReInitParameterValue(size_t index);
        void ReInitParameterValues();
        void RemoveParameterValue(size_t index, bool delFromMem = true);
        void AddParameterValue();       // add the last anim graph parameter to this instance
        void InsertParameterValue(size_t index);    // add the parameter of the animgraph, at a given index
        void MoveParameterValue(size_t oldIndex, size_t newIndex);   // move the parameter from old index to new index
        void RemoveAllParameters(bool delFromMem);

        template <typename T>
        MCORE_INLINE T* GetParameterValueChecked(size_t index) const
        {
            MCore::Attribute* baseAttrib = m_paramValues[index];
            if (baseAttrib->GetType() == T::TYPE_ID)
            {
                return static_cast<T*>(baseAttrib);
            }
            return nullptr;
        }

        MCORE_INLINE MCore::Attribute* GetParameterValue(size_t index) const                            { return m_paramValues[index]; }
        MCore::Attribute* FindParameter(const AZStd::string& name) const;
        AZ::Outcome<size_t> FindParameterIndex(const AZStd::string& name) const;

        bool SwitchToState(const char* stateName);
        bool TransitionToState(const char* stateName);

        void ResetUniqueDatas();
        void RecursiveInvalidateUniqueDatas();

        /**
         * Get the number of currently allocated unique datas.
         * Due to deferred initialization, unique datas of the anim graph objects are allocated when needed at runtime.
         * The number of allocated unique datas will equal GetNumUniqueObjectDatas() after all objects were activated.
         * @return The number of currently allocated unique datas.
         */
        size_t CalcNumAllocatedUniqueDatas() const;

        void ApplyMotionExtraction();

        void RecursiveResetFlags(uint32 flagsToDisable);
        void ResetFlagsForAllObjects(uint32 flagsToDisable);
        void ResetFlagsForAllNodes(uint32 flagsToDisable);
        void ResetFlagsForAllObjects();
        void ResetPoseRefCountsForAllNodes();
        void ResetRefDataRefCountsForAllNodes();

        void InitInternalAttributes();
        size_t GetNumInternalAttributes() const;
        MCore::Attribute* GetInternalAttribute(size_t attribIndex) const;

        void RemoveAllInternalAttributes();
        void ReserveInternalAttributes(size_t totalNumInternalAttributes);
        void RemoveInternalAttribute(size_t index, bool delFromMem = true);   // removes the internal attribute (does not update any indices of other attributes)
        size_t AddInternalAttribute(MCore::Attribute* attribute);           // returns the index of the new added attribute

        AnimGraphObjectData* FindOrCreateUniqueObjectData(const AnimGraphObject* object);
        AnimGraphNodeData* FindOrCreateUniqueNodeData(const AnimGraphNode* node);

        void AddUniqueObjectData();

        AnimGraphObjectData* GetUniqueObjectData(size_t index)                            { return m_uniqueDatas[index]; }
        size_t GetNumUniqueObjectDatas() const                                            { return m_uniqueDatas.size(); }
        void RemoveUniqueObjectData(size_t index, bool delFromMem);
        void RemoveUniqueObjectData(AnimGraphObjectData* uniqueData, bool delFromMem);
        void RemoveAllObjectData(bool delFromMem);

        void Update(float timePassedInSeconds);
        void OutputEvents();

        /**
         * Set if we want to automatically unregister the anim graph instance from the anim graph manager when we delete the anim graph instance.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have the anim graph instance unregistered, otherwise set it to false.
         */
        void SetAutoUnregisterEnabled(bool enabled);

        /**
         * Check if the anim graph instance is automatically being unregistered from the anim graph manager when this anim graph instance gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregisterEnabled() const;

        /**
         * Marks the actor as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        ActorInstance* FindActorInstanceFromParentDepth(size_t parentDepth) const;

        void SetVisualizeScale(float scale);
        float GetVisualizeScale() const;

        void SetVisualizationEnabled(bool enabled);
        bool GetVisualizationEnabled() const;

        bool GetRetargetingEnabled() const;
        void SetRetargetingEnabled(bool enabled);

        AnimGraphNode* GetRootNode() const;

        //-----------------------------------------------------------------------------------------------------------------

        /**
         * Add event handler to the anim graph instance.
         * @param eventHandler The new event handler to register, this must not be nullptr.
         */
        void AddEventHandler(AnimGraphInstanceEventHandler* eventHandler);

        /**
         * Remove the given event handler.
         * @param eventHandler A pointer to the event handler to remove.
         */
        void RemoveEventHandler(AnimGraphInstanceEventHandler* eventHandler);

        /**
         * Remove all event handlers.
         */
        void RemoveAllEventHandlers();

        void OnStateEnter(AnimGraphNode* state);
        void OnStateEntering(AnimGraphNode* state);
        void OnStateExit(AnimGraphNode* state);
        void OnStateEnd(AnimGraphNode* state);
        void OnStartTransition(AnimGraphStateTransition* transition);
        void OnEndTransition(AnimGraphStateTransition* transition);

        void CollectActiveAnimGraphNodes(AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType = AZ::TypeId::CreateNull()); // MCORE_INVALIDINDEX32 means all node types
        void CollectActiveNetTimeSyncNodes(AZStd::vector<AnimGraphNode*>* outNodes);

        MCORE_INLINE uint32 GetObjectFlags(size_t objectIndex) const                                            { return m_objectFlags[objectIndex]; }
        MCORE_INLINE void SetObjectFlags(size_t objectIndex, uint32 flags)                                      { m_objectFlags[objectIndex] = flags; }
        MCORE_INLINE void EnableObjectFlags(size_t objectIndex, uint32 flagsToEnable)                           { m_objectFlags[objectIndex] |= flagsToEnable; }
        MCORE_INLINE void DisableObjectFlags(size_t objectIndex, uint32 flagsToDisable)                         { m_objectFlags[objectIndex] &= ~flagsToDisable; }
        MCORE_INLINE void SetObjectFlags(size_t objectIndex, uint32 flags, bool enabled)
        {
            if (enabled)
            {
                m_objectFlags[objectIndex] |= flags;
            }
            else
            {
                m_objectFlags[objectIndex] &= ~flags;
            }
        }
        MCORE_INLINE bool GetIsObjectFlagEnabled(size_t objectIndex, uint32 flag) const                         { return (m_objectFlags[objectIndex] & flag) != 0; }

        MCORE_INLINE bool GetIsOutputReady(size_t objectIndex) const                                            { return (m_objectFlags[objectIndex] & OBJECTFLAGS_OUTPUT_READY) != 0; }
        MCORE_INLINE void SetIsOutputReady(size_t objectIndex, bool isReady)                                    { SetObjectFlags(objectIndex, OBJECTFLAGS_OUTPUT_READY, isReady); }

        MCORE_INLINE bool GetIsSynced(size_t objectIndex) const                                                 { return (m_objectFlags[objectIndex] & OBJECTFLAGS_SYNCED) != 0; }
        MCORE_INLINE void SetIsSynced(size_t objectIndex, bool isSynced)                                        { SetObjectFlags(objectIndex, OBJECTFLAGS_SYNCED, isSynced); }

        MCORE_INLINE bool GetIsResynced(size_t objectIndex) const                                               { return (m_objectFlags[objectIndex] & OBJECTFLAGS_RESYNC) != 0; }
        MCORE_INLINE void SetIsResynced(size_t objectIndex, bool isResynced)                                    { SetObjectFlags(objectIndex, OBJECTFLAGS_RESYNC, isResynced); }

        MCORE_INLINE bool GetIsUpdateReady(size_t objectIndex) const                                            { return (m_objectFlags[objectIndex] & OBJECTFLAGS_UPDATE_READY) != 0; }
        MCORE_INLINE void SetIsUpdateReady(size_t objectIndex, bool isReady)                                    { SetObjectFlags(objectIndex, OBJECTFLAGS_UPDATE_READY, isReady); }

        MCORE_INLINE bool GetIsTopDownUpdateReady(size_t objectIndex) const                                     { return (m_objectFlags[objectIndex] & OBJECTFLAGS_TOPDOWNUPDATE_READY) != 0; }
        MCORE_INLINE void SetIsTopDownUpdateReady(size_t objectIndex, bool isReady)                             { SetObjectFlags(objectIndex, OBJECTFLAGS_TOPDOWNUPDATE_READY, isReady); }

        MCORE_INLINE bool GetIsPostUpdateReady(size_t objectIndex) const                                        { return (m_objectFlags[objectIndex] & OBJECTFLAGS_POSTUPDATE_READY) != 0; }
        MCORE_INLINE void SetIsPostUpdateReady(size_t objectIndex, bool isReady)                                { SetObjectFlags(objectIndex, OBJECTFLAGS_POSTUPDATE_READY, isReady); }

        const InitSettings& GetInitSettings() const;
        const AnimGraphEventBuffer& GetEventBuffer() const;

        void AddFollowerGraph(AnimGraphInstance* follower, bool registerLeaderInsideFollower);
        void RemoveFollowerGraph(AnimGraphInstance* follower, bool removeLeaderFromFollower);
        AZStd::vector<AnimGraphInstance*>& GetFollowerGraphs();

        // Network related functions
        void CreateSnapshot(bool authoritative);
        void SetSnapshotSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer> serializer);
        void SetSnapshotChunkSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer> serializer);
        const AZStd::shared_ptr<AnimGraphSnapshot> GetSnapshot() const { return m_snapshot; }
        bool IsNetworkEnabled() const { return GetSnapshot(); }
        MCore::LcgRandom& GetLcgRandom() { return m_lcgRandom;  }

        void OnNetworkConnected();   
        void OnNetworkParamUpdate(const AttributeContainer& parameters);
        void OnNetworkActiveNodesUpdate(const AZStd::vector<AZ::u32>& activeNodes);
        void OnNetworkMotionNodePlaytimesUpdate(const MotionNodePlaytimeContainer& motionNodePlaytimes);

        void SetAutoReleaseRefDatas(bool automaticallyFreeRefDatas);
        void SetAutoReleasePoses(bool automaticallyFreePoses);
        void ReleaseRefDatas();
        void ReleasePoses();

    private:
        AnimGraph*                                          m_animGraph;
        ActorInstance*                                      m_actorInstance;
        AnimGraphInstance*                                  m_parentAnimGraphInstance; // If this anim graph instance is in a reference node, it will have a parent anim graph instance.
        AZStd::vector<AnimGraphInstance*>                   m_childAnimGraphInstances; // If this anim graph instance contains reference nodes, the anim graph instances will be listed here.
        AZStd::vector<MCore::Attribute*>                     m_paramValues;           // a value for each AnimGraph parameter (the control parameters)
        AZStd::vector<AnimGraphObjectData*>                 m_uniqueDatas;          // unique object data
        AZStd::vector<uint32>                                m_objectFlags;           // the object flags
        using EventHandlerVector = AZStd::vector<AnimGraphInstanceEventHandler*>;
        AZStd::vector<EventHandlerVector>                   m_eventHandlersByEventType; /**< The event handler to use to process events organized by EventTypes. */
        AZStd::vector<MCore::Attribute*>                    m_internalAttributes;
        MotionSet*                                          m_motionSet;             // the used motion set
        MCore::Mutex                                        m_mutex;
        InitSettings                                        m_initSettings;
        AnimGraphEventBuffer                                m_eventBuffer;           /**< The event buffer of the last update. */
        float                                               m_visualizeScale;
        bool                                                m_autoUnregister;        /**< Specifies whether we will automatically unregister this anim graph instance set from the anim graph manager or not, when deleting this object. */
        bool                                                m_enableVisualization;
        bool                                                m_retarget;              /**< Is retargeting enabled? */

        bool                                                m_autoReleaseAllPoses;
        bool                                                m_autoReleaseAllRefDatas;
        
        AZStd::vector<AnimGraphInstance*>                   m_followerGraphs;
        AZStd::vector<AnimGraphInstance*>                   m_leaderGraphs;

        // Network related members
        AZStd::shared_ptr<AnimGraphSnapshot>                m_snapshot;
        MCore::LcgRandom                                    m_lcgRandom;

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                                                m_isOwnedByRuntime;
#endif // EMFX_DEVELOPMENT_BUILD

        AnimGraphInstance(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings = nullptr);
        ~AnimGraphInstance();
        void RecursiveSwitchToEntryState(AnimGraphNode* node);
        void RecursiveResetCurrentState(AnimGraphNode* node);
        void RecursivePrepareNode(AnimGraphNode* node);
        void InitUniqueDatas();

        void AddLeaderGraph(AnimGraphInstance* leader);
        void RemoveLeaderGraph(AnimGraphInstance* leader);
        AZStd::vector<AnimGraphInstance*>& GetLeaderGraphs();
    };
}   // namespace EMotionFX
