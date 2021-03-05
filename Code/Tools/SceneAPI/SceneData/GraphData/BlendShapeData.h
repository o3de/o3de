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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS BlendShapeData
                : public SceneAPI::DataTypes::IBlendShapeData
            {
            public:
                AZ_RTTI(BlendShapeData, "{FF875C22-2E4F-4CE3-BA49-09BF78C70A09}", SceneAPI::DataTypes::IBlendShapeData)

                SCENE_DATA_API ~BlendShapeData() override;
                SCENE_DATA_API virtual unsigned int AddVertex(const Vector3& position, const Vector3& normal);

                //assume consistent winding - no stripping or fanning expected (3 index per face)
                SCENE_DATA_API virtual void AddFace(const Face& face);

                SCENE_DATA_API void SetVertexIndexToControlPointIndexMap(int vertexIndex, int controlPointIndex);

                SCENE_DATA_API size_t GetUsedControlPointCount() const override;
                SCENE_DATA_API int GetControlPointIndex(int vertexIndex) const override;
                SCENE_DATA_API int GetUsedPointIndexForControlPoint(int controlPointIndex) const override;

                //assume consistent winding - no stripping or fanning expected (3 index per face)
                SCENE_DATA_API unsigned int GetVertexCount() const override;
                SCENE_DATA_API unsigned int GetFaceCount() const override;

                SCENE_DATA_API const Vector3& GetPosition(unsigned int index) const override;
                SCENE_DATA_API const Vector3& GetNormal(unsigned int index) const override;

                SCENE_DATA_API unsigned int GetFaceVertexIndex(unsigned int face, unsigned int vertexIndex) const override;

                SCENE_DATA_API void GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const override;

            protected:
                AZStd::vector<Vector3>  m_positions;
                AZStd::vector<Vector3>  m_normals;
                AZStd::vector<Face>     m_faces;

                AZStd::unordered_map<int, int>                          m_vertexIndexToControlPointIndexMap;
                AZStd::unordered_map<int, int>                          m_controlPointToUsedVertexIndexMap;
            };
        } // GraphData
    } // SceneData
} // AZ
