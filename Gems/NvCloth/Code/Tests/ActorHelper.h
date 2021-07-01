/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/TransformData.h>

namespace UnitTest
{
    using SkinInfluence = AZStd::tuple<size_t, float>;
    using VertexSkinInfluences = AZStd::vector<SkinInfluence>;

    //! Helper class to setup an actor.
    class ActorHelper
        : public EMotionFX::Actor
    {
    public:
        explicit ActorHelper(const char* name);

        //! Adds a node to the skeleton.
        AZ::u32 AddJoint(
            const AZStd::string& name,
            const AZ::Transform localTransform = AZ::Transform::CreateIdentity(),
            const AZStd::string& parentName = "");

        //! Adds a collider to the cloh configuration.
        void AddClothCollider(const Physics::CharacterColliderNodeConfiguration& colliderNode);

        //! Finishes to construct the actor.
        //! @note This function must be called last, before using Actor.
        void FinishSetup();
    };

    AZ::Data::Asset<AZ::Data::AssetData> CreateAssetFromActor(
        AZStd::unique_ptr<EMotionFX::Actor> actor);

    Physics::CharacterColliderNodeConfiguration CreateSphereCollider(
        const AZStd::string& jointName,
        float radius,
        const AZ::Transform& offset = AZ::Transform::CreateIdentity());

    Physics::CharacterColliderNodeConfiguration CreateCapsuleCollider(
        const AZStd::string& jointName,
        float height, float radius,
        const AZ::Transform& offset = AZ::Transform::CreateIdentity());

    Physics::CharacterColliderNodeConfiguration CreateBoxCollider(
        const AZStd::string& jointName,
        const AZ::Vector3& dimensions,
        const AZ::Transform& offset = AZ::Transform::CreateIdentity());

    EMotionFX::Mesh* CreateEMotionFXMesh(
        const AZStd::vector<AZ::Vector3>& vertices,
        const AZStd::vector<AZ::u32>& indices,
        const AZStd::vector<VertexSkinInfluences>& skinningInfo = {},
        const AZStd::vector<AZ::Vector2>& uvs = {});
} // namespace UnitTest
