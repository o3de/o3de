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

#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <AzSceneDef.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Utilities
        {
            class LODSelector
            {
            public:
                // Use this function to select LOD meshes in certain LOD level. Note: LOD Rule Index is different than LOD Level because LOD 0 doesn't belongs to LOD Rule.
                // LOD Level = LOD Rule Index + 1.
                static size_t SelectLODMeshes(const SceneContainers::Scene& scene, SceneDataTypes::ISceneNodeSelectionList& selection, size_t lodRuleIndex);

                // Use this function to select LOD bones in certain LOD level.
                static void SelectLODBones(const SceneContainers::Scene& scene, SceneDataTypes::ISceneNodeSelectionList& selection, size_t lodRuleIndex);

                // Use this function to select LOD meshes and bones in certain LOD level.
                static size_t SelectLODNodes(const SceneContainers::Scene& scene, SceneDataTypes::ISceneNodeSelectionList& selection, size_t lodRuleIndex);

                // Use this function to find the root index of a LOD Level. For now, we are telling the user to follow a certain pattern in there LOD Group authorization. The root index of
                // LOD Level x should have name "LOD_x".
                static SceneContainers::SceneGraph::NodeIndex FindLODRootIndex(const SceneContainers::Scene& scene, size_t lodLevel);

                // Use this function to find the root path of a LOD Level. See FindLODRootIndex() for more details.
                static const char* FindLODRootPath(const SceneContainers::Scene& scene, size_t lodLevel);
            };
        }
    }
}