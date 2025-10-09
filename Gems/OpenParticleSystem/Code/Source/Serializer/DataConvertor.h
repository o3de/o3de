/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/variant.h>

#include <Serializer/ParticleBase.h>

namespace OpenParticle
{
    class DataConvertor final
    {
    public:
        AZ_TYPE_INFO(OpenParticle::DataConvertor, "{40a38417-ae47-4c23-9f9b-66cfc72d6b66}");
        using ValueType = AZStd::variant<AZStd::any>;

        static void Reflect(AZ::ReflectContext* context);

        DataConvertor() = default;
        ~DataConvertor() = default;

        template<typename T>
        static inline constexpr typename AZStd::underlying_type<T>::type ECast(const T& t)
        {
            return static_cast<typename AZStd::underlying_type<T>::type>(t);
        }

        static bool IsCustomType(AZ::TypeId id);

        static void GetDistIndex(AZStd::any& editValue, DistInfos& distIndex);
        static void UpdateDistIndex(AZStd::any& editValue, DistInfos& distIndex);
        static void GetParamId(AZStd::any& editValue, AZStd::vector<AZ::TypeId>& id);

        static AZStd::any ToRuntime(const AZStd::any& editValue);

        static void ToRuntime(OpenParticle::SystemConfig& editData, SimuCore::ParticleCore::ParticleSystem::Config& runtimeData);
        static void ToRuntime(OpenParticle::PreWarm& editData, SimuCore::ParticleCore::ParticleSystem::PreWarm& runtimeData);
        static void ToRuntime(OpenParticle::EmitterConfig& editData, SimuCore::ParticleCore::ParticleEmitter::Config& runtimeData);
        static void ToRuntime(OpenParticle::EmitBurstList& editData, SimuCore::ParticleCore::EmitBurstList& runtimeData);
        static void ToRuntime(OpenParticle::EmitSpawn& editData, SimuCore::ParticleCore::EmitSpawn& runtimeData);
        static void ToRuntime(OpenParticle::EmitSpawnOverMoving& editData, SimuCore::ParticleCore::EmitSpawnOverMoving& runtimeData);
        static void ToRuntime(OpenParticle::ParticleEventHandler& editData, SimuCore::ParticleCore::ParticleEventHandler& runtimeData);
        static void ToRuntime(OpenParticle::InheritanceHandler& editData, SimuCore::ParticleCore::InheritanceHandler& runtimeData);
        static void ToRuntime(OpenParticle::SpawnColor& editData, SimuCore::ParticleCore::SpawnColor& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLifetime& editData, SimuCore::ParticleCore::SpawnLifetime& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocBox& editData, SimuCore::ParticleCore::SpawnLocBox& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocPoint& editData, SimuCore::ParticleCore::SpawnLocPoint& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocSphere& editData, SimuCore::ParticleCore::SpawnLocSphere& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocCylinder& editData, SimuCore::ParticleCore::SpawnLocCylinder& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocSkeleton& editData, SimuCore::ParticleCore::SpawnLocSkeleton& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocTorus& editData, SimuCore::ParticleCore::SpawnLocTorus& runtimeData);
        static void ToRuntime(OpenParticle::SpawnRotation& editData, SimuCore::ParticleCore::SpawnRotation& runtimeData);
        static void ToRuntime(OpenParticle::SpawnSize& editData, SimuCore::ParticleCore::SpawnSize& runtimeData);
        static void ToRuntime(OpenParticle::SpawnVelDirection& editData, SimuCore::ParticleCore::SpawnVelDirection& runtimeData);
        static void ToRuntime(OpenParticle::SpawnVelSector& editData, SimuCore::ParticleCore::SpawnVelSector& runtimeData);
        static void ToRuntime(OpenParticle::SpawnVelCone& editData, SimuCore::ParticleCore::SpawnVelCone& runtimeData);
        static void ToRuntime(OpenParticle::SpawnVelSphere& editData, SimuCore::ParticleCore::SpawnVelSphere& runtimeData);
        static void ToRuntime(OpenParticle::SpawnVelConcentrate& editData, SimuCore::ParticleCore::SpawnVelConcentrate& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLightEffect& editData, SimuCore::ParticleCore::SpawnLightEffect& runtimeData);
        static void ToRuntime(OpenParticle::SpawnLocationEvent& editData, SimuCore::ParticleCore::SpawnLocationEvent& runtimeData);
        static void ToRuntime(OpenParticle::UpdateColor& editData, SimuCore::ParticleCore::UpdateColor& runtimeData);
        static void ToRuntime(OpenParticle::UpdateLocationEvent& editData, SimuCore::ParticleCore::UpdateLocationEvent& runtimeData);
        static void ToRuntime(OpenParticle::UpdateDeathEvent& editData, SimuCore::ParticleCore::UpdateDeathEvent& runtimeData);
        static void ToRuntime(OpenParticle::UpdateCollisionEvent& editData, SimuCore::ParticleCore::UpdateCollisionEvent& runtimeData);
        static void ToRuntime(OpenParticle::UpdateInheritanceEvent & editData, SimuCore::ParticleCore::UpdateInheritanceEvent & runtimeData);
        static void ToRuntime(OpenParticle::UpdateConstForce& editData, SimuCore::ParticleCore::UpdateConstForce& runtimeData);
        static void ToRuntime(OpenParticle::UpdateDragForce& editData, SimuCore::ParticleCore::UpdateDragForce& runtimeData);
        static void ToRuntime(OpenParticle::UpdateVortexForce& editData, SimuCore::ParticleCore::UpdateVortexForce& runtimeData);
        static void ToRuntime(OpenParticle::UpdateCurlNoiseForce& editData, SimuCore::ParticleCore::UpdateCurlNoiseForce& runtimeData);
        static void ToRuntime(OpenParticle::UpdateSizeLinear& editData, SimuCore::ParticleCore::UpdateSizeLinear& runtimeData);
        static void ToRuntime(OpenParticle::UpdateSizeByVelocity& editData, SimuCore::ParticleCore::UpdateSizeByVelocity& runtimeData);
        static void ToRuntime(OpenParticle::SizeScale& editData, SimuCore::ParticleCore::SizeScale& runtimeData);
        static void ToRuntime(OpenParticle::UpdateSubUv& editData, SimuCore::ParticleCore::UpdateSubUv& runtimeData);
        static void ToRuntime(OpenParticle::UpdateRotateAroundPoint& editData, SimuCore::ParticleCore::UpdateRotateAroundPoint& runtimeData);
        static void ToRuntime(OpenParticle::UpdateVelocity& editData, SimuCore::ParticleCore::UpdateVelocity& runtimeData);
        static void ToRuntime(OpenParticle::ParticleCollision& editData, SimuCore::ParticleCore::ParticleCollision& runtimeData);
        static void ToRuntime(OpenParticle::SpriteConfig& editData, SimuCore::ParticleCore::SpriteConfig& runtimeData);
        static void ToRuntime(OpenParticle::MeshConfig& editData, SimuCore::ParticleCore::MeshConfig& runtimeData);
        static void ToRuntime(OpenParticle::RibbonConfig& editData, SimuCore::ParticleCore::RibbonConfig& runtimeData);

        static void GetDistIndex(RibbonConfig& editData, DistInfos& distInfos);
        static void GetDistIndex(EmitSpawn& editData, DistInfos& distInfos);
        static void GetDistIndex(EmitSpawnOverMoving& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnColor& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnLifetime& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnLocPoint& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnRotation& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnSize& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnVelDirection& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnVelSector& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnVelCone& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnVelSphere& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnVelConcentrate& editData, DistInfos& distInfos);
        static void GetDistIndex(SpawnLightEffect& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateColor& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateConstForce& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateDragForce& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateSizeLinear& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateSizeByVelocity& editData, DistInfos& distInfos);
        static void GetDistIndex(SizeScale& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateVortexForce& editData, DistInfos& distInfos);
        static void GetDistIndex(UpdateVelocity& editData, DistInfos& distInfos);

        static void UpdateDistIndex(RibbonConfig& editData, DistInfos& distInfos);
        static void UpdateDistIndex(EmitSpawn& editData, DistInfos& distInfos);
        static void UpdateDistIndex(EmitSpawnOverMoving& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnColor& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnLifetime& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnLocPoint& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnRotation& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnSize& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnVelDirection& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnVelSector& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnVelCone& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnVelSphere& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnVelConcentrate& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SpawnLightEffect& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateColor& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateConstForce& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateDragForce& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateSizeLinear& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateSizeByVelocity& editData, DistInfos& distInfos);
        static void UpdateDistIndex(SizeScale& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateVortexForce& editData, DistInfos& distInfos);
        static void UpdateDistIndex(UpdateVelocity& editData, DistInfos& distInfos);

        static void GetParamId(RibbonConfig& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(EmitSpawn& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(EmitSpawnOverMoving& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnColor& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnLifetime& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnLocPoint& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnRotation& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnSize& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnVelDirection& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnVelSector& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnVelCone& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnVelSphere& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnVelConcentrate& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SpawnLightEffect& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateColor& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateConstForce& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateDragForce& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateSizeLinear& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateSizeByVelocity& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(SizeScale& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateVortexForce& editData, AZStd::vector<AZ::TypeId>& paramIds);
        static void GetParamId(UpdateVelocity& editData, AZStd::vector<AZ::TypeId>& paramIds);

        static SimuCore::ParticleCore::KeyPointInterpMode InterpModeToRuntime(const KeyPointInterpMode& value);
        static KeyPointInterpMode InterpModeToEditor(const SimuCore::ParticleCore::KeyPointInterpMode& value);

        static SimuCore::ParticleCore::CurveExtrapMode ExtrapModeToRuntime(const CurveExtrapMode& value);
        static CurveExtrapMode ExtrapModeToEditor(const SimuCore::ParticleCore::CurveExtrapMode& value);

        static SimuCore::ParticleCore::CurveTickMode CurveTickModeToRuntime(const CurveTickMode& value);
        static CurveTickMode CurveTickModeToEditor(const SimuCore::ParticleCore::CurveTickMode& value);

        static SimuCore::ParticleCore::RandomTickMode RandomTickModeToRuntime(const RandomTickMode& value);
        static RandomTickMode RandomTickModeToEditor(const SimuCore::ParticleCore::RandomTickMode& value);

        static SimuCore::ParticleCore::DistributionType DistributionTypeToRuntime(const DistributionType& value);
        static DistributionType DistributionTypeToEditor(const SimuCore::ParticleCore::DistributionType& value);
    };
} // namespace OpenParticle
