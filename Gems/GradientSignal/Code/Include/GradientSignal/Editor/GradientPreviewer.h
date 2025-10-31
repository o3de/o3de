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
    //! GradientPreviewer is a helper class that abstracts all of the common functionality needed to have a gradient preview widget
    //! on a gradient Editor component.
    //! To use:
    //! - Make the GradientPreviewer a member variable on the Editor component, serialize it, and add it to the Edit context.
    //! - Call Activate / Deactivate from the component's Activate/Deactivate methods
    //! - Call CancelPreviewRendering / RefreshPreviews when the component's configuration changes.
    class GradientPreviewer
        : private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private GradientPreviewContextRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientPreviewer, AZ::SystemAllocator);
        AZ_RTTI(GradientPreviewer, "{5962AFD7-0432-4D1D-9DF6-2046B1B78322}");

        static void Reflect(AZ::ReflectContext* context);

        bool GetPreviewSettingsVisible() const;
        void SetPreviewSettingsVisible(bool visible);

        void Activate(AZ::EntityId ownerEntityId);
        void Deactivate();

        //! GradientPreviewContextRequestBus overrides ...
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;
        bool GetConstrainToShape() const override;

        void SetPreviewEntity(AZ::EntityId boundsEntityId);

        void RefreshPreview() const;
        static void RefreshPreviews(const AzToolsFramework::EntityIdList& entities);
        static AzToolsFramework::EntityIdList CancelPreviewRendering();

    protected:
        //! AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        AZ::u32 GetPreviewSettingsVisibility() const;
        AZ::u32 GetPreviewPositionVisibility() const;
        AZ::u32 GetPreviewSizeVisibility() const;
        AZ::u32 GetPreviewConstrainToShapeVisibility() const;
        AZ::u32 PreviewSettingsAndSettingsVisibilityChanged() const;

        AZ::EntityId GetActiveBoundsEntityId() const;

        //! This is used by the preview so we can pass an invalid entity Id if our component is disabled
        AZ::EntityId GetGradientEntityId() const;

        //! The entity that owns the preview widget.
        AZ::EntityId m_ownerEntityId;

        //! If set, this entity will be queried for the preview bounds. If not set, previewCenter / previewExtents will be used.
        AZ::EntityId m_boundsEntityId;
        //! If boundsEntityId is set, this determines whether to use the AABB of that entity or the actual shape inside the AABB
        bool m_constrainToShape = false;

        //! If a specific entity is NOT defining the preview bounds, then define the preview bounds with a center point and extents.
        AZ::Vector3 m_previewCenter = AZ::Vector3(0.0f);
        AZ::Vector3 m_previewExtents = AZ::Vector3(1.0f); //! 1m sq preview...arbitrary default box size in meters chosen by design

        //! Controls whether or not to show the preview settings for this instance.
        //! The owning component can choose to hide the settings if it's in control of the preview settings (ex: Gradient Baker).
        bool m_previewSettingsVisible = true;
    };

} // namespace GradientSignal
