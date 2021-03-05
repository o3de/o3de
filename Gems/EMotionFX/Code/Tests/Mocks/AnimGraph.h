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

namespace EMotionFX
{
    class AnimGraph
    {
    public:
        //void RecursiveReinit();
        //bool InitAfterLoading();
        //void UpdateUniqueData();
        MOCK_CONST_METHOD0(GetFileName, const char*());
        //const AZStd::string& GetFileNameString() const;
        //void SetFileName(const char* fileName);
        //AnimGraphStateMachine* GetRootStateMachine() const;
        //void SetRootStateMachine(AnimGraphStateMachine* stateMachine);
        //AnimGraphNode* RecursiveFindNodeByName(const char* nodeName) const;
        //bool IsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const;
        MOCK_CONST_METHOD1(RecursiveFindNodeById, AnimGraphNode*(AnimGraphNodeId));
        MOCK_CONST_METHOD1(RecursiveFindTransitionById, AnimGraphStateTransition*(AnimGraphConnectionId));
        MOCK_CONST_METHOD2(RecursiveCollectNodesOfType, void(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes));
        MOCK_CONST_METHOD2(RecursiveCollectTransitionConditionsOfType, void(const AZ::TypeId& conditionType, MCore::Array<AnimGraphTransitionCondition*>* outConditions));
        MOCK_METHOD2(RecursiveCollectObjectsOfType, void(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects));
        MOCK_METHOD2(RecursiveCollectObjectsAffectedBy, void(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects));
        //uint32 RecursiveCalcNumNodes() const;
        //void RecursiveCalcStatistics(Statistics& outStatistics) const;
        //uint32 RecursiveCalcNumNodeConnections() const;
        //void DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan);
        //AZStd::string GenerateNodeName(const AZStd::unordered_set<AZStd::string>& nameReserveList, const char* prefix = "Node") const;
        MOCK_CONST_METHOD0(GetNumParameters, size_t());
        MOCK_CONST_METHOD0(GetNumValueParameters, size_t());
        MOCK_CONST_METHOD1(FindParameter, const Parameter*(size_t index));
        MOCK_CONST_METHOD1(FindValueParameter, const ValueParameter*(size_t index));
        MOCK_CONST_METHOD0(RecursivelyGetGroupParameters, GroupParameterVector());
        MOCK_CONST_METHOD0(RecursivelyGetValueParameters, const ValueParameterVector&());
        MOCK_CONST_METHOD0(GetChildParameters, const ParameterVector&());
        MOCK_CONST_METHOD0(GetChildValueParameters, ValueParameterVector());
        MOCK_CONST_METHOD1(FindParameterByName, const Parameter*(const AZStd::string& paramName));
        MOCK_CONST_METHOD1(FindValueParameterByName, const ValueParameter*(const AZStd::string& paramName));
        MOCK_CONST_METHOD1(FindGroupParameterByName, const GroupParameter*(const AZStd::string& groupName));
        MOCK_CONST_METHOD1(FindParentGroupParameter, const GroupParameter*(const Parameter* parameter));
        MOCK_CONST_METHOD1(FindParameterIndexByName, AZ::Outcome<size_t>(const AZStd::string& paramName));
        MOCK_CONST_METHOD1(FindValueParameterIndexByName, AZ::Outcome<size_t>(const AZStd::string& paramName));
        //MOCK_CONST_METHOD1(FindParameterIndex, AZ::Outcome<size_t>(const Parameter* parameter));
        MOCK_CONST_METHOD1(FindParameterIndex, AZ::Outcome<size_t>(Parameter* parameter));
        MOCK_CONST_METHOD1(FindValueParameterIndex, AZ::Outcome<size_t>(const ValueParameter* parameter));
        MOCK_CONST_METHOD1(FindRelativeParameterIndex, AZ::Outcome<size_t>(const Parameter* parameter));
        MOCK_METHOD2(AddParameter, bool(Parameter* parameter, const GroupParameter* parent));
        MOCK_METHOD3(InsertParameter, bool(size_t index, Parameter* parameter, const GroupParameter* parent));
        MOCK_METHOD2(RenameParameter, bool(Parameter* parameter, const AZStd::string& newName));
        MOCK_METHOD1(RemoveParameter, bool(Parameter* parameter));
        MOCK_METHOD1(TakeParameterFromParent, bool(const Parameter* parameter));
        MOCK_CONST_METHOD0(GetID, uint32());
        MOCK_METHOD1(SetID, void(uint32 id));
        MOCK_METHOD1(SetDirtyFlag, void(bool dirty));
        MOCK_CONST_METHOD0(GetDirtyFlag, bool());
        //void SetAutoUnregister(bool enabled);
        //bool GetAutoUnregister() const;
        //void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        //bool GetIsOwnedByRuntime() const;
        //void SetIsOwnedByAsset(bool isOwnedByAsset);
        //bool GetIsOwnedByAsset() const;
        //uint32 GetNumNodeGroups() const;
        //AnimGraphNodeGroup* GetNodeGroup(uint32 index) const;
        //AnimGraphNodeGroup* FindNodeGroupByName(const char* groupName) const;
        //uint32 FindNodeGroupIndexByName(const char* groupName) const;
        //void AddNodeGroup(AnimGraphNodeGroup* nodeGroup);
        //void RemoveNodeGroup(uint32 index, bool delFromMem = true);
        //void RemoveAllNodeGroups(bool delFromMem = true);
        //AnimGraphNodeGroup* FindNodeGroupForNode(AnimGraphNode* animGraphNode) const;
        //void FindAndRemoveCycles(AZStd::string* outRemovedConnectionsMessage = nullptr);
        //AnimGraphGameControllerSettings& GetGameControllerSettings();
        //bool GetRetargetingEnabled() const;
        //void SetRetargetingEnabled(bool enabled);
        //void RemoveAllObjectData(AnimGraphObject* object, bool delFromMem);
        //void AddObject(AnimGraphObject* object);
        //void RemoveObject(AnimGraphObject* object);
        //uint32 GetNumObjects() const;
        //AnimGraphObject* GetObject(uint32 index) const;
        //void ReserveNumObjects(uint32 numObjects);
        //uint32 GetNumNodes() const;
        //AnimGraphNode* GetNode(uint32 index) const;
        //void ReserveNumNodes(uint32 numNodes);
        //uint32 CalcNumMotionNodes() const;
        MOCK_CONST_METHOD0(GetNumAnimGraphInstances, size_t());
        MOCK_CONST_METHOD1(GetAnimGraphInstance, AnimGraphInstance*(size_t index));
        MOCK_METHOD1(ReserveNumAnimGraphInstances, void(size_t numInstances));
        MOCK_METHOD1(AddAnimGraphInstance, void(AnimGraphInstance* animGraphInstance));
        MOCK_METHOD1(RemoveAnimGraphInstance, void(AnimGraphInstance* animGraphInstance));
        //void Lock();
        //void Unlock();
        //bool SaveToFile(const AZStd::string& filename, AZ::SerializeContext* context) const;
        //void RemoveInvalidConnections(bool logWarnings=false);
    };
} // namespace EMotionFX
