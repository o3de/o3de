/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeTwoLinkIKNode
        : public AnimGraphNode
    {
    public:
        using NodeAlignmentData = AZStd::pair<AZStd::string, int>;

        AZ_RTTI(BlendTreeTwoLinkIKNode, "{0C3E8B7F-F810-47A6-B1A9-27BD4E4B5500}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum : uint16
        {
            INPUTPORT_POSE      = 0,
            INPUTPORT_GOALPOS   = 1,
            INPUTPORT_GOALROT   = 2,
            INPUTPORT_BENDDIR   = 3,
            INPUTPORT_WEIGHT    = 4,
            OUTPUTPORT_POSE     = 0
        };

        enum : uint16
        {
            PORTID_INPUT_POSE       = 0,
            PORTID_INPUT_GOALPOS    = 1,
            PORTID_INPUT_GOALROT    = 2,
            PORTID_INPUT_BENDDIR    = 3,
            PORTID_INPUT_WEIGHT     = 4,
            PORTID_OUTPUT_POSE      = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData() = default;

            void Update() override;

        public:
            size_t m_nodeIndexA = InvalidIndex;
            size_t m_nodeIndexB = InvalidIndex;
            size_t m_nodeIndexC = InvalidIndex;
            size_t m_endEffectorNodeIndex = InvalidIndex;
            size_t m_alignNodeIndex = InvalidIndex;
            size_t m_bendDirNodeIndex = InvalidIndex;
        };

        BlendTreeTwoLinkIKNode();
        ~BlendTreeTwoLinkIKNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const override                { return true; }
        AZ::Color GetVisualColor() const override               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetEndNodeName(const AZStd::string& endNodeName);
        void SetEndEffectorNodeName(const AZStd::string& endEffectorNodeName);
        void SetAlignToNode(const NodeAlignmentData& alignToNode);
        void SetBendDirNodeName(const AZStd::string& bendDirNodeName);
        void SetRotationEnabled(bool rotationEnabled);
        void SetRelativeBendDir(bool relativeBendDir);
        void SetExtractBendDir(bool extractBendDir);

        const AZStd::string& GetEndJointName() const { return m_endNodeName; }
        const AZStd::string& GetEndEffectorJointName() const { return m_endEffectorNodeName; }
        const AZStd::string& GetBendDirJointName() const { return m_bendDirNodeName; }
        const NodeAlignmentData& GetAlignToJointData() const { return m_alignToNode; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;

        static void CalculateMatrix(const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Matrix3x3* outForward);
        static bool Solve2LinkIK(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Vector3* outMidPos);

        AZ::Crc32 GetRelativeBendDirVisibility() const;

        NodeAlignmentData m_alignToNode; /**< Node name and the parent depth (0=current, 1=parent, 2=parent of parent, 3=parent of parent of parent, etc.). */
        AZStd::string                   m_endNodeName;
        AZStd::string                   m_endEffectorNodeName;
        AZStd::string                   m_bendDirNodeName;
        bool                            m_rotationEnabled;
        bool                            m_relativeBendDir;
        bool                            m_extractBendDir;
    };
} // namespace EMotionFX
