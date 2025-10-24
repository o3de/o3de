/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <particle/emit/ParticleEmit.h>
#include <particle/emit/ParticleEventHandler.h>
#include <particle/spawn/SpawnColor.h>
#include <particle/spawn/SpawnEvent.h>
#include <particle/spawn/SpawnLifetime.h>
#include <particle/spawn/SpawnLightEffect.h>
#include <particle/spawn/SpawnLocation.h>
#include <particle/spawn/SpawnRotation.h>
#include <particle/spawn/SpawnSize.h>
#include <particle/spawn/SpawnVelocity.h>
#include <particle/update/UpdateColor.h>
#include <particle/update/UpdateEvent.h>
#include <particle/update/UpdateForce.h>
#include <particle/update/UpdateSize.h>
#include <particle/update/UpdateSubUv.h>
#include <particle/update/UpdateVelocity.h>
#include <particle/update/UpdateRotateAroundPoint.h>
#include <particle/update/ParticleCollision.h>
#include <particle/core/Particle.h>
#include <particle/core/ParticleDistribution.h>
#include <particle/core/ParticleCurve.h>
#include <particle/core/ParticleRandom.h>
#include <particle/core/ParticleEmitter.h>
#include <particle/core/ParticleSystem.h>
#include <AzCore/Math/Color.h>

#include <AzCore/std/containers/array.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ValueObjFloat, "{D087CEBE-1C36-46D9-ABE7-8123563559FA}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ValueObjVec2, "{1904BB33-30EE-4058-99AE-B85C0A82D667}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ValueObjVec3, "{F86CBDA8-DBF0-4C1E-BDAA-8785003EFC60}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ValueObjVec4, "{86E0C4F5-C0DB-43B1-BEFF-7DAD378F92B8}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ValueObjColor, "{99E9D64D-1AD4-4DBC-93F5-CC5C6DB12B45}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ValueObjLinear, "{56EFCB59-443A-42DC-861F-09A0879236AE}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::Axis, "{fb406d4a-ad90-400b-986b-a876e4ac67db}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SimulateType, "{503769fe-05d3-4822-9f2c-a1332905898a}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::MeshSampleType, "{c9ce5b97-29d3-41e1-ad72-2a732019d9f1}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleSystem::PreWarm, "{7c91c584-ee7d-4646-80a2-9eafae63de5b}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleSystem::Config, "{5d52a668-8fce-4e0f-a265-312f6d7b8427}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleEmitter::Config, "{ef916f2d-476d-457f-a890-2e1dd1f56dc8}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SingleBurst, "{7736bd28-700f-483d-b977-d665909fe974}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::EmitBurstList, "{43935d19-e73c-45f6-8869-22c037cb7d4f}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::EmitSpawn, "{f7903d2a-ec92-482b-880c-118235ecb320}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::EmitSpawnOverMoving, "{fba739ad-5803-45ec-ac8f-894807c4515f}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleEventHandler, "{E412D223-9DA4-4606-9411-DAC33E0DA123}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::InheritanceHandler, "{0898BBBF-1AFC-4738-97B6-222C3B472D08}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnColor, "{d0918939-55d2-40ab-aae3-a57ceb425621}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLifetime, "{E561DE0A-4A16-4EA2-8AF9-428E64512481}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocBox, "{77a8e512-174f-46cb-b694-722b20802fb0}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocPoint, "{efe562ce-cd6f-4679-b90a-a9642c5c545f}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocSphere, "{4a2b0295-dacd-43c0-a29d-6661ed705a1b}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocCylinder, "{3c52cc2c-7204-46a0-a930-eab18a11dec4}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocSkeleton, "{34239928-ec0c-4025-9480-4f8a3731e4f3}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocTorus, "{47be697d-5772-4bff-ab61-ed33ea83ecff}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnSize, "{6c54315b-03bf-419e-bb38-22957855002f}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnVelDirection, "{e79592d2-7acb-4b70-9afa-6aee9ae164db}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnVelSector, "{976ec9e6-ff8c-47af-bef4-764bb82f1aff}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnVelCone, "{9d7144d9-8b76-4d2a-a720-b02eebffdd01}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnVelSphere, "{708fa89e-6879-4d1e-ae03-149b3ddd042c}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnVelConcentrate, "{073bd9b3-87a3-4213-8d89-67d74556da3c}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnRotation, "{54FD2AFA-381F-4864-99FC-15A3EC500F91}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLightEffect, "{5a3926e0-2024-4b2a-a9dc-75c3ef3b208f}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpawnLocationEvent, "{5BD37F0F-98DF-4471-A687-76CD4B268497}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateConstForce, "{fecb9e28-24b9-4e60-9d30-d9555e6b6781}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateDragForce, "{D6EFAFD7-F779-45C1-BDFC-40CAC7D9E3E1}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateVortexForce, "{cf64104f-f8ca-442b-9da2-683a6b2901cc}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateCurlNoiseForce, "{815B0EF7-1CAC-4307-9CF0-1F3F78E06436}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateColor, "{9DB730E2-F7EC-4697-B872-59D2E8A06010}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateLocationEvent, "{85CAA471-3B3E-4DAC-9AD8-C54F7D17735C}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateDeathEvent, "{CAE6FFF3-B40B-41C2-85AF-87CCEE2D6156}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateCollisionEvent, "{4FB8EF1E-057A-4FF9-8343-41A6929030BA}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateInheritanceEvent, "{BEC75B60-5BCD-4080-A1A5-7D8C1F50AE66}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateSizeLinear, "{15D462E4-89C8-4C65-BFE3-FEA361C9B64D}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateSizeByVelocity, "{7784C876-06B2-42BA-AD6C-41E64E02C673}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SizeScale, "{da788fd9-e485-42fe-9e59-9c2c0263c5b7}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateSubUv, "{CB67BE92-E34B-4660-98D6-50423B3842CD}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateRotateAroundPoint, "{fc896f56-d655-41ef-a31e-f9f372f737b2}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::UpdateVelocity, "{2863F8D2-6931-4933-906D-A4DC451C0287}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleCollision, "{BC04E19D-05CA-41F2-8DB8-5C98282BBE51}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RenderType, "{91b1e638-2228-46c3-9236-6c164a7a328d}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::Facing, "{b6f9faf4-cf4a-4638-a521-03820267f41a}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RibbonFacing, "{F6C422FF-6771-4EBB-9788-C1D2CC3FABB1}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::SpriteConfig, "{f4d6073a-d307-468d-8331-417159c52bd8}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::MeshConfig, "{5ed0be58-954f-45d3-9eed-01a5e69f77ae}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::TrailMode, "{9E100710-191C-4695-A381-2D66CF57BD65}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::TrailParam, "{568775EB-72AC-4337-B57B-571D3F890F12}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RibbonParam, "{2C7DE20B-C14B-4DD3-AE4D-1AF16D83809A}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RibbonConfig, "{58AADC83-0C64-43E5-B82F-40CAE71EE7B4}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleEventType, "{E4DCF7FB-E045-4A36-9499-DBBE2A177C0A}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::KeyPoint, "{6f815114-077c-417d-93db-054225aaefe1}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::DistributionType, "{749DCDC0-4464-4F23-AE8A-E9CA4DDEAA4D}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleDistribution, "{28CFB94F-616F-4FDB-ABA2-84F4258A59C2}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleCurve, "{24488c84-77c4-4c21-8403-4d5cb355da4c}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::ParticleRandom, "{04a73619-48bd-43ff-a78c-d37689c2068e}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RandomTickMode, "{ECEC429F-0DA5-4478-B9F9-765A8561E1FC}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::CurveExtrapMode, "{df01cd83-cf5a-44bb-ae9a-06320f29b543}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::CurveTickMode, "{D80C9DDA-C282-40D7-ABA8-A3D6D94420A8}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::KeyPointInterpMode, "{6c7661e9-226e-463f-b363-c6af69baeda3}");

    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::CpuCollisionType, "{9A6AAD60-4B1C-445E-B2F2-BD8340255F19}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::CollisionRadius, "{2B75634D-A762-4468-A173-C6A4B3238517}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RadiusCalculationType, "{5C5C96D8-C6F7-4497-9921-E4A9EFB32CC9}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::RadiusCalculationMethod, "{FACCE8B1-B09B-4C02-AE70-E99F19645F4E}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::Bounce, "{27E0D956-B88B-462F-8D9E-62BEC9116E77}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::Friction, "{C45EDD59-EC9A-4BCC-860C-B4012A5C3A7D}");
    AZ_TYPE_INFO_SPECIALIZE(SimuCore::ParticleCore::CollisionPlane, "{FC65D175-E35D-4080-A861-3B4E419C9003}");
} // namespace AZ

namespace OpenParticle
{
    class ParticleConfig
    {
    public:
        AZ_TYPE_INFO(ParticleConfig, "{4b8ee7e5-fcb9-4a42-9e16-6bc1c275735d}");
        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace OpenParticle
