/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AZ::SceneAPI::DataTypes::IMeshData::Face, "{F9F49C1A-014F-46F5-A46F-B56D8CB46C2B}");

    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = AZ::SceneAPI::DataTypes;

            void MeshData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MeshData>()->Version(1);
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneAPI::DataTypes::IMeshData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetUnitSizeInMeters", &MeshData::GetUnitSizeInMeters)
                        ->Method("GetOriginalUnitSizeInMeters", &MeshData::GetOriginalUnitSizeInMeters);

                    behaviorContext->Class<MeshData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetControlPointIndex", &MeshData::GetControlPointIndex)
                        ->Method("GetUsedControlPointCount", &MeshData::GetUsedControlPointCount)
                        ->Method("GetUsedPointIndexForControlPoint", &MeshData::GetUsedPointIndexForControlPoint)
                        ->Method("GetVertexCount", &MeshData::GetVertexCount)
                        ->Method("HasNormalData", &MeshData::HasNormalData)
                        ->Method("GetPosition", &MeshData::GetPosition)
                        ->Method("GetNormal", &MeshData::GetNormal)
                        ->Method("GetFaceCount", &MeshData::GetFaceCount)
                        ->Method("GetFaceInfo", &MeshData::GetFaceInfo)
                        ->Method("GetFaceMaterialId", &MeshData::GetFaceMaterialId)
                        ->Method("GetVertexIndex", &MeshData::GetVertexIndex);

                    behaviorContext->Class<SceneAPI::DataTypes::IMeshData::Face>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetVertexIndex", [](const SceneAPI::DataTypes::IMeshData::Face& self, int index)
                        {
                            if (index >= 0 && index < 3)
                            {
                                return self.vertexIndex[index];
                            }
                            return aznumeric_cast<unsigned int>(0);
                        });
                }
            }

            MeshData::~MeshData() = default;

            void MeshData::CloneAttributesFrom(const IGraphObject* sourceObject)
            {
                IMeshData::CloneAttributesFrom(sourceObject);
            }

            void MeshData::AddPosition(const AZ::Vector3& position)
            {
                m_positions.push_back(position);
            }

            void MeshData::AddNormal(const AZ::Vector3& normal)
            {
                m_normals.push_back(normal);
            }

            //assume consistent winding - no stripping or fanning expected (3 index per face)
            //indices can be used for position and normal
            void MeshData::AddFace(unsigned int index1, unsigned int index2, unsigned int index3, unsigned int faceMaterialId)
            {
                IMeshData::Face face;
                face.vertexIndex[0] = index1;
                face.vertexIndex[1] = index2;
                face.vertexIndex[2] = index3;
                m_faceList.push_back(face);
                m_faceMaterialIds.push_back(faceMaterialId);
            }

            void MeshData::AddFace(const DataTypes::IMeshData::Face& face, unsigned int faceMaterialId)
            {
                m_faceList.push_back(face);
                m_faceMaterialIds.push_back(faceMaterialId);
            }

            void MeshData::SetVertexIndexToControlPointIndexMap(int vertexIndex, int controlPointIndex)
            {
                m_vertexIndexToControlPointIndexMap[vertexIndex] = controlPointIndex;

                // The above hashmap stores the control point index (value) per vertex (key).
                // We construct an unordered set and fill in the control point indices in order to get access to the number of unique control points indices.
                if (m_controlPointToUsedVertexIndexMap.find(controlPointIndex) == m_controlPointToUsedVertexIndexMap.end())
                {
                    m_controlPointToUsedVertexIndexMap[controlPointIndex] = aznumeric_cast<unsigned int>(m_controlPointToUsedVertexIndexMap.size());
                }
            }

            int MeshData::GetControlPointIndex(int vertexIndex) const
            {
                AZ_Assert(m_vertexIndexToControlPointIndexMap.find(vertexIndex) != m_vertexIndexToControlPointIndexMap.end(), "Vertex index %i doesn't exist", vertexIndex);
                // Note: AZStd::unordered_map's operator [] doesn't have const version... 
                return m_vertexIndexToControlPointIndexMap.find(vertexIndex)->second;
            }

            size_t MeshData::GetUsedControlPointCount() const
            {
                return m_controlPointToUsedVertexIndexMap.size();
            }

            int MeshData::GetUsedPointIndexForControlPoint(int controlPointIndex) const
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

            unsigned int MeshData::GetVertexCount() const
            {
                return static_cast<unsigned int>(m_positions.size());
            }

            bool MeshData::HasNormalData() const
            {
                return m_normals.size() > 0;
            }

            const AZ::Vector3& MeshData::GetPosition(unsigned int index) const
            {
                AZ_Assert(index < m_positions.size(), "GetPosition index not in range");
                return m_positions[index];
            }

            const AZ::Vector3& MeshData::GetNormal(unsigned int index) const
            {
                AZ_Assert(index < m_normals.size(), "GetNormal index not in range");
                return m_normals[index];
            }

            unsigned int MeshData::GetFaceCount() const
            {
                return static_cast<unsigned int>(m_faceList.size());
            }

            const  DataTypes::IMeshData::Face& MeshData::GetFaceInfo(unsigned int index) const
            {
                AZ_Assert(index < m_faceList.size(), "GetFaceInfo index not in range");
                return m_faceList[index];
            }

            unsigned int MeshData::GetFaceMaterialId(unsigned int index) const
            {
                AZ_Assert(index < m_faceMaterialIds.size(), "GetFaceMaterialIds index not in range");
                return m_faceMaterialIds[index];
            }

            unsigned int MeshData::GetVertexIndex(int faceIndex, int vertexIndexInFace) const
            {
                AZ_Assert(faceIndex < m_faceList.size(), "GetFaceInfo index not in range");
                AZ_Assert(vertexIndexInFace < 3, "vertexIndexInFace index not in range");
                return m_faceList[faceIndex].vertexIndex[vertexIndexInFace];
            }

            void MeshData::GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("Positions", m_positions);
                int index = 0;
                for (const auto& position : m_positions)
                {
                    output.Write(AZStd::string::format("\t%d", index).c_str(), position);
                    ++index;
                }
                index = 0;
                output.Write("Normals", m_normals);
                for (const auto& normal : m_normals)
                {
                    output.Write(AZStd::string::format("\t%d", index).c_str(), normal);
                    ++index;
                }
                index = 0;
                output.Write("FaceList", m_faceList);
                for (const auto& face : m_faceList)
                {
                    output.WriteArray(AZStd::string::format("\t%d", index).c_str(), face.vertexIndex, 3);
                    ++index;
                }
                output.Write("FaceMaterialIds", m_faceMaterialIds);
            }
        }
    }
}
