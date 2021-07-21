/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        // set the array memory categories
        mAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_NODES);
        mChildIndices.SetMemoryCategory(EMFX_MEMCATEGORY_NODES);

        mParentIndex        = MCORE_INVALIDINDEX32;
        mNodeIndex          = MCORE_INVALIDINDEX32;     // hasn't been set yet
        mSkeletalLODs       = 0xFFFFFFFF;               // set all bits of the integer to 1, which enables this node in all LOD levels on default
        mSkeleton           = skeleton;
        mSemanticNameID     = MCORE_INVALIDINDEX32;
        mNodeFlags          = FLAG_INCLUDEINBOUNDSCALC;

        if (name)
        {
            mNameID         = MCore::GetStringIdPool().GenerateIdForString(name);
        }
        else
        {
            mNameID         = MCORE_INVALIDINDEX32;
        }
    }


    Node::Node(uint32 nameID, Skeleton* skeleton)
        : BaseObject()
    {
        // set the array memory categories
        mAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_NODES);
        mChildIndices.SetMemoryCategory(EMFX_MEMCATEGORY_NODES);

        mParentIndex        = MCORE_INVALIDINDEX32;
        mNodeIndex          = MCORE_INVALIDINDEX32;     // hasn't been set yet
        mSkeletalLODs       = 0xFFFFFFFF;// set all bits of the integer to 1, which enables this node in all LOD levels on default
        mSkeleton           = skeleton;
        mNameID             = nameID;
        mSemanticNameID     = MCORE_INVALIDINDEX32;
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
    Node* Node::Create(uint32 nameID, Skeleton* skeleton)
    {
        return aznew Node(nameID, skeleton);
    }


    /*
    // create a clone of this node
    Node* Node::Clone(Actor* actor) const
    {
        Node* result = Node::Create(GetName(), actor);

        // copy attributes
        result->mParentIndex        = mParentIndex;
        result->mNodeIndex          = mNodeIndex;
        result->mNameID             = mNameID;
        result->mSkeletalLODs       = mSkeletalLODs;
        //result->mMotionLODs           = mMotionLODs;
        result->mChildIndices       = mChildIndices;
        //result->mImportanceFactor = mImportanceFactor;
        result->mNodeFlags          = mNodeFlags;
        result->mOBB                = mOBB;
        result->mSemanticNameID     = mSemanticNameID;

        // copy the node attributes
        for (uint32 i=0; i<mAttributes.GetLength(); i++)
            result->AddAttribute( mAttributes[i]->Clone() );

        // copy the meshes
        const uint32 numLODs = mLODs.GetLength();
        if (result->mLODs.GetLength() < numLODs)
            result->mLODs.Resize( numLODs );

        for (uint32 i=0; i<numLODs; ++i)
        {
            Mesh* realMesh = mLODs[i].mMesh;
            if (realMesh)
                result->mLODs[i].mMesh = realMesh->Clone(actor, result);
            else
                result->mLODs[i].mMesh = nullptr;
        }

        // copy the collision meshes
        for (uint32 i=0; i<numLODs; ++i)
        {
            Mesh* realMesh = mLODs[i].mColMesh;
            if (realMesh)
                result->mLODs[i].mColMesh = realMesh->Clone(actor, result);
            else
                result->mLODs[i].mColMesh = nullptr;
        }

        // clone node stacks
        for (uint32 i=0; i<numLODs; ++i)
        {
            MeshDeformerStack* realStack = mLODs[i].mStack;
            if (realStack)
                result->mLODs[i].mStack = realStack->Clone(result->mLODs[i].mMesh, actor);
            else
                result->mLODs[i].mStack = nullptr;
        }

        // clone node collision stacks if desired
        for (uint32 i=0; i<numLODs; ++i)
        {
            MeshDeformerStack* realStack = mLODs[i].mColStack;
            if (realStack)
                result->mLODs[i].mColStack = realStack->Clone(result->mLODs[i].mColMesh, actor);
            else
                result->mLODs[i].mColStack = nullptr;
        }

        // return the resulting clone
        return result;
    }
    */

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
        result->mAttributes.Reserve(mAttributes.GetLength());
        for (uint32 i = 0; i < mAttributes.GetLength(); i++)
        {
            result->AddAttribute(mAttributes[i]->Clone());
        }

        // return the resulting clone
        return result;
    }


    // removes all attributes
    void Node::RemoveAllAttributes()
    {
        while (mAttributes.GetLength())
        {
            mAttributes.GetLast()->Destroy();
            mAttributes.RemoveLast();
        }
    }


    // get the total number of children
    uint32 Node::GetNumChildNodesRecursive() const
    {
        // the number of total child nodes which include the childs of the childs, too
        uint32 result = 0;

        // retrieve the number of child nodes of the actual node
        const uint32 numChildNodes = GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mSkeleton->GetNode(mChildIndices[i])->RecursiveCountChildNodes(result);
        }

        return result;
    }


    // recursively count the number of nodes down the hierarchy
    void Node::RecursiveCountChildNodes(uint32& numNodes)
    {
        // increase the counter
        numNodes++;

        // recurse down the hierarchy
        const uint32 numChildNodes = mChildIndices.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mSkeleton->GetNode(mChildIndices[i])->RecursiveCountChildNodes(numNodes);
        }
    }


    // recursively go through the parents until a root node is reached and store all parents inside an array
    void Node::RecursiveCollectParents(AZStd::vector<uint32>& parents, bool clearParentsArray) const
    {
        if (clearParentsArray)
        {
            parents.clear();
        }

        // loop until we reached a root node
        Node* node = const_cast<Node*>(this);
        while (node)
        {
            // get the parent index and add it to the list of parents if the current node is not a root node
            const uint32 parentIndex = node->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
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
    void Node::RemoveAttributeByType(uint32 attributeTypeID, uint32 occurrence)
    {
        // retrieve the number of attributes inside this node
        const uint32 numAttributes = GetNumAttributes();

        // counts the number of occurrences of the attribute to search for
        uint32 numOccurredAttibutes = 0;

        // iterate through all node attributes
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            // get the current node attribute
            NodeAttribute* nodeAttribute = GetAttribute(i);

            // check the type of the current node attribute and compare the two
            if (nodeAttribute->GetType() == attributeTypeID)
            {
                // increase the occurrence counter
                numOccurredAttibutes++;

                // check if the found attribute is the one we searched
                if (occurrence < numOccurredAttibutes)
                {
                    // remove the attribute and return
                    RemoveAttribute(i);
                    return;
                }
            }
        }
    }


    // remove all attributes of the given type from the node
    uint32 Node::RemoveAllAttributesByType(uint32 attributeTypeID)
    {
        uint32 attributeNumber = MCORE_INVALIDINDEX32;
        uint32 numAttributesRemoved = 0;

        // try to find a node of the given attribute type
        while ((attributeNumber = FindAttributeNumber(attributeTypeID)) != MCORE_INVALIDINDEX32)
        {
            // remove the attribute we found and go again
            RemoveAttribute(attributeNumber);

            // increase the number of removed attributes
            numAttributesRemoved++;
        }

        return numAttributesRemoved;
    }



    // recursively find the root node (expensive call)
    Node* Node::FindRoot() const
    {
        uint32 parentIndex = mParentIndex;
        Node* curNode = const_cast<Node*>(this);

        while (parentIndex != MCORE_INVALIDINDEX32)
        {
            curNode = mSkeleton->GetNode(parentIndex);
            parentIndex = curNode->GetParentIndex();
        }

        return curNode;
    }


    // get the parent node, or nullptr when it doesn't exist
    Node* Node::GetParentNode() const
    {
        if (mParentIndex != MCORE_INVALIDINDEX32)
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
            mNameID = MCORE_INVALIDINDEX32;
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
            mSemanticNameID = MCORE_INVALIDINDEX32;
        }
    }


    void Node::SetParentIndex(uint32 parentNodeIndex)
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
        return (mParentIndex == MCORE_INVALIDINDEX32);
    }


    //-------------------------------------------------------------------------------------

    void Node::AddAttribute(NodeAttribute* attribute)
    {
        mAttributes.Add(attribute);
    }


    uint32 Node::GetNumAttributes() const
    {
        return mAttributes.GetLength();
    }


    NodeAttribute* Node::GetAttribute(uint32 attributeNr)
    {
        // make sure we are in range
        MCORE_ASSERT(attributeNr < mAttributes.GetLength());

        // return the attribute
        return mAttributes[attributeNr];
    }


    uint32 Node::FindAttributeNumber(uint32 attributeTypeID) const
    {
        // check all attributes, and find where the specific attribute is
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mAttributes[i]->GetType() == attributeTypeID)
            {
                return i;
            }
        }

        // not found
        return MCORE_INVALIDINDEX32;
    }


    NodeAttribute* Node::GetAttributeByType(uint32 attributeType)
    {
        // check all attributes
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mAttributes[i]->GetType() == attributeType)
            {
                return mAttributes[i];
            }
        }

        // not found
        return nullptr;
    }


    // remove the given attribute
    void Node::RemoveAttribute(uint32 index)
    {
        mAttributes.Remove(index);
    }


    void Node::AddChild(uint32 nodeIndex)
    {
        mChildIndices.AddExact(nodeIndex);
    }


    void Node::SetChild(uint32 childNr, uint32 childNodeIndex)
    {
        mChildIndices[childNr] = childNodeIndex;
    }


    void Node::SetNumChildNodes(uint32 numChildNodes)
    {
        mChildIndices.Resize(numChildNodes);
    }


    void Node::PreAllocNumChildNodes(uint32 numChildNodes)
    {
        mChildIndices.Reserve(numChildNodes);
    }


    void Node::RemoveChild(uint32 nodeIndex)
    {
        mChildIndices.RemoveByValue(nodeIndex);
    }


    void Node::RemoveAllChildNodes()
    {
        mChildIndices.Clear();
    }


    bool Node::GetHasChildNodes() const
    {
        return (mChildIndices.GetLength() > 0);
    }


    void Node::SetNodeIndex(uint32 index)
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
