#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
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

                static void Reflect(ReflectContext* context);

                // Maximum number of color sets matches limitation set in assImp (AI_MAX_NUMBER_OF_COLOR_SETS)
                static constexpr AZ::u8 MaxNumColorSets = 8;
                // Maximum number of uv sets matches limitation set in assImp (AI_MAX_NUMBER_OF_TEXTURECOORDS)
                static constexpr AZ::u8 MaxNumUVSets = 8;

                SCENE_DATA_API ~BlendShapeData() override;
                SCENE_DATA_API void AddPosition(const AZ::Vector3& position);
                SCENE_DATA_API void AddNormal(const AZ::Vector3& normal);
                SCENE_DATA_API void AddTangentAndBitangent(const Vector4& tangent, const Vector3& bitangent);
                SCENE_DATA_API void AddUV(const Vector2& uv, AZ::u8 uvSetIndex);
                SCENE_DATA_API void AddColor(const SceneAPI::DataTypes::Color& color, AZ::u8 colorSetIndex);
                SCENE_DATA_API void ReserveData(
                    unsigned int numVertices, bool reserveTangents, const AZStd::bitset<MaxNumUVSets>& uvSetUsedFlags,
                    const AZStd::bitset<MaxNumColorSets>& colorSetUsedFlags);

                //assume consistent winding - no stripping or fanning expected (3 index per face)
                SCENE_DATA_API virtual void AddFace(const Face& face);

                SCENE_DATA_API void SetVertexIndexToControlPointIndexMap(int vertexIndex, int controlPointIndex);

                SCENE_DATA_API size_t GetUsedControlPointCount() const override;
                SCENE_DATA_API int GetControlPointIndex(int vertexIndex) const override;
                SCENE_DATA_API int GetUsedPointIndexForControlPoint(int controlPointIndex) const override;

                //assume consistent winding - no stripping or fanning expected (3 index per face)
                SCENE_DATA_API unsigned int GetVertexCount() const override;
                SCENE_DATA_API unsigned int GetFaceCount() const override;
                SCENE_DATA_API const Face& GetFaceInfo(unsigned int index) const override;

                SCENE_DATA_API const Vector3& GetPosition(unsigned int index) const override;
                SCENE_DATA_API const Vector3& GetNormal(unsigned int index) const override;
                SCENE_DATA_API void SetNormal(unsigned int index, const Vector3&) override;
                SCENE_DATA_API const Vector2& GetUV(unsigned int vertexIndex, unsigned int uvSetIndex) const;

                SCENE_DATA_API AZStd::vector<Vector4>& GetTangents();
                SCENE_DATA_API const AZStd::vector<Vector4>& GetTangents() const;
                SCENE_DATA_API AZStd::vector<Vector3>& GetBitangents();
                SCENE_DATA_API const AZStd::vector<Vector3>& GetBitangents() const;
                SCENE_DATA_API const AZStd::vector<Vector2>& GetUVs(AZ::u8 uvSetIndex) const;
                SCENE_DATA_API const AZStd::vector<SceneAPI::DataTypes::Color>& GetColors(AZ::u8 colorSetIndex) const;

                SCENE_DATA_API unsigned int GetFaceVertexIndex(unsigned int face, unsigned int vertexIndex) const override;

                SCENE_DATA_API void GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const override;

            protected:
                AZStd::vector<Vector3>  m_positions;
                AZStd::vector<Vector3>  m_normals;
                AZStd::vector<Vector4>  m_tangents;
                AZStd::vector<Vector3>  m_bitangents;
                AZStd::vector<AZ::Vector2> m_uvs[MaxNumUVSets];
                AZStd::vector<SceneAPI::DataTypes::Color> m_colors[MaxNumColorSets];

                AZStd::vector<Face>     m_faces;

                AZStd::unordered_map<int, int>                          m_vertexIndexToControlPointIndexMap;
                AZStd::unordered_map<int, int>                          m_controlPointToUsedVertexIndexMap;
            };
        } // GraphData
    } // SceneData
} // AZ
