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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPIExt/Rules/RootMotionExtractionRule.h>

namespace EMotionFX::Pipeline::Rule
{
    RootMotionExtractionRule::RootMotionExtractionRule()
    {
        m_data = AZStd::make_shared<RootMotionExtractionData>();
    }

    RootMotionExtractionRule::RootMotionExtractionRule(AZStd::shared_ptr<RootMotionExtractionData> data)
        : m_data(AZStd::move(data))
    {
    }

    void RootMotionExtractionRule::Reflect(AZ::ReflectContext* context)
    {
        RootMotionExtractionData::Reflect(context);

        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
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
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &RootMotionExtractionRule::m_data, "Root motion extraction data", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    }
} // EMotionFX::Pipeline::Rule
