#pragma once

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
