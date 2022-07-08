/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/Material/MaterialAssignmentId.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

class QPixmap;

namespace AZ
{
    namespace Render
    {
        //! EditorMaterialSystemComponentNotifications is an interface for handling notifications from EditorMaterialSystemComponent, like
        //! being informed that material preview images are available
        class EditorMaterialSystemComponentNotifications : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! Notify that a material preview image is ready
            virtual void OnRenderMaterialPreviewComplete(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap) = 0;
        };
        using EditorMaterialSystemComponentNotificationBus = AZ::EBus<EditorMaterialSystemComponentNotifications>;
    } // namespace Render
} // namespace AZ
