/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <Feature.h>
#include <Frame.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Velocity.h>

#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(Feature, MotionMatchAllocator)

    Feature::ExtractFeatureContext::ExtractFeatureContext(FeatureMatrix& featureMatrix, AnimGraphPosePool& posePool)
        : m_featureMatrix(featureMatrix)
        , m_posePool(posePool)
    {
    }

    Feature::QueryVectorContext::QueryVectorContext(const Pose& currentPose, const TrajectoryQuery& trajectoryQuery)
        : m_currentPose(currentPose)
        , m_trajectoryQuery(trajectoryQuery)
    {
    }

    Feature::FrameCostContext::FrameCostContext(const QueryVector& queryVector, const FeatureMatrix& featureMatrix)
        : m_queryVector(queryVector)
        , m_featureMatrix(featureMatrix)
    {
    }

    bool Feature::Init(const InitSettings& settings)
    {
        const Actor* actor = settings.m_actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();

        const Node* joint = skeleton->FindNodeByNameNoCase(m_jointName.c_str());
        m_jointIndex = joint ? joint->GetNodeIndex() : InvalidIndex;
        if (m_jointIndex == InvalidIndex)
        {
            AZ_Error("Motion Matching", false, "Feature::Init(): Cannot find index for joint named '%s'.", m_jointName.c_str());
            return false;
        }

        const Node* relativeToJoint = skeleton->FindNodeByNameNoCase(m_relativeToJointName.c_str());
        m_relativeToNodeIndex = relativeToJoint ? relativeToJoint->GetNodeIndex() : InvalidIndex;
        if (m_relativeToNodeIndex == InvalidIndex)
        {
            AZ_Error("Motion Matching", false, "Feature::Init(): Cannot find index for joint named '%s'.", m_relativeToJointName.c_str());
            return false;
        }

        // Set a default feature name in case it did not get set manually.
        if (m_name.empty())
        {
            AZStd::string featureTypeName = this->RTTI_GetTypeName();
            AzFramework::StringFunc::Replace(featureTypeName, "Feature", "");
            m_name = AZStd::string::format("%s (%s)", featureTypeName.c_str(), m_jointName.c_str());
        }
        return true;
    }

    void Feature::SetDebugDrawColor(const AZ::Color& color)
    {
        m_debugColor = color;
    }

    const AZ::Color& Feature::GetDebugDrawColor() const
    {
        return m_debugColor;
    }

    void Feature::SetDebugDrawEnabled(bool enabled)
    {
        m_debugDrawEnabled = enabled;
    }

    bool Feature::GetDebugDrawEnabled() const
    {
        return m_debugDrawEnabled;
    }

    float Feature::CalculateFrameCost([[maybe_unused]] size_t frameIndex, [[maybe_unused]] const FrameCostContext& context) const
    {
        AZ_Assert(false, "Feature::CalculateFrameCost(): Not implemented for the given feature.");
        return 0.0f;
    }

    void Feature::SetRelativeToNodeIndex(size_t nodeIndex)
    {
        m_relativeToNodeIndex = nodeIndex;
    }

    float Feature::GetNormalizedDirectionDifference(const AZ::Vector2& directionA, const AZ::Vector2& directionB) const
    {
        const float dotProduct = directionA.GetNormalized().Dot(directionB.GetNormalized());
        const float normalizedDirectionDifference = (2.0f - (1.0f + dotProduct)) * 0.5f;
        return AZ::GetAbs(normalizedDirectionDifference);
    }

    float Feature::GetNormalizedDirectionDifference(const AZ::Vector3& directionA, const AZ::Vector3& directionB) const
    {
        const float dotProduct = directionA.GetNormalized().Dot(directionB.GetNormalized());
        const float normalizedDirectionDifference = (2.0f - (1.0f + dotProduct)) * 0.5f;
        return AZ::GetAbs(normalizedDirectionDifference);
    }

    float Feature::CalcResidual(float value) const
    {
        if (m_residualType == ResidualType::Squared)
        {
            return value * value;
        }

        return AZ::Abs(value);
    }

    float Feature::CalcResidual(const AZ::Vector3& a, const AZ::Vector3& b) const
    {
        const float euclideanDistance = (b - a).GetLength();
        return CalcResidual(euclideanDistance);
    }

    AZ::Crc32 Feature::GetCostFactorVisibility() const
    {
        return AZ::Edit::PropertyVisibility::Show;
    }

    void Feature::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Feature>()
            ->Version(2)
            ->Field("id", &Feature::m_id)
            ->Field("name", &Feature::m_name)
            ->Field("jointName", &Feature::m_jointName)
            ->Field("relativeToJointName", &Feature::m_relativeToJointName)
            ->Field("debugDraw", &Feature::m_debugDrawEnabled)
            ->Field("debugColor", &Feature::m_debugColor)
            ->Field("costFactor", &Feature::m_costFactor)
            ->Field("residualType", &Feature::m_residualType)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Feature>("Feature", "Base class for a feature")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Feature::m_name, "Name", "Custom name of the feature used for identification and debug visualizations.")
            ->DataElement(AZ_CRC_CE("ActorNode"), &Feature::m_jointName, "Joint", "The joint to extract the data from.")
            ->DataElement(AZ_CRC_CE("ActorNode"), &Feature::m_relativeToJointName, "Relative To Joint", "When extracting feature data, convert it to relative-space to the given joint.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Feature::m_debugDrawEnabled, "Debug Draw", "Are debug visualizations enabled for this feature?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Feature::m_debugColor, "Debug Draw Color", "Color used for debug visualizations to identify the feature.")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Feature::m_costFactor, "Cost Factor", "The cost factor for the feature is multiplied with the actual and can be used to change a feature's influence in the motion matching search.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &Feature::GetCostFactorVisibility)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Feature::m_residualType, "Residual", "Use 'Squared' in case minimal differences should be ignored and larger differences should overweight others. Use 'Absolute' for linear differences and don't want the mentioned effect.")
                ->EnumAttribute(ResidualType::Absolute, "Absolute")
                ->EnumAttribute(ResidualType::Squared, "Squared")
            ;
    }
} // namespace EMotionFX::MotionMatching
