/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
