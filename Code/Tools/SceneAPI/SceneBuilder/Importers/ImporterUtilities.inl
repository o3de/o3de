/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            bool NodeIsOfType(const CoreSceneGraph& sceneGraph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid)
            {
                if (nodeIndex.IsValid() && sceneGraph.HasNodeContent(nodeIndex) &&
                    sceneGraph.GetNodeContent(nodeIndex)->RTTI_IsTypeOf(uuid))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            bool NodeParentIsOfType(const CoreSceneGraph& sceneGraph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid)
            {
                CoreGraphNodeIndex parentIndex = sceneGraph.GetNodeParent(nodeIndex);
                return NodeIsOfType(sceneGraph, parentIndex, uuid);
            }

            bool NodeHasAncestorOfType(const CoreSceneGraph& sceneGraph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid)
            {
                CoreGraphNodeIndex parentIndex = sceneGraph.GetNodeParent(nodeIndex);
                while (parentIndex.IsValid())
                {
                    if (NodeIsOfType(sceneGraph, parentIndex, uuid))
                    {
                        return true;
                    }
                    parentIndex = sceneGraph.GetNodeParent(parentIndex);
                }

                return false;
            }

            bool AreScenesEqual(const CoreScene& lhs, const CoreScene& rhs)
            {
                if (lhs.GetGraph().GetNodeCount() != rhs.GetGraph().GetNodeCount())
                {
                    return false;
                }

                if (!AreSceneGraphsEqual(lhs.GetGraph(), rhs.GetGraph()))
                {
                    return false;
                }

                return true;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
