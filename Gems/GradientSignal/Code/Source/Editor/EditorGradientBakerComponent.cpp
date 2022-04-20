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
                    ->DataElement(0, &GradientBakerConfig::m_gradientSampler, "Gradient", "Input gradient to bake the output image from.")
                    ->DataElement(0, &GradientBakerConfig::m_inputBounds, "Input Bounds", "Input bounds for where to sample the data.")
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
                ->Field("PreviewEntity", &EditorGradientBakerComponent::m_previewEntityId)
                ->Field("PreviewPosition", &EditorGradientBakerComponent::m_previewPosition)
                ->Field("PreviewSize", &EditorGradientBakerComponent::m_previewSize)
                ->Field("ConstrainToShape", &EditorGradientBakerComponent::m_constrainToShape)
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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &EditorGradientBakerComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::OnConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->UIElement(AZ_CRC("GradientPreviewer", 0x1dbbba45), "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC("GradientEntity", 0xe8531817), &EditorGradientBakerComponent::GetGradientEntityId)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview Settings")
                    ->DataElement(0, &EditorGradientBakerComponent::m_previewEntityId, "Pin Preview to Shape", "The entity whose shape represents the bounds to render the gradient preview")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::PreviewSettingsAndSettingsVisibilityChanged)
                    ->DataElement(0, &EditorGradientBakerComponent::m_previewPosition, "Preview Position", "Center of the preview bounds")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorGradientBakerComponent::GetPreviewPositionVisibility)
                    ->DataElement(0, &EditorGradientBakerComponent::m_previewSize, "Preview Size", "Size of the preview bounds")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorGradientBakerComponent::GetPreviewSizeVisibility)
                    ->DataElement(0, &EditorGradientBakerComponent::m_constrainToShape, "Constrain to Shape", "If checked, only renders the parts of the gradient inside the component's shape and not its entire bounding box")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientBakerComponent::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorGradientBakerComponent::GetPreviewConstrainToShapeVisibility)
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

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());

        if (!m_previewEntityId.IsValid())
        {
            // Default our preview entity to this entity
            m_previewEntityId = GetEntityId();
            SetDirty();
        }

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

    AZ::u32 EditorGradientBakerComponent::GetPreviewPositionVisibility() const
    {
        return m_previewEntityId.IsValid() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    AZ::u32 EditorGradientBakerComponent::GetPreviewSizeVisibility() const
    {
        if (m_previewEntityId.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_previewEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return AZ::Edit::PropertyVisibility::Hide;
            }
        }
        return AZ::Edit::PropertyVisibility::Show;
    }

    AZ::u32 EditorGradientBakerComponent::GetPreviewConstrainToShapeVisibility() const
    {
        return m_previewEntityId.IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 EditorGradientBakerComponent::PreviewSettingsAndSettingsVisibilityChanged() const
    {
        UpdatePreviewSettings();

        // We've changed the visibility of one or more properties, so refresh the entire component.
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void EditorGradientBakerComponent::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration change)
        GradientSignal::GradientPreviewRequestBus::Event(m_gradientEntityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

    AzToolsFramework::EntityIdList EditorGradientBakerComponent::CancelPreviewRendering() const
    {
        AzToolsFramework::EntityIdList entityIds;
        AZ::EBusAggregateResults<AZ::EntityId> cancelledPreviews;
        GradientSignal::GradientPreviewRequestBus::BroadcastResult(cancelledPreviews, &GradientSignal::GradientPreviewRequestBus::Events::CancelRefresh);

        // Gather up the EntityIds for any previews that were in progress when we cancelled them
        for (auto entityId : cancelledPreviews.values)
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
        return m_previewEntityId;
    }

    AZ::Aabb EditorGradientBakerComponent::GetPreviewBounds() const
    {
        AZ::Vector3 position = m_previewPosition;

        // If an input bounds was provided, attempt to use its shape bounds
        if (m_configuration.m_inputBounds.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_configuration.m_inputBounds, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return bounds;
            }
        }

        // If a shape entity was supplied, attempt to use its shape bounds or position
        if (m_previewEntityId.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_previewEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return bounds;
            }

            AZ::TransformBus::EventResult(position, m_previewEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
        }

        return AZ::Aabb::CreateCenterHalfExtents(position, m_previewSize / 2.0f);
    }

    bool EditorGradientBakerComponent::GetConstrainToShape() const
    {
        return m_constrainToShape && m_previewEntityId.IsValid();
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

        // Refresh any of the previews that we cancelled that were still in progress so they can be completed
        for (auto entityId : entityIds)
        {
            GradientSignal::GradientPreviewRequestBus::Event(entityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
        }

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call UpdatePreviewSettings explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
