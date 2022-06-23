/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPIExt/Rules/RootMotionExtractionRule.h>

namespace EMotionFX::Pipeline::Rule
{
    void RootMotionExtractionData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RootMotionExtractionData>()
            ->Version(1)
            ->Field("sampleJoint", &RootMotionExtractionData::m_sampleJoint)
            ->Field("transitionZeroX", &RootMotionExtractionData::m_transitionZeroXAxis)
            ->Field("transitionZeroY", &RootMotionExtractionData::m_transitionZeroYAxis)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<RootMotionExtractionData>("Root motion extraction data", "Root motion extraction data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_sampleJoint, "Sample joint", "Sample joint to extract motion data from. Usually the hip joint.")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_transitionZeroXAxis, "Force X Axis Transition to 0", "Force X Axis movement to be zero.")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionData::m_transitionZeroYAxis, "Force Y Axis Transition to 0", "Force Y Axis movement to be zero.");
        }
    }

    RootMotionExtractionData::RootMotionExtractionData()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    RootMotionExtractionRule::RootMotionExtractionRule()
        : ExternalToolRule<RootMotionExtractionData>()
    {
    }

    RootMotionExtractionRule::RootMotionExtractionRule(const RootMotionExtractionData& data)
        : m_data(data)
    {
    }

    void RootMotionExtractionRule::Reflect(AZ::ReflectContext* context)
    {
        // ExternalToolRule<RootMotionExtractionData>::Reflect(context);
        RootMotionExtractionData::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RootMotionExtractionRule, IRule>()
            ->Version(1)
            ->Field("data", &RootMotionExtractionRule::m_data)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<RootMotionExtractionRule>("Root motion extraction", "Extract motion from the sample joint.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionRule::m_data, "", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    }
} // EMotionFX::Pipeline::Rule
