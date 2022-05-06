/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Allocators.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <MotionMatchingData.h>
#include <MotionMatchingInstance.h>
#include <FrameDatabase.h>
#include <FeaturePosition.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <MCore/Source/AzCoreConversions.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeaturePosition, MotionMatchAllocator, 0)

    void FeaturePosition::FillQueryFeatureValues(size_t startIndex, AZStd::vector<float>& queryFeatureValues, const FrameCostContext& context)
    {
        const Transform invRootTransform = context.m_currentPose.GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
        const AZ::Vector3 worldInputPosition = context.m_currentPose.GetWorldSpaceTransform(m_jointIndex).m_position;
        const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(worldInputPosition);
        queryFeatureValues[startIndex + 0] = relativeInputPosition.GetX();
        queryFeatureValues[startIndex + 1] = relativeInputPosition.GetY();
        queryFeatureValues[startIndex + 2] = relativeInputPosition.GetZ();
    }

    void FeaturePosition::ExtractFeatureValues(const ExtractFeatureContext& context)
    {
        const Transform invRootTransform = context.m_framePose->GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
        const AZ::Vector3 nodeWorldPosition = context.m_framePose->GetWorldSpaceTransform(m_jointIndex).m_position;
        const AZ::Vector3 position = invRootTransform.TransformPoint(nodeWorldPosition);
        SetFeatureData(context.m_featureMatrix, context.m_frameIndex, position);
    }

    void FeaturePosition::DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
        const Pose& currentPose,
        const FeatureMatrix& featureMatrix,
        size_t frameIndex)
    {
        const Transform jointModelTM = currentPose.GetModelSpaceTransform(m_jointIndex);
        const Transform relativeToWorldTM = currentPose.GetWorldSpaceTransform(m_relativeToNodeIndex);

        const AZ::Vector3 position = GetFeatureData(featureMatrix, frameIndex);
        const AZ::Vector3 transformedPos = relativeToWorldTM.TransformPoint(position);

        constexpr float markerSize = 0.03f;
        debugDisplay.DepthTestOff();
        debugDisplay.SetColor(m_debugColor);
        debugDisplay.DrawBall(transformedPos, markerSize, /*drawShaded=*/false);
    }

    float FeaturePosition::CalculateFrameCost(size_t frameIndex, const FrameCostContext& context) const
    {
        const Transform invRootTransform = context.m_currentPose.GetWorldSpaceTransform(m_relativeToNodeIndex).Inversed();
        const AZ::Vector3 worldInputPosition = context.m_currentPose.GetWorldSpaceTransform(m_jointIndex).m_position;
        const AZ::Vector3 relativeInputPosition = invRootTransform.TransformPoint(worldInputPosition);
        const AZ::Vector3 framePosition = GetFeatureData(context.m_featureMatrix, frameIndex); // This is already relative to the root node
        return CalcResidual(relativeInputPosition, framePosition);
    }

    void FeaturePosition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FeaturePosition, Feature>()
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FeaturePosition>("FeaturePosition", "Matches joint positions.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ;
    }

    size_t FeaturePosition::GetNumDimensions() const
    {
        return 3;
    }

    AZStd::string FeaturePosition::GetDimensionName(size_t index) const
    {
        AZStd::string result = m_jointName;
        result += '.';

        switch (index)
        {
            case 0: { result += "PosX"; break; }
            case 1: { result += "PosY"; break; }
            case 2: { result += "PosZ"; break; }
            default: { result += Feature::GetDimensionName(index); }
        }

        return result;
    }

    AZ::Vector3 FeaturePosition::GetFeatureData(const FeatureMatrix& featureMatrix, size_t frameIndex) const
    {
        return featureMatrix.GetVector3(frameIndex, m_featureColumnOffset);
    }

    void FeaturePosition::SetFeatureData(FeatureMatrix& featureMatrix, size_t frameIndex, const AZ::Vector3& position)
    {
        featureMatrix.SetVector3(frameIndex, m_featureColumnOffset, position);
    }
} // namespace EMotionFX::MotionMatching
