/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRandomGradientComponent.h"

namespace GradientSignal
{
    void EditorRandomGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRandomGradientComponent, EditorGradientComponentBase>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorRandomGradientComponent>(
                    EditorRandomGradientComponent::s_componentName, EditorRandomGradientComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorRandomGradientComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "GenerateRandomSeed", "Generate a new random seed")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Generate Random Seed")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRandomGradientComponent::OnGenerateRandomSeed)
                    ;
            }
        }
    }

    AZ::Crc32 EditorRandomGradientComponent::OnGenerateRandomSeed()
    {
        // The random seed has to be at least 1 to be valid for this gradient type
        m_configuration.m_randomSeed = AZStd::max(rand(), 1);

        return EditorGradientComponentBase::ConfigurationChanged();
    }
}
