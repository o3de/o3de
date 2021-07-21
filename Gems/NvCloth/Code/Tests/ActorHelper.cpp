/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActorHelper.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/algorithm.h>

#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/VertexAttributeLayerAbstractData.h>
#include <Integration/Assets/ActorAsset.h>
#include <Tests/TestAssetCode/MeshFactory.h>

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
        const AZStd::vector<AZ::Vector3>& vertices,
        const AZStd::vector<AZ::u32>& indices,
        const AZStd::vector<VertexSkinInfluences>& skinningInfo,
        const AZStd::vector<AZ::Vector2>& uvs)
    {
        // Generate the normals for this mesh
        AZStd::vector<AZ::Vector4> particles(vertices.size());
        AZStd::transform(vertices.begin(), vertices.end(), particles.begin(), [](const AZ::Vector3& vertex) { return AZ::Vector4::CreateFromVector3(vertex); } );
        AZStd::vector<AZ::Vector3> normals;
        AZ::Interface<NvCloth::ITangentSpaceHelper>::Get()->CalculateNormals(
            particles, indices,
            normals);

        return EMotionFX::MeshFactory::Create(
            indices,
            vertices,
            normals,
            uvs,
            skinningInfo
        );
    }
} // namespace UnitTest
