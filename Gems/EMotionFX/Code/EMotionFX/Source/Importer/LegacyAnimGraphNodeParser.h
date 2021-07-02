/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>
#include <MCore/Source/Endian.h>
#include <MCore/Source/LogManager.h>
#include "Importer.h"
#include "AnimGraphFileFormat.h"
#include "../AnimGraphObject.h"
#include <MCore/Source/File.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include "../AnimGraphNode.h"


namespace EMotionFX
{
    class AnimGraphNode;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphTransitionCondition;
    class BlendTreeVector4ComposeNode;
    class BlendTreeVector3ComposeNode;
    class BlendTreeVector2ComposeNode;
    class BlendTreeMorphTargetNode;
    class BlendTreeFloatConstantNode;
    class BlendTreeLookAtNode;
    class BlendTreeTwoLinkIKNode;
    class BlendTreeFloatMath1Node;
    class AnimGraphStateTransition;
    class AnimGraphStateMachine;
    class BlendTreeRangeRemapperNode;
    class BlendTreeSmoothingNode;
    class BlendTreeVector3Math2Node;
    class BlendTreeVector3Math1Node;
    class BlendTreeFloatMath2Node;
    class BlendTreeBlend2LegacyNode;
    class AnimGraphVector2Condition;
    class AnimGraphTimeCondition;
    class AnimGraphTagCondition;
    class AnimGraphStateCondition;
    class AnimGraphPlayTimeCondition;
    class AnimGraphParameterCondition;
    class AnimGraphMotionCondition;
    class BlendTreeBlendNNode;
    class BlendTreeMaskLegacyNode;
    class BlendTreeTransformNode;
    class BlendTreeAccumTransformNode;
    class AnimGraphMotionNode;
    class BlendTreeVector4DecomposeNode;
    class BlendTreeVector3DecomposeNode;
    class BlendTreeVector2DecomposeNode;
    class BlendTree;
    class BlendTreePoseSwitchNode;
    class BlendSpace2DNode;
    class BlendSpace1DNode;
    class BlendTreeParameterNode;
    class AnimGraphExitNode;
    class AnimGraphEntryNode;
    class BlendTreeMirrorPoseNode;
    class BlendTreeDirectionToWeightNode;
    class AnimGraphBindPoseNode;
    class BlendTreeFinalNode;
    class AnimGraphNode;
    class BlendTreeMotionFrameNode;
    class BlendTreeFloatConditionNode;
    class BlendTreeFloatSwitchNode;
    class BlendTreeBoolLogicNode;

    const AZ::TypeId GetNewTypeIdByOldNodeTypeId(uint32 oldNodeTypeId);

    enum LegacyAttributeTypeId
    {
        ATTRIBUTE_FLOAT_TYPE_ID = 0x00000001,
        ATTRIBUTE_INT32_TYPE_ID = 0x00000002,
        ATTRIBUTE_BOOL_TYPE_ID = 0x00000004
    };

    enum LegacyERotationOrder
    {
        ROTATIONORDER_ZYX = 0,
        ROTATIONORDER_ZXY = 1,
        ROTATIONORDER_YZX = 2,
        ROTATIONORDER_YXZ = 3,
        ROTATIONORDER_XYZ = 4,
        ROTATIONORDER_XZY = 5
    };


    class LegacyAttributeHeader
    {
    public:
        LegacyAttributeHeader() { }

        AZ::u32 GetAttributeType() const
        {
            return m_attribType;
        }

        AZ::u32 GetAttributeSize() const
        {
            return m_attributeSize;
        }

        static bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType, LegacyAttributeHeader& attributeHeader);
    private:
        LegacyAttributeHeader(const LegacyAttributeHeader& src) :
            m_attribType(src.m_attribType)
            , m_attributeSize(src.m_attributeSize)
            , m_name(src.m_name)
        { }

        AZ::u32 m_attribType;
        AZ::u32 m_attributeSize;
        AZStd::string m_name;
    };

    template <class T>
    class LegacyAttribute
    {
    public:
        bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType);
        const T& GetValue() const;
    private:
        T m_value;
    };

    class LegacyAnimGraphNodeParser
    {
    public:
        static bool ParseAnimGraphNodeChunk(MCore::File* file,
            Importer::ImportParameters& importParams,
            const char* nodeName,
            FileFormat::AnimGraph_NodeHeader& nodeHeader,
            AnimGraphNode*& node);

        static bool ParseTransitionConditionChunk(MCore::File* file,
            Importer::ImportParameters& importParams,
            const FileFormat::AnimGraph_NodeHeader& nodeHeader,
            AnimGraphTransitionCondition*& transitionCondition);

        template <class T>
        static bool ParseLegacyAttributes(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);

        static bool Forward(MCore::File* stream, size_t numBytes);

    private:
        static bool InitializeNodeGeneralData(const char* nodeName, Importer::ImportParameters& importParams, FileFormat::AnimGraph_NodeHeader& nodeHeader, AnimGraphNode* node);
        static bool GetBlendSpaceNodeEvaluatorTypeId(uint32 legacyIndex, AZ::TypeId& value);
        static bool ConvertFloatAttributeValueToBool(float value);
        static bool TryGetFloatFromAttribute(MCore::File* stream, MCore::Endian::EEndianType endianType, const LegacyAttributeHeader& attributeHeader, float& outputValue);

        template <class T>
        static bool ParseAnimGraphNode(MCore::File* file,
            Importer::ImportParameters& importParams,
            const char* nodeName,
            FileFormat::AnimGraph_NodeHeader& nodeHeader,
            AnimGraphNode*& node)
        {
            node = aznew T();
            node->SetAnimGraph(importParams.mAnimGraph);
            if (!InitializeNodeGeneralData(nodeName, importParams, nodeHeader, node))
            {
                MCore::LogError("Error on initializing node general data");
                return false;
            }
            if (!ParseLegacyAttributes<T>(file, nodeHeader.mNumAttributes, importParams.mEndianType, importParams, *node))
            {
                MCore::LogError("Unable to parse node legacy attributes");
                return false;
            }
            return true;
        }

        template <class T>
        static bool ParseAnimGraphTransitionCondition(MCore::File* file,
            Importer::ImportParameters& importParams,
            const FileFormat::AnimGraph_NodeHeader& header,
            AnimGraphTransitionCondition*& transitionCondition)
        {
            transitionCondition = aznew T();
            return ParseLegacyAttributes<T>(file, header.mNumAttributes, importParams.mEndianType, importParams, *transitionCondition);
        }
    };

    template<class T, template<class> class LegacyAttribute>
    class LegacyAttributeArray
    {
    public:
        bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType);
        const AZStd::vector< LegacyAttribute<T> >& GetValue() const;
    private:
        bool PopulateAttributeDynamicArray(MCore::File* stream, MCore::Endian::EEndianType endianType);

        AZStd::vector< LegacyAttribute<T> > m_attributes;
        // Used when reading version 2 attribute array
        uint32 m_elementTypeId;
    };

    class LegacyAttributeSettingsParser
    {
    public:
        static bool Parse(MCore::File* stream, MCore::Endian::EEndianType endianType);
    };

    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector4ComposeNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3ComposeNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector2ComposeNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMorphTargetNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConstantNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeLookAtNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeTwoLinkIKNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatMath1Node>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateTransition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateMachine>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeRangeRemapperNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeSmoothingNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3Math2Node>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3Math1Node>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatMath2Node>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBlend2LegacyNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphVector2Condition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphTimeCondition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphTagCondition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateCondition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphPlayTimeCondition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphParameterCondition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphMotionCondition>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBlendNNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMaskLegacyNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeTransformNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeAccumTransformNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphMotionNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector4DecomposeNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector3DecomposeNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeVector2DecomposeNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTree>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreePoseSwitchNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendSpace2DNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendSpace1DNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeParameterNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphExitNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphEntryNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMirrorPoseNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeDirectionToWeightNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphBindPoseNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFinalNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeMotionFrameNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatConditionNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeFloatSwitchNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
    template<> bool LegacyAnimGraphNodeParser::ParseLegacyAttributes<BlendTreeBoolLogicNode>(MCore::File* stream, uint32 numAttributes, MCore::Endian::EEndianType endianType, Importer::ImportParameters& importParams, AnimGraphObject& animGraphObject);
} // Namespace EMotionFX
