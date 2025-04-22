/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/MotionData/RootMotionExtractionData.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>

namespace EMotionFX
{
    void RootMotionExtractionData::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RootMotionExtractionData>()
            ->Version(1)
            ->Field("sampleJoint", &RootMotionExtractionData::m_sampleJoint)
            ->Field("transitionZeroX", &RootMotionExtractionData::m_transitionZeroXAxis)
            ->Field("transitionZeroY", &RootMotionExtractionData::m_transitionZeroYAxis)
            ->Field("extractRotation", &RootMotionExtractionData::m_extractRotation)
            ->Field("smoothingMethod", &RootMotionExtractionData::m_smoothingMethod)
            ->Field("smoothPosition", &RootMotionExtractionData::m_smoothPosition)
            ->Field("smoothRotation", &RootMotionExtractionData::m_smoothRotation)
            ->Field("smoothFrameNum", &RootMotionExtractionData::m_smoothFrameNum)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<RootMotionExtractionData>("Root motion extraction data", "Root motion extraction data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement("ActorNode", &RootMotionExtractionData::m_sampleJoint, "Sample joint", "Sample joint to extract motion data from. Usually the hip joint.")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_extractRotation, "Rotation extraction", "Extract the rotation value from sample joint.")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &RootMotionExtractionData::m_smoothingMethod, "Smoothing method", "Select the smoothing method for the motion data.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->EnumAttribute(SmoothingMethod::None, "None")
                    ->EnumAttribute(SmoothingMethod::MovingAverage, "Moving average")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_smoothPosition, "Smooth position", "Apply smooth on the position of the root bone animation.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &RootMotionExtractionData::GetVisibilitySmoothEnabled)
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_smoothRotation, "Smooth rotation", "Apply smooth on the rotation of the root bone animation.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &RootMotionExtractionData::GetVisibilitySmoothEnabled)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &RootMotionExtractionData::m_smoothFrameNum, "Smooth frame num", "If the number is 1, it will average the closest 3 frames. If the number is 2, it will average the closest 5 frames (2 frames before and 2 frames after), etc.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 10)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &RootMotionExtractionData::GetVisibilitySmoothEnabled)
                ->ClassElement(AZ::Edit::ClassElements::Group, "Transition Extraction")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_transitionZeroXAxis, "Ignore X-Axis transition", "Force X Axis movement to be zero.")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_transitionZeroYAxis, "Ignore Y-Axis transition", "Force Y Axis movement to be zero.");
        }
    }

    AZ::Crc32 RootMotionExtractionData::GetVisibilitySmoothEnabled() const
    {
        return m_smoothingMethod == SmoothingMethod::None ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    void RootMotionExtractionData::FindBestMatchedJoints(const Actor* actor)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        for (size_t boneIndex = 0; boneIndex < skeleton->GetNumNodes(); ++boneIndex)
        {
            const AZStd::string_view boneName = skeleton->GetNode(boneIndex)->GetNameString();
            if (AzFramework::StringFunc::Find(boneName, m_sampleJoint) != AZStd::string::npos)
            {
                m_sampleJoint = boneName;
                break;
            }
        }
    }
} // namespace EMotionFX
