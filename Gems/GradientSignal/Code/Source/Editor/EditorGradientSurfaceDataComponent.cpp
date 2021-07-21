/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorGradientSurfaceDataComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    void EditorGradientSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorGradientSurfaceDataComponent, BaseClassType>()
                ->Version(1, &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType,1>)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorGradientSurfaceDataComponent>(
                    s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->UIElement("GradientPreviewer", "Previewer")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC("GradientEntity", 0xe8531817), &EditorGradientSurfaceDataComponent::GetGradientEntityId)
                    ->Attribute(AZ_CRC("GradientFilter", 0x99bf0362), &EditorGradientSurfaceDataComponent::GetFilterFunc)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
                    ;
            }
        }
    }

    void EditorGradientSurfaceDataComponent::Activate()
    {
        m_gradientEntityId = GetEntityId();
        
        BaseClassType::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorGradientSurfaceDataComponent::Deactivate()
    {
        // If the preview shouldn't be active, use an invalid entityId
        m_gradientEntityId = AZ::EntityId();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        
        BaseClassType::Deactivate();
    }

    AZ::u32 EditorGradientSurfaceDataComponent::ConfigurationChanged()
    {
        auto result = BaseClassType::ConfigurationChanged();

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return result;
    }

    AZ::EntityId EditorGradientSurfaceDataComponent::GetGradientEntityId() const
    {
        return m_gradientEntityId;
    }

    AZStd::function<float(float, const GradientSampleParams & params)> EditorGradientSurfaceDataComponent::GetFilterFunc()
    {
        // By default, our preview will show the gradient value that was queried from the gradient on this entity.
        // To show what this GradientSurfaceData component will produce, we use a custom FilterFunc to modify what
        // the preview will display.  We create a SurfacePoint, call ModifySurfacePoints() on that point, and return
        // the max value from any tags returned, or 0 if no tags were added to the point.

        // This allows us to view the results of the GradientSurfaceData modifications, including threshold clamping and
        // constraining to shape bounds.  Note that the primary gradient controls the "Pin Preview to Entity" preview setting
        // that affects *where* this preview is rendered.  If the primary gradient is pinned to a different entity that doesn't
        // overlap this component's shape constraint entity, this preview can end up all black.  To see a preview that lines up
        // with this shape constraint, the input gradient will need to "Pin Preview to Entity" to the same shape entity.

        return [this]([[maybe_unused]] float sampleValue, const GradientSampleParams& params)
        {
            // Create a fake surface point with the position we're sampling.
            SurfaceData::SurfacePoint point;
            point.m_position = params.m_position;
            SurfaceData::SurfacePointList pointList;
            pointList.emplace_back(point);

            // Send it into the component, see what emerges
            m_component.ModifySurfacePoints(pointList);

            // If the point was successfully modified, we should have one or more masks with a non-zero value.
            // Technically, they should all have the same value, but we'll grab the max from all of them in case
            // the underlying logic ever changes to allow separate ranges per tag.
            float result = 0.0f;
            for (auto& mask : pointList[0].m_masks)
            {
                result = AZ::GetMax(result, mask.second);
            }
            return result;
        };
    }
}
