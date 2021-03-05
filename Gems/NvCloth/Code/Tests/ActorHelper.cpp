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

#include <ActorHelper.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/MeshBuilder.h>
#include <EMotionFX/Source/Mesh.h>
#include <Integration/Assets/ActorAsset.h>

#include <NvCloth/ITangentSpaceHelper.h>

namespace UnitTest
{
    ActorHelper::ActorHelper(const char* name)
        : EMotionFX::Actor(name)
    {
    }

    AZ::u32 ActorHelper::AddJoint(
        const AZStd::string& name,
        const AZ::Transform localTransform,
        const AZStd::string& parentName)
    {
        EMotionFX::Node* parentNode = GetSkeleton()->FindNodeByNameNoCase(parentName.c_str());

        auto node = AddNode(
            GetNumNodes(),
            name.c_str(),
            (parentNode) ? parentNode->GetNodeIndex() : MCORE_INVALIDINDEX32);

        GetBindPose()->SetLocalSpaceTransform(node->GetNodeIndex(), localTransform);

        return node->GetNodeIndex();
    }

    void ActorHelper::AddClothCollider(const Physics::CharacterColliderNodeConfiguration& colliderNode)
    {
        GetPhysicsSetup()->GetConfig().m_clothConfig.m_nodes.push_back(colliderNode);
    }

    void ActorHelper::FinishSetup()
    {
        SetID(0);
        GetSkeleton()->UpdateNodeIndexValues(0);
        ResizeTransformData();
        PostCreateInit();
    }

    AZ::Data::Asset<AZ::Data::AssetData> CreateAssetFromActor(
        AZStd::unique_ptr<EMotionFX::Actor> actor)
    {
        AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom());
        AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset = AZ::Data::AssetManager::Instance().CreateAsset<EMotionFX::Integration::ActorAsset>(assetId);
        actorAsset.GetAs<EMotionFX::Integration::ActorAsset>()->SetData(AZStd::move(actor));
        return actorAsset;
    }

    Physics::CharacterColliderNodeConfiguration CreateSphereCollider(
        const AZStd::string& jointName,
        float radius,
        const AZ::Transform& offset)
    {
        auto colliderConf = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConf->m_position = offset.GetTranslation();
        colliderConf->m_rotation = offset.GetRotation();
        auto shapeConf = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);

        Physics::CharacterColliderNodeConfiguration collider;
        collider.m_name = jointName;
        collider.m_shapes.emplace_back(colliderConf, shapeConf);

        return collider;
    }

    Physics::CharacterColliderNodeConfiguration CreateCapsuleCollider(
        const AZStd::string& jointName,
        float height, float radius,
        const AZ::Transform& offset)
    {
        auto colliderConf = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConf->m_position = offset.GetTranslation();
        colliderConf->m_rotation = offset.GetRotation();
        auto shapeConf = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);

        Physics::CharacterColliderNodeConfiguration collider;
        collider.m_name = jointName;
        collider.m_shapes.emplace_back(colliderConf, shapeConf);

        return collider;
    }

    Physics::CharacterColliderNodeConfiguration CreateBoxCollider(
        const AZStd::string& jointName,
        const AZ::Vector3& dimensions,
        const AZ::Transform& offset)
    {
        auto colliderConf = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConf->m_position = offset.GetTranslation();
        colliderConf->m_rotation = offset.GetRotation();
        auto shapeConf = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);

        Physics::CharacterColliderNodeConfiguration collider;
        collider.m_name = jointName;
        collider.m_shapes.emplace_back(colliderConf, shapeConf);

        return collider;
    }

    EMotionFX::Mesh* CreateEMotionFXMesh(
        AZ::u32 nodeIndex,
        const AZStd::vector<AZ::Vector3>& vertices,
        const AZStd::vector<AZ::u32>& indices,
        const AZStd::vector<VertexSkinInfluences>& skinningInfo,
        const AZStd::vector<AZ::Vector2>& uvs,
        const AZStd::vector<AZ::Color>& clothData)
    {
        const AZ::u32 vertCount = aznumeric_cast<AZ::u32>(vertices.size());
        const AZ::u32 faceCount = aznumeric_cast<AZ::u32>(indices.size()) / 3;

        AZStd::unique_ptr<EMotionFX::MeshBuilder> meshBuilder = AZStd::make_unique<EMotionFX::MeshBuilder>(nodeIndex, vertCount);

        // Skinning info
        if (!skinningInfo.empty() && skinningInfo.size() == vertices.size())
        {
            EMotionFX::MeshBuilderSkinningInfo* skinningInfoBuilder = EMotionFX::MeshBuilderSkinningInfo::Create(vertCount);
            for (size_t vertex = 0; vertex < skinningInfo.size(); ++vertex)
            {
                for (const auto& influence : skinningInfo[vertex])
                {
                    skinningInfoBuilder->AddInfluence(static_cast<AZ::u32>(vertex), influence);
                }
            }
            skinningInfoBuilder->Optimize();

            meshBuilder->SetSkinningInfo(skinningInfoBuilder);
        }

        // Original vertex numbers
        EMotionFX::MeshBuilderVertexAttributeLayerUInt32* orgVtxLayer = EMotionFX::MeshBuilderVertexAttributeLayerUInt32::Create(
            vertCount,
            EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS,
            false,
            false
        );
        meshBuilder->AddLayer(orgVtxLayer);

        // The positions layer
        EMotionFX::MeshBuilderVertexAttributeLayerVector3* posLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(
            vertCount,
            EMotionFX::Mesh::ATTRIB_POSITIONS,
            false,
            true
        );
        meshBuilder->AddLayer(posLayer);

        // The normals layer
        EMotionFX::MeshBuilderVertexAttributeLayerVector3* normalsLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(
            vertCount,
            EMotionFX::Mesh::ATTRIB_NORMALS,
            false,
            true
        );
        meshBuilder->AddLayer(normalsLayer);

        // The UVs layer.
        EMotionFX::MeshBuilderVertexAttributeLayerVector2* uvsLayer = nullptr;
        if (!uvs.empty() && uvs.size() == vertices.size())
        {
            uvsLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector2::Create(vertCount, EMotionFX::Mesh::ATTRIB_UVCOORDS, false, false);
            meshBuilder->AddLayer(uvsLayer);
        }

        // The cloth layer.
        EMotionFX::MeshBuilderVertexAttributeLayerUInt32* clothLayer = nullptr;
        if (!clothData.empty() && clothData.size() == vertices.size())
        {
            clothLayer = EMotionFX::MeshBuilderVertexAttributeLayerUInt32::Create(vertCount, EMotionFX::Mesh::ATTRIB_CLOTH_DATA, false, false);
            meshBuilder->AddLayer(clothLayer);
        }

        const int materialId = 0;

        AZStd::vector<AZ::Vector3> normals;
        AZStd::vector<AZ::Vector4> particles(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            particles[i] = AZ::Vector4::CreateFromVector3(vertices[i]);
        }
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            particles, indices,
            normals);

        for (AZ::u32 faceNum = 0; faceNum < faceCount; ++faceNum)
        {
            meshBuilder->BeginPolygon(materialId);
            for (AZ::u32 vertexOfFace = 0; vertexOfFace < 3; ++vertexOfFace)
            {
                AZ::u32 vertexNum = faceNum * 3 + vertexOfFace;
                AZ::u32 vertexIndex = indices[vertexNum];

                orgVtxLayer->SetCurrentVertexValue(&vertexNum);
                posLayer->SetCurrentVertexValue(&vertices[vertexIndex]);
                normalsLayer->SetCurrentVertexValue(&normals[vertexIndex]);

                if (!uvs.empty())
                {
                    uvsLayer->SetCurrentVertexValue(&uvs[vertexIndex]);
                }

                if (!clothData.empty())
                {
                    const AZ::u32 clothVertexDataU32 = clothData[vertexIndex].ToU32();
                    clothLayer->SetCurrentVertexValue(&clothVertexDataU32);
                }

                meshBuilder->AddPolygonVertex(vertexNum);
            }
            meshBuilder->EndPolygon();
        }

        return EMotionFX::Mesh::CreateFromMeshBuilder(meshBuilder.get());
    }
} // namespace UnitTest
