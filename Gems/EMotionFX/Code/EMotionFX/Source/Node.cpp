/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Node.h"
#include "NodeAttribute.h"
#include "Skeleton.h"
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Node, NodeAllocator)

    Node::Node(const char* name, Skeleton* skeleton)
        : MCore::RefCounted()
    {
        m_parentIndex        = InvalidIndex;
        m_nodeIndex          = InvalidIndex;     // hasn't been set yet
        m_skeletalLoDs       = 0xFFFFFFFF;               // set all bits of the integer to 1, which enables this node in all LOD levels on default
        m_skeleton           = skeleton;
        m_semanticNameId     = InvalidIndex32;
        m_nodeFlags          = FLAG_INCLUDEINBOUNDSCALC;

        if (name)
        {
            m_nameId         = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            m_nameId         = InvalidIndex32;
        }
    }


    Node::Node(uint32 nameID, Skeleton* skeleton)
        : MCore::RefCounted()
    {
        m_parentIndex        = InvalidIndex;
        m_nodeIndex          = InvalidIndex;     // hasn't been set yet
        m_skeletalLoDs       = 0xFFFFFFFF;// set all bits of the integer to 1, which enables this node in all LOD levels on default
        m_skeleton           = skeleton;
        m_nameId             = nameID;
        m_semanticNameId     = InvalidIndex32;
        m_nodeFlags          = FLAG_INCLUDEINBOUNDSCALC;
    }


    Node::~Node()
    {
        // get rid of all node attributes
        RemoveAllAttributes();

        // remove all child nodes
        RemoveAllChildNodes();
    }


    // create a node
    Node* Node::Create(const char* name, Skeleton* skeleton)
    {
        return aznew Node(name, skeleton);
    }


    // create a node
    Node* Node::Create(uint32 nameID, Skeleton* skeleton)
    {
        return aznew Node(nameID, skeleton);
    }


    // create a clone of this node
    Node* Node::Clone(Skeleton* skeleton) const
    {
        Node* result = Node::Create(m_nameId, skeleton);

        // copy attributes
        result->m_parentIndex        = m_parentIndex;
        result->m_nodeIndex          = m_nodeIndex;
        result->m_skeletalLoDs       = m_skeletalLoDs;
        result->m_childIndices       = m_childIndices;
        result->m_nodeFlags          = m_nodeFlags;
        result->m_semanticNameId     = m_semanticNameId;

        // copy the node attributes
        result->m_attributes.reserve(m_attributes.size());
        for (const NodeAttribute* attribute : m_attributes)
        {
            result->AddAttribute(attribute->Clone());
        }

        // return the resulting clone
        return result;
    }


    // removes all attributes
    void Node::RemoveAllAttributes()
    {
        while (!m_attributes.empty())
        {
            m_attributes.back()->Destroy();
            m_attributes.pop_back();
        }
    }


    // get the total number of children
    size_t Node::GetNumChildNodesRecursive() const
    {
        // the number of total child nodes which include the childs of the childs, too
        size_t result = 0;

        // retrieve the number of child nodes of the actual node
        for (size_t childIndex : m_childIndices)
        {
            m_skeleton->GetNode(childIndex)->RecursiveCountChildNodes(result);
        }

        return result;
    }


    // recursively count the number of nodes down the hierarchy
    void Node::RecursiveCountChildNodes(size_t& numNodes)
    {
        // increase the counter
        numNodes++;

        // recurse down the hierarchy
        for (size_t childIndex : m_childIndices)
        {
            m_skeleton->GetNode(childIndex)->RecursiveCountChildNodes(numNodes);
        }
    }


    // recursively go through the parents until a root node is reached and store all parents inside an array
    void Node::RecursiveCollectParents(AZStd::vector<size_t>& parents, bool clearParentsArray) const
    {
        if (clearParentsArray)
        {
            parents.clear();
        }

        // loop until we reached a root node
        const Node* node = this;
        while (node)
        {
            // get the parent index and add it to the list of parents if the current node is not a root node
            const size_t parentIndex = node->GetParentIndex();
            if (parentIndex != InvalidIndex)
            {
                // check if the parent is already in our array, if not add it so that we only store each node once
                if (AZStd::find(begin(parents), end(parents), parentIndex) == end(parents))
                {
                    parents.emplace_back(parentIndex);
                }
            }

            // down the hierarchy
            node = node->GetParentNode();
        }
    }


    // remove the given attribute of the given type from the node
    void Node::RemoveAttributeByType(uint32 attributeTypeID, size_t occurrence)
    {
        const auto foundAttribute = AZStd::find_if(begin(m_attributes), end(m_attributes), [attributeTypeID, occurrence, currentOccurrence = size_t{0}] (const NodeAttribute* attribute) mutable
        {
            if (attribute->GetType() == attributeTypeID)
            {
                ++currentOccurrence;
                return occurrence < currentOccurrence;
            }
            return false;
        });

        m_attributes.erase(foundAttribute);
    }


    // remove all attributes of the given type from the node
    size_t Node::RemoveAllAttributesByType(uint32 attributeTypeID)
    {
        return AZStd::erase_if(m_attributes, [attributeTypeID](const NodeAttribute* attribute)
        {
            return attribute->GetType() == attributeTypeID;
        });
    }



    // recursively find the root node (expensive call)
    Node* Node::FindRoot() const
    {
        size_t parentIndex = m_parentIndex;
        const Node* curNode = this;

        while (parentIndex != InvalidIndex)
        {
            curNode = m_skeleton->GetNode(parentIndex);
            parentIndex = curNode->GetParentIndex();
        }

        return const_cast<Node*>(curNode);
    }


    // get the parent node, or nullptr when it doesn't exist
    Node* Node::GetParentNode() const
    {
        if (m_parentIndex != InvalidIndex)
        {
            return m_skeleton->GetNode(m_parentIndex);
        }

        return nullptr;
    }


    // set the name
    void Node::SetName(const char* name)
    {
        if (name)
        {
            m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            m_nameId = InvalidIndex32;
        }
    }


    // set the semantic name
    void Node::SetSemanticName(const char* name)
    {
        if (name)
        {
            m_semanticNameId = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            m_semanticNameId = InvalidIndex32;
        }
    }


    void Node::SetParentIndex(size_t parentNodeIndex)
    {
        m_parentIndex = parentNodeIndex;
    }


    // get the name
    const char* Node::GetName() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId).c_str();
    }


    // get the name of the node as pointer to chars
    const AZStd::string& Node::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId);
    }


    // get the semantic name
    const char* Node::GetSemanticName() const
    {
        return MCore::GetStringIdPool().GetName(m_semanticNameId).c_str();
    }


    // get the semantic name of the node as pointer to chars
    const AZStd::string& Node::GetSemanticNameString() const
    {
        return MCore::GetStringIdPool().GetName(m_semanticNameId);
    }


    // returns true if this is a root node, so if it has no parents
    bool Node::GetIsRootNode() const
    {
        return (m_parentIndex == InvalidIndex);
    }


    //-------------------------------------------------------------------------------------

    void Node::AddAttribute(NodeAttribute* attribute)
    {
        m_attributes.emplace_back(attribute);
    }


    size_t Node::GetNumAttributes() const
    {
        return m_attributes.size();
    }


    NodeAttribute* Node::GetAttribute(size_t attributeNr)
    {
        // make sure we are in range
        MCORE_ASSERT(attributeNr < m_attributes.size());

        // return the attribute
        return m_attributes[attributeNr];
    }


    size_t Node::FindAttributeNumber(uint32 attributeTypeID) const
    {
        // check all attributes, and find where the specific attribute is
        const auto foundAttribute = AZStd::find_if(begin(m_attributes), end(m_attributes), [attributeTypeID](const NodeAttribute* attribute)
        {
            return attribute->GetType() == attributeTypeID;
        });
        return foundAttribute != end(m_attributes) ? AZStd::distance(begin(m_attributes), foundAttribute) : InvalidIndex;
    }


    NodeAttribute* Node::GetAttributeByType(uint32 attributeType)
    {
        // check all attributes
        const auto foundAttribute = AZStd::find_if(begin(m_attributes), end(m_attributes), [attributeType](const NodeAttribute* attribute)
        {
            return attribute->GetType() == attributeType;
        });
        return foundAttribute != end(m_attributes) ? *foundAttribute : nullptr;
    }


    // remove the given attribute
    void Node::RemoveAttribute(size_t index)
    {
        m_attributes.erase(AZStd::next(begin(m_attributes), index));
    }


    void Node::AddChild(size_t nodeIndex)
    {
        m_childIndices.emplace_back(nodeIndex);
    }


    void Node::SetChild(size_t childNr, size_t childNodeIndex)
    {
        m_childIndices[childNr] = childNodeIndex;
    }


    void Node::SetNumChildNodes(size_t numChildNodes)
    {
        m_childIndices.resize(numChildNodes);
    }


    void Node::PreAllocNumChildNodes(size_t numChildNodes)
    {
        m_childIndices.reserve(numChildNodes);
    }


    void Node::RemoveChild(size_t nodeIndex)
    {
        if (const auto it = AZStd::find(begin(m_childIndices), end(m_childIndices), nodeIndex); it != end(m_childIndices))
        {
            m_childIndices.erase(it);
        }
    }


    void Node::RemoveAllChildNodes()
    {
        m_childIndices.clear();
    }


    bool Node::GetHasChildNodes() const
    {
        return !m_childIndices.empty();
    }


    void Node::SetNodeIndex(size_t index)
    {
        m_nodeIndex = index;
    }



    void Node::SetSkeletalLODLevelBits(size_t bitValues)
    {
        m_skeletalLoDs = bitValues;
    }


    void Node::SetSkeletalLODStatus(size_t lodLevel, bool enabled)
    {
        MCORE_ASSERT(lodLevel <= 63);
        if (enabled)
        {
            m_skeletalLoDs |= (1ull << lodLevel);
        }
        else
        {
            m_skeletalLoDs &= ~(1ull << lodLevel);
        }
    }


    void Node::SetIncludeInBoundsCalc(bool includeThisNode)
    {
        if (includeThisNode)
        {
            m_nodeFlags |= FLAG_INCLUDEINBOUNDSCALC;
        }
        else
        {
            m_nodeFlags &= ~FLAG_INCLUDEINBOUNDSCALC;
        }
    }

    void Node::SetIsCritical(bool isCritical)
    {
        if (isCritical)
        {
            m_nodeFlags |= FLAG_CRITICAL;
        }
        else
        {
            m_nodeFlags &= ~FLAG_CRITICAL;
        }
    }


    bool Node::GetIsAttachmentNode() const
    {
        return (m_nodeFlags & FLAG_ATTACHMENT) != 0;
    }


    void Node::SetIsAttachmentNode(bool isAttachmentNode)
    {
        if (isAttachmentNode)
        {
            m_nodeFlags |= FLAG_ATTACHMENT;
        }
        else
        {
            m_nodeFlags &= ~FLAG_ATTACHMENT;
        }
    }
} // namespace EMotionFX
