#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
            class RuleContainer;
        }
        namespace Utilities
        {
            SCENE_CORE_API DataTypes::MatrixType DetermineWorldTransform(
                const Containers::Scene& scene,
                const Containers::SceneGraph::NodeIndex nodeIndex,
                const Containers::RuleContainer& ruleContainer);
        } // Utilities
    } // SceneAPI
} // AZ
