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

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Public/Model/Model.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace OpenParticle
{
    //! ParticleArchive is responsible for storing (and fixing up after load) the runtime
    //! data for the particle systems.
    //! Instead of storing the objects using serialize, it memcpys the image of the data into a buffer,
    //! and then stores the offset into that buffer which they can be found.
    //! So for example, EmitterInfo below has a AZ::u32 member called `m_config`.  This member is not the
    //! actual config, but represents the offset in the buffer that the memory image of a
    //! ParticleSystem::Config struct can be found.
    //! This implies that all data in all such objects must be essentially memcpyable.
    //! The >> operator can copy from a SimuCore::ParticleCore::ParticleSystem object into a ParticleArchive.
    //! The << operator copies the buffers from a ParticleArchive into a SimuCore::ParticleCore::ParticleSystem object
    //! Note that the ParticleSystem object uses these buffers in-place without unpacking them - it copies the buffers
    //! and the offset of the object in the buffer, then uses them in-place.
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

            AZ::u32 m_config = 0;
            AZStd::pair<AZ::Uuid, AZ::u32> m_render;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_model;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_skeletonModel;
            AZStd::vector<AZStd::tuple<AZ::Uuid, AZ::u32>> m_effectors;
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
        AZ::u32 m_systemConfig = 0;
        AZ::u32 m_preWarm = 0;
        AZStd::vector<EmitterInfo> m_emitterInfos;
        AZStd::unordered_map<AZ::u32, AZStd::vector<AZ::u8>> m_buffers;
    };

} // namespace OpenParticle
