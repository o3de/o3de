/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "Pose.h"
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    // forward declarations
    class Node;
    class Transform;


    class EMFX_API Skeleton
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:

        static Skeleton* Create();

        Skeleton* Clone();

        MCORE_INLINE size_t GetNumNodes() const                 { return m_nodes.size(); }
        MCORE_INLINE Node* GetNode(size_t index) const          { return m_nodes[index]; }

        void ReserveNodes(size_t numNodes);
        void AddNode(Node* node);
        void RemoveNode(size_t nodeIndex, bool delFromMem = true);
        void RemoveAllNodes(bool delFromMem = true);

        MCORE_INLINE const Pose* GetBindPose() const            { return &m_bindPose; }
        MCORE_INLINE Pose* GetBindPose()                        { return &m_bindPose; }

        /**
         * Search for a node by name (case sensitive).
         * @param name The name of the node (case sensitive).
         * @return A pointer to the node, returns nullptr when not found.
         */
        Node* FindNodeByName(const char* name) const;
        Node* FindNodeByName(const AZStd::string& name) const;

        /**
         * Find a joint's index and pointer by name.
         * @param name The name of the joint, non case sensitive.
         * @param outIndex This will contain the resulting index, or MCORE_INVALIDINDEX32 in case not found.
         * @result This returns a pointer to the joint or nullptr when not found. In case of a nullptr, the outIndex will be set to MCORE_INVALIDINDEX32 as well.
         */
        Node* FindNodeAndIndexByName(const AZStd::string& name, size_t& outIndex) const;

        /**
         * Search for a node by name (non case sensitive), returns nullptr when no node can be found.
         * Note: when there are two nodes with the same non case sensitive name, the first one is returned.
         * So if there is a node named "Blah" and one named "BLAH" and we search on "blah", the first one ("Blah") is returned.
         * @param name The name of the node, non case sensitive.
         * @result A pointer to the node with the given name, or nullptr when none could be found.
         */
        Node* FindNodeByNameNoCase(const char* name) const;

        /**
         * Search for a node on ID. Returns nullptr when no node found.
         * A node ID is based on the name. So all nodes (also inside other actors) with the same name will have the same ID.
         * This can be used for faster searching, compared to searching by name, which uses string compares.
         * @param id The ID to search for.
         * @return A pointer to the node, or nullptr when no node with the given ID found.
         */
        Node* FindNodeByID(size_t id) const;

        /**
         * Set the value of a given node.
         * @param index The node number, which must be in range of [0..GetNumNodes()-1].
         * @param node The node value to set at this index.
         */
        void SetNode(size_t index, Node* node);

        /**
         * Set the number of nodes.
         * This resizes the array of pointers to nodes, but doesn't actually create the nodes.
         * @param numNodes The number of nodes to allocate space for.
         */
        void SetNumNodes(size_t numNodes);

        /**
         * Update all the node index values that are returned by the Node::GetNodeIndex() method.
         * When the order or layout of the nodes array changes, for example after removing a node, the index values of
         * the nodes have to be updated. As node number 5 could become node number 4 in the example case.
         * @param startNode The node number to start updating from.
         */
        void UpdateNodeIndexValues(size_t startNode = 0);

        /**
         * Get the number of root nodes in the actor. A root node is a node without any parent.
         * @result The number of root nodes inside the actor.
         */
        MCORE_INLINE size_t GetNumRootNodes() const                             { return m_rootNodes.size(); }

        /**
         * Get the node number/index of a given root node.
         * @param nr The root node number, which must be in range of [0..GetNumRootNodes()-1].
         * @result The node index of the given root node.
         */
        MCORE_INLINE size_t GetRootNodeIndex(size_t nr) const                   { return m_rootNodes[nr]; }

        /**
         * Pre-allocate space for the root nodes array.
         * This does not alter the value returned by GetNumRootNodes() though.
         * @param numNodes The absolute number of nodes to pre-allocate space for.
         */
        void ReserveRootNodes(size_t numNodes);

        /**
         * Add a root node to the actor.
         * This doesn't modify the node itself, but it will add the node to the list of root nodes.
         * @param nodeIndex The node number/index to add and mark as root node inside the actor.
         */
        void AddRootNode(size_t nodeIndex);

        /**
         * Remove a given root node from the list of root nodes stored inside the actor.
         * This doesn't really remove the node itself, but it just unregisters it as root node inside the actor.
         * @param nr The root node to remove, which must be in range of [0..GetNumRootNodes()-1].
         */
        void RemoveRootNode(size_t nr);

        /**
         * Removes all root nodes from the actor.
         * This doesn't remove the actual nodes, but it just unregisters them as root nodes, so that the GetNumRootNodes()
         * method will return a value of zero.
         */
        void RemoveAllRootNodes();

        void LogNodes();
        size_t CalcHierarchyDepthForNode(size_t nodeIndex) const;

    private:
        AZStd::vector<Node*>     m_nodes;         /**< The nodes, including root nodes. */
        mutable AZStd::unordered_map<AZStd::string, Node*> m_nodesMap;
        AZStd::vector<size_t>    m_rootNodes;     /**< The root nodes only. */
        Pose                    m_bindPose;      /**< The bind pose. */

        Skeleton();
        ~Skeleton();
    };
}   // namespace EMotionFX
