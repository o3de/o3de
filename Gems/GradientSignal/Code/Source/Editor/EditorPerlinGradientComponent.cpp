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

#include "GradientSignal_precompiled.h"
#include "EditorPerlinGradientComponent.h"

namespace GradientSignal
{
    void EditorPerlinGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPerlinGradientComponent, EditorGradientComponentBase>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPerlinGradientComponent>(
                    EditorPerlinGradientComponent::s_componentName, EditorPerlinGradientComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorPerlinGradientComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "GenerateRandomSeed", "Generate a new random seed")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Generate Random Seed")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorPerlinGradientComponent::OnGenerateRandomSeed)
                    ;
            }
        }
    }

    AZ::Crc32 EditorPerlinGradientComponent::OnGenerateRandomSeed()
    {
        // The random seed has to be at least 1 to be valid on all platforms for this gradient type
        m_configuration.m_randomSeed = AZStd::max(rand(), 1);

        return EditorGradientComponentBase::ConfigurationChanged();
    }
}