/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EntityTypes.h>

#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>

namespace GradientSignal
{
    class GradientPreviewer
        : private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private GradientPreviewContextRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientPreviewer, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientPreviewer, "{5962AFD7-0432-4D1D-9DF6-2046B1B78322}");

        static void Reflect(AZ::ReflectContext* context);

        bool GetPreviewSettingsVisible() const;
        void SetPreviewSettingsVisible(bool visible);

        void Activate(AZ::EntityId ownerEntityId);
        void Deactivate();

        //! AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        //! GradientPreviewContextRequestBus overrides ...
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;
        bool GetConstrainToShape() const override;

        void SetPreviewEntity(AZ::EntityId boundsEntityId);

        static AzToolsFramework::EntityIdList CancelPreviewRendering();

    protected:
        AZ::u32 GetPreviewSettingsVisibility() const;
        AZ::u32 GetPreviewPositionVisibility() const;
        AZ::u32 GetPreviewSizeVisibility() const;
        AZ::u32 GetPreviewConstrainToShapeVisibility() const;
        AZ::u32 PreviewSettingsAndSettingsVisibilityChanged() const;

        // This is used by the preview so we can pass an invalid entity Id if our component is disabled
        AZ::EntityId GetGradientEntityId() const;

        void UpdatePreviewSettings() const;

        AZ::EntityId m_ownerEntityId;

        // Controls whether or not to use a specific entity to define the preview bounds
        AZ::EntityId m_boundsEntityId;
        // If a specific entity is used, determines whether to use the AABB or the actual shape inside the AABB
        bool m_constrainToShape = false;

        // If a specific entity is NOT defining the preview bounds, then define the preview bounds with a center point and extents.
        AZ::Vector3 m_previewCenter = AZ::Vector3(0.0f);
        AZ::Vector3 m_previewExtents = AZ::Vector3(1.0f); // 1m sq preview...arbitrary default box size in meters chosen by design

        // Controls whether or not to show the preview settings for this instance.
        // If the settings aren't shown, it should be because the owner always has specific settings that need to be set.
        bool m_previewSettingsVisible = true;
    };

} // namespace GradientSignal
