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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>

namespace EMotionFX
{
    /**
     * Footplant Inverse Kinematics solver node.
     */
    class EMFX_API BlendTreeFootIKNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFootIKNode, "{2F863ABA-8885-461F-98D0-F900745C45AF}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_POSE              = 0,
            INPUTPORT_FOOTHEIGHT        = 1,
            INPUTPORT_RAYCASTLENGTH     = 2,
            INPUTPORT_MAXHIPADJUST      = 3,
            INPUTPORT_MAXFOOTADJUST     = 4,
            INPUTPORT_IKBLENDSPEED      = 5,
            INPUTPORT_FOOTBLENDSPEED    = 6,
            INPUTPORT_HIPBLENDSPEED     = 7,
            INPUTPORT_ADJUSTHIP         = 8,
            INPUTPORT_FOOTLOCK          = 9,
            INPUTPORT_WEIGHT            = 10,

            OUTPUTPORT_POSE             = 0
        };

        enum
        {
            PORTID_INPUT_POSE               = 0,
            PORTID_INPUT_WEIGHT             = 1,
            PORTID_INPUT_FOOTHEIGHT         = 2,
            PORTID_INPUT_RAYCASTLENGTH      = 3,
            PORTID_INPUT_MAXHIPADJUST       = 4,
            PORTID_INPUT_MAXFOOTADJUST      = 5,
            PORTID_INPUT_ADJUSTHIP          = 6,
            PORTID_INPUT_FOOTLOCK           = 7,
            PORTID_INPUT_IKBLENDSPEED       = 8,
            PORTID_INPUT_HIPBLENDSPEED      = 9,
            PORTID_INPUT_FOOTBLENDSPEED     = 10,

            PORTID_OUTPUT_POSE = 0
        };

        enum LegId : AZ::u8
        {
            Left    = 0,
            Right   = 1
        };

        enum LegJointId : AZ::u8
        {
            UpperLeg    = 0,
            Knee        = 1,
            Foot        = 2,
            Toe         = 3
        };

        enum class LegFlags : AZ::u8
        {
            FootDown        = 1 << 0,
            ToeDown         = 1 << 1,
            FirstUpdate     = 1 << 2,
            IkEnabled       = 1 << 3,
            Locked          = 1 << 4,
            LockedFirstTime = 1 << 5,
            AllowLocking    = 1 << 6,
            Unlocking       = 1 << 7
        };

        struct Leg
        {
            AZ::u32 m_jointIndices[4];      // Use LegJointId as index into this array.
            AZ::u8  m_flags         = static_cast<AZ::u8>(LegFlags::FirstUpdate);
            AZ::Vector3             m_footLockPosition = AZ::Vector3::CreateZero();
            AZ::Quaternion          m_footLockRotation;
            AZ::Quaternion          m_currentFootRot;
            float   m_footHeight    = 0.0f; // The height of the foot joint in model space.
            float   m_toeHeight     = 0.0f; // The height of the toe joint in model space.
            float   m_legLength     = 0.0f; // The length of the leg (the length of both upper and lower leg summed up).
            float   m_weight        = 0.0f; // The current IK weight
            float   m_footLength    = 0.0f; // The length of the foot (distance between toe and foot joint)
            float   m_targetWeight  = 0.0f; // The target IK weight
            float   m_unlockBlendT  = 0.0f; // Unlocking blend weight, where 0 means fully locked and 1 means fully unlocked.

            AZ_INLINE void EnableFlag(LegFlags flag)            { m_flags |= static_cast<AZ::u8>(flag); }
            AZ_INLINE void DisableFlag(LegFlags flag)           { m_flags &= ~static_cast<AZ::u8>(flag); }
            AZ_INLINE void SetFlag(LegFlags flag, bool on)      { if (on) m_flags |= static_cast<AZ::u8>(flag); else m_flags &= ~static_cast<AZ::u8>(flag); }
            AZ_INLINE bool IsFlagEnabled(LegFlags flag) const   { return (m_flags & static_cast<AZ::u8>(flag)) != 0; }
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL
            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance)
            {
            }
            ~UniqueData() override = default;

            // Skip loading and saving of unique data state.
            // This is done to prevent a crash related to recording the event buffer.
            uint32 Save(uint8 * outputBuffer) const override
            {
                AZ_UNUSED(outputBuffer);
                return 0;
            }

            uint32 Load(const uint8 * dataBuffer) override
            {
                AZ_UNUSED(dataBuffer);
                return 0;
            }

            void Update() override;

        public:
            Leg         m_legs[2]; // Use LegId as index into this array.
            float       m_hipCorrectionTarget   = 0.0f;
            float       m_curHipCorrection      = 0.0f;
            float       m_timeDelta             = 0.0f;
            AZ::u32     m_hipJointIndex         = MCORE_INVALIDINDEX32;
            AnimGraphEventBuffer m_eventBuffer;
        };

        BlendTreeFootIKNode();
        ~BlendTreeFootIKNode() override = default;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        bool InitLegs(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);

        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const override                { return true; }
        AZ::Color GetVisualColor() const override               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void SetLeftFootJointName(AZStd::string_view name)      { m_leftFootJointName = name; }
        void SetRightFootJointName(AZStd::string_view name)     { m_rightFootJointName = name; }
        void SetLeftToeJointName(AZStd::string_view name)       { m_leftToeJointName = name; }
        void SetRightToeJointName(AZStd::string_view name)      { m_rightToeJointName = name; }
        void SetHipJointName(AZStd::string_view name)           { m_hipJointName = name; }
        void SetFootHeigthOffset(float value)                   { m_footHeightOffset = value; }
        void SetRaycastLength(float value)                      { m_raycastLength = value; }
        void SetMaxHipAdjustment(float value)                   { m_maxHipAdjustment = value; }
        void SetMaxFootAdjustment(float value)                  { m_maxFootAdjustment = value; }
        void SetStretchMaxFactor(float value)                   { m_stretchFactorMax = value; }
        void SetStretchTreshold(float value)                    { m_stretchThreshold = value; }
        void SetLegBlendSpeed(float value)                      { m_ikBlendSpeed = value; }
        void SetFootBlendSpeed(float value)                     { m_footBlendSpeed = value; }
        void SetHipBlendSpeed(float value)                      { m_hipBlendSpeed = value; }
        void SetAdjustHip(bool enabled)                         { m_adjustHip = enabled; }
        void SetFootLock(bool enabled)                          { m_footLock = enabled; }
        void SetForceUseRaycastBus(bool enabled)                { m_forceUseRaycastBus = enabled; }

        const AZStd::string& GetHipJointName() const            { return m_hipJointName; }
        float GetFootHeigthOffset() const                       { return m_footHeightOffset; }
        float GetRaycastLength() const                          { return m_raycastLength; }
        float GetMaxHipAdjustment() const                       { return m_maxHipAdjustment; }
        float GetMaxFootAdjustment() const                      { return m_maxFootAdjustment; }
        float GetStretchMaxFactor() const                       { return m_stretchFactorMax; }
        float GetStretchTreshold() const                        { return m_stretchThreshold; }
        float GetLegBlendSpeed() const                          { return m_ikBlendSpeed; }
        float GetFootBlendSpeed() const                         { return m_footBlendSpeed; }
        float GetHipBlendSpeed() const                          { return m_hipBlendSpeed; }
        bool GetAdjustHip() const                               { return m_adjustHip; }
        bool GetFootLock() const                                { return m_footLock; }
        bool GetForceUseRaycastBus() const                      { return m_forceUseRaycastBus; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        enum class FootPlantDetectionMethod : uint8
        {
            Automatic       = 1,
            MotionEvents    = 0,
        };

        struct RaycastResult
        {
            AZ::Vector3 m_position;
            AZ::Vector3 m_normal;
            bool        m_intersected;
        };

        struct IntersectionResults
        {
            RaycastResult m_footResult;
            RaycastResult m_toeResult;
        };

        struct IKSolveParameters
        {
            AnimGraphInstance*      m_animGraphInstance = nullptr;
            ActorInstance*          m_actorInstance     = nullptr;
            UniqueData*             m_uniqueData        = nullptr;
            const Pose*             m_inputPose         = nullptr;
            Pose*                   m_outputPose        = nullptr;
            IntersectionResults*    m_intersections     = nullptr;
            float                   m_weight            = 1.0f;
            float                   m_hipHeightAdj      = 0.0f;
            float                   m_deltaTime         = 1.0f / 60.0f;
            bool                    m_invertAlign       = false;
            bool                    m_forceIKDisabled   = false;
            bool                    m_footLock          = false;

            IKSolveParameters() = default;
        };

        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        bool InitLeg(LegId legId, const AZStd::string& footJointName, const AZStd::string& toeJointName, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        bool Solve2LinkIK(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Vector3* outMidPos);
        void CalculateMatrix(const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Matrix3x3* outForward);
        void Raycast(LegId legId, LegJointId joinId, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, const Pose& inputPose, RaycastResult& raycastResult);
        void SolveLegIK(LegId legId, const IKSolveParameters& solveParams);
        float AdjustHip(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, Pose& inputPose, Pose& outputPose, IntersectionResults* intersectionResults, bool allowAdjust);
        void UpdateLegLength(LegId legId, UniqueData* uniqueData, const Pose& inputPose);
        float CalculateIKWeightFactor(LegId legId, const IKSolveParameters& solveParams);
        void InterpolateWeight(LegId legId, UniqueData* uniqueData, float timeDelta, float ikBlendSpeed);
        void GenerateRayStartEnd(LegId legId, LegJointId jointId, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, const Pose& inputPose, AZ::Vector3& outRayStart, AZ::Vector3& outRayEnd) const;
        bool IsBelowSurface(const AZ::Vector3& position, const AZ::Vector3& intersectionPoint, const AZ::Vector3& intersectionNormal, float threshold) const;
        AZ::Quaternion CalculateFootRotation(LegId legId, const IKSolveParameters& solveParams) const;
        float GetFootHeightOffset(AnimGraphInstance* animGraphInstance) const;
        float GetRaycastLength(AnimGraphInstance* animGraphInstance) const;
        float GetMaxHipAdjustment(AnimGraphInstance* animGraphInstance) const;
        float GetMaxFootAdjustment(AnimGraphInstance* animGraphInstance) const;
        float GetIKBlendSpeed(AnimGraphInstance* animGraphInstance) const;
        float GetFootBlendSpeed(AnimGraphInstance* animGraphInstance) const;
        float GetHipBlendSpeed(AnimGraphInstance* animGraphInstance) const;
        bool GetAdjustHip(AnimGraphInstance* animGraphInstance) const;
        bool GetFootLock(AnimGraphInstance* animGraphInstance) const;
        float GetActorInstanceScale(const ActorInstance* actorInstance) const;

        static constexpr float s_ikSpeedMultiplier = 8.0f;
        static constexpr float s_visualizeFootPlaneScale = 0.15f;
        static constexpr float s_surfaceThreshold = 0.01f;

        AZStd::string   m_leftFootJointName;
        AZStd::string   m_leftToeJointName;
        AZStd::string   m_rightFootJointName;
        AZStd::string   m_rightToeJointName;
        AZStd::string   m_hipJointName;
        float           m_footHeightOffset      = 0.0f;
        float           m_raycastLength         = 1.0f;
        float           m_maxHipAdjustment      = 0.5f;
        float           m_maxFootAdjustment     = 1.0f;
        float           m_stretchFactorMax      = 1.1f;
        float           m_stretchThreshold      = 0.96f;
        float           m_ikBlendSpeed          = 1.0f;
        float           m_footBlendSpeed        = 1.0f;
        float           m_hipBlendSpeed         = 1.0f;
        bool            m_adjustHip             = true;
        bool            m_footLock              = true;
        bool            m_forceUseRaycastBus    = false;
        FootPlantDetectionMethod m_footPlantMethod = FootPlantDetectionMethod::Automatic;
    };
} // namespace EMotionFX
