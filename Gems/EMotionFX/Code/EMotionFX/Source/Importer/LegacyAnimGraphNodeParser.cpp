/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LegacyAnimGraphNodeParser.h"
#include <MCore/Source/FastMath.h>
#include <MCore/Source/AzCoreConversions.h>
#include "../ConstraintTransformRotationAngles.h"
#include "ChunkProcessors.h"
#include "../AnimGraph.h"
#include "../AnimGraphStateMachine.h"
#include "../AnimGraphBindPoseNode.h"
#include "../AnimGraphHubNode.h"
#include "../AnimGraphParameterCondition.h"
#include "../AnimGraphVector2Condition.h"
#include "../AnimGraphMotionCondition.h"
#include "../AnimGraphStateCondition.h"
#include "../AnimGraphTimeCondition.h"
#include "../AnimGraphPlayTimeCondition.h"
#include "../AnimGraphTagCondition.h"
#include "../BlendTreeMotionFrameNode.h"
#include "../BlendTreeBlendNNode.h"
#include "../BlendTreeFloatConditionNode.h"
#include "../BlendTreeFloatSwitchNode.h"
#include "../BlendTreeBoolLogicNode.h"
#include "../AnimGraphMotionNode.h"
#include "../BlendTreeAccumTransformNode.h"
#include "../BlendTreeBlend2LegacyNode.h"
#include "../BlendTreeFloatConstantNode.h"
#include "../BlendTreeFloatMath1Node.h"
#include "../BlendTreeFloatMath2Node.h"
#include "../BlendTreeMorphTargetNode.h"
#include "../BlendTreeVector2ComposeNode.h"
#include "../BlendTreeVector3ComposeNode.h"
#include "../BlendTreeVector4ComposeNode.h"
#include "../BlendTreeVector3Math1Node.h"
#include "../BlendTreeVector3Math2Node.h"
#include "../BlendTreeSmoothingNode.h"
#include "../BlendTreeRangeRemapperNode.h"
#include "../BlendTreeTwoLinkIKNode.h"
#include "../BlendTreeLookAtNode.h"
#include "../BlendTreeTransformNode.h"
#include "../BlendTreeMaskLegacyNode.h"
#include "../BlendTreePoseSwitchNode.h"
#include "../BlendTreeVector2DecomposeNode.h"
#include "../BlendTreeVector3DecomposeNode.h"
#include "../BlendTreeVector4DecomposeNode.h"
#include "../BlendTreeDirectionToWeightNode.h"
#include "../BlendTreeMirrorPoseNode.h"
#include "../AnimGraphEntryNode.h"
#include "../AnimGraphExitNode.h"
#include "../BlendTreeParameterNode.h"
#include "../BlendSpace1DNode.h"
#include "../BlendSpace2DNode.h"
#include "../BlendTreeFinalNode.h"
#include "../BlendTree.h"
#include "../BlendSpaceNode.h"
#include "../AnimGraphStateTransition.h"
#include "../AnimGraphMotionCondition.h"
#include "../AnimGraphParameterCondition.h"
#include "../AnimGraphPlayTimeCondition.h"
#include "../AnimGraphStateCondition.h"
#include "../AnimGraphTagCondition.h"
#include "../AnimGraphTimeCondition.h"
#include "../AnimGraphVector2Condition.h"
#include "../BlendTreeFloatConstantNode.h"
#include "../BlendTreeMorphTargetNode.h"
#include "../BlendTreeVector2ComposeNode.h"
#include "../BlendTreeVector3ComposeNode.h"
#include "../BlendTreeVector4ComposeNode.h"
#include "../TwoStringEventData.h"


namespace EMotionFX
{
    const AZ::TypeId GetNewTypeIdByOldNodeTypeId(uint32 oldNodeTypeId)
    {
        switch (oldNodeTypeId)
        {
            case 0x00000017: return azrtti_typeid<AnimGraphBindPoseNode>(); break;
            case 0x00000005: return azrtti_typeid<AnimGraphStateMachine>(); break;
            case 0x00000002: return azrtti_typeid<AnimGraphMotionNode>(); break;
            case 0x38658581: return azrtti_typeid<AnimGraphHubNode>(); break;
            case 0x32521069: return azrtti_typeid<AnimGraphExitNode>(); break;
            case 0x38020071: return azrtti_typeid<AnimGraphEntryNode>(); break;
            case 0x00000006: return azrtti_typeid<BlendTree>(); break;
            case 0x00000001: return azrtti_typeid<BlendTreeFinalNode>(); break;
            case 0x00022100: return azrtti_typeid<BlendSpace1DNode>(); break;
            case 0x00022200: return azrtti_typeid<BlendSpace2DNode>(); break;
            case 0x00000004: return azrtti_typeid<BlendTreeBlend2LegacyNode>(); break;
            case 0x00000013: return azrtti_typeid<BlendTreeBlendNNode>(); break;
            case 0x00000007: return azrtti_typeid<BlendTreeParameterNode>(); break;
            case 0x00000008: return azrtti_typeid<BlendTreeFloatMath1Node>(); break;
            case 0x00000009: return azrtti_typeid<BlendTreeFloatMath2Node>(); break;
            case 0x00000010: return azrtti_typeid<BlendTreeFloatConditionNode>(); break;
            case 0x00000148: return azrtti_typeid<BlendTreeFloatConstantNode>(); break;
            case 0x00000012: return azrtti_typeid<BlendTreeFloatSwitchNode>(); break;
            case 0x00000011: return azrtti_typeid<BlendTreeBoolLogicNode>(); break;
            case 0x00000014: return azrtti_typeid<BlendTreePoseSwitchNode>(); break;
            case 0x00000016: return azrtti_typeid<BlendTreeMaskLegacyNode>(); break;
            case 0x00002445: return azrtti_typeid<BlendTreeMorphTargetNode>(); break;
            case 0x00000018: return azrtti_typeid<BlendTreeMotionFrameNode>(); break;
            case 0x00000020: return azrtti_typeid<BlendTreeVector3Math1Node>(); break;
            case 0x00000021: return azrtti_typeid<BlendTreeVector3Math2Node>(); break;
            case 0x00000126: return azrtti_typeid<BlendTreeVector2DecomposeNode>(); break;
            case 0x00000227: return azrtti_typeid<BlendTreeVector3DecomposeNode>(); break;
            case 0x00000129: return azrtti_typeid<BlendTreeVector4DecomposeNode>(); break;
            case 0x00000228: return azrtti_typeid<BlendTreeVector2ComposeNode>(); break;
            case 0x00000128: return azrtti_typeid<BlendTreeVector3ComposeNode>(); break;
            case 0x00000229: return azrtti_typeid<BlendTreeVector4ComposeNode>(); break;
            case 0x00000456: return azrtti_typeid<BlendTreeSmoothingNode>(); break;
            case 0x02094017: return azrtti_typeid<BlendTreeRangeRemapperNode>(); break;
            case 0x38427080: return azrtti_typeid<BlendTreeDirectionToWeightNode>(); break;
            case 0x00000214: return azrtti_typeid<BlendTreeMirrorPoseNode>(); break;
            case 0x00001286: return azrtti_typeid<BlendTreeTwoLinkIKNode>(); break;
            case 0x00040360: return azrtti_typeid<BlendTreeLookAtNode>(); break;
            case 0x00012345: return azrtti_typeid<BlendTreeTransformNode>(); break;
            case 0x00012346: return azrtti_typeid<BlendTreeAccumTransformNode>(); break;
            case 0x00001000: return azrtti_typeid<AnimGraphStateTransition>(); break;
            case 0x00002000: return azrtti_typeid<AnimGraphParameterCondition>(); break;
            case 0x00002123: return azrtti_typeid<AnimGraphVector2Condition>(); break;
            case 0x00002001: return azrtti_typeid<AnimGraphMotionCondition>(); break;
            case 0x09502005: return azrtti_typeid<AnimGraphStateCondition>(); break;
            case 0x00005210: return azrtti_typeid<AnimGraphTimeCondition>(); break;
            case 0x00029610: return azrtti_typeid<AnimGraphPlayTimeCondition>(); break;
            case 0x00005321: return azrtti_typeid<AnimGraphTagCondition>(); break;
            default: return AZ::TypeId::CreateNull();
        };
    }

    class LegacyAttributeRotation
    {
    public:
        LegacyAttributeRotation() 
        {}

        LegacyAttributeRotation(const LegacyAttributeRotation& src):
            m_rotation(src.m_rotation),
            m_degrees(src.m_degrees),
            m_order(src.m_order)
        {}

        LegacyAttributeRotation& operator=(const LegacyAttributeRotation& src)
        {
            m_rotation = src.m_rotation;
            m_degrees = src.m_degrees;
            m_order = src.m_order;
            return *this;
        }

        void SetRotation(const AZ::Quaternion& rotation)
        {
            m_rotation = rotation;
        }

        void SetDegrees(const AZ::Vector3& degrees)
        {
            m_degrees = degrees;
        }

        void SetOrder(LegacyERotationOrder order)
        {
            m_order = order;
        }

        const AZ::Quaternion& GetRotation() const
        {
            return m_rotation;
        }

        const AZ::Vector3& GetDegrees() const
        {
            return m_degrees;
        }

        LegacyERotationOrder GetOrder() const
        {
            return m_order;
        }

    private:
        AZ::Quaternion          m_rotation;  /**< The unit quaternion rotation. */
        AZ::Vector3             m_degrees;   /**< The rotation angles. As programmer you don't need to setup these values. They are only to display in the GUI. */
        LegacyERotationOrder    m_order;     /**< The rotation order, which defaults to ZYX. */
    };

    class LegacyStateFilterLocal
    {
    public:
        LegacyStateFilterLocal() { }

        LegacyStateFilterLocal(const LegacyStateFilterLocal& src) :
            m_nodeNames(src.m_nodeNames),
            m_groupNames(src.m_groupNames)
        { }

        LegacyStateFilterLocal& operator=(const LegacyStateFilterLocal& src)
        {
            m_nodeNames = src.m_nodeNames;
            m_groupNames = src.m_groupNames;
            return *this;
        }

        void SetNodeNmes(const AZStd::vector<AZStd::string>& nodeNames)
        {
            m_nodeNames = nodeNames;
        }

        void SetGroupNames(const AZStd::vector<AZStd::string>& groupNames)
        {
            m_groupNames = groupNames;
        }

        const AZStd::vector<AZStd::string>& GetNodeNames() const
        {
            return m_nodeNames;
        }

        const AZStd::vector<AZStd::string>& GetGroupNames() const
        {
            return m_groupNames;
        }

    private:
        AZStd::vector<AZStd::string> m_nodeNames;
        AZStd::vector<AZStd::string> m_groupNames;
    };

bool LegacyAnimGraphNodeParser::Forward(MCore::File* stream, size_t numBytes)
{
    if(!stream->Forward(numBytes))
    {
        AZ_Error("EMotionFX", false, "Unable to skip data in stream");
        return false;
    }
    return true;
}

template<>
bool LegacyAttribute<BlendSpaceNode::BlendSpaceMotion>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    AZStd::string motionId;

    AZ::u32 numCharacters;
    if (stream->Read(&numCharacters, sizeof(AZ::u32)) != sizeof(AZ::u32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);

    if (numCharacters > 0)
    {
        motionId.resize(numCharacters);
        if (stream->Read(motionId.data(), numCharacters) != numCharacters)
        {
            AZ_Error("EMotionFX", false, "Unable to read data in stream");
            return false;
        }
    }

    AZ::Vector2 position;
    if (stream->Read(&position, MCore::AttributeVector2::sizeofVector2) != MCore::AttributeVector2::sizeofVector2)
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    float x = position.GetX();
    float y = position.GetY();
    MCore::Endian::ConvertFloat(&x, endianType);
    MCore::Endian::ConvertFloat(&y, endianType);
    position.Set(x, y);


    AZ::u8 streamTypeFlags;
    if (stream->Read(&streamTypeFlags, sizeof(AZ::u8)) != sizeof(AZ::u8))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags typeFlags = static_cast<BlendSpaceNode::BlendSpaceMotion::TypeFlags>(streamTypeFlags);
    m_value.Set(motionId, position, typeFlags);
    return true;
}

template<>
bool LegacyAttribute<AZStd::vector<AZStd::string>>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    // Read the number of entries
    uint32 numEntries;
    if (stream->Read(&numEntries, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numEntries, endianType);

    m_value.resize(numEntries);
    for (uint32 i = 0; i < numEntries; ++i)
    {
        // Read the number of string bytes to follow
        uint32 numStringBytes;
        if (stream->Read(&numStringBytes, sizeof(uint32)) != sizeof(uint32))
        {
            AZ_Error("EMotionFX", false, "LegacyParameterMask: unable to read data in stream");
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numStringBytes, endianType);
        AZStd::string name;
        if (numStringBytes > 0)
        {
            name.resize(numStringBytes);
            // Read the string data
            if (stream->Read(name.data(), numStringBytes) != numStringBytes)
            {
                AZ_Error("EMotionFX", false, "LegacyParameterMask: Unable to read data in stream");
                return false;
            }
        }
        m_value[i] = name;
    }

    return true;
}

template<>
bool LegacyAttribute<AZ::Vector2>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    // Read the value
    AZ::Vector2 streamValue;
    if (stream->Read(&streamValue, MCore::AttributeVector2::sizeofVector2) != MCore::AttributeVector2::sizeofVector2)
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    // Convert endian
    MCore::Endian::ConvertVector2(&streamValue, endianType);
    m_value = streamValue;
    return true;
}

template<>
bool LegacyAttribute<LegacyAttributeRotation>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    uint8 version;
    if(stream->Read(&version, sizeof(uint8)) != sizeof(uint8))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    switch (version)
    {
    case 1:
    {
        AZ::PackedVector3f streamValue;
        if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) != sizeof(AZ::PackedVector3f))
        {
            return false;
        }

        // Convert endian
        AZ::Vector3 value(streamValue);
        MCore::Endian::ConvertVector3(&value, endianType);

        // Read only the degrees, automatically calculate the quaternion
        m_value.SetDegrees(value);
        const AZ::Quaternion rotation = MCore::AzEulerAnglesToAzQuat(MCore::Math::DegreesToRadians(value.GetX()), MCore::Math::DegreesToRadians(value.GetY()), MCore::Math::DegreesToRadians(value.GetZ()));
        m_value.SetRotation(rotation);
    }
    break;
    case 2:
    {
        AZ::PackedVector3f streamValue;
        if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) != sizeof(AZ::PackedVector3f))
        {
            return false;
        }

        // Convert endian
        AZ::Vector3 value(streamValue);
        MCore::Endian::ConvertVector3(&value, endianType);
        m_value.SetDegrees(value);

        AZ::Quaternion streamValueQ;
        if (stream->Read(&streamValueQ, sizeof(AZ::Quaternion)) != sizeof(AZ::Quaternion))
        {
            return false;
        }

        // Convert endian
        MCore::Endian::ConvertQuaternion(&streamValueQ, endianType);
        m_value.SetRotation(streamValueQ);
    }
    break;
    case 3:
    {
        // Read the value
        AZ::PackedVector3f streamValue;
        if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) != sizeof(AZ::PackedVector3f))
        {
            return false;
        }

        // Convert endian
        AZ::Vector3 value(streamValue);
        MCore::Endian::ConvertVector3(&value, endianType);
        m_value.SetDegrees(value);

        // Read the quaternion
        AZ::Quaternion streamValueQ;
        if (stream->Read(&streamValueQ, sizeof(AZ::Quaternion)) != sizeof(AZ::Quaternion))
        {
            return false;
        }

        // Convert endian
        MCore::Endian::ConvertQuaternion(&streamValueQ, endianType);
        m_value.SetRotation(streamValueQ);

        // Read the rotation order
        uint8 order = 0;
        if (stream->Read(&order, sizeof(uint8)) != sizeof(uint8))
        {
            return false;
        }

        m_value.SetOrder(static_cast<LegacyERotationOrder>(order));
    }
    break;
    default:
    {
        AZ_Error("EMotionFX", false, "Unknown version %u parsing legacy attribute rotation", version);
        return false;
    }
    break;
    }

    return true;
}

template<>
bool LegacyAttribute<AZ::PackedVector3f>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    // Read the value
    AZ::PackedVector3f streamValue(0.0f);
    if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) != sizeof(AZ::PackedVector3f))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    // Convert endian
    AZ::Vector3 value(streamValue);
    MCore::Endian::ConvertVector3(&value, endianType);

    m_value = AZ::PackedVector3f(value.GetX(), value.GetY(), value.GetZ());
    return true;
}

template<>
bool LegacyAttribute<bool>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    AZ_UNUSED(endianType);
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    int8 streamValue;
    if (stream->Read(&streamValue, sizeof(int8)) != sizeof(int8))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    m_value = (streamValue == 0) ? false : true;
    return true;
}

template<>
bool LegacyAttribute<BlendTreeTwoLinkIKNode::NodeAlignmentData>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    // Read the number of characters
    uint32 numCharacters;
    if (stream->Read(&numCharacters, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    // Convert endian
    MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);
    AZStd::string nodeName;
    // Read the character data
    if (numCharacters > 0)
    {
        nodeName.resize(numCharacters);
        if (stream->Read( nodeName.data(), sizeof(char) * numCharacters) != (sizeof(char) * numCharacters))
        {
            AZ_Error("EMotionFX", false, "Unable to read data in stream");
            return false;
        }
    }

    // Read the parent depth
    uint32 parentDepth;
    if (stream->Read(&parentDepth, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    // Convert endian
    MCore::Endian::ConvertUnsignedInt32(&parentDepth, endianType);
    m_value.first = nodeName;
    m_value.second = parentDepth;
    return true;
}

template<>
bool LegacyAttribute<LegacyStateFilterLocal>::Parse(MCore::File* stream, MCore::Endian::EEndianType streamEndianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        AZ_Error("EMotionFX", false, "Unable to skip version for legacy attribute State Filter");
        return false;
    }

    // Read the number of group entries
    uint32 numGroupEntries;
    if (stream->Read(&numGroupEntries, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read num groups");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numGroupEntries, streamEndianType);

    // Read the number of node entries
    uint32 numNodeEntries;
    if (stream->Read(&numNodeEntries, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read num nodes");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numNodeEntries, streamEndianType);

    // Read the group entries
    AZStd::string convTemp;

    AZStd::vector<AZStd::string> groupNames;
    groupNames.resize(numGroupEntries);

    for (uint32 i = 0; i < numGroupEntries; ++i)
    {
        // Read the number of string bytes to follow
        uint32 numStringBytes;
        if (stream->Read(&numStringBytes, sizeof(uint32)) != sizeof(uint32))
        {
            AZ_Error("EMotionFX", false, "Unable to read group name size");
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numStringBytes, streamEndianType);

        // Read the string data
        if (numStringBytes > 0)
        {
            convTemp.resize(numStringBytes);
            if (stream->Read(convTemp.data(), numStringBytes) != numStringBytes)
            {
                AZ_Error("EMotionFX", false, "Unable to read group name");
                return false;
            }
        }
        else
        {
            convTemp.clear();
        }

        // Add the entry to the mask
        groupNames[i] = convTemp;
    }

    // Read the node entries
    AZStd::vector<AZStd::string> nodeNames;
    nodeNames.resize(numNodeEntries);
    for (uint32 i = 0; i < numNodeEntries; ++i)
    {
        // Read the number of string bytes to follow
        uint32 numStringBytes;
        if (stream->Read(&numStringBytes, sizeof(uint32)) != sizeof(uint32))
        {
            AZ_Error("EMotionFX", false, "Unable to read node name size");
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numStringBytes, streamEndianType);

        // Read the string data
        if (numStringBytes > 0)
        {
            convTemp.resize(numStringBytes);
            if (stream->Read(convTemp.data(), numStringBytes) != numStringBytes)
            {
                AZ_Error("EMotionFX", false, "Unable to read node name");
                return false;
            }
        }
        else
        {
            convTemp.clear();
        }

        // Add the entry to the mask
        nodeNames[i] = convTemp;
    }
    m_value.SetNodeNmes(nodeNames);
    m_value.SetGroupNames(groupNames);

    return true;
}

template<>
bool LegacyAttribute<int32>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    int32 streamValue;
    if (stream->Read(&streamValue, sizeof(int32)) != sizeof(int32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    MCore::Endian::ConvertSignedInt32(&streamValue, endianType);
    m_value = streamValue;
    return true;
}

template<>
bool LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    // Read the number of entries
    uint32 numEntries;
    if (stream->Read(&numEntries, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numEntries, endianType);
    m_value.resize(numEntries);
    // Read the entries        
    for (uint32 i = 0; i < numEntries; ++i)
    {
        // Read the weight
        float weight;
        if (stream->Read(&weight, sizeof(float)) != sizeof(float))
        {
            AZ_Error("EMotionFX", false, "Unable to read data in stream");
            return false;
        }
        MCore::Endian::ConvertFloat(&weight, endianType);

        // Read the number of string bytes to follow
        uint32 numStringBytes;
        if (stream->Read(&numStringBytes, sizeof(uint32)) != sizeof(uint32))
        {
            AZ_Error("EMotionFX", false, "Unable to read data in stream");
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numStringBytes, endianType);
        AZStd::string name;

        if (numStringBytes > 0)
        {
            name.resize(numStringBytes);

            // Read the string data
            if (stream->Read(name.data(), numStringBytes) != numStringBytes)
            {
                AZ_Error("EMotionFX", false, "Unable to read data in stream");
                return false;
            }
        }

        // Add the entry to the mask
        m_value[i].first = name;
        m_value[i].second = weight;
    }

    return true;
}

template<>
bool LegacyAttribute<AZStd::string>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    // Read the number of characters
    uint32 numCharacters;
    if (stream->Read(&numCharacters, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    // Convert endian
    MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);
    if (numCharacters == 0)
    {
        m_value.clear();
        return true;
    }

    m_value.resize(numCharacters);
    if (stream->Read(m_value.data(), numCharacters) != numCharacters)
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    return true;
}

template<>
bool LegacyAttribute<float>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Skip the version (not used byt this legacy attribute)
    if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint8)))
    {
        return false;
    }

    float streamValue;
    if (stream->Read(&streamValue, sizeof(float)) != sizeof(float))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }

    MCore::Endian::ConvertFloat(&streamValue, endianType);

    m_value = streamValue;
    return true;
}

template <class T>
const T& LegacyAttribute<T>::GetValue() const
{
    return m_value;
}

bool LegacyAnimGraphNodeParser::GetBlendSpaceNodeEvaluatorTypeId(uint32 legacyIndex, AZ::TypeId& value)
{
    bool result = true;
    switch (legacyIndex)
    {
        case 0:
        {
            value = azrtti_typeid<BlendSpaceParamEvaluatorNone>();
        }
        break;
        case 1:
        {
            value = azrtti_typeid<BlendSpaceFrontBackVelocityParamEvaluator>();
        }
        break;
        case 2:
        {
            value = azrtti_typeid<BlendSpaceLeftRightVelocityParamEvaluator>();
        }
        break;
        case 3:
        {
            value = azrtti_typeid<BlendSpaceMoveSpeedParamEvaluator>();
        }
        break;
        case 4:
        {
            value = azrtti_typeid<BlendSpaceTravelDirectionParamEvaluator>();
        }
        break;
        case 5:
        {
            value = azrtti_typeid<BlendSpaceTravelDistanceParamEvaluator>();
        }
        break;
        case 6:
        {
            value = azrtti_typeid<BlendSpaceTravelSlopeParamEvaluator>();
        }
        break;
        case 7:
        {
            value = azrtti_typeid<BlendSpaceTurnAngleParamEvaluator>();
        }
        break;
        case 8:
        {
            value = azrtti_typeid<BlendSpaceTurnSpeedParamEvaluator>();
        }
        break;
        default:
        {
            result = false;
            AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::GetBlendSpaceNodeEvaluatorTypeId: Unknown typeid index");
        }
        break;
    }
    return result;
}

bool LegacyAnimGraphNodeParser::InitializeNodeGeneralData(const char* nodeName, Importer::ImportParameters& importParams, FileFormat::AnimGraph_NodeHeader& nodeHeader, AnimGraphNode* node)
{
    AnimGraph* animGraph = importParams.mAnimGraph;
    node->SetName(nodeName);

    node->SetVisualPos(nodeHeader.mVisualPosX, nodeHeader.mVisualPosY);
    node->SetIsCollapsed(nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_COLLAPSED);

    const AZ::Color color(
        MCore::ExtractRed(nodeHeader.mVisualizeColor)/255.0f,
        MCore::ExtractGreen(nodeHeader.mVisualizeColor)/255.0f,
        MCore::ExtractBlue(nodeHeader.mVisualizeColor)/255.0f,
        1.0f);
    node->SetVisualizeColor(color);

    if (importParams.mAnimGraphSettings->mDisableNodeVisualization == false)
    {
        node->SetVisualization((nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_VISUALIZED) != 0);
    }
    else
    {
        node->SetVisualization(false);
    }

    node->ReserveChildNodes(nodeHeader.mNumChildNodes);

    if (node->GetSupportsDisable())
    {
        node->SetIsEnabled(!(nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_DISABLED));
    }

    MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);

    // Add the new node to the list of loaded nodes
    blendNodes.Add(node);

    AZStd::map<AZ::u64, uint32>& entryNodeIndexToStateMachineIdLookupTable = SharedHelperData::GetEntryStateToStateMachineTable(importParams.mSharedData);

    // Add the node to the anim graph
    if (nodeHeader.mParentIndex == MCORE_INVALIDINDEX32)
    {
        AZ_Assert(azrtti_typeid(node) == azrtti_typeid<AnimGraphStateMachine>(), "");
        AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(node);

        // Set the root state machine
        if (animGraph->GetRootStateMachine() == nullptr)
        {
            animGraph->SetRootStateMachine(stateMachine);
        }
        else
        {
            AZ_Warning("EMotionFX", false, "Anim graph already contains a root state machine. Skipping additional root state machines.");
        }
    }
    else
    {
        blendNodes[nodeHeader.mParentIndex]->AddChildNode(node);

        AZStd::rbtree_const_iterator<AZStd::pair<AZ::u64, uint32>> tableIter = entryNodeIndexToStateMachineIdLookupTable.find(AZ::u64(blendNodes[nodeHeader.mParentIndex]->GetId()));
        if (tableIter != entryNodeIndexToStateMachineIdLookupTable.end())
        {
            AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeById(AnimGraphNodeId((*tableIter).first));
            // The type id check will remove the pointer
            if (!animGraphNode)
            {
                AZ_Error("EMotionFX", false, "Unable to find expected State Machine that needs a registered entry state");
                AZ_Assert(false, "Unable to find expected State Machine that needs a registered entry state");
                return false;
            }
            if (azrtti_typeid(animGraphNode) != azrtti_typeid<AnimGraphStateMachine>())
            {
                AZ_Error("EMotionFX", false, "Unexpected parent node type");
                AZ_Assert(false, "Unexpected parent node type");
                return false;
            }
            AnimGraphStateMachine& parentStateMachine = *(static_cast<AnimGraphStateMachine*>(animGraphNode));
            uint32 entryStateChildNodeIndex = (*tableIter).second;
            if (entryStateChildNodeIndex < parentStateMachine.GetNumChildNodes())
            {
                parentStateMachine.SetEntryStateId(parentStateMachine.GetChildNode(entryStateChildNodeIndex)->GetId());
                entryNodeIndexToStateMachineIdLookupTable.erase(tableIter);
            }
        }

        // Set the final node
        if (azrtti_typeid(node) == azrtti_typeid<BlendTreeFinalNode>())
        {
            AZ_Assert(azrtti_typeid(blendNodes[nodeHeader.mParentIndex]) == azrtti_typeid<BlendTree>(), "");
            BlendTree* blendTree = static_cast<BlendTree*>(blendNodes[nodeHeader.mParentIndex]);
            blendTree->SetFinalNodeId(node->GetId());
        }

        // Update the virtual final output node
        if (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_VIRTUALFINALOUTPUT)
        {
            AZ_Assert(azrtti_typeid(blendNodes[nodeHeader.mParentIndex]) == azrtti_typeid<BlendTree>(), "");
            BlendTree* blendTree = static_cast<BlendTree*>(blendNodes[nodeHeader.mParentIndex]);
            blendTree->SetVirtualFinalNode(node);
        }
        return true;
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector4ComposeNode>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3ComposeNode>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector2ComposeNode>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}


template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMorphTargetNode>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeMorphTargetNode& blendTreeMorphTargetNode = static_cast<BlendTreeMorphTargetNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttributeArray<AZStd::string, LegacyAttribute> legacyAttributeArrayStrings;
                if (!legacyAttributeArrayStrings.Parse(stream, endianType))
                {
                    return false;
                }
                AZStd::vector<AZStd::string> morphTargetNames;
                morphTargetNames.resize(legacyAttributeArrayStrings.GetValue().size());
                for (int i = 0; i < legacyAttributeArrayStrings.GetValue().size(); i++)
                {
                    morphTargetNames[i] = legacyAttributeArrayStrings.GetValue()[i].GetValue();
                }
                blendTreeMorphTargetNode.SetMorphTargetNames(morphTargetNames);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMorphTargetNode>: Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}



template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConstantNode>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeFloatConstantNode& blendTreeFloatConstantNode = static_cast<BlendTreeFloatConstantNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatConstantNode.SetValue(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConstantNode>: Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeLookAtNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeLookAtNode& blendTreeLookAtNode = static_cast<BlendTreeLookAtNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeLookAtNode.SetTargetNodeName(legacyAttribute.GetValue());
            }
            break;
            case 1:
            {
                LegacyAttribute<AZ::Vector2> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeLookAtNode.SetLimitMin(legacyAttribute.GetValue());
            }
            break;
            case 2:
            {
                LegacyAttribute<AZ::Vector2> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeLookAtNode.SetLimitMax(legacyAttribute.GetValue());
            }
            break;
            case 3:
            {
                LegacyAttribute<LegacyAttributeRotation> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }

                blendTreeLookAtNode.SetConstraintRotation(legacyAttribute.GetValue().GetRotation());
            }
            break;
            case 4:
            {
                LegacyAttribute<LegacyAttributeRotation> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }

                blendTreeLookAtNode.SetPostRotation(legacyAttribute.GetValue().GetRotation());
            }
            break;
            case 5:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeLookAtNode.SetFollowSpeed(floatValue);
            }
            break;
            case 6:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                ConstraintTransformRotationAngles::EAxis axis = static_cast<ConstraintTransformRotationAngles::EAxis>(static_cast<uint32>(floatValue));
                blendTreeLookAtNode.SetTwistAxis(axis);
            }
            break;
            case 7:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool limitsEnabled = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeLookAtNode.SetLimitsEnabled(limitsEnabled);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeTwoLinkIKNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeTwoLinkIKNode& blendTreeTwoLinkIKNode = static_cast<BlendTreeTwoLinkIKNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTwoLinkIKNode.SetEndNodeName(legacyAttribute.GetValue());
            }
            break;
            case 1:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTwoLinkIKNode.SetEndEffectorNodeName(legacyAttribute.GetValue());
            }
            break;
            case 2:
            {
                LegacyAttribute<BlendTreeTwoLinkIKNode::NodeAlignmentData> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTwoLinkIKNode.SetAlignToNode(legacyAttribute.GetValue());
            }
            break;
            case 3:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTwoLinkIKNode.SetBendDirNodeName(legacyAttribute.GetValue());
            }
            break;
            case 4:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool boolValue = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeTwoLinkIKNode.SetRotationEnabled(boolValue);
            }
            break;
            case 5:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool boolValue = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeTwoLinkIKNode.SetRelativeBendDir(boolValue);
            }
            break;
            case 6:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool boolValue = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeTwoLinkIKNode.SetExtractBendDir(boolValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}


template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatMath1Node>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeFloatMath1Node& blendTreeFloatMath1Node = static_cast<BlendTreeFloatMath1Node&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeFloatMath1Node::EMathFunction eMathFunction = static_cast<BlendTreeFloatMath1Node::EMathFunction>(static_cast<uint32>(floatValue));
                blendTreeFloatMath1Node.SetMathFunction(eMathFunction);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}
template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateTransition>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AnimGraphStateTransition& animGraphStateTransition = static_cast<AnimGraphStateTransition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:// ATTRIB_DISABLED
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_DISABLED");
                    return false;
                }
                bool isDisabled = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                animGraphStateTransition.SetIsDisabled(isDisabled);
            }
            break;
            case 1:// ATTRIB_PRIORITY
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_PRIORITY");
                    return false;
                }
                int32 intValue = static_cast<int32>(floatValue);
                animGraphStateTransition.SetPriority(intValue);
            }
            break;
            case 2:// ATTRIB_CANBEINTERRUPTED
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_CANBEINTERRUPTED");
                    return false;
                }
                bool canBeInterrupted = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                animGraphStateTransition.SetCanBeInterrupted(canBeInterrupted);
            }
            break;
            case 3:// ATTRIB_CANINTERRUPTOTHERTRANSITIONS
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_CANINTERRUPTOTHERTRANSITIONS");
                    return false;
                }
                bool canInterruptOtherTransitions = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                animGraphStateTransition.SetCanInterruptOtherTransitions(canInterruptOtherTransitions);
            }
            break;
            case 4:// ATTRIB_ALLOWSELFINTERRUPTION
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_ALLOWSELFINTERRUPTION");
                    return false;
                }
                bool allowSelfInterruption = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                animGraphStateTransition.SetCanInterruptItself(allowSelfInterruption);
            }
            break;
            case 5:// ATTRIB_ALLOWEDSTATES
            {
                LegacyAttribute<LegacyStateFilterLocal> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse legacy attribute state filter");
                    return false;
                }

                animGraphStateTransition.SetGroups(legacyAttribute.GetValue().GetGroupNames());
                AZStd::vector<AnimGraphNodeId> nodeIDs;
                nodeIDs.resize(legacyAttribute.GetValue().GetNodeNames().size());
                int index = 0;
                for (int i = 0; i < legacyAttribute.GetValue().GetNodeNames().size(); i++)
                {
                    AnimGraphNode* node = importParams.mAnimGraph->RecursiveFindNodeByName(legacyAttribute.GetValue().GetNodeNames()[i].c_str());
                    if (!node)
                    {
                        AZ_Warning("EMotionFX", false, "Missing allowed wild card transition on node (%s): The state (%s) in the allowed source state list does not exist in the graph. Removing."
                            , animGraphStateTransition.GetTargetNode()->GetName(), legacyAttribute.GetValue().GetNodeNames()[i].c_str());
                    }
                    else
                    {
                        nodeIDs[index++] = node->GetId();
                    }
                }
                animGraphStateTransition.SetStateIds(nodeIDs);
            }
            break;
            case 6:// ATTRIB_BLENDTIME
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_BLENDTIME");
                    return false;
                }
                animGraphStateTransition.SetBlendTime(floatValue);
            }
            break;
            case 7:// ATTRIB_SYNC
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_SYNC");
                    return false;
                }
                AnimGraphStateTransition::ESyncMode eSyncMode = static_cast<AnimGraphObject::ESyncMode>(static_cast<uint32>(floatValue));
                animGraphStateTransition.SetSyncMode(eSyncMode);
            }
            break;
            case 8:// ATTRIB_EVENTMODE
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_EVENTMODE");
                    return false;
                }
                AnimGraphStateTransition::EEventMode eventMode = static_cast<AnimGraphObject::EEventMode>(static_cast<uint32>(floatValue));
                animGraphStateTransition.SetEventFilterMode(eventMode);
            }
            break;
            case 9:// ATTRIB_INTERPOLATIONTYPE
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_INTERPOLATIONTYPE");
                    return false;
                }
                AnimGraphStateTransition::EInterpolationType interpolationType = static_cast<AnimGraphStateTransition::EInterpolationType>(static_cast<uint32>(floatValue));
                animGraphStateTransition.SetInterpolationType(interpolationType);
            }
            break;
            case 10:// ATTRIB_EASEIN_SMOOTH
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_EASEIN_SMOOTH");
                    return false;
                }
                animGraphStateTransition.SetEaseInSmoothness(floatValue);
            }
            break;
            case 11:// ATTRIB_EASEOUT_SMOOTH
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "Unable to parse ATTRIB_EASEOUT_SMOOTH");
                    return false;
                }
                animGraphStateTransition.SetEaseOutSmoothness(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count parsing AnimGraphStateTransition");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateMachine>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    AnimGraphStateMachine& animGraphStateMachine = static_cast<AnimGraphStateMachine&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool alwaysStartInEntryState = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                animGraphStateMachine.SetAlwaysStartInEntryState(alwaysStartInEntryState);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeRangeRemapperNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeRangeRemapperNode& blendTreeRangeRemapperNode = static_cast<BlendTreeRangeRemapperNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeRangeRemapperNode.SetInputMin(floatValue);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeRangeRemapperNode.SetInputMax(floatValue);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeRangeRemapperNode.SetOutputMin(floatValue);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeRangeRemapperNode.SetOutputMax(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeSmoothingNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeSmoothingNode& blendTreeSmoothingNode = static_cast<BlendTreeSmoothingNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeSmoothingNode.SetInterpolationSpeed(floatValue);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool useStartValue = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeSmoothingNode.SetUseStartVAlue(useStartValue);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeSmoothingNode.SetStartVAlue(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3Math2Node>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeVector3Math2Node& blendTreeVector3Math2Node = static_cast<BlendTreeVector3Math2Node&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeVector3Math2Node::EMathFunction eMathFunction = static_cast<BlendTreeVector3Math2Node::EMathFunction>(static_cast<uint32>(floatValue));
                blendTreeVector3Math2Node.SetMathFunction(eMathFunction);
            }
            break;
            case 1:
            {
                LegacyAttribute<AZ::PackedVector3f> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeVector3Math2Node.SetDefaultValue(AZ::Vector3(legacyAttribute.GetValue()));
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3Math1Node>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeVector3Math1Node& blendTreeVector3Math1Node = static_cast<BlendTreeVector3Math1Node&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeVector3Math1Node::EMathFunction eMathFunction = static_cast<BlendTreeVector3Math1Node::EMathFunction>(static_cast<uint32>(floatValue));
                blendTreeVector3Math1Node.SetMathFunction(eMathFunction);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatMath2Node>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeFloatMath2Node& blendTreeFloatMath2Node = static_cast<BlendTreeFloatMath2Node&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeFloatMath2Node::EMathFunction eMathFunction = static_cast<BlendTreeFloatMath2Node::EMathFunction>(static_cast<uint32>(floatValue));
                blendTreeFloatMath2Node.SetMathFunction(eMathFunction);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatMath2Node.SetDefaultValue(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBlend2LegacyNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeBlend2LegacyNode& blendTreeBlend2Node = static_cast<BlendTreeBlend2LegacyNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphObject::ESyncMode syncMode = static_cast<AnimGraphObject::ESyncMode>(static_cast<uint32>(floatValue));
                blendTreeBlend2Node.SetSyncMode(syncMode);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphObject::EEventMode eventMode = static_cast<AnimGraphObject::EEventMode>(static_cast<uint32>(floatValue));
                blendTreeBlend2Node.SetEventMode(eventMode);
            }
            break;
            case 2:
            {
                LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>> legacyAttributeNodeMask;
                if (!legacyAttributeNodeMask.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeBlend2Node.SetWeightedNodeMask(legacyAttributeNodeMask.GetValue());
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool isAdditive = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeBlend2Node.SetAdditiveBlending(isAdditive);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;

            }
        }
    }
    return true;
}

bool LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(float value)
{
    return value > MCore::Math::epsilon;
}

bool LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(MCore::File* stream
    , MCore::Endian::EEndianType endianType
    , const LegacyAttributeHeader& attributeHeader
    , float& outputValue)
{
    bool result = true;
    switch (attributeHeader.GetAttributeType())
    {
    case EMotionFX::LegacyAttributeTypeId::ATTRIBUTE_BOOL_TYPE_ID:
    {
        LegacyAttribute<bool> legacyAttribute;
        if (!legacyAttribute.Parse(stream, endianType))
        {
            result = false;
        }
        else
        {
            outputValue = legacyAttribute.GetValue() ? 1.0f : 0.0f;
        }
    }
    break;
    case EMotionFX::LegacyAttributeTypeId::ATTRIBUTE_INT32_TYPE_ID:
    {
        LegacyAttribute<int32> legacyAttribute;
        if (!legacyAttribute.Parse(stream, endianType))
        {
            result = false;
        }
        else
        {
            outputValue = static_cast<float>(legacyAttribute.GetValue());
        }
    }
    break;
    case EMotionFX::LegacyAttributeTypeId::ATTRIBUTE_FLOAT_TYPE_ID:
    {
        LegacyAttribute<float> legacyAttribute;
        if (!legacyAttribute.Parse(stream, endianType))
        {
            result = false;
        }
        outputValue = legacyAttribute.GetValue();
    }
    break;
    default:
    {
        AZ_Error("EMotionFX", false, "Unexpected attribute type");
        result = false;
    }
    break;
    }
    if (!result)
    {
        AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::TryGetFloatFromAttribute Unable to parse attribute value");
    }
    return result;
}

bool LegacyAttributeHeader::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType, LegacyAttributeHeader& attributeHeader)
{
    AZStd::string name;
    name.resize(32);

    // Read the attribute size
    uint32 attribType;
    if (stream->Read(&attribType, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&attribType, endianType);

    // Read the attribute size
    uint32 attributeSize;
    if (stream->Read(&attributeSize, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&attributeSize, endianType);

    // First read the number of characters
    uint32 numCharacters;
    if (stream->Read(&numCharacters, sizeof(uint32)) != sizeof(uint32))
    {
        AZ_Error("EMotionFX", false, "Unable to read data in stream");
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);

    // Read the string
    if (numCharacters > 0)
    {
        name.resize(numCharacters);
        if (stream->Read(name.data(), numCharacters) != numCharacters)
        {
            AZ_Error("EMotionFX", false, "Unable to read data in stream");
            return false;
        }
    }

    attributeHeader.m_attribType = attribType;
    attributeHeader.m_attributeSize = attributeSize;
    attributeHeader.m_name = name;
    return true;
}

template<class T, template<class> class LegacyAttribute>
bool LegacyAttributeArray<T, LegacyAttribute>::PopulateAttributeDynamicArray(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Read the number of values
    uint32 numValues;
    if (stream->Read(&numValues, sizeof(uint32)) != sizeof(uint32))
    {
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numValues, endianType);

    for (uint32 i = 0; i < numValues; ++i)
    {
        LegacyAttribute<T> legacyAttribute;
        legacyAttribute.Parse(stream, endianType);
        m_attributes.push_back(legacyAttribute);
    }
    return true;
}

template<class T, template<class> class LegacyAttribute>
bool LegacyAttributeArray<T, LegacyAttribute>::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Write the version of the attribute settings format
    uint8 version;
    if (stream->Read(&version, sizeof(uint8)) != sizeof(uint8))
    {
        AZ_Error("EMotionFX", false, "LegacyAttributeArray::Parse - Failed to read the attribute version.");
        return false;
    }
    if (version == 2)
    {
        // Read the attribute type id
        uint32 attribType;
        if (stream->Read(&attribType, sizeof(uint32)) != sizeof(uint32))
        {
            AZ_Error("EMotionFX", false, "AttributeArray::ReadData() - Failed to read the attribute type ID.");
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&attribType, endianType);
        m_elementTypeId = attribType;

        if (!LegacyAttributeSettingsParser::Parse(stream, endianType))
        {
            return false;
        }
    }
    return PopulateAttributeDynamicArray(stream, endianType);
}

template<class T, template<class> class LegacyAttribute>
const AZStd::vector< LegacyAttribute<T> > & LegacyAttributeArray<T, LegacyAttribute>::GetValue() const
{
    return m_attributes;
}

bool LegacyAttributeSettingsParser::Parse(MCore::File* stream, MCore::Endian::EEndianType endianType)
{
    // Write the version of the attribute settings format
    uint8 version;
    if (stream->Read(&version, sizeof(uint8)) != sizeof(uint8))
    {
        AZ_Error("EMotionFX", false, "LegacyAttributeSettingsParser::Parse - Failed to read the attribute version.");
        return false;
    }

    if (version == 2)
    {
        // Read the flags (new in version 2)
        uint16 flags = 0;
        if (stream->Read(&flags, sizeof(uint16)) != sizeof(uint16))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt16(&flags, endianType);
    }

    // Read the internal name
    uint32 numChars;
    if (stream->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
    {
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);

    AZStd::string tempString;
    if (numChars > 0)
    {
        tempString.resize(numChars);
        if (stream->Read(tempString.data(), numChars) != numChars)
        {
            return false;
        }
    }

    // Read the name
    if (stream->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
    {
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
    tempString.clear();
    if (numChars > 0)
    {
        tempString.resize(numChars);
        if (stream->Read(tempString.data(), numChars) != numChars)
        {
            return false;
        }
    }

    // Read the description
    if (stream->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
    {
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
    tempString.clear();
    if (numChars > 0)
    {
        tempString.resize(numChars);
        if (stream->Read(tempString.data(), numChars) != numChars)
        {
            return false;
        }
    }

    // Read the interface type
    uint32 interfaceType;
    if (stream->Read(&interfaceType, sizeof(uint32)) != sizeof(uint32))
    {
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&interfaceType, endianType);

    // Read the number of combobox values
    uint32 numComboValues;
    if (stream->Read(&numComboValues, sizeof(uint32)) != sizeof(uint32))
    {
        return false;
    }
    MCore::Endian::ConvertUnsignedInt32(&numComboValues, endianType);

    // Read the combo strings
    for (uint32 i = 0; i < numComboValues; ++i)
    {
        tempString.clear();
        if (stream->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
        if (numChars > 0)
        {
            tempString.resize(numChars);
            if (stream->Read(tempString.data(), numChars) != numChars)
            {
                return false;
            }
        }
    }

    for (int skipCounter = 0; skipCounter < 3; ++skipCounter)
    {
        // Skipping attribute type (default value of settings)
        if (!LegacyAnimGraphNodeParser::Forward(stream, sizeof(uint32)))
        {
            return false;
        }

        // Read the attribute size
        uint32 attributeSize;
        if (stream->Read(&attributeSize, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&attributeSize, endianType);
        // Skipping attribute content (default value of settings)
        if (!LegacyAnimGraphNodeParser::Forward(stream, attributeSize))
        {
            return false;
        }
    }

    return true;
}

template class LegacyAttribute<LegacyStateFilterLocal>;
template class LegacyAttribute<int32>;
template class LegacyAttribute<bool>;
template class LegacyAttribute<float>;
template class LegacyAttribute<AZStd::string>;
template class LegacyAttribute<AZStd::vector<AZStd::string>>;
template class LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>>;
template class LegacyAttribute<AZ::PackedVector3f>;
template class LegacyAttribute<LegacyAttributeRotation>;
template class LegacyAttribute<BlendTreeTwoLinkIKNode::NodeAlignmentData>;
template class LegacyAttribute<AZ::Vector2>;
template class LegacyAttribute<BlendSpaceNode::BlendSpaceMotion>;
template class LegacyAttributeArray<AZStd::string, LegacyAttribute>;
template class LegacyAttributeArray<BlendSpaceNode::BlendSpaceMotion, LegacyAttribute>;


bool LegacyAnimGraphNodeParser::ParseTransitionConditionChunk(MCore::File* file,
    Importer::ImportParameters& importParams,
    const FileFormat::AnimGraph_NodeHeader& nodeHeader,
    AnimGraphTransitionCondition*& transitionCondition)
{
    const AZ::TypeId conditionType = GetNewTypeIdByOldNodeTypeId(nodeHeader.mTypeID);
    if (conditionType.IsNull())
    {
        AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser: Cannot convert legacy transition condition id '0x%x' to uuid.", nodeHeader.mTypeID);
        return false;
    }

    if (conditionType == azrtti_typeid<AnimGraphMotionCondition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphMotionCondition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphMotionCondition");
            return false;
        }
    }
    else if (conditionType == azrtti_typeid<AnimGraphParameterCondition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphParameterCondition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphParameterCondition");
            return false;
        }
        transitionCondition->SetAnimGraph(importParams.mAnimGraph);
    }
    else if (conditionType == azrtti_typeid<AnimGraphPlayTimeCondition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphPlayTimeCondition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphPlayTimeCondition");
            return false;
        }
    }
    else if (conditionType == azrtti_typeid<AnimGraphStateCondition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphStateCondition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphStateCondition");
            return false;
        }
    }
    else if (conditionType == azrtti_typeid<AnimGraphTagCondition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphTagCondition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphTagCondition");
            return false;
        }
    }
    else if (conditionType == azrtti_typeid<AnimGraphTimeCondition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphTimeCondition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphTimeCondition");
            return false;
        }
    }
    else if (conditionType == azrtti_typeid<AnimGraphVector2Condition>())
    {
        if (!ParseAnimGraphTransitionCondition<AnimGraphVector2Condition>(file, importParams, nodeHeader, transitionCondition))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphVector2Condition");
            return false;
        }
    }
    else
    {
        AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser: Cannot parse transition condition with uuid %s. No node parser available", conditionType.ToString<AZStd::string>().c_str());
        return false;
    }

    return true;
}


bool LegacyAnimGraphNodeParser::ParseAnimGraphNodeChunk(MCore::File* file,
    Importer::ImportParameters& importParams,
    const char* nodeName,
    FileFormat::AnimGraph_NodeHeader& nodeHeader,
    AnimGraphNode*& node)
{
    const AZ::TypeId nodeType = GetNewTypeIdByOldNodeTypeId(nodeHeader.mTypeID);
    if (nodeType.IsNull())
    {
        AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser: Cannot convert legacy node id '0x%x' to uuid.", nodeHeader.mTypeID);
        return false;
    }

    if (nodeType == azrtti_typeid<EMotionFX::AnimGraphBindPoseNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<AnimGraphBindPoseNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphBindPoseNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTree>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTree>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTree");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeFinalNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeFinalNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeFinalNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeMotionFrameNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeMotionFrameNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeMotionFrameNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeBlendNNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeBlendNNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeBlendNNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeFloatConditionNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeFloatConditionNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeFloatConditionNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeFloatSwitchNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeFloatSwitchNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeFloatSwitchNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeBoolLogicNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeBoolLogicNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeBoolLogicNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::AnimGraphMotionNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<AnimGraphMotionNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphMotionNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeAccumTransformNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeAccumTransformNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeAccumTransformNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeBlend2LegacyNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeBlend2LegacyNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeBlend2Node");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeFloatMath1Node>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeFloatMath1Node>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeFloatMath1Node");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeFloatMath2Node>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeFloatMath2Node>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeFloatMath2Node");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector3Math1Node>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector3Math1Node>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector3Math1Node");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector3Math2Node>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector3Math2Node>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector3Math2Node");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeSmoothingNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeSmoothingNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeSmoothingNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeRangeRemapperNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeRangeRemapperNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeRangeRemapperNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<AnimGraphStateMachine>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphStateMachine");
            return false;
        }
    
        // Only for this node we need to read legacy Custom Data.

        // Read the entry state child node index
        uint32 entryStateNodeIndex;
        if (file->Read(&entryStateNodeIndex, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }

        // Convert endian if needed
        MCore::Endian::ConvertUnsignedInt32(&entryStateNodeIndex, importParams.mEndianType);
        AZStd::map<AZ::u64, uint32>& entryNodeIndexToStateMachineIdLookupTable = SharedHelperData::GetEntryStateToStateMachineTable(importParams.mSharedData);
        if (!entryNodeIndexToStateMachineIdLookupTable.insert(AZStd::pair<AZ::u64, uint32>(node->GetId(), entryStateNodeIndex)).second)
        {
            AZ_Assert(false, "Same entry state id for different state machines found");
            AZ_Error("EMotionFX", false, "Same entry state id for different state machines found");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeTwoLinkIKNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeTwoLinkIKNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeTwoLinkIKNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeLookAtNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeLookAtNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeLookAtNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeTransformNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeTransformNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeTransformNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeMaskLegacyNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeMaskLegacyNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeMaskLegacyNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreePoseSwitchNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreePoseSwitchNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreePoseSwitchNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector2DecomposeNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector2DecomposeNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector2DecomposeNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector3DecomposeNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector3DecomposeNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector3DecomposeNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector4DecomposeNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector4DecomposeNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector4DecomposeNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector2ComposeNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector2ComposeNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector2ComposeNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector3ComposeNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector3ComposeNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector3ComposeNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeVector4ComposeNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeVector4ComposeNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeVector4ComposeNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeDirectionToWeightNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeDirectionToWeightNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeDirectionToWeightNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeMirrorPoseNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeMirrorPoseNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeMirrorPoseNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::AnimGraphEntryNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<AnimGraphEntryNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphEntryNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::AnimGraphExitNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<AnimGraphExitNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse AnimGraphExitNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeParameterNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeParameterNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendSpace1DNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendSpace1DNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendSpace1DNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendSpace2DNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendSpace2DNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendSpace2DNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeMorphTargetNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeMorphTargetNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeMorphTargetNode");
            return false;
        }
    }
    else if (nodeType == azrtti_typeid<EMotionFX::BlendTreeFloatConstantNode>())
    {
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNode<BlendTreeFloatConstantNode>(file, importParams, nodeName, nodeHeader, node))
        {
            AZ_Error("EMotionFX", false, "Unable to parse BlendTreeFloatConstantNode");
            return false;
        }
    }
    else
    {
        AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser: Cannot parse node with uuid %s. No node parser available", nodeType.ToString<AZStd::string>().c_str());
        return false;
    }

    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphVector2Condition>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    AnimGraphVector2Condition& animGraphVector2Condition = static_cast<AnimGraphVector2Condition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                animGraphVector2Condition.SetParameterName(legacyAttribute.GetValue());
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphVector2Condition::EOperation operation = static_cast<AnimGraphVector2Condition::EOperation>(static_cast<uint32>(floatValue));
                animGraphVector2Condition.SetOperation(operation);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphParameterCondition::EFunction function = static_cast<AnimGraphParameterCondition::EFunction>(static_cast<uint32>(floatValue));
                animGraphVector2Condition.SetFunction(function);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphVector2Condition.SetTestValue(floatValue);
            }
            break;
            case 4:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphVector2Condition.SetRangeValue(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphVector2Condition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}


template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphTimeCondition>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    AnimGraphTimeCondition& animGraphTimeCondition = static_cast<AnimGraphTimeCondition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphTimeCondition.SetCountDownTime(floatValue);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                const bool useRandomization = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                animGraphTimeCondition.SetUseRandomization(useRandomization);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphTimeCondition.SetMinRandomTime(floatValue);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphTimeCondition.SetMaxRandomTime(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphTimeCondition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphTagCondition>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    AnimGraphTagCondition& animGraphTagCondition = static_cast<AnimGraphTagCondition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphTagCondition::EFunction function = static_cast<AnimGraphTagCondition::EFunction>(static_cast<int32>(floatValue));
                animGraphTagCondition.SetFunction(function);
            }
            break;
            case 1:
            {
                LegacyAttributeArray<AZStd::string, LegacyAttribute> legacyAttributeArrayStrings;
                if (!legacyAttributeArrayStrings.Parse(stream, endianType))
                {
                    return false;
                }
                AZStd::vector<AZStd::string> tags;
                tags.resize(legacyAttributeArrayStrings.GetValue().size());
                for (int i = 0; i < legacyAttributeArrayStrings.GetValue().size(); i++)
                {
                    tags[i] = legacyAttributeArrayStrings.GetValue()[i].GetValue();
                }
                animGraphTagCondition.SetTags(tags);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphTagCondition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateCondition>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AnimGraphStateCondition& animGraphStateCondition = static_cast<AnimGraphStateCondition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AnimGraphNode* node = importParams.mAnimGraph->RecursiveFindNodeByName(legacyAttribute.GetValue().c_str());
                if (node)
                {
                    animGraphStateCondition.SetStateId(node->GetId());
                }
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphStateCondition::TestFunction testFunction = static_cast<AnimGraphStateCondition::TestFunction>(static_cast<int32>(floatValue));
                animGraphStateCondition.SetTestFunction(testFunction);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphStateCondition.SetPlayTime(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateCondition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}



template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphPlayTimeCondition>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AnimGraphPlayTimeCondition& animGraphPlayTimeCondition = static_cast<AnimGraphPlayTimeCondition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AnimGraphNode* node = importParams.mAnimGraph->RecursiveFindNodeByName(legacyAttribute.GetValue().c_str());
                if (node)
                {
                    animGraphPlayTimeCondition.SetNodeId(node->GetId());
                }
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphPlayTimeCondition.SetPlayTime(floatValue);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphPlayTimeCondition::Mode mode = static_cast<AnimGraphPlayTimeCondition::Mode>(static_cast<int32>(floatValue));
                animGraphPlayTimeCondition.SetMode(mode);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphPlayTimeCondition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphParameterCondition>(MCore::File* stream,
    uint32 numAttributes,
    MCore::Endian::EEndianType endianType,
    Importer::ImportParameters& importParams,
    AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    AnimGraphParameterCondition& animGraphParameterCondition = static_cast<AnimGraphParameterCondition&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                animGraphParameterCondition.SetParameterName(legacyAttribute.GetValue());
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphParameterCondition.SetTestValue(floatValue);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphParameterCondition.SetRangeValue(floatValue);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphParameterCondition::EFunction function = static_cast<AnimGraphParameterCondition::EFunction>(static_cast<uint32>(floatValue));
                animGraphParameterCondition.SetFunction(function);
            }
            break;
            case 4:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                animGraphParameterCondition.SetTestString(legacyAttribute.GetValue());
            }
            break;
            case 5:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphParameterCondition::EStringFunction stringFunction = static_cast<AnimGraphParameterCondition::EStringFunction>(static_cast<uint32>(floatValue));
                animGraphParameterCondition.SetStringFunction(stringFunction);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphParameterCondition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}


template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphMotionCondition>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AnimGraphMotionCondition& animGraphMotionCondition = static_cast<AnimGraphMotionCondition&>(animGraphObject);

    AZStd::string eventType;
    AZStd::string eventParameter;

    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0: // ATTRIB_MOTIONNODE
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AnimGraphNode* motionNode = importParams.mAnimGraph->RecursiveFindNodeByName(legacyAttribute.GetValue().c_str());
                if (motionNode)
                {
                    animGraphMotionCondition.SetMotionNodeId(motionNode->GetId());
                }
            }
            break;
            case 1: // ATTRIB_FUNCTION
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphMotionCondition::TestFunction testFunction = static_cast<AnimGraphMotionCondition::TestFunction>(static_cast<int32>(floatValue));
                animGraphMotionCondition.SetTestFunction(testFunction);
            }
            break;
            case 2: // ATTRIB_NUMLOOPS
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AZ::u32 numLoops = static_cast<AZ::u32>(floatValue);
                animGraphMotionCondition.SetNumLoops(numLoops);
            }
            break;
            case 3: // ATTRIB_PLAYTIME
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                animGraphMotionCondition.SetPlayTime(floatValue);
            }
            break;
            case 4: // ATTRIB_EVENTTYPE
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                eventType = legacyAttribute.GetValue();
            }
            break;
            case 5: // ATTRIB_EVENTPARAMETER
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                eventParameter = legacyAttribute.GetValue();
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphMotionCondition>. Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }

    AZStd::shared_ptr<const EventData> eventData = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>(eventType, eventParameter);
    animGraphMotionCondition.SetEventDatas({eventData});

    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBlendNNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeBlendNNode& blendTreeBlendNNode = static_cast<BlendTreeBlendNNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                const AnimGraphObject::ESyncMode syncMode = static_cast<AnimGraphObject::ESyncMode>(static_cast<uint32>(floatValue));
                blendTreeBlendNNode.SetSyncMode(syncMode);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                const AnimGraphObject::EEventMode eventMode = static_cast<AnimGraphObject::EEventMode>(static_cast<uint32>(floatValue));
                blendTreeBlendNNode.SetEventMode(eventMode);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBlendNNode>: unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMaskLegacyNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeMaskLegacyNode& blendTreeMaskNode = static_cast<BlendTreeMaskLegacyNode&>(animGraphObject);
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }
        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AZStd::vector<AZStd::string> maskStrings(legacyAttribute.GetValue().size());
                for(int i = 0; i < legacyAttribute.GetValue().size(); i++)
                {
                    maskStrings[i] = legacyAttribute.GetValue()[i].first;
                }
                blendTreeMaskNode.SetMask0(maskStrings);
            }
            break;
            case 1:
            {
                LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AZStd::vector<AZStd::string> maskStrings(legacyAttribute.GetValue().size());
                for (int i = 0; i < legacyAttribute.GetValue().size(); i++)
                {
                    maskStrings[i] = legacyAttribute.GetValue()[i].first;
                }
                blendTreeMaskNode.SetMask1(maskStrings);
            }
            break;
            case 2:
            {
                LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AZStd::vector<AZStd::string> maskStrings(legacyAttribute.GetValue().size());
                for (int i = 0; i < legacyAttribute.GetValue().size(); i++)
                {
                    maskStrings[i] = legacyAttribute.GetValue()[i].first;
                }
                blendTreeMaskNode.SetMask2(maskStrings);
            }
            break;
            case 3:
            {
                LegacyAttribute<AZStd::vector<BlendTreeBlend2LegacyNode::WeightedMaskEntry>> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                AZStd::vector<AZStd::string> maskStrings(legacyAttribute.GetValue().size());
                for (int i = 0; i < legacyAttribute.GetValue().size(); i++)
                {
                    maskStrings[i] = legacyAttribute.GetValue()[i].first;
                }
                blendTreeMaskNode.SetMask3(maskStrings);
            }
            break;
            case 4:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool eventFlag = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeMaskNode.SetOutputEvents0(eventFlag);
            }
            break;
            case 5:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool eventFlag = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeMaskNode.SetOutputEvents1(eventFlag);
            }
            break;
            case 6:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool eventFlag = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeMaskNode.SetOutputEvents2(eventFlag);
            }
            break;
            case 7:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool eventFlag = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeMaskNode.SetOutputEvents3(eventFlag);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count parsing BlendTreeTransformNode");
                return false;
            }
            break;
            }
        }
    }
    return true;
}


template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeTransformNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeTransformNode& blendTreeTransformNode = static_cast<BlendTreeTransformNode&>(animGraphObject);
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }
        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetTargetNodeName(legacyAttribute.GetValue());
            }
            break;
            case 1:
            {
                LegacyAttribute<AZ::PackedVector3f> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetMinTranslation(AZ::Vector3(legacyAttribute.GetValue()));
            }
            break;
            case 2:
            {
                LegacyAttribute<AZ::PackedVector3f> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetMaxTranslation(AZ::Vector3(legacyAttribute.GetValue()));
            }
            break;
            case 3:
            {
                LegacyAttribute<LegacyAttributeRotation> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetMinRotation(legacyAttribute.GetValue().GetDegrees());
            }
            break;
            case 4:
            {
                LegacyAttribute<LegacyAttributeRotation> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetMaxRotation(legacyAttribute.GetValue().GetDegrees());
            }
            break;
            case 5:
            {
                LegacyAttribute<AZ::PackedVector3f> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetMinScale(AZ::Vector3(legacyAttribute.GetValue()));
            }
            break;
            case 6:
            {
                LegacyAttribute<AZ::PackedVector3f> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeTransformNode.SetMaxScale(AZ::Vector3(legacyAttribute.GetValue()));
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count parsing BlendTreeTransformNode");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeAccumTransformNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeAccumTransformNode& blendTreeAccumTransformNode = static_cast<BlendTreeAccumTransformNode&>(animGraphObject);
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }
        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendTreeAccumTransformNode.SetTargetNodeName(legacyAttribute.GetValue());
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeAccumTransformNode::Axis axis = static_cast<BlendTreeAccumTransformNode::Axis>(static_cast<int32>(floatValue));
                blendTreeAccumTransformNode.SetTranslationAxis(axis);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeAccumTransformNode::Axis axis = static_cast<BlendTreeAccumTransformNode::Axis>(static_cast<int32>(floatValue));
                blendTreeAccumTransformNode.SetRotationAxis(axis);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeAccumTransformNode::ScaleAxis axis = static_cast<BlendTreeAccumTransformNode::ScaleAxis>(static_cast<int32>(floatValue));
                blendTreeAccumTransformNode.SetScaleAxis(axis);
            }
            break;
            case 4:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeAccumTransformNode.SetTranslateSpeed(floatValue);
            }
            break;
            case 5:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeAccumTransformNode.SetRotateSpeed(floatValue);
            }
            break;
            case 6:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeAccumTransformNode.SetScaleSpeed(floatValue);
            }
            break;
            case 7:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool translateInvert = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeAccumTransformNode.SetInvertTranslation(translateInvert);
            }
            break;
            case 8:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool rotateInvert = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeAccumTransformNode.SetInvertRotation(rotateInvert);
            }
            break;
            case 9:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                bool scaleInvert = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(floatValue);
                blendTreeAccumTransformNode.SetInvertScale(scaleInvert);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeAccumTransformNode>: unexpected attributes count");
                return false;
            }
            break;
            }
        }
    }

    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphMotionNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    AnimGraphMotionNode& animGraphMotionNode = static_cast<AnimGraphMotionNode&>(animGraphObject);
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }
        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttributeArray<AZStd::string, LegacyAttribute> legacyAttributeArrayStrings;
                if (!legacyAttributeArrayStrings.Parse(stream, endianType))
                {
                    return false;
                }
                const AZStd::vector<LegacyAttribute<AZStd::string> > & stringAttributes = legacyAttributeArrayStrings.GetValue();
                AZStd::vector<AZStd::string> stringValues;
                for (LegacyAttribute<AZStd::string> attrStr : stringAttributes)
                {
                    stringValues.push_back(attrStr.GetValue());
                }
                animGraphMotionNode.SetMotionIds(stringValues);
            }
            break;
            case 1:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool loop = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetLoop(loop);
            }
            break;
            case 2:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool retarget = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetRetarget(retarget);
            }
            break;
            case 3:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool reverse = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetReverse(reverse);
            }
            break;
            case 4:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool emitEvents = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetEmitEvents(emitEvents);
            }
            break;
            case 5:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool mirror = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetMirrorMotion(mirror);
            }
            break;
            case 6:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool motionExtraction = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetMotionExtraction(motionExtraction);
            }
            break;
            case 7:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                animGraphMotionNode.SetMotionPlaySpeed(legacyAttributeFloat.GetValue());
            }
            break;
            case 8:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                AnimGraphMotionNode::EIndexMode eIndexMode = static_cast<AnimGraphMotionNode::EIndexMode>(static_cast<uint32>(legacyAttributeFloat.GetValue()));
                animGraphMotionNode.SetIndexMode(eIndexMode);
            }
            break;
            case 9:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool nextMotionAfterLoop = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetNextMotionAfterLoop(nextMotionAfterLoop);
            }
            break;
            case 10:
            {
                LegacyAttribute<float> legacyAttributeFloat;
                if (!legacyAttributeFloat.Parse(stream, endianType))
                {
                    return false;
                }
                bool nextMotionAfterLoop = LegacyAnimGraphNodeParser::ConvertFloatAttributeValueToBool(legacyAttributeFloat.GetValue());
                animGraphMotionNode.SetNextMotionAfterLoop(nextMotionAfterLoop);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphMotionNode>: unexpected attributes count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector4DecomposeNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3DecomposeNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector2DecomposeNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTree>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreePoseSwitchNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendSpace2DNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendSpace2DNode& blendSpace2DNode = static_cast<BlendSpace2DNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "BlendSpace2DNode: Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0: // ATTRIB_CALCULATION_METHOD_X
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return true;
                }
                BlendSpaceNode::ECalculationMethod calculationMethod = static_cast<BlendSpaceNode::ECalculationMethod>(static_cast<int>(floatValue));
                blendSpace2DNode.SetCalculationMethodX(calculationMethod);
            }
            break;
            case 1: // ATTRIB_EVALUATOR_X
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return true;
                }
                uint32 evaluatorIndex = static_cast<uint32>(floatValue);
                AZ::TypeId evaluatorId;
                if (!GetBlendSpaceNodeEvaluatorTypeId(evaluatorIndex, evaluatorId))
                {
                    return false;
                }
                blendSpace2DNode.SetEvaluatorTypeX(evaluatorId);
            }
            break;
            case 2: // ATTRIB_CALCULATION_METHOD_Y
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return true;
                }
                BlendSpaceNode::ECalculationMethod calculationMethod = static_cast<BlendSpaceNode::ECalculationMethod>(static_cast<int>(floatValue));
                blendSpace2DNode.SetCalculationMethodY(calculationMethod);
            }
            break;
            case 3: // ATTRIB_EVALUATOR_Y
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return true;
                }
                uint32 evaluatorIndex = static_cast<uint32>(floatValue);
                AZ::TypeId evaluatorId;
                if (!GetBlendSpaceNodeEvaluatorTypeId(evaluatorIndex, evaluatorId))
                {
                    return false;
                }
                blendSpace2DNode.SetEvaluatorTypeY(evaluatorId);
            }
            break;
            case 4: // ATTRIB_SYNC
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return true;
                }
                AnimGraphObject::ESyncMode syncMode = static_cast<AnimGraphObject::ESyncMode>(static_cast<uint32>(floatValue));
                blendSpace2DNode.SetSyncMode(syncMode);
            }
            break;
            case 5: //ATTRIB_SYNC_LEADERMOTION
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendSpace2DNode.SetSyncLeaderMotionId(legacyAttribute.GetValue());
            }
            break;
            case 6: //ATTRIB_EVENTMODE
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return true;
                }
                BlendSpaceNode::EBlendSpaceEventMode eventMode = static_cast<BlendSpaceNode::EBlendSpaceEventMode>(static_cast<uint32>(floatValue));
                blendSpace2DNode.SetEventFilterMode(eventMode);
            }
            break;
            case 7: // ATTRIB_MOTIONS
            {
                // Parsing an array of string attributes
                LegacyAttributeArray<BlendSpaceNode::BlendSpaceMotion, LegacyAttribute> legacyAttributeArray;
                if (!legacyAttributeArray.Parse(stream, endianType))
                {
                    return false;
                }
                // Get the attribute array and process it
                AZStd::vector<BlendSpaceNode::BlendSpaceMotion> blendSpaceMotions;
                blendSpaceMotions.resize(legacyAttributeArray.GetValue().size());
                int newVectorIndex = 0;
                for (LegacyAttribute<BlendSpaceNode::BlendSpaceMotion> legacyAttribute : legacyAttributeArray.GetValue())
                {
                    blendSpaceMotions[newVectorIndex++] = legacyAttribute.GetValue();
                }
                blendSpace2DNode.SetMotions(blendSpaceMotions);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendSpace2DNode>: Unexpected number of attributes");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendSpace1DNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendSpace1DNode& blendSpace1DNode = static_cast<BlendSpace1DNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "BlendSpace1DNode: Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "BlendSpace1DNode: Unable to parse calculation method");
                    return false;
                }
                BlendSpaceNode::ECalculationMethod eCalculationMethod = static_cast<BlendSpaceNode::ECalculationMethod>(static_cast<uint32>(floatValue));
                blendSpace1DNode.SetCalculationMethod(eCalculationMethod);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    AZ_Error("EMotionFX", false, "BlendSpace1DNode: Unable to parse evaluator");
                    return false;
                }
                uint32 evaluatorIndex = static_cast<uint32>(floatValue);
                AZ::TypeId evaluatorUUid;
                if (!GetBlendSpaceNodeEvaluatorTypeId(evaluatorIndex, evaluatorUUid))
                {
                    return false;
                }
                blendSpace1DNode.SetEvaluatorType(evaluatorUUid);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                AnimGraphNode::ESyncMode eSyncMmode = static_cast<AnimGraphNode::ESyncMode>(static_cast<uint32>(floatValue));
                blendSpace1DNode.SetSyncMode(eSyncMmode);
            }
            break;
            case 3:
            {
                LegacyAttribute<AZStd::string> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    return false;
                }
                blendSpace1DNode.SetSyncLeaderMotionId(legacyAttribute.GetValue());
            }
            break;
            case 4:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendSpaceNode::EBlendSpaceEventMode eventMode = static_cast<BlendSpaceNode::EBlendSpaceEventMode>(static_cast<uint32>(floatValue));
                blendSpace1DNode.SetEventFilterMode(eventMode);
            }
            break;
            case 5:
            {
                // Parsing an array of string attributes
                LegacyAttributeArray<BlendSpaceNode::BlendSpaceMotion, LegacyAttribute> legacyAttributeArray;
                if (!legacyAttributeArray.Parse(stream, endianType))
                {
                    return false;
                }
                // Get the attribute array and process it
                AZStd::vector<BlendSpaceNode::BlendSpaceMotion> blendSpaceMotions;
                blendSpaceMotions.resize(legacyAttributeArray.GetValue().size());
                int newVectorIndex = 0;
                for (LegacyAttribute<BlendSpaceNode::BlendSpaceMotion> legacyAttribute : legacyAttributeArray.GetValue())
                {
                    blendSpaceMotions[newVectorIndex++] = legacyAttribute.GetValue();
                }
                blendSpace1DNode.SetMotions(blendSpaceMotions);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendSpace1DNode>: Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeParameterNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    BlendTreeParameterNode& blendTreeParameterNode = static_cast<BlendTreeParameterNode&>(animGraphObject);

    // ****************************************************************************************
    // Please Note: this is a fix to enable a safe destruction of the node if the parser fails
    // ****************************************************************************************
    blendTreeParameterNode.SetAnimGraph(importParams.mAnimGraph);
    // ****************************************************************************************

    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            // ****************************************************************************************
            // Please Note: this is a fix to enable a safe destruction of the node if the parser fails
            // ****************************************************************************************
            blendTreeParameterNode.SetAnimGraph(nullptr);
            // ****************************************************************************************
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                LegacyAttribute<AZStd::vector<AZStd::string>> legacyAttribute;
                if (!legacyAttribute.Parse(stream, endianType))
                {
                    // ****************************************************************************************
                    // Please Note: this is a fix to enable a safe destruction of the node if the parser fails
                    // ****************************************************************************************
                    blendTreeParameterNode.SetAnimGraph(nullptr);
                    // ****************************************************************************************

                    return false;
                }
                const AZStd::vector<AZStd::string>& parameterNames = legacyAttribute.GetValue();
                blendTreeParameterNode.SetParameters(parameterNames);
            }
            break;
            default:
            {
                // ****************************************************************************************
                // Please Note: this is a fix to enable a safe destruction of the node if the parser fails
                // ****************************************************************************************
                blendTreeParameterNode.SetAnimGraph(nullptr);
                // ****************************************************************************************

                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeParameterNode>: Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }

    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphExitNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphEntryNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMirrorPoseNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeDirectionToWeightNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphBindPoseNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFinalNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(stream);
    AZ_UNUSED(numAttributes);
    AZ_UNUSED(endianType);
    AZ_UNUSED(importParams);
    AZ_UNUSED(animGraphObject);
    // Do nothing
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMotionFrameNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeMotionFrameNode& blendTreeFloatConditionNode = static_cast<BlendTreeMotionFrameNode&>(animGraphObject);
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            // This node needs only one attribute, if any more are found inthe file they will be skipped
            if (parsedAttributeCount == 0)
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatConditionNode.SetNormalizedTimeValue(floatValue);
            }
            else
            {
                AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConditionNode> : Unexpected attribute count");
                return false;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConditionNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeFloatConditionNode& blendTreeFloatConditionNode = static_cast<BlendTreeFloatConditionNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
                case 0:
                {
                    float floatValue;
                    if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                    {
                        return false;
                    }

                    BlendTreeFloatConditionNode::EFunction eFunction = (BlendTreeFloatConditionNode::EFunction)((uint32)floatValue);
                    blendTreeFloatConditionNode.SetFunction(eFunction);
                    break;
                }

                case 1:
                {
                    if (legacyAttributeHeader.GetAttributeType() == LegacyAttributeTypeId::ATTRIBUTE_FLOAT_TYPE_ID)
                    {
                        float floatValue;
                        if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                        {
                            return false;
                        }

                        blendTreeFloatConditionNode.SetDefaultValue(floatValue);
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Unexpected Attribute type in legacy data. Float expected found %u instead", legacyAttributeHeader.GetAttributeType());
                        return false;
                    }
                    break;
                }

                case 2:
                {
                    if (legacyAttributeHeader.GetAttributeType() == LegacyAttributeTypeId::ATTRIBUTE_FLOAT_TYPE_ID)
                    {
                        float floatValue;
                        if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                        {
                            return false;
                        }
                        blendTreeFloatConditionNode.SetTrueResult(floatValue);
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Unexpected Attribute type in legacy data. Float expected found %u instead", legacyAttributeHeader.GetAttributeType());
                        return false;
                    }
                    break;
                }

                case 3:
                {
                    if (legacyAttributeHeader.GetAttributeType() == LegacyAttributeTypeId::ATTRIBUTE_FLOAT_TYPE_ID)
                    {
                        float floatValue;
                        if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                        {
                            return false;
                        }
                        blendTreeFloatConditionNode.SetFalseResult(floatValue);
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Unexpected Attribute type in legacy data. Float expected found %u instead", legacyAttributeHeader.GetAttributeType());
                        return false;
                    }
                    break;
                }

                case 4:
                {
                    if (legacyAttributeHeader.GetAttributeType() == LegacyAttributeTypeId::ATTRIBUTE_FLOAT_TYPE_ID)
                    {
                        float floatValue;
                        if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                        {
                            return false;
                        }
                        BlendTreeFloatConditionNode::EReturnMode returnMode = static_cast<BlendTreeFloatConditionNode::EReturnMode>(static_cast<uint32>(floatValue));
                        blendTreeFloatConditionNode.SetFalseReturnMode(returnMode);
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Unexpected Attribute type in legacy data. Float expected found %u instead", legacyAttributeHeader.GetAttributeType());
                        return false;
                    }
                    break;
                }

                case 5:
                {
                    if (legacyAttributeHeader.GetAttributeType() == LegacyAttributeTypeId::ATTRIBUTE_FLOAT_TYPE_ID)
                    {
                        float floatValue;
                        if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                        {
                            return false;
                        }
                        BlendTreeFloatConditionNode::EReturnMode returnMode = static_cast<BlendTreeFloatConditionNode::EReturnMode>(static_cast<uint32>(floatValue));
                        blendTreeFloatConditionNode.SetTrueReturnMode(returnMode);
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Unexpected Attribute type in legacy data. Float expected found %u instead", legacyAttributeHeader.GetAttributeType());
                        return false;
                    }
                    break;
                }

                default:
                {
                    AZ_Error("EMotionFX", false, "LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConditionNode>: Unexpected State in Legacy Node parser");
                    return false;
                }
            }
        }
    }

    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatSwitchNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeFloatSwitchNode& blendTreeFloatSwitchNode = static_cast<BlendTreeFloatSwitchNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatSwitchNode.SetValue0(floatValue);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatSwitchNode.SetValue1(floatValue);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatSwitchNode.SetValue2(floatValue);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatSwitchNode.SetValue3(floatValue);
            }
            break;
            case 4:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeFloatSwitchNode.SetValue4(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

template<>
bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBoolLogicNode>(MCore::File* stream,
     uint32 numAttributes,
     MCore::Endian::EEndianType endianType,
     Importer::ImportParameters& importParams,
     AnimGraphObject& animGraphObject)
{
    AZ_UNUSED(importParams);
    BlendTreeBoolLogicNode& blendTreeBoolLogicNode = static_cast<BlendTreeBoolLogicNode&>(animGraphObject);
    // For all attributes
    for (uint32 parsedAttributeCount = 0; parsedAttributeCount < numAttributes; ++parsedAttributeCount)
    {
        LegacyAttributeHeader legacyAttributeHeader;
        if (!LegacyAttributeHeader::Parse(stream, endianType, legacyAttributeHeader))
        {
            AZ_Error("EMotionFX", false, "Unable to parse attribute header");
            return false;
        }

        if (legacyAttributeHeader.GetAttributeSize() > 0)
        {
            switch (parsedAttributeCount)
            {
            case 0:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                BlendTreeBoolLogicNode::EFunction function = static_cast<BlendTreeBoolLogicNode::EFunction>(static_cast<int32>(floatValue));
                blendTreeBoolLogicNode.SetFunction(function);
            }
            break;
            case 1:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeBoolLogicNode.SetDefaultValue(floatValue > MCore::Math::epsilon);
            }
            break;
            case 2:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeBoolLogicNode.SetTrueResult(floatValue);
            }
            break;
            case 3:
            {
                float floatValue;
                if (!LegacyAnimGraphNodeParser::TryGetFloatFromAttribute(stream, endianType, legacyAttributeHeader, floatValue))
                {
                    return false;
                }
                blendTreeBoolLogicNode.SetFalseResult(floatValue);
            }
            break;
            default:
            {
                AZ_Error("EMotionFX", false, "Unexpected attribute count");
                return false;
            }
            break;
            }
        }
    }
    return true;
}

}
