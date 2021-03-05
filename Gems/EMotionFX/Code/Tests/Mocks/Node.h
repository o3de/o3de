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
    class Actor;
    class Skeleton;
    class NodeAttribute;

    class Node
    {
        friend class Actor;

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        static Node* Create(const char* name, Skeleton* skeleton);
        static Node* Create(uint32 nameID, Skeleton* skeleton);
        MOCK_CONST_METHOD1(Clone, Node*(Skeleton* skeleton));
        MOCK_METHOD1(SetParentIndex, void(uint32 parentNodeIndex));
        MOCK_CONST_METHOD0(GetParentIndex, uint32());
        MOCK_CONST_METHOD0(GetParentNode, Node*());
        MOCK_CONST_METHOD2(RecursiveCollectParents, void(MCore::Array<uint32>& parents, bool clearParentsArray));
        MOCK_METHOD1(SetName, void(const char* name));
        MOCK_CONST_METHOD0(GetName, const char*());
        MOCK_CONST_METHOD0(GetNameString, const AZStd::string&());
        MOCK_METHOD1(SetSemanticName, void(const char* name));
        MOCK_CONST_METHOD0(GetSemanticName, const char*());
        MOCK_CONST_METHOD0(GetSemanticNameString, const AZStd::string&());
        MOCK_CONST_METHOD0(GetID, uint32());
        MOCK_CONST_METHOD0(GetSemanticID, uint32());
        MOCK_CONST_METHOD0(GetNumChildNodes, uint32());
        MOCK_CONST_METHOD0(GetNumChildNodesRecursive, uint32());
        MOCK_CONST_METHOD1(GetChildIndex, uint32(uint32 nr));
        MOCK_CONST_METHOD1(CheckIfIsChildNode, bool(uint32 nodeIndex));
        MOCK_METHOD1(AddChild, void(uint32 nodeIndex));
        MOCK_METHOD2(SetChild, void(uint32 childNr, uint32 childNodeIndex));
        MOCK_METHOD1(SetNumChildNodes, void(uint32 numChildNodes));
        MOCK_METHOD1(PreAllocNumChildNodes, void(uint32 numChildNodes));
        MOCK_METHOD1(RemoveChild, void(uint32 nodeIndex));
        MOCK_METHOD0(RemoveAllChildNodes, void());
        MOCK_CONST_METHOD0(GetIsRootNode, bool());
        MOCK_CONST_METHOD0(GetHasChildNodes, bool());
        MOCK_CONST_METHOD0(FindRoot, Node*());
        MOCK_METHOD1(AddAttribute, void(NodeAttribute* attribute));
        MOCK_CONST_METHOD0(GetNumAttributes, uint32());
        MOCK_METHOD1(GetAttribute, NodeAttribute*(uint32 attributeNr));
        MOCK_METHOD1(GetAttributeByType, NodeAttribute*(uint32 attributeType));
        MOCK_CONST_METHOD1(FindAttributeNumber, uint32(uint32 attributeTypeID));
        MOCK_METHOD0(RemoveAllAttributes, void());
        MOCK_METHOD1(RemoveAttribute, void(uint32 index));
        MOCK_METHOD2(RemoveAttributeByType, void(uint32 attributeTypeID, uint32 occurrence));
        MOCK_METHOD1(RemoveAllAttributesByType, uint32(uint32 attributeTypeID));
        MOCK_METHOD1(SetNodeIndex, void(uint32 index));
        MOCK_CONST_METHOD0(GetNodeIndex, uint32());
        MOCK_METHOD1(SetSkeletalLODLevelBits, void(uint32 bitValues));
        MOCK_METHOD2(SetSkeletalLODStatus, void(uint32 lodLevel, bool enabled));
        MOCK_CONST_METHOD1(GetSkeletalLODStatus, bool(uint32 lodLevel));
        MOCK_CONST_METHOD0(GetIncludeInBoundsCalc, bool());
        MOCK_METHOD1(SetIncludeInBoundsCalc, void(bool includeThisNode));
        MOCK_CONST_METHOD0(GetIsAttachmentNode, bool());
        MOCK_METHOD1(SetIsAttachmentNode, void(bool isAttachmentNode));
    };
} // namespace EMotionFX
