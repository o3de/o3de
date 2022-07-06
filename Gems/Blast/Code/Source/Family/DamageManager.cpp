/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Family/DamageManager.h>

#include <AzFramework/Physics/PhysicsScene.h>

#include <Blast/BlastActor.h>
#include <Blast/BlastSystemBus.h>
#include <Family/ActorTracker.h>
#include <Family/BlastFamily.h>

namespace Blast
{
    template<typename T>
    using DamagePair = DamageManager::DamagePair<T>;

    DamagePair<NvBlastExtRadialDamageDesc> DamageManager::RadialDamageInternal(
        BlastActor& actor, float damage, const AZ::Vector3& localPosition, float minRadius, float maxRadius)
    {
        auto desc = AZStd::make_unique<NvBlastExtRadialDamageDesc>(NvBlastExtRadialDamageDesc{
            damage,
            {localPosition.GetX(), localPosition.GetY(), localPosition.GetZ()},
            minRadius,
            maxRadius,
        });
        auto programParams = AZStd::make_unique<NvBlastExtProgramParams>(NvBlastExtProgramParams{
            desc.get(), m_blastMaterial->GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

        const NvBlastDamageProgram program{NvBlastExtFalloffGraphShader, NvBlastExtFalloffSubgraphShader};
        actor.Damage(program, programParams.get());

        return AZStd::make_pair(AZStd::move(desc), AZStd::move(programParams));
    }

    DamagePair<NvBlastExtShearDamageDesc> DamageManager::ShearDamageInternal(
        BlastActor& actor, float damage, const AZ::Vector3& localPosition, float minRadius, float maxRadius,
        const AZ::Vector3& normal)
    {
        auto desc = AZStd::make_unique<NvBlastExtShearDamageDesc>(NvBlastExtShearDamageDesc{
            damage,
            {normal.GetX(), normal.GetY(), normal.GetZ()},
            {localPosition.GetX(), localPosition.GetY(), localPosition.GetZ()},
            minRadius,
            maxRadius,
        });
        auto programParams = AZStd::make_unique<NvBlastExtProgramParams>(NvBlastExtProgramParams{
            desc.get(), m_blastMaterial->GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

        const NvBlastDamageProgram program{NvBlastExtShearGraphShader, NvBlastExtShearSubgraphShader};
        actor.Damage(program, programParams.get());

        return AZStd::make_pair(AZStd::move(desc), AZStd::move(programParams));
    }

    DamagePair<NvBlastExtImpactSpreadDamageDesc> DamageManager::ImpactSpreadDamageInternal(
        BlastActor& actor, float damage, const AZ::Vector3& localPosition, float minRadius, float maxRadius)
    {
        auto desc = AZStd::make_unique<NvBlastExtImpactSpreadDamageDesc>(NvBlastExtImpactSpreadDamageDesc{
            damage,
            {localPosition.GetX(), localPosition.GetY(), localPosition.GetZ()},
            minRadius,
            maxRadius,
        });
        auto programParams = AZStd::make_unique<NvBlastExtProgramParams>(NvBlastExtProgramParams{
            desc.get(), m_blastMaterial->GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

        const NvBlastDamageProgram program{NvBlastExtImpactSpreadGraphShader, NvBlastExtImpactSpreadSubgraphShader};
        actor.Damage(program, programParams.get());

        return AZStd::make_pair(AZStd::move(desc), AZStd::move(programParams));
    }

    DamagePair<NvBlastExtCapsuleRadialDamageDesc> DamageManager::CapsuleDamageInternal(
        BlastActor& actor, float damage, const AZ::Vector3& localPosition0, const AZ::Vector3& localPosition1,
        float minRadius, float maxRadius)
    {
        auto desc = AZStd::make_unique<NvBlastExtCapsuleRadialDamageDesc>(NvBlastExtCapsuleRadialDamageDesc{
            damage,
            {localPosition0.GetX(), localPosition0.GetY(), localPosition0.GetZ()},
            {localPosition1.GetX(), localPosition1.GetY(), localPosition1.GetZ()},
            minRadius,
            maxRadius,
        });
        auto programParams = AZStd::make_unique<NvBlastExtProgramParams>(NvBlastExtProgramParams{
            desc.get(), m_blastMaterial->GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

        const NvBlastDamageProgram program{NvBlastExtCapsuleFalloffGraphShader, NvBlastExtCapsuleFalloffSubgraphShader};
        actor.Damage(program, programParams.get());

        return AZStd::make_pair(AZStd::move(desc), AZStd::move(programParams));
    }

    DamagePair<NvBlastExtTriangleIntersectionDamageDesc> DamageManager::TriangleDamageInternal(
        BlastActor& actor, float damage, const AZ::Vector3& localPosition0, const AZ::Vector3& localPosition1,
        const AZ::Vector3& localPosition2)
    {
        auto desc =
            AZStd::make_unique<NvBlastExtTriangleIntersectionDamageDesc>(NvBlastExtTriangleIntersectionDamageDesc{
                damage,
                {localPosition0.GetX(), localPosition0.GetY(), localPosition0.GetZ()},
                {localPosition1.GetX(), localPosition1.GetY(), localPosition1.GetZ()},
                {localPosition2.GetX(), localPosition2.GetY(), localPosition2.GetZ()},
            });
        auto programParams = AZStd::make_unique<NvBlastExtProgramParams>(NvBlastExtProgramParams{
            desc.get(), m_blastMaterial->GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

        const NvBlastDamageProgram program
            {NvBlastExtTriangleIntersectionGraphShader, NvBlastExtTriangleIntersectionSubgraphShader};
        actor.Damage(program, programParams.get());

        return AZStd::make_pair(AZStd::move(desc), AZStd::move(programParams));
    }

    AZ::Vector3 DamageManager::TransformToLocal(BlastActor& actor, const AZ::Vector3& globalPosition)
    {
        const AZ::Transform hitToActorTransform(actor.GetSimulatedBody()->GetTransform().GetInverse());
        const AZ::Vector3 hitPos = hitToActorTransform.TransformPoint(globalPosition);
        return hitPos;
    }

    AZStd::vector<BlastActor*> DamageManager::OverlapSphere(
        [[maybe_unused]] ActorTracker& actorTracker, [[maybe_unused]] const float radius, [[maybe_unused]] const AZ::Transform& pose)
    {
        AZStd::vector<BlastActor*> overlappedActors;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateSphereOverlapRequest(
                    radius,
                    pose,
                    [&actorTracker](const AzPhysics::SimulatedBody* worldBody, [[maybe_unused]] const Physics::Shape* shape) -> bool
                    {
                        return actorTracker.GetActorByBody(worldBody) != nullptr;
                    });

                AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(sceneHandle, &request);
                if (result)
                {
                    AZStd::transform(
                        result.m_hits.begin(), result.m_hits.end(), AZStd::back_inserter(overlappedActors),
                        [&actorTracker, sceneInterface, sceneHandle](const AzPhysics::SceneQueryHit& overlapHit) -> BlastActor*
                        {
                            const AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, overlapHit.m_bodyHandle);
                            return actorTracker.GetActorByBody(body);
                        });
                }
            }
        }
        return overlappedActors;
    }

    AZStd::vector<BlastActor*> DamageManager::OverlapCapsule(
        [[maybe_unused]] ActorTracker& actorTracker, const AZ::Vector3& position0, const AZ::Vector3& position1, [[maybe_unused]] float radius)
    {
        AZStd::vector<BlastActor*> overlappedActors;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                const float height = position0.GetDistance(position1);

                const auto pose = AZ::Transform::CreateFromQuaternionAndTranslation(
                    AZ::Quaternion::CreateFromVector3(position1 - position0), (position0 + position1) / 2.);

                AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateCapsuleOverlapRequest(
                    height,
                    radius,
                    pose,
                    [&actorTracker](const AzPhysics::SimulatedBody* worldBody, [[maybe_unused]] const Physics::Shape* shape) -> bool
                    {
                        return actorTracker.GetActorByBody(worldBody) != nullptr;
                    });

                AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(sceneHandle, &request);
                if (result)
                {
                    AZStd::transform(
                        result.m_hits.begin(), result.m_hits.end(), AZStd::back_inserter(overlappedActors),
                        [&actorTracker, sceneInterface, sceneHandle](const AzPhysics::SceneQueryHit& overlapHit) -> BlastActor*
                        {
                            const AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, overlapHit.m_bodyHandle);
                            return actorTracker.GetActorByBody(body);
                        });
                }
            }
        }
        return overlappedActors;
    }
} // namespace Blast
