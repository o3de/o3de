/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>


namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS MeshData
                : public AZ::SceneAPI::DataTypes::IMeshData
            {
            public:
                AZ_RTTI(MeshData, "{a2589bd4-42fb-40ba-a38d-cfcd6e9ea169}", AZ::SceneAPI::DataTypes::IMeshData)

                static void Reflect(ReflectContext* context);

                SCENE_DATA_API ~MeshData() override;

                SCENE_DATA_API void CloneAttributesFrom(const IGraphObject* sourceObject) override;

                //assumes 1 to 1 mapping for these position, normal, color, uv
                //positions with more than one normal or uv (seam) will duplicate shared values in multiple verts
                SCENE_DATA_API virtual void AddPosition(const AZ::Vector3& position);
                SCENE_DATA_API virtual void AddNormal(const AZ::Vector3& normal);

                //assume consistent winding - no stripping or fanning expected (3 index per face)
                SCENE_DATA_API void AddFace(unsigned int index1, unsigned int index2, unsigned int index3,
                    unsigned int faceMaterialId = AZ::SceneAPI::DataTypes::IMeshData::s_invalidMaterialId);
                SCENE_DATA_API void AddFace(const AZ::SceneAPI::DataTypes::IMeshData::Face& face,
                    unsigned int faceMaterialId = AZ::SceneAPI::DataTypes::IMeshData::s_invalidMaterialId);

                SCENE_DATA_API void SetVertexIndexToControlPointIndexMap(int vertexIndex, int controlPointIndex);
                SCENE_DATA_API size_t GetUsedControlPointCount() const override;
                SCENE_DATA_API int GetControlPointIndex(int vertexIndex) const override;
                SCENE_DATA_API int GetUsedPointIndexForControlPoint(int controlPointIndex) const override;

                SCENE_DATA_API unsigned int GetVertexCount() const override;
                SCENE_DATA_API bool HasNormalData() const override;

                SCENE_DATA_API const AZ::Vector3& GetPosition(unsigned int index) const override;
                SCENE_DATA_API const AZ::Vector3& GetNormal(unsigned int index) const override;
                SCENE_DATA_API void SetNormal(unsigned int index, const AZ::Vector3& normal) override;

                SCENE_DATA_API unsigned int GetFaceCount() const override;
                SCENE_DATA_API const AZ::SceneAPI::DataTypes::IMeshData::Face& GetFaceInfo(unsigned int index) const override;
                SCENE_DATA_API unsigned int GetFaceMaterialId(unsigned int index) const override;

                SCENE_DATA_API unsigned int GetVertexIndex(int faceIndex, int vertexIndexInFace) const override;

                SCENE_DATA_API void GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZ::Vector3>                              m_positions;
                AZStd::vector<AZ::Vector3>                              m_normals;

                AZStd::vector<AZ::SceneAPI::DataTypes::IMeshData::Face> m_faceList;

                AZStd::vector<unsigned int>                             m_faceMaterialIds;

                AZStd::unordered_map<int, int>                          m_vertexIndexToControlPointIndexMap;
                AZStd::unordered_map<int, int>                          m_controlPointToUsedVertexIndexMap;
            };
        }
    }
}
