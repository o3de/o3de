/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/EMotionFXConfig.h>


namespace EMotionFX
{
    class EMFX_API AnimGraphNodeGroup
    {
    public:
        AZ_RTTI(AnimGraphNodeGroup, "{27CFCB13-2799-41D1-9E9F-8182FD6C90BB}")
        AZ_CLASS_ALLOCATOR_DECL

        /**
         * The default constructor.
         * This does not assign a name and there will be nodes inside this group on default.
         */
        AnimGraphNodeGroup();

        /**
         * Extended constructor.
         * @param groupName The name of the group. Please keep in mind that it is not allowed to have two groups with the same name inside a anim graph.
         */
        AnimGraphNodeGroup(const char* groupName);

        /**
         * Another extended constructor.
         * @param groupName The name of the group. Please keep in mind that it is not allowed to have two groups with the same name inside a anim graph.
         * @param numNodes The number of nodes to create inside the group. This will have all uninitialized values for the node ids in the group, so be sure that you
         *                 set them all to some valid node index using the AnimGraphNodeGroup::SetNode(...) method. This constructor automatically calls the SetNumNodes(...) method.
         */
        AnimGraphNodeGroup(const char* groupName, size_t numNodes);

        /**
         * The destructor.
         */
        virtual ~AnimGraphNodeGroup();

        /**
         * Set the name of the group. Please keep in mind that group names must be unique inside the anim graph objects. So you should not have two or more groups with the same name.
         * @param groupName The name of the group.
         */
        void SetName(const char* groupName);

        /**
         * Get the name of the group as null terminated character buffer.
         * @result The name of the group.
         */
        const char* GetName() const;

        /**
         * Get the name of the group, in form of a AZStd::string object.
         * @result The name as a reference to a AZStd::string object.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Set the color of the group.
         * @param color The 32-bit color in COLREF format (0xAABBGGRR) the nodes of the group will be filled with.
         */
        void SetColor(const AZ::u32& color);

        /**
         * Get the color of the group.
         * @result The 32-bit color in COLREF format (0xAABBGGRR) the nodes of the group will be filled with.
         */
        AZ::u32 GetColor() const;

        /**
         * Check the visibility flag.
         * This flag has been set by the user and identifies if this node group is visible or not.
         * @result True when the node group is marked as visible, otherwise false is returned.
         */
        bool GetIsVisible() const;

        /**
         * Change the visibility state.
         * @param isVisible Set to true when the node group is visible, otherwise set it to false.
         */
        void SetIsVisible(bool isVisible);

        /**
         * Set the number of nodes that remain inside this group.
         * This will resize the array of node ids. Don't forget to initialize the node values after increasing the number of nodes.
         * @param numNodes The number of nodes that are inside this group.
         */
        void SetNumNodes(size_t numNodes);

        /**
         * Get the number of nodes that remain inside this group.
         * @result The number of nodes inside this group.
         */
        size_t GetNumNodes() const;

        /**
         * Set the value of a given node.
         * @param index The node number inside this group, which must be in range of [0..GetNumNodes()-1].
         * @param nodeID The value for the given node. This is the node id where this group will belong to.
         *               To get access to the actual node object use AnimGraph::RecursiveFindNodeByID( nodeID ).
         */
        void SetNode(size_t index, AnimGraphNodeId nodeId);

        /**
         * Get the node id for a given node inside the group.
         * @param index The node number inside this group, which must be in range of [0..GetNumNodes()-1].
         * @result The node id, which points inside the Actor object. Use AnimGraph::RecursiveFindNodeByID( nodeID ) to get access to the node information.
         */
        AnimGraphNodeId GetNode(size_t index) const;

        /**
         * Check if the node with the given id is inside the node group.
         * @param nodeID The node id to check.
         * @result True in case the node with the given id is inside the node group, false if not.
         */
        bool Contains(AnimGraphNodeId nodeId) const;

        /**
         * Add a given node to this group.
         * Please keep in mind that performing an AddNode will result in a reallocation being done.
         * It is much better to use SetNumNodes(...) in combination with SetNode(...) upfront if the total number of nodes is known upfront.
         * @param nodeID The value for the given node. This is the node id where this group will belong to. To get access to the actual node object use AnimGraph::RecursiveFindNodeByID( nodeID ).
         */
        void AddNode(AnimGraphNodeId nodeId);

        /**
         * Remove a given node from the group by its node id (the value returned by AnimGraphNode::GetID().
         * If you wish to remove for example the 3rd node inside this group, then use RemoveNodeByGroupIndex(...) instead.
         * Removing a node which is not part of this group will do nothing, except that it wastes performance as it will perform a search inside the list of nodes inside this group.
         * @param nodeID The value for the given node. This is the node id where this group will belong to. To get access to the actual node object use AnimGraph::RecursiveFindNodeByID( nodeID ).
         */
        void RemoveNodeById(AnimGraphNodeId nodeId);

        /**
         * Remove a given node from the group by the array element index.
         * If for example you wish to remove the 3rd node from the group, you can call RemoveNodeByGroupIndex( 2 ).
         * If you wish to remove a node by its node id, then use RemoveNodeByID(...) instead.
         * @param index The node index in the group. So for example an index value of 5 will remove the sixth node from the group.
         *              The index value must be in range of [0..GetNumNodes() - 1].
         */
        void RemoveNodeByGroupIndex(size_t index);

        /**
         * Clear the node group. This removes all nodes.
         */
        void RemoveAllNodes();

        /**
         * Initialize the node group based on another group. Please note that the name of this group will also be copied and it is not allowed to have multiple groups with the same name in the same animgraph.
         * @param other The node group to initialize from.
         */
        void InitFrom(const AnimGraphNodeGroup& other);

        /**
         * Toggles the rendering of a QLineEdit widget to edit the name of the group from the graph widget.
         * @param nameEditOngoing Whether to trigger the rendering of a text edit widget to change the group name.
         */
        void SetNameEditOngoing(bool nameEditOngoing);

        /**
         * Checks whether the group name is currently being edited from the graph widget.
         * @result True if the group name is currently being edited, false otherwise.
         */
        bool IsNameEditOngoing() const;

        /**
         * Returns the id of the parent AnimGraphNode to which this group belongs.
         * If the group belongs to the root level, this function will return an invalid id.
         * This function is used to make sure that groups can be filtered by level.
         */
        AnimGraphNodeId GetParentNodeId() const;

        /**
         * Sets the id of the parent AnimGraphNode for this group.
         */
        void SetParentNodeId(AnimGraphNodeId nodeId);

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AZStd::vector<AZ::u64>  m_nodeIds;          /**< The node ids that are inside this group. */
        AZStd::string           m_name;             /**< The unique identification number for the node group name. */
        AZ::u32                 m_color;            /**< The color the nodes of the group will be filled with. */
        bool                    m_isVisible;
        bool                    m_nameEditOngoing = false; /**< Whether the user is currently typing a new name */
        AnimGraphNodeId         m_parentNodeId; /**< The id of the parent AnimGraphNode */
    };
} // namespace EMotionFX
