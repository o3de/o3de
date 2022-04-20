/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorGradientBakerComponent.h"

#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>


namespace GradientSignal
{
    void GradientBakerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientBakerConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Gradient", &GradientBakerConfig::m_gradientSampler)
                ->Field("InputBounds", &GradientBakerConfig::m_inputBounds)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientBakerConfig>(
                    "Gradient Baker", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::DisplayOrder, 50) // There's no special meaning to 50, we just need this class to move down and display below any children
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_gradientSampler, "Gradient", "Input gradient to bake the output image from.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientBakerConfig::m_inputBounds, "Input Bounds", "Input bounds for where to sample the data.")
                    ;
            }
        }
    }

    void EditorGradientBakerComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientBakerConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGradientBakerComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Configuration", &EditorGradientBakerComponent::m_configuration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorGradientBakerComponent>(
                    EditorGradientBakerComponent::s_componentName, EditorGradientBakerComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorGradientBakerComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGradientBakerComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->UIElement(AZ_CRC_CE("GradientPreviewer"), "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &EditorGradientBakerComponent::GetGradientEntityId)
                    ;
            }
        }
    }

    void EditorGradientBakerComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        m_gradientEntityId = GetEntityId();

        SectorDataNotificationBus::Handler::BusConnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        GradientPreviewContextRequestBus::Handler::BusConnect(GetEntityId());

        m_configuration.m_gradientSampler.m_ownerEntityId = GetEntityId();

        // Validation needs to happen after the ownerEntity is set in case the validation needs that data
        if (!m_configuration.m_gradientSampler.ValidateGradientEntityId())
        {
            SetDirty();
        }

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);

        // Connect to GradientRequestBus after the gradient sampler and dependency monitor is configured
        // before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());

        UpdatePreviewSettings();
    }

    void EditorGradientBakerComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        // If the preview shouldn't be active, use an invalid entityId
        m_gradientEntityId = AZ::EntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientPreviewContextRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        SectorDataNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorGradientBakerComponent::OnCompositionChanged()
    {
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGradientBakerComponent::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration change)
        GradientSignal::GradientPreviewRequestBus::Event(m_gradientEntityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

    AzToolsFramework::EntityIdList EditorGradientBakerComponent::CancelPreviewRendering() const
    {
        AzToolsFramework::EntityIdList entityIds;
        AZ::EBusAggregateResults<AZ::EntityId> canceledPreviews;
        GradientSignal::GradientPreviewRequestBus::BroadcastResult(canceledPreviews, &GradientSignal::GradientPreviewRequestBus::Events::CancelRefresh);

        // Gather up the EntityIds for any previews that were in progress when we canceled them
        for (auto entityId : canceledPreviews.values)
        {
            if (entityId.IsValid())
            {
                entityIds.push_back(entityId);
            }
        }

        return entityIds;
    }

    AZ::EntityId EditorGradientBakerComponent::GetPreviewEntity() const
    {
        // Our preview entity will always be ourself since we want to preview
        // exactly what's going to be in the baked image.
        return GetEntityId();
    }

    AZ::Aabb EditorGradientBakerComponent::GetPreviewBounds() const
    {
        AZ::Aabb bounds = AZ::Aabb::CreateNull();

        if (m_configuration.m_inputBounds.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_configuration.m_inputBounds, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        return bounds;
    }

    AZ::EntityId EditorGradientBakerComponent::GetGradientEntityId() const
    {
        return m_gradientEntityId;
    }

    float EditorGradientBakerComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        return m_configuration.m_gradientSampler.GetValue(sampleParams);
    }

    void EditorGradientBakerComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        m_configuration.m_gradientSampler.GetValues(positions, outValues);
    }

    bool EditorGradientBakerComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    void EditorGradientBakerComponent::OnSectorDataConfigurationUpdated() const
    {
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void EditorGradientBakerComponent::OnSelected()
    {
        UpdatePreviewSettings();
    }

    void EditorGradientBakerComponent::OnDeselected()
    {
        UpdatePreviewSettings();
    }

    void EditorGradientBakerComponent::OnConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = CancelPreviewRendering();

        // Refresh any of the previews that we canceled that were still in progress so they can be completed
        for (auto entityId : entityIds)
        {
            GradientSignal::GradientPreviewRequestBus::Event(entityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
        }

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call UpdatePreviewSettings explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
