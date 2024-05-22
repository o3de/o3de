/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    // forward declaration
    class MotionInstance;
    class Mesh;
    class Actor;
    class MeshDeformerStack;
    class NodeAttribute;
    class Skeleton;


    /**
     * The node class.
     * A node in EMotion FX is a connection point in the hierarchy of the character.
     * Each node can have one parent, but multiple child nodes.
     * Every node has a transformation describing the orientation of the node, so where it is positioned
     * and how it is scaled and rotated in 3D space.
     * Examples of nodes could be a bone, like an upper arm or lower arm, a finger, a head, a gun, a cigarette, a camera, a light, etc.
     * So basically it can be everything, which can be animated (but it doesn't have to be animated of course).
     */
    class EMFX_API Node
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Actor;

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * The node flags (options).
         */
        enum ENodeFlags : AZ::u8
        {
            FLAG_INCLUDEINBOUNDSCALC = 1 << 0,           /**< Specifies whether we have to include this node in the bounds calculation or not (true on default). */
            FLAG_ATTACHMENT = 1 << 1,           /**< Indicates if this node is an attachment node or not (false on default). */
            FLAG_CRITICAL = 1 << 2            /**< Indicates if this node is a critical node. A critical node is always included the skeleton and cannot be optimized out (false on default). */
        };

        /**
         * Create method.
         * @param name The name of the node.
         * @param skeleton The skeleton where this node will belong to, you still need to manually add it to the skeleton though.
         */
        static Node* Create(const char* name, Skeleton* skeleton);

        /**
         * Create method using a name ID.
         * @param nameID The name ID, generated using the MCore::GetStringIdPool().
         * @param skeleton The skeleton where this node will belong to, you still need to manually add it to the skeleton though.
         */
        static Node* Create(uint32 nameID, Skeleton* skeleton);

        /**
         * Clone the node.
         * @param skeleton The skeleton that the cloned node will be part of, you still need to manually add it to this skeleton though.
         * @result The cloned node.
         */
        Node* Clone(Skeleton* skeleton) const;

        /**
         * Set the parent node index.
         * When this is set to MCORE_INVALIDINDEX32 then this is considered as no parent.
         * In that case this node is a root node.
         * @param parentNodeIndex The node index of the node where to link this node to.
         */
        void SetParentIndex(size_t parentNodeIndex);

        /**
         * Get the parent node's index.
         * This is either a valid index, or MCORE_INVALIDINDEX32 in case there is no parent node.
         * @result The index of the parent node, or MCORE_INVALIDINDEX32 in case this node has no parent.
         */
        MCORE_INLINE size_t GetParentIndex() const                              { return m_parentIndex; }

        /**
         * Get the parent node as node pointer.
         * @result Returns the pointer to the parent node, or nullptr in case there is no parent.
         */
        Node* GetParentNode() const;

        /**
         * Recursively go through the parents until a root node is reached and store all parents inside an array.
         * @param parents The array to which parent and the parents of the parents of the node will be added.
         * @param clearParentsArray When true the given parents array will be cleared before filling it.
         */
        void RecursiveCollectParents(AZStd::vector<size_t>& parents, bool clearParentsArray = true) const;

        /**
         * Set the node name.
         * @param name The new node name, which should be unique.
         */
        void SetName(const char* name);

        /**
         * Gets the name of the node in form of a C character buffer.
         * @result A pointer to the null terminated buffer, containing the name of the node.
         */
        const char* GetName() const;

        /**
         * Gets the name of the node in form of String object.
         * @result The string containing the node name.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Set the semantic name.
         * @param name The new node name, which should be unique.
         */
        void SetSemanticName(const char* name);

        /**
         * Gets the semantic name.
         * @result A pointer to the null terminated buffer, containing the semantic name of the node.
         */
        const char* GetSemanticName() const;

        /**
         * Gets the semantic name as string.
         * @result The string containing the semantic name.
         */
        const AZStd::string& GetSemanticNameString() const;

        /**
         * Get the unique ID of this node, which has been generated based on the name.
         * You are guaranteed that nodes having the same name have the same ID.
         * This means you can do fast compares between nodes by using the ID instead of a comparison
         * between the node names.
         * However, ever time you run the application, the ID values for given names can be different.
         * So if you run your program, the node with name "Arm" might have an ID of 10. While next time you
         * run the program this node might have an ID of for example 36.
         * Still it is guaranteed that every time you run your program all nodes with the name "Arm" will have the
         * same ID number.
         * @result The node ID number, which can be used for fast compares between nodes.
         */
        MCORE_INLINE uint32 GetID() const                                       { return m_nameId; }

        /**
         * Get the semantic name ID.
         * To get the name you can also use GetSemanticName() and GetSemanticNameString().
         * @result The semantic name ID.
         */
        MCORE_INLINE uint32 GetSemanticID() const                               { return m_semanticNameId; }

        /**
         * Get the number of child nodes attached to this node.
         * @result The number of child nodes.
         */
        MCORE_INLINE size_t GetNumChildNodes() const                            { return m_childIndices.size(); }

        /**
         * Get the number of child nodes down the hierarchy of this node.
         * The current node is not included in the count.
         * @return The total number of nodes down the hierarchy of this node.
         */
        size_t GetNumChildNodesRecursive() const;

        /**
         * Get a given child's node index.
         * @param nr The child number.
         * @result The index of the child node, which is a node number inside the actor.
         */
        MCORE_INLINE size_t GetChildIndex(size_t nr) const                      { return m_childIndices[nr]; }

        /**
         * Checks if the given node is a child of this node.
         * @param nodeIndex The node to check whether it is a child or not.
         * @result True if the given node is a child, false if not.
         */
        MCORE_INLINE bool CheckIfIsChildNode(size_t nodeIndex) const            { return (AZStd::find(begin(m_childIndices), end(m_childIndices), nodeIndex) != end(m_childIndices)); }

        /**
         * Add a child to this node.
         * @param nodeIndex The index of the child node to add.
         */
        void AddChild(size_t nodeIndex);

        /**
         * Set the value for a given child node.
         * @param childNr The child number, which must be in range of [0..GetNumChildNodes()-1].
         * @param childNodeIndex The node index for this child.
         */
        void SetChild(size_t childNr, size_t childNodeIndex);

        /**
         * Resize the array of child nodes.
         * This will grow the child node array so that the value returned by GetNumChildNodes() will return the same value as you specify as parameter here.
         * @param numChildNodes The number of child nodes to create. Be sure to initialize all of the child nodes using SetChild() though!
         */
        void SetNumChildNodes(size_t numChildNodes);

        /**
         * Preallocate the array of child nodes.
         * Unlike SetNumChildNodes, this will NOT grow the child node array as reported by GetNumChildNodes(). However, it internally pre-allocates memory to make the AddChild() calls faster.
         * @param numChildNodes The number of child nodes to pre-allocate space for.
         */
        void PreAllocNumChildNodes(size_t numChildNodes);

        /**
         * Removes a given child (does not delete it from memory though).
         * Beware, the child node will still point to this node after executing this method!
         * So you have to adjust the parent pointer of the child node manually.
         * @param nodeIndex The index of the child to remove.
         */
        void RemoveChild(size_t nodeIndex);

        /**
         * Removes all child nodes (not from memory though but just clears the childs pointers in this node).
         * Beware, the child nodes will still point to this node after executing this method!
         * So you have to adjust their parent indices manually.
         */
        void RemoveAllChildNodes();

        /**
         * Check if this node is a root node. A node is seen as root node when it has no parent.
         * So if Node::GetParentNode() equals nullptr we are dealing with a root node.
         * @result Returns true when this node is a root node, otherwise false is returned.
         */
        bool GetIsRootNode() const;

        /**
         * Check if this node has any child nodes.
         * @result Returns true when this node has child nodes, otherwise false is returned.
         */
        bool GetHasChildNodes() const;

        /**
         * Find the root node in the hierarchy.
         * @result A pointer to the root node (can point to itself).
         */
        Node* FindRoot() const;

        /**
         * Add a node attribute to this node.
         * @param attribute The attribute to add to the node.
         */
        void AddAttribute(NodeAttribute* attribute);

        /**
         * Get the number of node attributes.
         * @result The number of node attributes for this node.
         */
        size_t GetNumAttributes() const;

        /**
         * Get a given node attribute.
         * You will need to typecast to the correct type of the attribute yourself.
         * You can get the attribute type by using the NodeAttribute::GetType() method.
         * @param attributeNr The attribute number to get from the node. You can find the attribute number
         * when you have the attribute type by using the FindAttributeNumber() method.
         * @result A pointer to the node attribute.
         * @see FindNodeAttributeNumber
         */
        NodeAttribute* GetAttribute(size_t attributeNr);

        /**
         * Get a given node attribute of a given type.
         * The method will return nullptr when no attribute of the specified type could be found.
         * @result A pointer to the node attribute, or nullptr when not found.
         */
        NodeAttribute* GetAttributeByType(uint32 attributeType);

        /**
         * Find the node attribute number of a given type.
         * When there are no node attributes, or there is no attribute of the specified type
         * this method will return MCORE_INVALIDINDEX32.
         * @param attributeTypeID The attribute type ID (returned by NodeAttribute::GetType()).
         * @result The first located attribute number which is of the given type, or MCORE_INVALIDINDEX32 when the attribute of this type could not be located.
         */
        size_t FindAttributeNumber(uint32 attributeTypeID) const;

        /**
         * Removes all node attributes from this node.
         * The previously allocated attributes will be deleted from memory automatically.
         */
        void RemoveAllAttributes();

        /**
         * Remove the given node attribute from this node.
         * @param index The index of the node attribute to remove.
         */
        void RemoveAttribute(size_t index);

        /**
         * Remove the given node attribute from this node which occurs at the given position.
         * If the given occurrence is bigger than the number of actual attributes inside the node
         * no node attribute will be removed.
         * @param attributeTypeID The attribute type ID (returned by NodeAttribute::GetType()).
         * @param occurrence The number of node attributes which will be skipped until we reached the
         *                   node to remove.
         */
        void RemoveAttributeByType(uint32 attributeTypeID, size_t occurrence = 0);

        /**
         * Removes all node attributes from this node of the given type.
         * @param attributeTypeID The attribute type ID (returned by NodeAttribute::GetType()).
         * @result The number of attributes that have been removed.
         */
        size_t RemoveAllAttributesByType(uint32 attributeTypeID);

        //--------------------------------------------

        /**
         * Set the node index for this node.
         * This has to be the index inside the array of nodes inside the Actor.
         * So Actor::GetNode( nodeIndex ) will return this node.
         * @param index The index to use.
         */
        void SetNodeIndex(size_t index);

        /**
         * Get the node index value.
         * This is the index inside the array of nodes inside the Actor.
         * So Actor::GetNode( nodeIndex ) will return this node.
         * @result The index of the node.
         */
        MCORE_INLINE size_t GetNodeIndex() const                                        { return m_nodeIndex; }

        //------------------------------

        /**
         * Set the skeletal LOD status for all LOD levels.
         * Each bit specifies whether this node is active in a given LOD or not.
         * Bit 0 represents LOD 0, bit 1 represents LOD 1, etc.
         * @param bitValues The unsigned 32-bits integer that contains the settings for each LOD.
         */
        void SetSkeletalLODLevelBits(size_t bitValues);

        /**
         * Set the skeletal LOD status for a given LOD level.
         * This enables or disables the node entirely from all calculations inside EMotion FX.
         * It is important to know that the nodes that you disable should not be used anywhere inside skinning information of meshes or other places
         * where there transformations are needed.
         * There are 32 possible LOD levels. On default all nodes are enabled in a given LOD that hasn't been initialized.
         * @param lodLevel The skeletal LOD level to change the settings for. This must be in range of [0..31].
         * @param enabled Set to true when you wish the node to be enabled in the given LOD, or false when you wish to disable it in the given LOD.
         */
        void SetSkeletalLODStatus(size_t lodLevel, bool enabled);

        /**
         * Get the skeletal LOD status for this node at a given skeletal LOD.
         * @param lodLevel The skeletal LOD level to check.
         * @result Returns true when this node is enabled in the specified LOD level. Otherwise false is returned.
         */
        MCORE_INLINE bool GetSkeletalLODStatus(size_t lodLevel) const                   { return (m_skeletalLoDs & (1ull << lodLevel)) != 0; }

        //--------------------------------------------

        /**
         * Check whether this node is included inside the bounding volume calculations or not.
         * Sometimes it is useful to disable some nodes from the bounding volume calculation.
         * Examples of these are nodes that are lights or cameras, or nodes that just always stay at the origin, while
         * your character moves away from the origin without motion based actor repositioning.
         * On default all nodes are included inside the bounding volume calculations.
         * @result Returns true when this node will be included in the bounds calculation, or false when it won't.
         */
        MCORE_INLINE bool GetIncludeInBoundsCalc() const                                { return m_nodeFlags & FLAG_INCLUDEINBOUNDSCALC; }

        /**
         * Specify whether this node should be included inside the bounding volume calculations or not.
         * Sometimes it is useful to disable some nodes from the bounding volume calculation.
         * Examples of these are nodes that are lights or cameras, or nodes that just always stay at the origin, while
         * your character moves away from the origin without motion based actor repositioning.
         * On default all nodes are included inside the bounding volume calculations.
         * @param includeThisNode Set to true when you want this node to be included in the bounds calculation, or set to false when
         *                        it should be excluded.
         */
        void SetIncludeInBoundsCalc(bool includeThisNode);

        /**
         * Check whether this node is critcal and should not be optimized out in any situations.
         * Sometimes we perform optimization process on the node. This flag make sure that critical node will always be included in the actor heirarchy.
         * @result Returns true when this node is critical, or false when it won't.
         */
        MCORE_INLINE bool GetIsCritical() const { return m_nodeFlags & FLAG_CRITICAL; }

        /**
         * Specify whether this node is critcal and should not be optimized out in any situations.
         * @param isCritical Set to true when you want this node to be critical.
         */
        void SetIsCritical(bool isCritical);

        /**
         * Check if the node is an attachment node.
         * @return True if the node is an attachment node, false if not.
         */
        bool GetIsAttachmentNode() const;

        /**
         * Set if the node is an attachment node.
         * @return True if the node is an attachment node, false if not.
         */
        void SetIsAttachmentNode(bool isAttachmentNode);

    private:
        size_t      m_nodeIndex;         /**< The node index, which is the index into the array of nodes inside the Skeleton class. */
        size_t      m_parentIndex;       /**< The parent node index, or MCORE_INVALIDINDEX32 when there is no parent. */
        size_t      m_skeletalLoDs;      /**< The skeletal LOD status values. Each bit represents if this node is enabled or disabled in the given LOD. */
        uint32      m_nameId;            /**< The ID, which is generated from the name. You can use this for fast compares between nodes. */
        uint32      m_semanticNameId;    /**< The semantic name ID, for example "LeftHand" or "RightFoot" or so, this can be used for retargeting. */
        Skeleton*   m_skeleton;          /**< The skeleton where this node belongs to. */
        AZStd::vector<size_t>            m_childIndices;      /**< The indices that point to the child nodes. */
        AZStd::vector<NodeAttribute*>    m_attributes;        /**< The node attributes. */
        uint8                           m_nodeFlags;         /**< The node flags are used to store boolean attributes of the node as single bits. */

        /**
         * Constructor.
         * @param name The name of the node.
         * @param skeleton The skeleton where this node will belong to.
         */
        Node(const char* name, Skeleton* skeleton);

        /**
         * Constructor.
         * @param nameID The name ID, generated using the MCore::GetStringIdPool().
         * @param skeleton The skeleton where this node will belong to.
         */
        Node(uint32 nameID, Skeleton* skeleton);

        /**
         * The destructor.
         * Beware, when destructing this node, the child nodes of this node point to an already destructed Node!
         * So be sure to set adjust the parent pointers of the child nodes before deleting this node.
         */
        ~Node();

        /**
         * Recursively count the number of nodes down the hierarchy of this node.
         * @param numNodes The integer containing the current node count. This counter will be increased during recursion.
         */
        void RecursiveCountChildNodes(size_t& numNodes);

        friend class SkeletonModel;
    };
} // namespace EMotionFX
