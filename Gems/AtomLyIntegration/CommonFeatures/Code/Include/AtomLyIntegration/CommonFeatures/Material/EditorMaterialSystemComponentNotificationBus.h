/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignmentId.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>

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
            //! Using a mutex because this EBus is accessed from multiple threads.
            using MutexType = AZStd::mutex;

            //! This notification is sent when a material preview image for a given entity and material assignment has been rendered by the
            //! preview rendering system
            virtual void OnRenderMaterialPreviewRendered(
                [[maybe_unused]] const AZ::EntityId& entityId,
                [[maybe_unused]] const AZ::Render::MaterialAssignmentId& materialAssignmentId,
                [[maybe_unused]] const QPixmap& pixmap){};

            //! This notification is sent after a material preview image has been rendered and cached
            virtual void OnRenderMaterialPreviewReady(
                [[maybe_unused]] const AZ::EntityId& entityId,
                [[maybe_unused]] const AZ::Render::MaterialAssignmentId& materialAssignmentId,
                [[maybe_unused]] const QPixmap& pixmap){};
        };
        using EditorMaterialSystemComponentNotificationBus = AZ::EBus<EditorMaterialSystemComponentNotifications>;
    } // namespace Render
} // namespace AZ
