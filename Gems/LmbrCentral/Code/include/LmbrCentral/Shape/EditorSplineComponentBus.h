/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /// Editor only listener for Spline changes.
    class EditorSplineComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        /// Did the type (linear/Bezier/Catmull-Rom) of the spline change.
        virtual void OnSplineTypeChanged() {}

    protected:
        ~EditorSplineComponentNotifications() = default;
    };

    /// Type to inherit to provide EditorPolygonPrismShapeComponentRequests
    using EditorSplineComponentNotificationBus = AZ::EBus<EditorSplineComponentNotifications>;
} // namespace LmbrCentral
