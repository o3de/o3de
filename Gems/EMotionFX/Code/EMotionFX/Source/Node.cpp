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
    AZ_CLASS_ALLOCATOR_IMPL(Node, NodeAllocator, 0)

    Node::Node(const char* name, Skeleton* skeleton)
        : BaseObject()
    {
        mParentIndex        = InvalidIndex;
        mNodeIndex          = InvalidIndex;     // hasn't been set yet
        mSkeletalLODs       = 0xFFFFFFFF;               // set all bits of the integer to 1, which enables this node in all LOD levels on default
        mSkeleton           = skeleton;
        mSemanticNameID     = InvalidIndex;
        mNodeFlags          = FLAG_INCLUDEINBOUNDSCALC;

        if (name)
        {
            mNameID         = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            mNameID         = InvalidIndex;
        }
    }


    Node::Node(size_t nameID, Skeleton* skeleton)
        : BaseObject()
    {
        mParentIndex        = InvalidIndex;
        mNodeIndex          = InvalidIndex;     // hasn't been set yet
        mSkeletalLODs       = 0xFFFFFFFF;// set all bits of the integer to 1, which enables this node in all LOD levels on default
        mSkeleton           = skeleton;
        mNameID             = nameID;
        mSemanticNameID     = InvalidIndex;
        mNodeFlags          = FLAG_INCLUDEINBOUNDSCALC;
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
    Node* Node::Create(size_t nameID, Skeleton* skeleton)
    {
        return aznew Node(nameID, skeleton);
    }


    // create a clone of this node
    Node* Node::Clone(Skeleton* skeleton) const
    {
        Node* result = Node::Create(mNameID, skeleton);

        // copy attributes
        result->mParentIndex        = mParentIndex;
        result->mNodeIndex          = mNodeIndex;
        result->mSkeletalLODs       = mSkeletalLODs;
        result->mChildIndices       = mChildIndices;
        result->mNodeFlags          = mNodeFlags;
        result->mSemanticNameID     = mSemanticNameID;

        // copy the node attributes
        result->mAttributes.reserve(mAttributes.size());
        for (const NodeAttribute* mAttribute : mAttributes)
        {
            result->AddAttribute(mAttribute->Clone());
        }

        // return the resulting clone
        return result;
    }


    // removes all attributes
    void Node::RemoveAllAttributes()
    {
        while (!mAttributes.empty())
        {
            mAttributes.back()->Destroy();
            mAttributes.pop_back();
        }
    }


    // get the total number of children
    size_t Node::GetNumChildNodesRecursive() const
    {
        // the number of total child nodes which include the childs of the childs, too
        size_t result = 0;

        // retrieve the number of child nodes of the actual node
        for (size_t childIndex : mChildIndices)
        {
            mSkeleton->GetNode(childIndex)->RecursiveCountChildNodes(result);
        }

        return result;
    }


    // recursively count the number of nodes down the hierarchy
    void Node::RecursiveCountChildNodes(size_t& numNodes)
    {
        // increase the counter
        numNodes++;

        // recurse down the hierarchy
        for (size_t childIndex : mChildIndices)
        {
            mSkeleton->GetNode(childIndex)->RecursiveCountChildNodes(numNodes);
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
        const auto foundAttribute = AZStd::find_if(begin(mAttributes), end(mAttributes), [attributeTypeID, occurrence, currentOccurrence = size_t{0}] (const NodeAttribute* attribute) mutable
        {
            if (attribute->GetType() == attributeTypeID)
            {
                ++currentOccurrence;
                return occurrence < currentOccurrence;
            }
            return false;
        });

        mAttributes.erase(foundAttribute);
    }


    // remove all attributes of the given type from the node
    size_t Node::RemoveAllAttributesByType(uint32 attributeTypeID)
    {
        return AZStd::erase_if(mAttributes, [attributeTypeID](const NodeAttribute* attribute)
        {
            return attribute->GetType() == attributeTypeID;
        });
    }



    // recursively find the root node (expensive call)
    Node* Node::FindRoot() const
    {
        size_t parentIndex = mParentIndex;
        const Node* curNode = this;

        while (parentIndex != InvalidIndex)
        {
            curNode = mSkeleton->GetNode(parentIndex);
            parentIndex = curNode->GetParentIndex();
        }

        return const_cast<Node*>(curNode);
    }


    // get the parent node, or nullptr when it doesn't exist
    Node* Node::GetParentNode() const
    {
        if (mParentIndex != InvalidIndex)
        {
            return mSkeleton->GetNode(mParentIndex);
        }

        return nullptr;
    }


    // set the name
    void Node::SetName(const char* name)
    {
        if (name)
        {
            mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            mNameID = InvalidIndex;
        }
    }


    // set the semantic name
    void Node::SetSemanticName(const char* name)
    {
        if (name)
        {
            mSemanticNameID = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            mSemanticNameID = InvalidIndex;
        }
    }


    void Node::SetParentIndex(size_t parentNodeIndex)
    {
        mParentIndex = parentNodeIndex;
    }


    // get the name
    const char* Node::GetName() const
    {
        return MCore::GetStringIdPool().GetName(mNameID).c_str();
    }


    // get the name of the node as pointer to chars
    const AZStd::string& Node::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(mNameID);
    }


    // get the semantic name
    const char* Node::GetSemanticName() const
    {
        return MCore::GetStringIdPool().GetName(mSemanticNameID).c_str();
    }


    // get the semantic name of the node as pointer to chars
    const AZStd::string& Node::GetSemanticNameString() const
    {
        return MCore::GetStringIdPool().GetName(mSemanticNameID);
    }


    // returns true if this is a root node, so if it has no parents
    bool Node::GetIsRootNode() const
    {
        return (mParentIndex == InvalidIndex);
    }


    //-------------------------------------------------------------------------------------

    void Node::AddAttribute(NodeAttribute* attribute)
    {
        mAttributes.emplace_back(attribute);
    }


    size_t Node::GetNumAttributes() const
    {
        return mAttributes.size();
    }


    NodeAttribute* Node::GetAttribute(size_t attributeNr)
    {
        // make sure we are in range
        MCORE_ASSERT(attributeNr < mAttributes.size());

        // return the attribute
        return mAttributes[attributeNr];
    }


    size_t Node::FindAttributeNumber(uint32 attributeTypeID) const
    {
        // check all attributes, and find where the specific attribute is
        const auto foundAttribute = AZStd::find_if(begin(mAttributes), end(mAttributes), [attributeTypeID](const NodeAttribute* attribute)
        {
            return attribute->GetType() == attributeTypeID;
        });
        return foundAttribute != end(mAttributes) ? AZStd::distance(begin(mAttributes), foundAttribute) : InvalidIndex;
    }


    NodeAttribute* Node::GetAttributeByType(uint32 attributeType)
    {
        // check all attributes
        const auto foundAttribute = AZStd::find_if(begin(mAttributes), end(mAttributes), [attributeType](const NodeAttribute* attribute)
        {
            return attribute->GetType() == attributeType;
        });
        return foundAttribute != end(mAttributes) ? *foundAttribute : nullptr;
    }


    // remove the given attribute
    void Node::RemoveAttribute(size_t index)
    {
        mAttributes.erase(AZStd::next(begin(mAttributes), index));
    }


    void Node::AddChild(size_t nodeIndex)
    {
        mChildIndices.emplace_back(nodeIndex);
    }


    void Node::SetChild(size_t childNr, size_t childNodeIndex)
    {
        mChildIndices[childNr] = childNodeIndex;
    }


    void Node::SetNumChildNodes(size_t numChildNodes)
    {
        mChildIndices.resize(numChildNodes);
    }


    void Node::PreAllocNumChildNodes(size_t numChildNodes)
    {
        mChildIndices.reserve(numChildNodes);
    }


    void Node::RemoveChild(size_t nodeIndex)
    {
        if (const auto it = AZStd::find(begin(mChildIndices), end(mChildIndices), nodeIndex); it != end(mChildIndices))
        {
            mChildIndices.erase(it);
        }
    }


    void Node::RemoveAllChildNodes()
    {
        mChildIndices.clear();
    }


    bool Node::GetHasChildNodes() const
    {
        return !mChildIndices.empty();
    }


    void Node::SetNodeIndex(size_t index)
    {
        mNodeIndex = index;
    }



    void Node::SetSkeletalLODLevelBits(uint32 bitValues)
    {
        mSkeletalLODs = bitValues;
    }


    void Node::SetSkeletalLODStatus(uint32 lodLevel, bool enabled)
    {
        MCORE_ASSERT(lodLevel <= 31);
        if (enabled)
        {
            mSkeletalLODs |= (1 << lodLevel);
        }
        else
        {
            mSkeletalLODs &= ~(1 << lodLevel);
        }
    }


    void Node::SetIncludeInBoundsCalc(bool includeThisNode)
    {
        if (includeThisNode)
        {
            mNodeFlags |= FLAG_INCLUDEINBOUNDSCALC;
        }
        else
        {
            mNodeFlags &= ~FLAG_INCLUDEINBOUNDSCALC;
        }
    }

    void Node::SetIsCritical(bool isCritical)
    {
        if (isCritical)
        {
            mNodeFlags |= FLAG_CRITICAL;
        }
        else
        {
            mNodeFlags &= ~FLAG_CRITICAL;
        }
    }


    bool Node::GetIsAttachmentNode() const
    {
        return (mNodeFlags & FLAG_ATTACHMENT) != 0;
    }


    void Node::SetIsAttachmentNode(bool isAttachmentNode)
    {
        if (isAttachmentNode)
        {
            mNodeFlags |= FLAG_ATTACHMENT;
        }
        else
        {
            mNodeFlags &= ~FLAG_ATTACHMENT;
        }
    }
} // namespace EMotionFX
