/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <GradientSignal/Editor/GradientPreviewer.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace GradientSignal
{
    void GradientPreviewer::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientPreviewer>()
                ->Version(0)
                ->Field("BoundsEntity", &GradientPreviewer::m_boundsEntityId)
                ->Field("PreviewCenter", &GradientPreviewer::m_previewCenter)
                ->Field("PreviewExtents", &GradientPreviewer::m_previewExtents)
                ->Field("ConstrainToShape", &GradientPreviewer::m_constrainToShape)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<GradientPreviewer>("Previewer", "")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->UIElement(AZ_CRC_CE("GradientPreviewer"), "Previewer")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ_CRC_CE("GradientEntity"), &GradientPreviewer::GetGradientEntityId)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview Settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &GradientPreviewer::GetPreviewSettingsVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientPreviewer::m_boundsEntityId, "Pin Preview to Shape",
                        "The entity whose shape represents the bounds to render the gradient preview")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientPreviewer::PreviewSettingsAndSettingsVisibilityChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &GradientPreviewer::GetPreviewSettingsVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientPreviewer::m_previewCenter, "Preview Position",
                        "Center of the preview bounds")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientPreviewer::RefreshPreview)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &GradientPreviewer::GetPreviewPositionVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &GradientPreviewer::m_previewExtents, "Preview Size", "Size of the preview bounds")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientPreviewer::RefreshPreview)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &GradientPreviewer::GetPreviewSizeVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientPreviewer::m_constrainToShape, "Constrain to Shape",
                        "If checked, only renders the parts of the gradient inside the component's shape and not its entire bounding box")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientPreviewer::RefreshPreview)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &GradientPreviewer::GetPreviewConstrainToShapeVisibility)
                    ->EndGroup()
                    ;
            }
        }
    }

    void GradientPreviewer::Activate(AZ::EntityId ownerEntityId)
    {
        m_ownerEntityId = ownerEntityId;

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(ownerEntityId);
        GradientPreviewContextRequestBus::Handler::BusConnect(ownerEntityId);

        RefreshPreview();
    }

    void GradientPreviewer::Deactivate()
    {
        // If the preview shouldn't be active, use an invalid entityId
        m_ownerEntityId = AZ::EntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientPreviewContextRequestBus::Handler::BusDisconnect();
    }

    AZ::EntityId GradientPreviewer::GetActiveBoundsEntityId() const
    {
        if (m_boundsEntityId.IsValid())
        {
            return m_boundsEntityId;
        }

        // If there's no bounds entity already set, then default it to the owning entity.
        return m_ownerEntityId;
    }

    bool GradientPreviewer::GetPreviewSettingsVisible() const
    {
        return m_previewSettingsVisible;
    }

    void GradientPreviewer::SetPreviewSettingsVisible(bool visible)
    {
        m_previewSettingsVisible = visible;
    }

    AZ::u32 GradientPreviewer::GetPreviewSettingsVisibility() const
    {
        return m_previewSettingsVisible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::u32 GradientPreviewer::GetPreviewPositionVisibility() const
    {
        return (GetActiveBoundsEntityId().IsValid() || !m_previewSettingsVisible)
            ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    AZ::u32 GradientPreviewer::GetPreviewSizeVisibility() const
    {
        if (GetActiveBoundsEntityId().IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                bounds, GetActiveBoundsEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return AZ::Edit::PropertyVisibility::Hide;
            }
        }
        return m_previewSettingsVisible ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 GradientPreviewer::GetPreviewConstrainToShapeVisibility() const
    {
        return (GetActiveBoundsEntityId().IsValid() && m_previewSettingsVisible)
            ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 GradientPreviewer::PreviewSettingsAndSettingsVisibilityChanged() const
    {
        RefreshPreview();

        // We've changed the visibility of one or more properties, so refresh the entire component.
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void GradientPreviewer::OnSelected()
    {
        RefreshPreview();
    }

    void GradientPreviewer::OnDeselected()
    {
        RefreshPreview();
    }

    AZ::EntityId GradientPreviewer::GetPreviewEntity() const
    {
        return GetActiveBoundsEntityId();
    }

    void GradientPreviewer::SetPreviewEntity(AZ::EntityId boundsEntityId)
    {
        m_boundsEntityId = boundsEntityId;
    }

    AZ::Aabb GradientPreviewer::GetPreviewBounds() const
    {
        AZ::Vector3 position = m_previewCenter;

        // if a shape entity was supplied, attempt to use its shape bounds or position
        if (GetActiveBoundsEntityId().IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                bounds, GetActiveBoundsEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return bounds;
            }

            AZ::TransformBus::EventResult(position, GetActiveBoundsEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
        }

        return AZ::Aabb::CreateCenterHalfExtents(position, m_previewExtents / 2.0f);
    }

    bool GradientPreviewer::GetConstrainToShape() const
    {
        return m_constrainToShape && GetActiveBoundsEntityId().IsValid();
    }

    AZ::EntityId GradientPreviewer::GetGradientEntityId() const
    {
        return m_ownerEntityId;
    }

    void GradientPreviewer::RefreshPreview() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration
        // change)
        RefreshPreviews({ m_ownerEntityId });
    }

    void GradientPreviewer::RefreshPreviews(const AzToolsFramework::EntityIdList& entities)
    {
        for (auto entityId : entities)
        {
            GradientSignal::GradientPreviewRequestBus::Event(entityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
        }
    }

    AzToolsFramework::EntityIdList GradientPreviewer::CancelPreviewRendering()
    {
        AzToolsFramework::EntityIdList entityIds;
        AZ::EBusAggregateResults<AZ::EntityId> canceledPreviews;
        GradientSignal::GradientPreviewRequestBus::BroadcastResult(
            canceledPreviews, &GradientSignal::GradientPreviewRequestBus::Events::CancelRefresh);

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
} // namespace GradientSignal
