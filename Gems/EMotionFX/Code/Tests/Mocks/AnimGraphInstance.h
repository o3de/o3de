/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class AnimGraphInstance
    {
    public:
        virtual ~AnimGraphInstance() = default;

        //void Output(Pose* outputPose);
        //void Start();
        //void Stop();
        //ActorInstance* GetActorInstance() const;
        //AnimGraph* GetAnimGraph() const;
        //MotionSet* GetMotionSet() const;
        //void SetParentAnimGraphInstance(AnimGraphInstance* parentAnimGraphInstance);
        //AnimGraphInstance* GetParentAnimGraphInstance() const;
        //void RemoveChildAnimGraphInstance(AnimGraphInstance* childAnimGraphInstance);
        //bool GetParameterValueAsFloat(const char* paramName, float* outValue);
        //bool GetParameterValueAsBool(const char* paramName, bool* outValue);
        //bool GetParameterValueAsInt(const char* paramName, int32* outValue);
        //bool GetVector2ParameterValue(const char* paramName, AZ::Vector2* outValue);
        //bool GetVector3ParameterValue(const char* paramName, AZ::Vector3* outValue);
        //bool GetVector4ParameterValue(const char* paramName, AZ::Vector4* outValue);
        //bool GetRotationParameterValue(const char* paramName, MCore::Quaternion* outRotation);
        //bool GetParameterValueAsFloat(size_t paramIndex, float* outValue);
        //bool GetParameterValueAsBool(size_t paramIndex, bool* outValue);
        //bool GetParameterValueAsInt(size_t paramIndex, int32* outValue);
        //bool GetVector2ParameterValue(size_t paramIndex, AZ::Vector2* outValue);
        //bool GetVector3ParameterValue(size_t paramIndex, AZ::Vector3* outValue);
        //bool GetVector4ParameterValue(size_t paramIndex, AZ::Vector4* outValue);
        //bool GetRotationParameterValue(size_t paramIndex, MCore::Quaternion* outRotation);
        //void SetMotionSet(MotionSet* motionSet);
        //void CreateParameterValues();
        MOCK_METHOD0(AddMissingParameterValues, void());
        MOCK_METHOD1(ReInitParameterValue, void(size_t index));
        MOCK_METHOD0(ReInitParameterValues, void());
        MOCK_METHOD2(RemoveParameterValueImpl, void(size_t index, bool delFromMem));
        virtual void RemoveParameterValue(size_t index, bool delFromMem = true) { RemoveParameterValueImpl(index, delFromMem); }
        //void AddParameterValue();
        MOCK_METHOD2(MoveParameterValue, void(size_t oldIndex, size_t newIndex));
        MOCK_METHOD1(InsertParameterValue, void(size_t index));
        //void RemoveAllParameters(bool delFromMem);
        //template <typename T>
        //T* GetParameterValueChecked(size_t index) const;
        //MCore::Attribute* GetParameterValue(size_t index) const;
        //MCore::Attribute* FindParameter(const AZStd::string& name) const;
        //AZ::Outcome<size_t> FindParameterIndex(const AZStd::string& name) const;
        //bool SwitchToState(const char* stateName);
        //bool TransitionToState(const char* stateName);
        //void ResetUniqueData();
        //void UpdateUniqueData();
        //void ApplyMotionExtraction();
        //void RecursiveResetFlags(uint32 flagsToDisable);
        //void ResetFlagsForAllObjects(uint32 flagsToDisable);
        //void ResetFlagsForAllNodes(uint32 flagsToDisable);
        //void ResetFlagsForAllObjects();
        //void ResetPoseRefCountsForAllNodes();
        //void ResetRefDataRefCountsForAllNodes();
        //void InitInternalAttributes();
        //size_t GetNumInternalAttributes() const;
        //MCore::Attribute* GetInternalAttribute(size_t attribIndex) const;
        //void RemoveAllInternalAttributes();
        //void ReserveInternalAttributes(size_t totalNumInternalAttributes);
        //void RemoveInternalAttribute(size_t index, bool delFromMem = true);
        //uint32 AddInternalAttribute(MCore::Attribute* attribute);
        //void AddUniqueObjectData();
        //void RegisterUniqueObjectData(AnimGraphObjectData* data);
        //AnimGraphObjectData* GetUniqueObjectData(size_t index);
        //size_t GetNumUniqueObjectDatas() const;
        //void SetUniqueObjectData(size_t index, AnimGraphObjectData* data);
        //void RemoveUniqueObjectData(size_t index, bool delFromMem);
        //void RemoveUniqueObjectData(AnimGraphObjectData* uniqueData, bool delFromMem);
        //void RemoveAllObjectData(bool delFromMem);
        //void Update(float timePassedInSeconds);
        //void OutputEvents();
        //void SetAutoUnregisterEnabled(bool enabled);
        //bool GetAutoUnregisterEnabled() const;
        //void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        //bool GetIsOwnedByRuntime() const;
        //ActorInstance* FindActorInstanceFromParentDepth(uint32 parentDepth) const;
        //void SetVisualizeScale(float scale);
        //float GetVisualizeScale() const;
        //void SetVisualizationEnabled(bool enabled);
        //bool GetVisualizationEnabled() const;
        //bool GetRetargetingEnabled() const;
        //void SetRetargetingEnabled(bool enabled);
        //AnimGraphNode* GetRootNode() const;
        //void AddEventHandler(AnimGraphInstanceEventHandler* eventHandler);
        //void RemoveEventHandler(AnimGraphInstanceEventHandler* eventHandler);
        //void RemoveAllEventHandlers();
        //void OnStateEnter(AnimGraphNode* state);
        //void OnStateEntering(AnimGraphNode* state);
        //void OnStateExit(AnimGraphNode* state);
        //void OnStateEnd(AnimGraphNode* state);
        //void OnStartTransition(AnimGraphStateTransition* transition);
        //void OnEndTransition(AnimGraphStateTransition* transition);
        //void CollectActiveAnimGraphNodes(AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType = AZ::TypeId::CreateNull());
        //void CollectActiveNetTimeSyncNodes(AZStd::vector<AnimGraphNode*>* outNodes);
        //uint32 GetObjectFlags(uint32 objectIndex) const;
        //void SetObjectFlags(uint32 objectIndex, uint32 flags);
        //void EnableObjectFlags(uint32 objectIndex, uint32 flagsToEnable);
        //void DisableObjectFlags(uint32 objectIndex, uint32 flagsToDisable);
        //void SetObjectFlags(uint32 objectIndex, uint32 flags, bool enabled);
        //bool GetIsObjectFlagEnabled(uint32 objectIndex, uint32 flag) const;
        //bool GetIsOutputReady(uint32 objectIndex) const;
        //void SetIsOutputReady(uint32 objectIndex, bool isReady);
        //bool GetIsSynced(uint32 objectIndex) const;
        //void SetIsSynced(uint32 objectIndex, bool isSynced);
        //bool GetIsResynced(uint32 objectIndex) const;
        //void SetIsResynced(uint32 objectIndex, bool isResynced);
        //bool GetIsUpdateReady(uint32 objectIndex) const;
        //void SetIsUpdateReady(uint32 objectIndex, bool isReady);
        //bool GetIsTopDownUpdateReady(uint32 objectIndex) const;
        //void SetIsTopDownUpdateReady(uint32 objectIndex, bool isReady);
        //bool GetIsPostUpdateReady(uint32 objectIndex) const;
        //void SetIsPostUpdateReady(uint32 objectIndex, bool isReady);
        //const InitSettings& GetInitSettings() const;
        //const AnimGraphEventBuffer& GetEventBuffer() const;
        //void AddFollowerGraph(AnimGraphInstance* follower, bool registerLeaderInsideFollower);
        //void RemoveFollowerGraph(AnimGraphInstance* follower, bool removeLeaderFromFollower);
        //AZStd::vector<AnimGraphInstance*>& GetFollowerGraphs();
        //void CreateSnapshot(bool authoritative);
        //void SetSnapshotSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer> serializer);
        //void SetSnapshotChunkSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer> serializer);
        //const AZStd::shared_ptr<AnimGraphSnapshot> GetSnapshot() const;
        //bool IsNetworkEnabled() const;
        //MCore::LcgRandom& GetLcgRandom();
        //void OnNetworkConnected();   
        //void OnNetworkParamUpdate(const AttributeContainer& parameters);
        //void OnNetworkActiveNodesUpdate(const AZStd::vector<AZ::u32>& activeNodes);
        //void OnNetworkMotionNodePlaytimesUpdate(const MotionNodePlaytimeContainer& motionNodePlaytimes);
        //void SetAutoReleaseRefDatas(bool automaticallyFreeRefDatas);
        //void SetAutoReleasePoses(bool automaticallyFreePoses);
        //void ReleaseRefDatas();
        //void ReleasePoses();
    };
} // namespace EMotionFX
