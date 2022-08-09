/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Editor/GradientPreviewer.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>

namespace GradientSignal
{
    void GradientPreviewer::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientPreviewer>()
                ->Version(0)
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
                    ->EndGroup()
                    ;
            }
        }
    }

    void GradientPreviewer::Activate(AZ::EntityId ownerEntityId, AZ::EntityId boundsEntityId)
    {
        m_ownerEntityId = ownerEntityId;
        m_boundsEntityId = boundsEntityId;

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(ownerEntityId);
        GradientPreviewContextRequestBus::Handler::BusConnect(ownerEntityId);

        UpdatePreviewSettings();
    }

    void GradientPreviewer::Deactivate()
    {
        // If the preview shouldn't be active, use an invalid entityId
        m_ownerEntityId = AZ::EntityId();
        m_boundsEntityId = AZ::EntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientPreviewContextRequestBus::Handler::BusDisconnect();
    }

    void GradientPreviewer::OnSelected()
    {
        UpdatePreviewSettings();
    }

    void GradientPreviewer::OnDeselected()
    {
        UpdatePreviewSettings();
    }

    AZ::EntityId GradientPreviewer::GetPreviewEntity() const
    {
        return m_boundsEntityId;
    }

    AZ::Aabb GradientPreviewer::GetPreviewBounds() const
    {
        AZ::Aabb bounds = AZ::Aabb::CreateNull();

        if (m_boundsEntityId.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                bounds, m_boundsEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        return bounds;
    }

    AZ::EntityId GradientPreviewer::GetGradientEntityId() const
    {
        return m_ownerEntityId;
    }

    void GradientPreviewer::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration
        // change)
        GradientSignal::GradientPreviewRequestBus::Event(m_ownerEntityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

    AzToolsFramework::EntityIdList GradientPreviewer::CancelPreviewRendering() const
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
