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

#include <AzCore/Math/Vector3.h>
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>
#include <AzCore/std/containers/array.h>


namespace EMotionFX
{


    class EMFX_API BlendTreeRotationLimitNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRotationLimitNode, "{FFDFEDA2-1FFB-449A-B749-F4C8F7401D2D}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        class RotationLimit
        {
        public:
            AZ_RTTI(BlendTreeRotationLimitNode::RotationLimit, "{C1B00477-DC27-4E9B-8822-B8241378B2F4}")
            AZ_CLASS_ALLOCATOR_DECL

            friend class BlendTreeRotationLimitNode;

            enum Axis : AZ::u8
            {
                AXIS_X = 0,
                AXIS_Y = 1,
                AXIS_Z = 2
            };

            RotationLimit() = default;
            RotationLimit(BlendTreeRotationLimitNode::RotationLimit::Axis axis);
            virtual ~RotationLimit() = default;

            float GetLimitMin() const;
            float GetLimitMax() const;
            const char* GetLabel() const;
            void SetMin(float min);
            void SetMax(float max);

            static void Reflect(AZ::ReflectContext* context);

            static const float s_rotationLimitRangeMin;
            static const float s_rotationLimitRangeMax;
        private:
            float   m_min = 0.0f;
            float   m_max = 360.0f;
            BlendTreeRotationLimitNode::RotationLimit::Axis m_axis = BlendTreeRotationLimitNode::RotationLimit::Axis::AXIS_X;
        };



        enum
        {
            INPUTPORT_ROTATION = 0,
            INPUTPORT_POSE = 1,
            OUTPUTPORT_RESULT_QUATERNION = 0
        };

        enum
        {
            PORTID_INPUT = 0,
            PORTID_INPUT_POSE = 1,

            PORTID_OUTPUT_QUATERNION = 0
        };

        BlendTreeRotationLimitNode();
        ~BlendTreeRotationLimitNode() = default;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override { return AZ::Color(0.0f, 0.48f, 0.65f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetRotationLimitsX(float min, float max);
        void SetRotationLimitsY(float min, float max);
        void SetRotationLimitsZ(float min, float max);
        void SetTwistAxis(ConstraintTransformRotationAngles::EAxis twistAxis);

        static void Reflect(AZ::ReflectContext* context);

    private:
        const RotationLimit& GetRotationLimitX() const;
        const RotationLimit& GetRotationLimitY() const;
        const RotationLimit& GetRotationLimitZ() const;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void ExecuteMathLogic(EMotionFX::AnimGraphInstance* animGraphInstance);

        AZStd::array<RotationLimit, 3> m_rotationLimits = { { RotationLimit(RotationLimit::Axis::AXIS_X), RotationLimit(RotationLimit::Axis::AXIS_Y), RotationLimit(RotationLimit::Axis::AXIS_Z) } };
        ConstraintTransformRotationAngles m_constraintTransformRotationAngles;
        ConstraintTransformRotationAngles::EAxis m_twistAxis;
    };
}   // namespace EMotionFX
