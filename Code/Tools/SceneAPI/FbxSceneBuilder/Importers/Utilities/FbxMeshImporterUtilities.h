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

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
    }

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
        class FbxSceneSystem;
        namespace FbxSceneBuilder
        {
            bool BuildSceneMeshFromFbxMesh(const AZStd::shared_ptr<SceneData::GraphData::MeshData>& mesh,
                const FbxSDKWrapper::FbxMeshWrapper& sourceMesh, const FbxSceneSystem& sceneSystem);
            bool BuildSceneBlendShapeFromFbxBlendShape(const AZStd::shared_ptr<SceneData::GraphData::BlendShapeData>& blendShape,
                const AZStd::shared_ptr<const FbxSDKWrapper::FbxMeshWrapper>& sourceMesh, const FbxSceneSystem& sceneSystem);
        }
    }
}