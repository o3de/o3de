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

#if defined(EMOTIONFXANIMATION_EDITOR)
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#endif

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
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<RootMotionExtractionData>("Root motion extraction data", "Root motion extraction data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement("NodeListSelection", &RootMotionExtractionData::m_sampleJoint, "Sample joint", "Sample joint to extract motion data from. Usually the hip joint.")
                    //->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_extractRotation, "Rotation extraction", "Extract the rotation value from sample joint.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Transition Extraction")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_transitionZeroXAxis, "Ignore X-Axis transition", "Force X Axis movement to be zero.")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_transitionZeroYAxis, "Ignore Y-Axis transition", "Force Y Axis movement to be zero.")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &RootMotionExtractionData::m_smoothingMethod, "Smoothing method", "Select the smoothing method for the motion data.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->EnumAttribute(SmoothingMethod::None, "None")
                    ->EnumAttribute(SmoothingMethod::MovingAverage, "Moving average");
        }
    }
}
