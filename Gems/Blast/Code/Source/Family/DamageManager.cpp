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
#include <StdAfx.h>

#include <Family/DamageManager.h>

#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
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
            desc.get(), m_blastMaterial.GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

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
            desc.get(), m_blastMaterial.GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

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
            desc.get(), m_blastMaterial.GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

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
            desc.get(), m_blastMaterial.GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

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
            desc.get(), m_blastMaterial.GetNativePointer(), actor.GetFamily().GetPxAsset().getAccelerator()});

        const NvBlastDamageProgram program
            {NvBlastExtTriangleIntersectionGraphShader, NvBlastExtTriangleIntersectionSubgraphShader};
        actor.Damage(program, programParams.get());

        return AZStd::make_pair(AZStd::move(desc), AZStd::move(programParams));
    }

    AZ::Vector3 DamageManager::TransformToLocal(BlastActor& actor, const AZ::Vector3& globalPosition)
    {
        const AZ::Transform hitToActorTransform(actor.GetWorldBody()->GetTransform().GetInverse());
        const AZ::Vector3 hitPos = hitToActorTransform.TransformPoint(globalPosition);
        return hitPos;
    }

    AZStd::vector<BlastActor*> DamageManager::OverlapSphere(
        ActorTracker& actorTracker, const float radius, const AZ::Transform& pose)
    {
        AZStd::shared_ptr<Physics::World> defaultWorld;
        Physics::DefaultWorldBus::BroadcastResult(defaultWorld, &Physics::DefaultWorldRequests::GetDefaultWorld);

        auto overlapHits = defaultWorld->OverlapSphere(
            radius, pose,
            [&actorTracker](const Physics::WorldBody* worldBody, [[maybe_unused]] const Physics::Shape* shape) -> bool
            {
                return actorTracker.GetActorByBody(worldBody) != nullptr;
            });

        AZStd::vector<BlastActor*> overlappedActors;

        AZStd::transform(
            overlapHits.begin(), overlapHits.end(), AZStd::back_inserter(overlappedActors),
            [&actorTracker](const Physics::OverlapHit& overlapHit) -> BlastActor*
            {
                return actorTracker.GetActorByBody(overlapHit.m_body);
            });

        return overlappedActors;
    }

    AZStd::vector<BlastActor*> DamageManager::OverlapCapsule(
        ActorTracker& actorTracker, const AZ::Vector3& position0, const AZ::Vector3& position1, float radius)
    {
        AZStd::shared_ptr<Physics::World> defaultWorld;
        Physics::DefaultWorldBus::BroadcastResult(defaultWorld, &Physics::DefaultWorldRequests::GetDefaultWorld);

        const float height = position0.GetDistance(position1);

        const auto pose = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromVector3(position1 - position0), (position0 + position1) / 2.);

        auto overlapHits = defaultWorld->OverlapCapsule(
            height, radius, pose,
            [&actorTracker](const Physics::WorldBody* worldBody, [[maybe_unused]] const Physics::Shape* shape) -> bool
            {
                return actorTracker.GetActorByBody(worldBody) != nullptr;
            });

        AZStd::vector<BlastActor*> overlappedActors;

        AZStd::transform(
            overlapHits.begin(), overlapHits.end(), AZStd::back_inserter(overlappedActors),
            [&actorTracker](const Physics::OverlapHit& overlapHit) -> BlastActor*
            {
                return actorTracker.GetActorByBody(overlapHit.m_body);
            });

        return overlappedActors;
    }
} // namespace Blast
