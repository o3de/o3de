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
        }
        namespace Utilities
        {
            // Searches for any entries in the scene graph that are derived or match the given type.
            //      If "checkVirtualTypes" is true, a matching entry is also checked if it's not a
            //      virtual type.
            template<typename T>
            bool DoesSceneGraphContainDataLike(const Containers::Scene& scene, bool checkVirtualTypes);
            SCENE_CORE_API DataTypes::MatrixType BuildWorldTransform(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex nodeIndex);
            // Searches only the immediate children of nodeIndex for a child node that matches the specified type.
            // Returns the NodeIndex if it exists, or and invalid NodeIndex if it doesn't
            SCENE_CORE_API Containers::SceneGraph::NodeIndex GetImmediateChildOfType(
                const Containers::SceneGraph& graph, const Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::TypeId& typeId);
        } // Utilities
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.inl>
