/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required files
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    /**
     * A node group like the name already says is a group that contains a list of nodes.
     * The group contains a list of node numbers, which index inside the actor where the groups belong to.
     * It is possible to enable and disable individual groups. Disabling a group will skip all calculations done on the nodes that are inside the group.
     * You can enable and disable individual groups per ActorInstance object.
     * Skipped calculations include the calculation of local space matrices, forward kinematics to calculate world space matrices and blending.
     * It is important that you do not use the nodes that are disabled. For example if a given node is disabled and you try to get its transformation, then this transformation
     * might contain incorrect or even uninitialized data.
     */
    class EMFX_API NodeGroup
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        NodeGroup(const AZStd::string& groupName = {}, size_t numNodes = 0, bool enabledOnDefault = true);
        NodeGroup(const NodeGroup& aOther);
        NodeGroup& operator=(const NodeGroup& aOther);

        /**
         * Set the name of the group. Please keep in mind that group names must be unique inside the Actor objects. So you should not have two or more groups with the same name.
         * @param groupName The name of the group.
         */
        void SetName(const AZStd::string& groupName);

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
         * Set the number of nodes that remain inside this group.
         * This will resize the array of node indices. Don't forget to initialize the node values after increasing the number of nodes though.
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
         * @param nodeIndex The value for the given node. This is the node index which points inside the Actor object where this group will belong to.
         *                  To get access to the actual node object use Actor::GetNode( nodeIndex ).
         */
        void SetNode(size_t index, uint16 nodeIndex);

        /**
         * Get the node index for a given node inside the group.
         * @param index The node number inside this group, which must be in range of [0..GetNumNodes()-1].
         * @result The node number, which points inside the Actor object. Use Actor::GetNode( returnValue ) to get access to the node information.
         */
        uint16 GetNode(size_t index) const;

        /**
         * Enable all nodes that remain inside this group, for a given actor instance.
         * The reason why you specify an ActorInstance as parameter is because the node groups are stored inside the Actor objects, while you can enable and disable
         * nodes individually per ActorInstance.
         * Nodes that are enabled will be processed by fully inside the EMotion FX pipeline. This includes things like motion sampling, local space matrix construction,
         * world space transformation calculations and blending operations. Disabled nodes will skip those calculations.
         * @param targetActorInstance The actor instance object in which we will enable all the nodes that are inside this group.
         */
        void EnableNodes(ActorInstance* targetActorInstance);

        /**
         * Disable all nodes that remain inside this group, for a given actor instance.
         * The reason why you specify an ActorInstance as parameter is because the node groups are stored inside the Actor objects, while you can enable and disable
         * nodes individually per ActorInstance.
         * Nodes that are enabled will be processed by fully inside the EMotion FX pipeline. This includes things like motion sampling, local space matrix construction,
         * world space transformation calculations and blending operations. Disabled nodes will skip those calculations.
         * @param targetActorInstance The actor instance object in which we will enable all the nodes that are inside this group.
         */
        void DisableNodes(ActorInstance* targetActorInstance);

        /**
         * Add a given node to this group.
         * Please keep in mind that performing an AddNode will result in a reallocation being done.
         * It is much better to use SetNumNodes(...) in combination with SetNode(...) upfront if the total number of nodes is known upfront.
         * @param nodeIndex The node index which points inside the Actor object where this group will be added to. This relates to the node returned by Actor::GetNode( nodeIndex ).
         */
        void AddNode(uint16 nodeIndex);

        /**
         * Remove a given node from the group by its node number (the value returned by GetNode(...).
         * If you wish to remove for example the 3rd node inside this group, then use RemoveNodeByGroupIndex(...) instead.
         * Removing a node index which is not part of this group will do nothing, except that it wastes performance as it will perform a search inside the list of nodes inside this group.
         * @param nodeIndex The node number to remove. This value related to Actor::GetNode( nodeIndex ).
         */
        void RemoveNodeByNodeIndex(uint16 nodeIndex);

        /**
         * Remove a given node from the group by the array element index.
         * If for example you wish to remove the 3rd node from the group, you can call RemoveNodeByGroupIndex( 2 ).
         * If you wish to remove a node by its node number, then use RemoveNodeByNodeIndex(...) instead.
         * @param index The node index in the group. So for example an index value of 5 will remove the sixth node from the group.
         *              The index value must be in range of [0..GetNumNodes() - 1].
         */
        void RemoveNodeByGroupIndex(size_t index);

        /**
         * Get direct access to the array of node indices that are part of this group.
         * @result A reference to the array of nodes inside this group. Please use this with care.
         */
        AZStd::vector<uint16>& GetNodeArray();

        /**
         * Check whether this group is enabled after actor instance creation time.
         * If the group is enabled, it means that all nodes inside this group will be enabled once an actor instance is created.
         * When this is set to false, all nodes inside the group will be disabled on default.
         * It is possible however that another group enables or disables specific nodes that are also inside this group again, if a node is in multiple groups.
         & @result Returns true when all nodes inside this group will be in an enabled state when creating a new actor instance using this group. Otherwise false is returned.
         */
        bool GetIsEnabledOnDefault() const;

        /**
         * Set whether this group is enabled after actor creation time or not.
         * If the group is enabled, it means that all nodes inside this group will be enabled once an actor instance is created.
         * When this is set to false, all nodes inside the group will be disabled on default.
         * It is possible however that another group enables or disables specific nodes that are also inside this group again, if a node is in multiple groups.
         & @result Returns true when all nodes inside this group will be in an enabled state when creating a new actor instance using this group. Otherwise false is returned.
         */
        void SetIsEnabledOnDefault(bool enabledOnDefault);

    private:
        AZStd::string               m_name;              /**< The name of the group. */
        AZStd::vector<uint16>       m_nodes;             /**< The node index numbers that are inside this group. */
        bool                        m_enabledOnDefault;  /**< Specifies whether this group is enabled on default (true) or disabled (false). With on default we mean after directly after the actor instance using this group has been created. */
    };
}   // namespace EMotionFX
