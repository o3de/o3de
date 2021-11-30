/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/bitset.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(SceneAPI::DataTypes::IBlendShapeData::Face, "{C972EC9A-3A5C-47CD-9A92-ECB4C0C0451C}");

    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = SceneAPI::DataTypes;

            BlendShapeData::~BlendShapeData() = default;

            void BlendShapeData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<BlendShapeData, SceneAPI::DataTypes::IBlendShapeData>()
                        ->Version(1);
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneAPI::DataTypes::IBlendShapeData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetUsedControlPointCount", &SceneAPI::DataTypes::IBlendShapeData::GetUsedControlPointCount)
                        ->Method("GetControlPointIndex", &SceneAPI::DataTypes::IBlendShapeData::GetControlPointIndex)
                        ->Method("GetUsedPointIndexForControlPoint", &SceneAPI::DataTypes::IBlendShapeData::GetUsedPointIndexForControlPoint)
                        ->Method("GetVertexCount", &SceneAPI::DataTypes::IBlendShapeData::GetVertexCount)
                        ->Method("GetFaceCount", &SceneAPI::DataTypes::IBlendShapeData::GetFaceCount)
                        ->Method("GetFaceInfo", &SceneAPI::DataTypes::IBlendShapeData::GetFaceInfo)
                        ->Method("GetPosition", &SceneAPI::DataTypes::IBlendShapeData::GetPosition)
                        ->Method("GetNormal", &SceneAPI::DataTypes::IBlendShapeData::GetNormal)
                        ->Method("GetFaceVertexIndex", &SceneAPI::DataTypes::IBlendShapeData::GetFaceVertexIndex);

                    behaviorContext->Class<SceneAPI::DataTypes::IBlendShapeData::Face>("BlendShapeDataFace")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetVertexIndex", [](const SceneAPI::DataTypes::IBlendShapeData::Face& self, int index)
                        {
                            if (index >= 0 && index < 3)
                            {
                                return self.vertexIndex[index];
                            }
                            return aznumeric_cast<unsigned int>(0);
                        });

                    behaviorContext->Class<BlendShapeData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetUV", &BlendShapeData::GetUV)
                        ->Method("GetTangent", [](const BlendShapeData& self, size_t index)
                        {
                            if (index < self.GetTangents().size())
                            {
                                return self.GetTangents().at(index);
                            }
                            AZ_Error("SceneGraphData", false, "Cannot get to tangent at index(%zu)", index);
                            return Vector4::CreateZero();
                        })
                        ->Method("GetBitangent", [](const BlendShapeData& self, size_t index)
                        {
                            if (index < self.GetBitangents().size())
                            {
                                return self.GetBitangents().at(index);
                            }
                            AZ_Error("SceneGraphData", false, "Cannot get to bitangents at index(%zu)", index);
                            return Vector3::CreateZero();
                        })
                        ->Method("GetColor", [](const BlendShapeData& self, AZ::u8 colorSetIndex, AZ::u8 colorIndex)
                        {
                            SceneAPI::DataTypes::Color color(0,0,0,0);
                            if (colorSetIndex < MaxNumColorSets)
                            {
                                const AZStd::vector<SceneAPI::DataTypes::Color>& colorChannel = self.GetColors(colorSetIndex);
                                if (colorIndex < colorChannel.size())
                                {
                                    return colorChannel[colorIndex];
                                }
                            }
                            AZ_Error("SceneGraphData", false, "Cannot get to color setIndex(%d) at colorIndex(%d)", colorSetIndex, colorIndex);
                            return color;
                        });
                }
            }

            void BlendShapeData::AddPosition(const Vector3& position)
            {
                m_positions.push_back(position);
            }

            void BlendShapeData::AddNormal(const Vector3& normal)
            {
                m_normals.push_back(normal);
            }

            void BlendShapeData::AddTangentAndBitangent(const Vector4& tangent, const Vector3& bitangent)
            {
                m_tangents.push_back(tangent);
                m_bitangents.push_back(bitangent);
            }

            void BlendShapeData::AddUV(const Vector2& uv, AZ::u8 uvSetIndex)
            {
                if (uvSetIndex >= MaxNumUVSets)
                {
                    AZ_ErrorOnce("SceneGraphData", false, "uvSetIndex %zu is greater or equal than the maximum uv sets %zu.", uvSetIndex, MaxNumUVSets);
                    return;
                }
                m_uvs[uvSetIndex].push_back(uv);
            }

            void BlendShapeData::AddColor(const SceneAPI::DataTypes::Color& color, AZ::u8 colorSetIndex)
            {
                if (colorSetIndex >= MaxNumColorSets)
                {
                    AZ_ErrorOnce("SceneGraphData", false, "colorSetIndex %zu is greater or equal than the maximum color sets %zu.", colorSetIndex, MaxNumColorSets);
                    return;
                }
                m_colors[colorSetIndex].push_back(color);
            }

            void BlendShapeData::ReserveData(
                unsigned int numVertices, bool reserveTangents, const AZStd::bitset<MaxNumUVSets>& uvSetUsedFlags,
                const AZStd::bitset<MaxNumColorSets>& colorSetUsedFlags)
            {
                m_positions.reserve(numVertices);
                m_normals.reserve(numVertices);
                if (reserveTangents)
                {
                    m_tangents.reserve(numVertices);
                    m_bitangents.reserve(numVertices);
                }

                for (AZ::u8 uvSetIndex = 0; uvSetIndex < MaxNumUVSets; ++uvSetIndex)
                {
                    if (uvSetUsedFlags[uvSetIndex])
                    {
                        m_uvs[uvSetIndex].reserve(numVertices);
                    }
                }

                for (AZ::u8 colorSetIndex = 0; colorSetIndex < MaxNumColorSets; ++colorSetIndex)
                {
                    if (colorSetUsedFlags[colorSetIndex])
                    {
                        m_colors[colorSetIndex].reserve(numVertices);
                    }
                }
            }

            void BlendShapeData::AddFace(const Face& face)
            {
                m_faces.push_back(face);
            }

            void BlendShapeData::SetVertexIndexToControlPointIndexMap(int vertexIndex, int controlPointIndex)
            {
                m_vertexIndexToControlPointIndexMap[vertexIndex] = controlPointIndex;

                // The above hashmap stores the control point index (value) per vertex (key).
                // We construct an unordered set and fill in the control point indices in order to get access to the number of unique control points indices.
                m_controlPointToUsedVertexIndexMap.emplace(controlPointIndex, aznumeric_cast<unsigned int>(m_controlPointToUsedVertexIndexMap.size()));
            }

            int BlendShapeData::GetControlPointIndex(int vertexIndex) const
            {
                auto iter = m_vertexIndexToControlPointIndexMap.find(vertexIndex);
                AZ_Assert(iter != m_vertexIndexToControlPointIndexMap.end(), "Vertex index %i doesn't exist.", vertexIndex);
                // Note: AZStd::unordered_map's operator [] doesn't have const version... 
                return iter->second;
            }

            size_t BlendShapeData::GetUsedControlPointCount() const
            {
                return m_controlPointToUsedVertexIndexMap.size();
            }

            int BlendShapeData::GetUsedPointIndexForControlPoint(int controlPointIndex) const
            {
                auto iter = m_controlPointToUsedVertexIndexMap.find(controlPointIndex);
                if (iter != m_controlPointToUsedVertexIndexMap.end())
                {
                    return iter->second;
                }
                else
                {
                    return -1; // That control point is not used in this mesh
                }
            }

            unsigned int BlendShapeData::GetVertexCount() const
            {
                return static_cast<unsigned int>(m_positions.size());
            }

            unsigned int BlendShapeData::GetFaceCount() const
            {
                return static_cast<unsigned int>(m_faces.size());
            }

            const BlendShapeData::Face& BlendShapeData::GetFaceInfo(unsigned int index) const
            {
                AZ_Assert(index < m_faces.size(), "GetFaceInfo index not in range");
                return m_faces[index];
            }

            const Vector3& BlendShapeData::GetPosition(unsigned int index) const
            {
                AZ_Assert(index < m_positions.size(), "GetPosition index not in range");
                return m_positions[index];
            }

            const Vector3& BlendShapeData::GetNormal(unsigned int index) const
            {
                AZ_Assert(index < m_normals.size(), "GetNormal index not in range");
                return m_normals[index];
            }

            const Vector2& BlendShapeData::GetUV(unsigned int vertexIndex, unsigned int uvSetIndex) const
            {
                AZ_Assert(uvSetIndex < MaxNumUVSets, "uvSet index out of range");
                AZ_Assert(vertexIndex < m_uvs[uvSetIndex].size(), "uvSet index out of range");
                return m_uvs[uvSetIndex][vertexIndex];
            }

            AZStd::vector<Vector4>& BlendShapeData::GetTangents()
            {
                return m_tangents;
            }

            const AZStd::vector<Vector4>& BlendShapeData::GetTangents() const
            {
                return m_tangents;
            }

            AZStd::vector<Vector3>& BlendShapeData::GetBitangents()
            {
                return m_bitangents;
            }

            const AZStd::vector<Vector3>& BlendShapeData::GetBitangents() const
            {
                return m_bitangents;
            }

            const AZStd::vector<Vector2>& BlendShapeData::GetUVs(AZ::u8 uvSetIndex) const
            {
                AZ_Assert(uvSetIndex < MaxNumUVSets, "uvSet index out of range");
                return m_uvs[uvSetIndex];
            }

            const AZStd::vector<SceneAPI::DataTypes::Color>& BlendShapeData::GetColors(AZ::u8 colorSetIndex) const
            {
                AZ_Assert(colorSetIndex < MaxNumColorSets, "colorSet index out of range");
                return m_colors[colorSetIndex];
            }

            unsigned int BlendShapeData::GetFaceVertexIndex(unsigned int face, unsigned int vertexIndex) const
            {
                AZ_Assert(face < m_faces.size(), "GetFaceVertexPositionIndex face index not in range");
                AZ_Assert(vertexIndex < 3, "GetFaceVertexPositionIndex vertexIndex index not in range");
                return m_faces[face].vertexIndex[vertexIndex];
            }

            void BlendShapeData::GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("Positions", m_positions);
                output.Write("Normals", m_normals);
                output.Write("Faces", m_faces);
            }
        } // GraphData
    } // SceneData
} // AZ
