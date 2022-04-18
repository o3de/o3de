/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace RecastNavigation
{
    class RecastNavigationSurveyorRequests
        : public AZ::ComponentBus
    {
    public:
        virtual void CollectGeometry(BoundedGeometry& geometryData) = 0;
        virtual AZ::Aabb GetWorldBounds() = 0;
    };

    using RecastNavigationSurveyorRequestBus = AZ::EBus<RecastNavigationSurveyorRequests>;

} // namespace RecastNavigation
