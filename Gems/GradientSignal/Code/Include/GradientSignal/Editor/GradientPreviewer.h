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

        void Activate(AZ::EntityId ownerEntityId, AZ::EntityId boundsEntityId);
        void Deactivate();

        //! AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        //! GradientPreviewContextRequestBus overrides ...
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;

        AzToolsFramework::EntityIdList CancelPreviewRendering() const;

    protected:
        // This is used by the preview so we can pass an invalid entity Id if our component is disabled
        AZ::EntityId GetGradientEntityId() const;

        void UpdatePreviewSettings() const;

        AZ::EntityId m_ownerEntityId;
        AZ::EntityId m_boundsEntityId;
    };

} // namespace GradientSignal
