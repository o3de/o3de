/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>

namespace LmbrCentral
{
    /*!
     * RenderBoundsNotifications
     * Events dispatched by components with custom bounds.
     */
    class RenderBoundsNotifications
        : public AZ::ComponentBus
    {
    public:

        /**
         * Notifies listeners that bounds were reset.
         */
        virtual void OnRenderBoundsReset() {}
    };

    using RenderBoundsNotificationBus = AZ::EBus<RenderBoundsNotifications>;
} // namespace LmbrCentral
