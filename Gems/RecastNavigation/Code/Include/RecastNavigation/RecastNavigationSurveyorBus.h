/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <Components/RecastHelpers.h>

namespace RecastNavigation
{
    class RecastNavigationSurveyorRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize) = 0;
        virtual int GetNumberOfTiles([[maybe_unused]] float tileSize) const { return 1; }
        
        //! Returns the world bounds that this surveyor is responsible for.
        //! @return An axis aligned bounding box of the world bounds.
        virtual AZ::Aabb GetWorldBounds() const = 0;

        virtual bool IsTiled() const = 0;
    };

    using RecastNavigationSurveyorRequestBus = AZ::EBus<RecastNavigationSurveyorRequests>;

} // namespace RecastNavigation
