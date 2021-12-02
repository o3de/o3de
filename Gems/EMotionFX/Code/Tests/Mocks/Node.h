/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        MOCK_METHOD1(SetParentIndex, void(size_t parentNodeIndex));
        MOCK_CONST_METHOD0(GetParentIndex, size_t());
        MOCK_CONST_METHOD0(GetParentNode, Node*());
        MOCK_CONST_METHOD2(RecursiveCollectParents, void(AZStd::vector<size_t>& parents, bool clearParentsArray));
        MOCK_METHOD1(SetName, void(const char* name));
        MOCK_CONST_METHOD0(GetName, const char*());
        MOCK_CONST_METHOD0(GetNameString, const AZStd::string&());
        MOCK_METHOD1(SetSemanticName, void(const char* name));
        MOCK_CONST_METHOD0(GetSemanticName, const char*());
        MOCK_CONST_METHOD0(GetSemanticNameString, const AZStd::string&());
        MOCK_CONST_METHOD0(GetID, uint32());
        MOCK_CONST_METHOD0(GetSemanticID, uint32());
        MOCK_CONST_METHOD0(GetNumChildNodes, size_t());
        MOCK_CONST_METHOD0(GetNumChildNodesRecursive, size_t());
        MOCK_CONST_METHOD1(GetChildIndex, size_t(size_t nr));
        MOCK_CONST_METHOD1(CheckIfIsChildNode, bool(size_t nodeIndex));
        MOCK_METHOD1(AddChild, void(size_t nodeIndex));
        MOCK_METHOD2(SetChild, void(size_t childNr, size_t childNodeIndex));
        MOCK_METHOD1(SetNumChildNodes, void(size_t numChildNodes));
        MOCK_METHOD1(PreAllocNumChildNodes, void(size_t numChildNodes));
        MOCK_METHOD1(RemoveChild, void(size_t nodeIndex));
        MOCK_METHOD0(RemoveAllChildNodes, void());
        MOCK_CONST_METHOD0(GetIsRootNode, bool());
        MOCK_CONST_METHOD0(GetHasChildNodes, bool());
        MOCK_CONST_METHOD0(FindRoot, Node*());
        MOCK_METHOD1(AddAttribute, void(NodeAttribute* attribute));
        MOCK_CONST_METHOD0(GetNumAttributes, size_t());
        MOCK_METHOD1(GetAttribute, NodeAttribute*(size_t attributeNr));
        MOCK_METHOD1(GetAttributeByType, NodeAttribute*(uint32 attributeType));
        MOCK_CONST_METHOD1(FindAttributeNumber, size_t(uint32 attributeTypeID));
        MOCK_METHOD0(RemoveAllAttributes, void());
        MOCK_METHOD1(RemoveAttribute, void(size_t index));
        MOCK_METHOD2(RemoveAttributeByType, void(uint32 attributeTypeID, size_t occurrence));
        MOCK_METHOD1(RemoveAllAttributesByType, size_t(uint32 attributeTypeID));
        MOCK_METHOD1(SetNodeIndex, void(size_t index));
        MOCK_CONST_METHOD0(GetNodeIndex, size_t());
        MOCK_METHOD1(SetSkeletalLODLevelBits, void(uint32 bitValues));
        MOCK_METHOD2(SetSkeletalLODStatus, void(size_t lodLevel, bool enabled));
        MOCK_CONST_METHOD1(GetSkeletalLODStatus, bool(size_t lodLevel));
        MOCK_CONST_METHOD0(GetIncludeInBoundsCalc, bool());
        MOCK_METHOD1(SetIncludeInBoundsCalc, void(bool includeThisNode));
        MOCK_CONST_METHOD0(GetIsAttachmentNode, bool());
        MOCK_METHOD1(SetIsAttachmentNode, void(bool isAttachmentNode));
    };
} // namespace EMotionFX
