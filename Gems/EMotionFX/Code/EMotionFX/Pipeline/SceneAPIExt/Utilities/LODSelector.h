/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                // Use this function to select LOD bones in certain LOD level. Note: LOD Rule Index is different than LOD Level because LOD 0 doesn't belongs to LOD Rule.
                // LOD Level = LOD Rule Index + 1.
                // SelectBaseBones: Select all bones in base LOD if doesn't find any bones in a specific LOD.
                static size_t SelectLODBones(const SceneContainers::Scene& scene, SceneDataTypes::ISceneNodeSelectionList& selection, size_t lodRuleIndex, bool selectBaseBones = false);

                // Use this function to find the root index of a LOD Level. For now, we are telling the user to follow a certain pattern in there LOD Group authorization. The root index of
                // LOD Level x should have name "LOD_x".
                static SceneContainers::SceneGraph::NodeIndex FindLODRootIndex(const SceneContainers::Scene& scene, size_t lodLevel);

                // Use this function to find the root path of a LOD Level. See FindLODRootIndex() for more details.
                static const char* FindLODRootPath(const SceneContainers::Scene& scene, size_t lodLevel);
            };
        }
    }
}
