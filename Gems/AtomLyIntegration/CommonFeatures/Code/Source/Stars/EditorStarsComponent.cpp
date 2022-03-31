/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorStarsComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace AZ::Render 
{
    void EditorStarsComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorStarsComponent, BaseClass>()
                ->Version(1)
                ;
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StarsComponentConfig>("Stars Config", "Star Config Data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Game") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StarsComponentConfig::m_intensityFactor, "Intensity factor", "Intensity factor")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.0f) // 1000 nits max for HDR
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StarsComponentConfig::m_radiusFactor, "Radius factor", "Star radius factor")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 64.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StarsComponentConfig::m_starsAsset, "Stars Asset", "Stars data")
                    ;

                editContext->Class<StarsComponentController>(
                    "StarsComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StarsComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<EditorStarsComponent>(
                    "Stars", "Renders stars in the background")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c));
            }
        }
    }

    EditorStarsComponent::EditorStarsComponent(const StarsComponentConfig& config)
        : BaseClass(config)
    {
        m_controller.m_visible = IsVisible();
    }

    u32 EditorStarsComponent::OnConfigurationChanged()
    {
        if (m_prevAssetId != m_controller.GetConfiguration().m_starsAsset.GetId())
        {
            m_controller.OnStarsAssetChanged();
            m_prevAssetId = m_controller.GetConfiguration().m_starsAsset.GetId();
        }

        m_controller.OnConfigChanged();

        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorStarsComponent::OnEntityVisibilityChanged(bool visibility)
    {
        m_controller.m_visible = visibility;
        if (m_controller.m_starsFeatureProcessor)
        {
            m_controller.m_starsFeatureProcessor->Enable(visibility);
        }
    }
}
