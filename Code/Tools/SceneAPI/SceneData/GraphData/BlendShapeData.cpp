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

#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = SceneAPI::DataTypes;

            BlendShapeData::~BlendShapeData() = default;

            unsigned int BlendShapeData::AddVertex(const Vector3& position, const Vector3& normal)
            {
                m_positions.push_back(position);
                m_normals.push_back(normal);
                return static_cast<unsigned int>(m_positions.size()-1);
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
                AZ_Assert(iter != m_vertexIndexToControlPointIndexMap.end(), "Vertex index %i doesn't exist", vertexIndex);
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
