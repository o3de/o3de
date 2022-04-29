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
        virtual void BindGeometryCollectionEventHandler(AZ::Event<AZStd::shared_ptr<BoundedGeometry>>::Handler& handler) = 0;

        virtual void StartCollectingGeometry(float tileSize, float cellSize) = 0;
        virtual int GetNumberOfTiles([[maybe_unused]] float tileSize, [[maybe_unused]]float cellSize) const { return 1; }
        
        //! Returns the world bounds that this surveyor is responsible for.
        //! @return An axis aligned bounding box of the world bounds.
        virtual AZ::Aabb GetWorldBounds() const = 0;
    };

    using RecastNavigationSurveyorRequestBus = AZ::EBus<RecastNavigationSurveyorRequests>;

} // namespace RecastNavigation
