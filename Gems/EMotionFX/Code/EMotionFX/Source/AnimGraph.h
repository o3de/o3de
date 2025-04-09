/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <MCore/Source/Distance.h>
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraphMotionNode;
    class AnimGraphNode;
    class AnimGraphNodeGroup;
    class AnimGraphObject;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphTransitionCondition;
    class BlendTree;

    //
    class EMFX_API AnimGraph
    {
    public:
        AZ_RTTI(AnimGraph, "{BD543125-CFEE-426C-B0AC-129F2A4C6BC8}")
        AZ_CLASS_ALLOCATOR_DECL

        /**
        * Picks a random color for the anim graph.
        * @result The generated color.
        */
        static AZ::Color RandomGraphColor();

        AnimGraph();
        virtual ~AnimGraph();

        void RecursiveReinit();
        bool InitAfterLoading();

        /// Recursive invalidate unique data for all corresponding anim graph instances.
        void RecursiveInvalidateUniqueDatas();

        const char* GetFileName() const;
        const AZStd::string& GetFileNameString() const;
        void SetFileName(const char* fileName);

        AnimGraphStateMachine* GetRootStateMachine() const;
        void SetRootStateMachine(AnimGraphStateMachine* stateMachine);

        AnimGraphNode* RecursiveFindNodeByName(const char* nodeName) const;
        bool IsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const;
        AnimGraphNode* RecursiveFindNodeById(AnimGraphNodeId nodeId) const;
        AnimGraphStateTransition* RecursiveFindTransitionById(AnimGraphConnectionId transitionId) const;

        void RecursiveCollectNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const; // note: outNodes is NOT cleared internally, nodes are added to the array
        void RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, AZStd::vector<AnimGraphTransitionCondition*>* outConditions) const; // note: outNodes is NOT cleared internally, nodes are added to the array

        // Collects all objects of type and/or derived type
        void RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects);

        void RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects);

        size_t RecursiveCalcNumNodes() const;

        struct Statistics
        {
            size_t m_maxHierarchyDepth;
            size_t m_numStateMachines;
            size_t m_numStates;
            size_t m_numTransitions;
            size_t m_numWildcardTransitions;
            size_t m_numTransitionConditions;

            Statistics();
        };

        void RecursiveCalcStatistics(Statistics& outStatistics) const;
        size_t RecursiveCalcNumNodeConnections() const;

        void DecreaseInternalAttributeIndices(size_t decreaseEverythingHigherThan);

        AZStd::string GenerateNodeName(const AZStd::unordered_set<AZStd::string>& nameReserveList, const char* prefix = "Node") const;

        //-------------------------------------------------------------------------------------------------------------

        /**
        * Get the total number of parameters inside this anim graph. This will be the number of parameters from all group parameters
        * counting the groups (therefore is the amount of parameters that have a value)
        * @result The number of parameters.
        */
        size_t GetNumParameters() const;

        /**
         * Get the total number of value parameters inside this anim graph. This will be the number of parameters from all group parameters.
         * without counting the groups.
         * @result The number of value parameters.
         */
        size_t GetNumValueParameters() const;

        /**
        * Find a parameter given an index.
        * @param[in] index The index of the parameter to return (index based on all parameters, returned parameter could be a group).
        * @result A pointer to the parameter.
        */
        const Parameter* FindParameter(size_t index) const;

        /**
         * Find a value parameter given an index.
         * @param[in] index The index of the value parameter to return (index based on all the graph's value parameters).
         * @result A pointer to the value parameter.
         */
        const ValueParameter* FindValueParameter(size_t index) const;

        /**
        * Get all the group parameters contained in this anim graph, recursively. This means that if a group is contained in
        * another group within this anim graph, it is returned as well.
        * @result The group parameters contained in this anim graph, recursively.
        */
        GroupParameterVector RecursivelyGetGroupParameters() const;

        /**
        * Get all the value parameters contained in this anim graph, recursively. This means that if a group contains more value
        * parameters, those are returned as well.
        * @result The value parameters contained in this anim graph, recursively.
        */
        const ValueParameterVector& RecursivelyGetValueParameters() const;

        /**
        * Get all the parameters contained directly by this anim graph. This means that if this anim graph contains a group which contains
        * more parameters, those are not returned (but the group is).
        * @result The parameters contained directly by this anim graph.
        */
        const ParameterVector& GetChildParameters() const;

        /**
        * Get all the value parameters contained directly by this anim graph. This means that if this anim graph contains a group which contains
        * more value parameters, those are not returned.
        * @result The value parameters contained directly by this anim graph.
        */
        ValueParameterVector GetChildValueParameters() const;

        /**
         * Find parameter by name.
         * @param[in] paramName The name of the parameter to search for.
         * @result A pointer to the parameter with the given name. nullptr will be returned in case it has not been found.
         */
        const Parameter* FindParameterByName(const AZStd::string& paramName) const;

        /**
        * Find a value parameter by name. The above function is more generic and this more specific. If the client already knows that
        * is searching for a value parameter, this function will be faster to execute.
        * @param[in] paramName The name of the value parameter to search for.
        * @result A pointer to the value parameter with the given name. nullptr will be returned in case it has not been found.
        */
        const ValueParameter* FindValueParameterByName(const AZStd::string& paramName) const;

        /**
        * Find a group parameter by name.
        * @param groupName The group name to search for.
        * @result A pointer to the group parameter with the given name. nullptr will be returned in case it has not been found.
        */
        GroupParameter* FindGroupParameterByName(const AZStd::string& groupName) const;

        /**
        * Find the group parameter the given parameter is part of.
        * @param[in] parameter The parameter to search the group parameter for.
        * @result A pointer to the group parameter in which the given parameter is in. nullptr will be returned in case the parameter is not part of a group parameter.
        */
        const GroupParameter* FindParentGroupParameter(const Parameter* parameter) const;

        /**
         * Find parameter index by name.
         * @param[in] paramName The name of the parameter to search for.
         * @result The index of the parameter with the given name. AZ::Failure will be returned in case it has not been found.
         */
        AZ::Outcome<size_t> FindParameterIndexByName(const AZStd::string& paramName) const;

        /**
        * Find value parameter index by name. Index is relative to other value parameters.
        * @param[in] paramName The name of the value parameter to search for.
        * @result The index of the parameter with the given name. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindValueParameterIndexByName(const AZStd::string& paramName) const;

        /**
        * Find parameter index by parameter.
        * @param[in] parameter The parameter to search for.
        * @result The index of the parameter. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindParameterIndex(Parameter* parameter) const;
        AZ::Outcome<size_t> FindParameterIndex(const Parameter* parameter) const;

        /**
        * Find value parameter index by parameter. Index is relative to other value parameters.
        * @param[in] parameter The parameter to search for.
        * @result The index of the parameter. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindValueParameterIndex(const ValueParameter* parameter) const;

        /**
        * Find parameter index by parameter. Index is relative to its siblings.
        * @param[in] parameter The parameter to search for.
        * @result The index of the parameter. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindRelativeParameterIndex(const Parameter* parameter) const;

        /**
         * Add the given parameter.
         * The parameter will be fully managed and destroyed by this anim graph.
         * @param[in] parameter A pointer to the parameter to add.
         * @param[in] parent Parent group parameter. If nullptr, the parameter will be added to the root group parameter
         * @result true if the parameter was added
         */
        bool AddParameter(Parameter* parameter, const GroupParameter* parent = nullptr);

        /**
         * Insert the given parameter at the specified index. The index is relative to the parent.
         * The parameter will be fully managed and destroyed by this anim graph.
         * @param[in] index The index at which position the new parameter shall be inserted.
         * @param[in] parameter A pointer to the attribute info to insert.
         * @param[in] parent Parent group parameter. If nullptr, the parameter will be added to the root group parameter
         * @result success if the parameter was added, the returned index is the absolute index among all parameter values
         */
        bool InsertParameter(size_t index, Parameter* parameter, const GroupParameter* parent = nullptr);

        /**
        * Rename a parameter's name.
        * @param[in] parameter Parameter to rename.
        * @param[in] newName New name.
        */
        bool RenameParameter(Parameter* parameter, const AZStd::string& newName);

        /**
        * Removes the parameter specified by index. The parameter will be deleted.
        * @param[in] parameter The parameter to remove.
        */
        bool RemoveParameter(Parameter* parameter);

        /**
        * Iterate over all group parameters and make sure the given parameter is not part of any of the groups anymore.
        * @param[in] parameterIndex The index of the parameter to be removed from all group parameters.
        */
        bool TakeParameterFromParent(const Parameter* parameter);

        //-------------------------------------------------------------------------------------------------------------

        /**
         * Get the unique identification number for this anim graph.
         * @return The unique identification number.
         */
        uint32 GetID() const;

        /**
         * Set the unique identification number for this anim graph.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Set the dirty flag which indicates whether the user has made changes to this anim graph. This indicator is set to true
         * when the user changed something like adding a node. When the user saves this anim graph, the indicator is usually set to false.
         * @param dirty The dirty flag.
         */
        void SetDirtyFlag(bool dirty);

        /**
         * Get the dirty flag which indicates whether the user has made changes to this anim graph. This indicator is set to true
         * when the user changed something like adding a node. When the user saves this anim graph, the indicator is usually set to false.
         * @return The dirty flag.
         */
        bool GetDirtyFlag() const;

        /**
         * Set if we want to automatically unregister this anim graph from this anim graph manager when we delete this anim graph.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have this anim graph unregistered, otherwise set it to false.
         */
        void SetAutoUnregister(bool enabled);

        /**
         * Check if this anim graph is automatically being unregistered from this anim graph manager when this anim graph motion gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregister() const;

        /**
         * Marks the anim graph as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        /**
         * Marks the anim graph as owned by an asset, as opposed to the tool suite.
         */
        void SetIsOwnedByAsset(bool isOwnedByAsset);
        bool GetIsOwnedByAsset() const;

        //-------------------------------------------------------------------------------------------------------------

        /**
         * Get the number of node groups.
         * @result The number of node groups.
         */
        size_t GetNumNodeGroups() const;

        /**
         * Get a pointer to the given node group.
         * @param index The node group index, which must be in range of [0..GetNumNodeGroups()-1].
         */
        AnimGraphNodeGroup* GetNodeGroup(size_t index) const;

        /**
         * Find a node group based on the name and return a pointer.
         * @param groupName The group name to search for.
         * @result A pointer to the node group with the given name. nullptr will be returned in case there is no node group in this anim graph with the given name.
         */
        AnimGraphNodeGroup* FindNodeGroupByName(const char* groupName) const;

        /**
         * Find a node group index based on the name and return its index.
         * @param groupName The group name to search for.
         * @result The index of the node group inside this anim graph, MCORE_INVALIDINDEX32 in case the node group wasn't found.
         */
        size_t FindNodeGroupIndexByName(const char* groupName) const;

        /**
         * Add the given node group.
         * @param nodeGroup A pointer to the new node group to add.
         */
        void AddNodeGroup(AnimGraphNodeGroup* nodeGroup);

        /**
         * Remove the node group at the given index.
         * @param index The node group index to remove. This value must be in range of [0..GetNumNodeGroups()-1].
         * @param delFromMem Set to true (default) when you wish to also delete the specified group from memory.
         */
        void RemoveNodeGroup(size_t index, bool delFromMem = true);

        /**
         * Remove all node groups.
         * @param delFromMem Set to true (default) when you wish to also delete the node groups from memory.
         */
        void RemoveAllNodeGroups(bool delFromMem = true);

        /**
         * Find the node group the given node is part of and return a pointer to it.
         * @param[in] animGraphNode The node to search the node group for.
         * @result A pointer to the node group in which the given node is in. nullptr will be returned in case the node is not part of a node group.
         */
        AnimGraphNodeGroup* FindNodeGroupForNode(AnimGraphNode* animGraphNode) const;

        /**
        * Finds cycles and removes connections that produce them. This method is intended to be used after loading
        * @param[out] outRemovedConnectionsMessage if specified, it will compose a message of the connections that were removed
        */
        void FindAndRemoveCycles(AZStd::string* outRemovedConnectionsMessage = nullptr);

        //-------------------------------------------------------------------------------------------------------------

        bool GetRetargetingEnabled() const;
        void SetRetargetingEnabled(bool enabled);

        void RemoveAllObjectData(AnimGraphObject* object, bool delFromMem);    // remove all unique object datas
        void AddObject(AnimGraphObject* object);       // registers the object in the array and modifies the object's object index value
        void RemoveObject(AnimGraphObject* object);    // doesn't actually remove it from memory, just removes it from the list

        size_t GetNumObjects() const                                                          { return m_objects.size(); }
        AnimGraphObject* GetObject(size_t index) const                                        { return m_objects[index]; }
        void ReserveNumObjects(size_t numObjects);

        size_t GetNumNodes() const                                                            { return m_nodes.size(); }
        AnimGraphNode* GetNode(size_t index) const                                            { return m_nodes[index]; }
        void ReserveNumNodes(size_t numNodes);
        size_t CalcNumMotionNodes() const;

        size_t GetNumAnimGraphInstances() const                                               { return m_animGraphInstances.size(); }
        AnimGraphInstance* GetAnimGraphInstance(size_t index) const                           { return m_animGraphInstances[index]; }
        void ReserveNumAnimGraphInstances(size_t numInstances);
        void AddAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance);

        void Lock();
        void Unlock();

        static void Reflect(AZ::ReflectContext* context);

        static AnimGraph* LoadFromFile(const AZStd::string& filename, AZ::SerializeContext* context, const AZ::ObjectStream::FilterDescriptor& loadFilter = AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        static AnimGraph* LoadFromBuffer(const void* buffer, const AZStd::size_t length, AZ::SerializeContext* context);

        bool SaveToFile(const AZStd::string& filename, AZ::SerializeContext* context) const;

        void RemoveInvalidConnections(bool logWarnings=false);

    private:
        void RecursiveCalcStatistics(Statistics& outStatistics, AnimGraphNode* animGraphNode, size_t currentHierarchyDepth = 0) const;

        void OnRetargetingEnabledChanged();

        void AddValueParameterToIndexByNameCache(const size_t index, const AZStd::string& parameterName);
        void RemoveValueParameterToIndexByNameCache(const size_t index, const AZStd::string& parameterName);

        GroupParameter                                  m_rootParameter;   /**< root group parameter. */
        ValueParameterVector                            m_valueParameters;      /**< Cached version of all parameters with values. */
        AZStd::unordered_map<AZStd::string_view, size_t> m_valueParameterIndexByName; /**< Cached version of parameter index by name to accelerate lookups. */
        AZStd::vector<AnimGraphNodeGroup*>              m_nodeGroups;
        AZStd::vector<AnimGraphObject*>                 m_objects;
        AZStd::vector<AnimGraphNode*>                    m_nodes;
        AZStd::vector<AnimGraphInstance*>               m_animGraphInstances;
        AZStd::string                                   m_fileName;
        AnimGraphStateMachine*                          m_rootStateMachine;
        MCore::Mutex                                    m_lock;
        uint32                                          m_id;                    /**< The unique identification number for this anim graph. */
        bool                                            m_autoUnregister;        /**< Specifies whether we will automatically unregister this anim graph set from this anim graph manager or not, when deleting this object. */
        bool                                            m_retarget;              /**< Is retargeting enabled on default? */
        bool                                            m_dirtyFlag;             /**< The dirty flag which indicates whether the user has made changes to this anim graph since the last file save operation. */

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                                            m_isOwnedByRuntime;      /**< Set if the anim graph is used/owned by the engine runtime. */
        bool                                            m_isOwnedByAsset;        /**< Set if the anim graph is used/owned by an asset. */
#endif // EMFX_DEVELOPMENT_BUILD
    };
}   // namespace EMotionFX
