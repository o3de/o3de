/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace Multiplayer
{
    namespace Physics
    {
        //! Performs rewind-aware ray cast in the default physics world.
        //! @param request The ray cast request to make.
        //! @return Returns a structure that contains a list of Hits.
        AzPhysics::SceneQueryHits RayCast(const AzPhysics::RayCastRequest& request);

        //! Performs rewind-aware shape cast in the default physics world.
        //! @param request The shape cast request to make.
        //! @return Returns a structure that contains a list of Hits.
        AzPhysics::SceneQueryHits ShapeCast(const AzPhysics::ShapeCastRequest& request);

        //! Performs rewind-aware overlap in the default physics world.
        //! @param request The overlap request to make.
        //! @return Returns a structure that contains a list of Hits.
        AzPhysics::SceneQueryHits Overlap(const AzPhysics::OverlapRequest& request);

    } // namespace Physics
} // namespace Multiplayer
