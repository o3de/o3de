/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/MeshInfo.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshInfo, EMStudio::UIAllocator)

    MeshInfo::MeshInfo(EMotionFX::Actor* actor, [[maybe_unused]] EMotionFX::Node* node, size_t lodLevel, EMotionFX::Mesh* mesh)
        : m_lod(lodLevel)
    {
        // vertices, indices and polygons etc.
        m_verticesCount = mesh->GetNumVertices();
        m_indicesCount = mesh->GetNumIndices();
        m_polygonsCount = mesh->GetNumPolygons();
        m_isTriangleMesh = mesh->CheckIfIsTriangleMesh();
        m_isQuadMesh = mesh->CheckIfIsQuadMesh();
        m_orgVerticesCount = mesh->GetNumOrgVertices();

        if (m_orgVerticesCount)
        {
            m_vertexDupeRatio = (float)mesh->GetNumVertices() / (float)mesh->GetNumOrgVertices();
        }
        else
        {
            m_vertexDupeRatio = 0.0f;
        }

        // skinning influences
        mesh->CalcMaxNumInfluences(m_verticesByInfluences);
        
        // sub meshes
        const size_t numSubMeshes = mesh->GetNumSubMeshes();
        for (size_t i = 0; i < numSubMeshes; ++i)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(i);
            m_submeshes.emplace_back(actor, lodLevel, subMesh);
        }

        // vertex attribute layers
        const size_t numVertexAttributeLayers = mesh->GetNumVertexAttributeLayers();
        AZStd::string tmpString;
        for (uint32 i = 0; i < numVertexAttributeLayers; ++i)
        {
            EMotionFX::VertexAttributeLayer* attributeLayer = mesh->GetVertexAttributeLayer(i);

            const uint32 attributeLayerType = attributeLayer->GetType();
            switch (attributeLayerType)
            {
            case EMotionFX::Mesh::ATTRIB_POSITIONS:
                tmpString = "Vertex positions";
                break;
            case EMotionFX::Mesh::ATTRIB_NORMALS:
                tmpString = "Vertex normals";
                break;
            case EMotionFX::Mesh::ATTRIB_TANGENTS:
                tmpString = "Vertex tangents";
                break;
            case EMotionFX::Mesh::ATTRIB_UVCOORDS:
                tmpString = "Vertex uv coordinates";
                break;
            case EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS:
                tmpString = "Original vertex numbers";
                break;
            case EMotionFX::Mesh::ATTRIB_BITANGENTS:
                tmpString = "Vertex bitangents";
                break;
            default:
                tmpString = AZStd::string::format("Unknown data (TypeID=%d)", attributeLayerType);
            }

            if (!attributeLayer->GetNameString().empty())
            {
                tmpString += AZStd::string::format(" [%s]", attributeLayer->GetName());
            }

            m_attributeLayers.emplace_back(attributeLayer->GetTypeString(), tmpString);
        }


        // shared vertex attribute layers
        const size_t numSharedVertexAttributeLayers = mesh->GetNumSharedVertexAttributeLayers();
        for (size_t i = 0; i < numSharedVertexAttributeLayers; ++i)
        {
            EMotionFX::VertexAttributeLayer* attributeLayer = mesh->GetSharedVertexAttributeLayer(i);

            const uint32 attributeLayerType = attributeLayer->GetType();
            switch (attributeLayerType)
            {
            case EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID:
                tmpString = "Skinning info";
                break;
            default:
                tmpString = AZStd::string::format("Unknown data (TypeID=%d)", attributeLayerType);
            }

            if (!attributeLayer->GetNameString().empty())
            {
                tmpString += AZStd::string::format(" [%s]", attributeLayer->GetName());
            }

            m_sharedAttributeLayers.emplace_back(attributeLayer->GetTypeString(), tmpString);
        }
    }

    void MeshInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MeshInfo>()
            ->Version(1)
            ->Field("lod", &MeshInfo::m_lod)
            ->Field("verticesCount", &MeshInfo::m_verticesCount)
            ->Field("indicesCount", &MeshInfo::m_indicesCount)
            ->Field("polygonsCount", &MeshInfo::m_polygonsCount)
            ->Field("isTriangleMesh", &MeshInfo::m_isTriangleMesh)
            ->Field("isQuadMesh", &MeshInfo::m_isQuadMesh)
            ->Field("orgVerticesCount", &MeshInfo::m_orgVerticesCount)
            ->Field("vertexDupeRatio", &MeshInfo::m_vertexDupeRatio)
            ->Field("verticesByInfluence", &MeshInfo::m_verticesByInfluences)
            ->Field("submeshes", &MeshInfo::m_submeshes)
            ->Field("attributeLayers", &MeshInfo::m_attributeLayers)
            ->Field("sharedAttributeLayers", &MeshInfo::m_sharedAttributeLayers)
           ;
        
        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MeshInfo>("Mesh info", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_lod, "LOD level", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_verticesCount, "Vertices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_indicesCount, "Indices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_polygonsCount, "Polygons", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_isTriangleMesh, "Is triangle mesh", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_isQuadMesh, "Is quad mesh", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_orgVerticesCount, "Org vertices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_vertexDupeRatio, "Vertex dupe ratio", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_verticesByInfluences, "Vertices by influence", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_submeshes, "Sub meshes", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_attributeLayers, "Attribute layers", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_sharedAttributeLayers, "Shared attribute layers", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ;

    }

} // namespace EMStudio
