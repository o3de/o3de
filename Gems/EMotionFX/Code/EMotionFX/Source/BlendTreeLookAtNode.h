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

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Quaternion.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeLookAtNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeLookAtNode, "{7FBBFD4A-3B17-47D6-8419-8F8F5B89C1B3}", AnimGraphNode);
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_POSE      = 0,
            INPUTPORT_GOALPOS   = 1,
            INPUTPORT_WEIGHT    = 2,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE       = 0,
            PORTID_INPUT_GOALPOS    = 1,
            PORTID_INPUT_WEIGHT     = 2,
            PORTID_OUTPUT_POSE      = 0
        };

        enum TwistAxis : AZ::u8
        {
            TWISTAXIS_X = 0,
            TWISTAXIS_Y = 1,
            TWISTAXIS_Z = 2
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
            AZ::Quaternion mRotationQuat = AZ::Quaternion::CreateIdentity();
            float mTimeDelta = 0.0f;
            uint32 mNodeIndex = InvalidIndex32;
            bool mFirstUpdate = true;
        };

        BlendTreeLookAtNode();
        ~BlendTreeLookAtNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        bool GetSupportsVisualization() const override              { return true; }
        bool GetHasOutputPose() const override                      { return true; }
        bool GetSupportsDisable() const override                    { return true; }
        AZ::Color GetVisualColor() const override                   { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetTargetNodeName(const AZStd::string& targetNodeName);
        void SetConstraintRotation(const AZ::Quaternion& constraintRotation);
        void SetPostRotation(const AZ::Quaternion& postRotation);
        void SetLimitMin(const AZ::Vector2& limitMin);
        void SetLimitMax(const AZ::Vector2& limitMax);
        void SetFollowSpeed(float followSpeed);
        void SetTwistAxis(ConstraintTransformRotationAngles::EAxis twistAxis);
        void SetLimitsEnabled(bool limitsEnabled);
        void SetSmoothingEnabled(bool smoothingEnabled);

        const AZStd::string& GetTargetNodeName() const { return m_targetNodeName; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        AZ::Crc32 GetLimitWidgetsVisibility() const;
        AZ::Crc32 GetFollowSpeedVisibility() const;

        AZStd::string                               m_targetNodeName;
        AZ::Quaternion                              m_constraintRotation;
        AZ::Quaternion                              m_postRotation;
        AZ::Vector2                                 m_limitMin;
        AZ::Vector2                                 m_limitMax;
        float                                       m_followSpeed;
        ConstraintTransformRotationAngles::EAxis    m_twistAxis;
        bool                                        m_limitsEnabled;
        bool                                        m_smoothing;
    };
} // namespace EMotionFX
