/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

struct aiNode;
struct aiScene;

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MeshData;
            class BlendShapeData;
        }
    }

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGraphObject;
        }
        struct AssImpSceneNodeAppendedContext;
        class SceneSystem;

        namespace SceneBuilder
        {
            bool BuildSceneMeshFromAssImpMesh(const aiNode* currentNode, const aiScene* scene, const SceneSystem& sceneSystem, AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>>& meshes,
                const AZStd::function<AZStd::shared_ptr<AZ::SceneData::GraphData::MeshData>()>& makeMeshFunc);

            typedef AZ::Outcome<const AZ::SceneData::GraphData::MeshData*, Events::ProcessingResult> GetMeshDataFromParentResult;
            GetMeshDataFromParentResult GetMeshDataFromParent(AssImpSceneNodeAppendedContext& context);

            // If a node in the original scene file has a mesh with multiple materials on it, the associated AssImp
            // node will have multiple meshes on it, broken apart per material. This returns the total number
            // of vertices on all meshes on the given node.
            uint64_t GetVertexCountForAllMeshesOnNode(const aiNode& node, const aiScene& scene);
        }
    }
}
