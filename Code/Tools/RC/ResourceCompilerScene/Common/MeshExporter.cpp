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

#include <Cry_Geo.h>
#include <IIndexedMesh.h>
#include <CGFContent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/MeshExporter.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;

        MeshExporter::MeshExporter()
        {
            BindToCall(&MeshExporter::ProcessMesh);
        }

        void MeshExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MeshExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult MeshExporter::ProcessMesh(NodeExportContext& context) const
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData = 
                azrtti_cast<const SceneDataTypes::IMeshData*>(graph.GetNodeContent(context.m_nodeIndex));
            if (meshData)
            {
                SceneEvents::ProcessingResultCombiner result;

                CMesh* mesh = new CMesh();
                result += SceneEvents::Process<MeshNodeExportContext>(context, *mesh, Phase::Construction);

                MeshNodeExportContext meshNodeContextFilling(context, *mesh, Phase::Filling);
                SetMeshFaces(*meshData, *mesh, context.m_physicalizeType);
                if (!SetMeshVertices(*meshData, *mesh))
                {
                    return SceneEvents::ProcessingResult::Failure;
                }
                if (!SetMeshNormals(*meshData, *mesh))
                {
                    return SceneEvents::ProcessingResult::Failure;
                }
                SetMeshTopologyIds(*meshData, *mesh, context);
                context.m_node.type = CNodeCGF::NODE_MESH;
                context.m_node.pMesh = mesh;
                result += SceneEvents::Process(meshNodeContextFilling);

                MeshNodeExportContext meshNodeContextFinalizing(context, *mesh, Phase::Finalizing);
                context.m_container.GetExportInfo()->bNoMesh = false;
                result += SceneEvents::Process(meshNodeContextFinalizing);
                
                return result.GetResult();
            }
            else
            {
                return SceneEvents::ProcessingResult::Ignored;
            }
        }

        void MeshExporter::SetMeshFaces(const SceneDataTypes::IMeshData& meshData, CMesh& mesh, EPhysicsGeomType physicalizeType) const
        {
            if (meshData.GetFaceCount() == 0)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "No mesh faces specified.");
                return;
            }

            mesh.ReallocStream(CMesh::FACES, 0, meshData.GetFaceCount());
            for (uint32_t i = 0; i < meshData.GetFaceCount(); ++i)
            {
                const SceneDataTypes::IMeshData::Face& face = meshData.GetFaceInfo(i);
                mesh.m_pFaces[i].v[0] = face.vertexIndex[0];
                mesh.m_pFaces[i].v[1] = face.vertexIndex[1];
                mesh.m_pFaces[i].v[2] = face.vertexIndex[2];

                // Create and use a unified subset if the mesh is chosen to be physicalized
                if (physicalizeType == PHYS_GEOM_TYPE_DEFAULT_PROXY ||
                    physicalizeType == PHYS_GEOM_TYPE_OBSTRUCT ||
                    physicalizeType == PHYS_GEOM_TYPE_NO_COLLIDE)
                {
                    mesh.m_pFaces[i].nSubset = 0;
                    if (mesh.m_subsets.empty())
                    {
                        SMeshSubset meshSubset;
                        meshSubset.nMatID = 0;
                        mesh.m_subsets.push_back(meshSubset);
                    }
                }
                else
                {
                    int materialIndex = meshData.GetFaceMaterialId(i);
                    mesh.m_pFaces[i].nSubset = materialIndex;

                    while (mesh.m_subsets.size() <= materialIndex)
                    {
                        SMeshSubset meshSubset;
                        meshSubset.nMatID = mesh.m_subsets.size();
                        mesh.m_subsets.push_back(meshSubset);
                    }
                }
            }
        }

        bool MeshExporter::SetMeshVertices(const SceneDataTypes::IMeshData& meshData, CMesh& mesh) const
        {
            mesh.ReallocStream(CMesh::POSITIONS, 0, meshData.GetVertexCount());
            for (uint32_t i = 0; i < meshData.GetVertexCount(); ++i)
            {
                const Vector3& position = meshData.GetPosition(i);
                if (!position.IsFinite())
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Invalid vertex data detected at index %d", i);
                    return false;
                }
                mesh.m_pPositions[i] = Vec3(position.GetX(), position.GetY(), position.GetZ());
            }
            return true;
        }

        bool MeshExporter::SetMeshNormals(const SceneDataTypes::IMeshData& meshData, CMesh& mesh) const
        {
            // Mesh requires normals. If they're missing add a stream of default normals.
            mesh.ReallocStream(CMesh::NORMALS, 0, meshData.GetVertexCount());
            if (meshData.HasNormalData())
            {
                for (int i = 0; i < meshData.GetVertexCount(); ++i)
                {
                    const Vector3& normal = meshData.GetNormal(i);
                    if (!normal.IsFinite())
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Invalid normal data detected at index %d", i);
                        return false;
                    }
                    mesh.m_pNorms[i] = SMeshNormal(Vec3(normal.GetX(), normal.GetY(), normal.GetZ()));
                }
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "No mesh normals detected. Adding default normals.");
                static const SMeshNormal defaultNormal(Vec3(1.0f, 0.0f, 0.0f));
                for (int i = 0; i < meshData.GetVertexCount(); ++i)
                {
                    mesh.m_pNorms[i] = defaultNormal;
                }
            }
            return true;
        }

        void MeshExporter::SetMeshTopologyIds(const SceneAPI::DataTypes::IMeshData& meshData, CMesh& mesh, NodeExportContext& context) const
        {
            // Note, If this is a skin mesh create dummy topology id data even though it seems to be unnecessary data. 
            // Currently just provide it to prevent crash during skin mesh processing due to data misalignments.
            if (context.m_group.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::ISkinGroup::TYPEINFO_Uuid()))
            {
                mesh.ReallocStream(CMesh::TOPOLOGY_IDS, 0, meshData.GetVertexCount());
            }
        }
    } // namespace RC
} // namespace AZ
