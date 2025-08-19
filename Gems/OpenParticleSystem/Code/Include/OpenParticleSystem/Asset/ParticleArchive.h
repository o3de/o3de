/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <particle/core/ParticleDataPool.h>
#include <particle/core/ParticleEmitter.h>
#include <particle/core/ParticleRender.h>
#include <particle/core/ParticleSystem.h>

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace OpenParticle
{
    class ParticleArchive
    {
    public:
        AZ_TYPE_INFO(OpenParticle::ParticleArchive, "{585ed717-740d-46d2-ae19-d8dabbd5ddb7}");
        static void Reflect(AZ::ReflectContext* context);

        ParticleArchive() = default;

        ~ParticleArchive() = default;

        struct EmitterInfo
        {
            AZ_TYPE_INFO(OpenParticle::ParticleArchive::EmitterInfo, "{D3C0616F-D317-4F76-9191-BB2DA61332C4}");
            static void Reflect(AZ::ReflectContext* context);

            uint32_t m_config = 0;
            AZStd::pair<AZ::Uuid, uint32_t> m_render;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_model;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_skeletonModel;
            AZStd::vector<AZStd::tuple<AZ::Uuid, uint32_t>> m_effectors;
            SimuCore::ParticleCore::MeshSampleType m_meshSampleType;
        };

        void operator>>(const SimuCore::ParticleCore::ParticleSystem& system);

        void operator<<(SimuCore::ParticleCore::ParticleSystem& system);

        ParticleArchive& Begin(AZ::SerializeContext* context);

        ParticleArchive& SystemConfig(const AZStd::any& val);

        ParticleArchive& PreWarm(const AZStd::any& val);

        ParticleArchive& EmitterBegin(const AZStd::any& val);

        ParticleArchive& RenderConfig(const AZStd::any& val);

        ParticleArchive& Material(const AZ::Data::Asset<AZ::RPI::MaterialAsset>& asset);

        ParticleArchive& Model(const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset);

        ParticleArchive& SkeletonModel(const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset);

        ParticleArchive& AddEffector(const AZStd::any& val);

        ParticleArchive& EmitterEnd();

        ParticleArchive& MeshType(SimuCore::ParticleCore::MeshSampleType& type);

        friend class ParticleSystem;

    private:
        void Reset();

        AZ::SerializeContext* m_serializeContext = nullptr;
        uint32_t m_systemConfig = 0;
        uint32_t m_preWarm = 0;
        AZStd::vector<EmitterInfo> m_emitterInfos;
        AZStd::unordered_map<uint32_t, AZStd::vector<uint8_t>> m_buffers;
    };

} // namespace OpenParticle
