/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Serializer/ParticleAssetData.h>

namespace OpenParticle
{
    void ParticleAssetData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleAssetData>()
                ->Version(0)
                ->Field("emitters", &ParticleAssetData::m_emitters)
                ->Field("LODs", &ParticleAssetData::m_lods)
                ->Field("distribution", &ParticleAssetData::m_distribution);
        }
    }

    ParticleAssetData::ParticleAssetData()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext != nullptr, "invalid serialize context");
    }

    ParticleAssetData::~ParticleAssetData()
    {
        for (auto& emitter : m_emitters)
        {
            delete emitter;
            emitter = nullptr;
        }
        m_emitters.clear();
    }

    ParticleAssetData::ParticleAssetData(const ParticleAssetData& other)
    {
        Reset();
        m_name = other.m_name;
        m_config = other.m_config;
        m_preWarm = other.m_preWarm;
        m_lods = other.m_lods;
        for (auto& emitter : other.m_emitters)
        {
            auto newEmitter = aznew EmitterInfo();
            newEmitter->m_name = emitter->m_name;
            newEmitter->m_config = emitter->m_config;
            newEmitter->m_renderConfig = emitter->m_renderConfig;
            newEmitter->m_material = emitter->m_material;
            newEmitter->m_model = emitter->m_model;
            newEmitter->m_skeletonModel = emitter->m_skeletonModel;
            newEmitter->m_meshSampleType = emitter->m_meshSampleType;

            for (auto& emit : emitter->m_emitModules)
            {
                newEmitter->m_emitModules.emplace_back(emit);
            }
            for (auto& spawn : emitter->m_spawnModules)
            {
                newEmitter->m_spawnModules.emplace_back(spawn);
            }
            for (auto& update : emitter->m_updateModules)
            {
                newEmitter->m_updateModules.emplace_back(update);
            }
            for (auto& event : emitter->m_eventModules)
            {
                newEmitter->m_eventModules.emplace_back(event);
            }
            m_emitters.emplace_back(newEmitter);
        }
        m_distribution = other.m_distribution;
        m_serializeContext = other.m_serializeContext;
    }

    ParticleAssetData& ParticleAssetData::operator = (const ParticleAssetData& other)
    {
        this->Reset();
        this->m_name = other.m_name;
        this->m_config = other.m_config;
        this->m_preWarm = other.m_preWarm;
        this->m_lods = other.m_lods;
        for (auto& emitter : other.m_emitters)
        {
            auto newEmitter = aznew EmitterInfo();
            newEmitter->m_name = emitter->m_name;
            newEmitter->m_config = emitter->m_config;
            newEmitter->m_renderConfig = emitter->m_renderConfig;
            newEmitter->m_material = emitter->m_material;
            newEmitter->m_model = emitter->m_model;
            newEmitter->m_skeletonModel = emitter->m_skeletonModel;
            newEmitter->m_meshSampleType = emitter->m_meshSampleType;

            for (auto& emit : emitter->m_emitModules)
            {
                newEmitter->m_emitModules.emplace_back(emit);
            }
            for (auto& spawn : emitter->m_spawnModules)
            {
                newEmitter->m_spawnModules.emplace_back(spawn);
            }
            for (auto& update : emitter->m_updateModules)
            {
                newEmitter->m_updateModules.emplace_back(update);
            }
            for (auto& event : emitter->m_eventModules)
            {
                newEmitter->m_eventModules.emplace_back(event);
            }
            this->m_emitters.emplace_back(newEmitter);
        }
        this->m_distribution = other.m_distribution;
        this->m_serializeContext = other.m_serializeContext;
        return *this;
    }

    ParticleAssetData::CreateParticleResult ParticleAssetData::CreateParticleAsset(
        AZ::Data::AssetId assetId, [[maybe_unused]] AZStd::string_view sourceFilePath, bool elevateWarnings) const
    {
        (void)elevateWarnings;
        AZ::Data::Asset<OpenParticle::ParticleAsset> particleAsset =
            AZ::Data::AssetManager::Instance().FindOrCreateAsset<OpenParticle::ParticleAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);

        OpenParticle::ParticleAsset* assetValue = particleAsset.Get();
        ParticleArchive& archive = assetValue->m_particleArchive;

        auto fn = [&archive](AZStd::list<AZStd::any>& list)
        {
            for (auto& any : list)
            {
                archive.AddEffector(any);
            }
        };

        archive.Begin(m_serializeContext);
        archive.SystemConfig(m_config);
        archive.PreWarm(m_preWarm);

        // note that any "continue" in here will result in the emitter being dropped/deleted
        // and no particle will appear.

        for (auto& emitter : m_emitters)
        {
            auto materialAsset = emitter->m_material;
            if (!materialAsset.GetId().IsValid())
            {
                AZ_Error("ParticleAssetData", false, "Cannot create particle data - no material assigned to render in emitter %s", emitter->m_name.c_str());
                return AZ::Failure(CreateParticleAssetFailure::MaterialMissing);
            }

            AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
            AZ::Data::Asset<AZ::RPI::ModelAsset> skeletonAsset;

            if (emitter->m_renderConfig.empty())
            {
                AZ_Error("ParticleAssetData", false, "RenderType is missing in emitter %s", emitter->m_name.c_str());
                return AZ::Failure(CreateParticleAssetFailure::RenderTypeMissing);
            }

            // we can only check this if the material is available.
            // Its possible the user got a new project, and has not yet compiled all materials.  in this case,
            // we cannot load the material in order to check it.  When this happens, we skip this check, because
            // we know that the material will become available later, and when it does, this particle will be rebuilt, and this
            // check would succeed.
            if (materialAsset.IsReady())
            {
                auto materialType = materialAsset.Get()->GetMaterialTypeAsset();
                if ((emitter->m_renderConfig.is<SimuCore::ParticleCore::SpriteConfig>()) &&
                    !((materialType.GetHint().ends_with("particlesprite.azmaterialtype"))))
                {
                    AZ_Error(
                        "ParticleAssetData",
                        false,
                        "SpriteParticle needs material with ParticleSprite materialtype but is %s instead in emitter %s",
                        materialType.ToString<AZStd::string>().c_str(),
                        emitter->m_name.c_str());
                    return AZ::Failure(CreateParticleAssetFailure::IncorrectMaterialType);
                }

                if ((emitter->m_renderConfig.is<SimuCore::ParticleCore::MeshConfig>()) &&
                    !(materialType.GetHint().ends_with("particlemesh.azmaterialtype")))
                {
                    AZ_Error(
                        "ParticleAssetData",
                        false,
                        "Mesh Particle needs material with ParticleMesh materialtype but is %s instead in emitter %s",
                        materialType.ToString<AZStd::string>().c_str(),
                        emitter->m_name.c_str());
                    return AZ::Failure(CreateParticleAssetFailure::IncorrectMaterialType);
                }

                if ((emitter->m_renderConfig.is<SimuCore::ParticleCore::RibbonConfig>()) &&
                    !(materialType.GetHint().ends_with("particleribbon.azmaterialtype")))
                {
                    AZ_Error(
                        "ParticleAssetData",
                        false,
                        "Ribbon Particle needs material with ParticleMesh materialtype but is %s instead in emitter %s",
                        materialType.ToString<AZStd::string>().c_str(),
                        emitter->m_name.c_str());
                    return AZ::Failure(CreateParticleAssetFailure::IncorrectMaterialType);
                }
            }

            bool skeletonSpawn = false;
            for (auto spawnModule : emitter->m_spawnModules)
            {
                if (spawnModule.get_type_info().m_id == azrtti_typeid<SimuCore::ParticleCore::SpawnLocSkeleton>())
                {
                    skeletonSpawn = true;
                    break;
                }
            }
            if (emitter->m_renderConfig.is<SimuCore::ParticleCore::MeshConfig>())
            {
                // note that checking (!emitter->m_model) verifies whether its loaded right now, not just whether its
                // valid.  We might want to parse the asset data without actually loading the model, so we check the id,
                // not the actual model data.
                if (!emitter->m_model.GetId().IsValid())
                {
                    // mesh config, but with no model assigned, we cannot make a valid output.
                    AZ_Error("ParticleAssetData", false, "Emitter Renderer set to `Mesh` but no mesh asset assigned in emitter %s.  Particles will not function.",
                        emitter->m_name.c_str());
                    return AZ::Failure(CreateParticleAssetFailure::MeshAssetMissing);
                }
                modelAsset = emitter->m_model;
            }

            if (skeletonSpawn)
            {
                skeletonAsset = emitter->m_skeletonModel;
            }

            archive.EmitterBegin(emitter->m_config);
            archive.MeshType(emitter->m_meshSampleType);
            fn(emitter->m_emitModules);
            fn(emitter->m_spawnModules);
            fn(emitter->m_updateModules);
            fn(emitter->m_eventModules);
            archive.Material(materialAsset);
            if (emitter->m_renderConfig.is<SimuCore::ParticleCore::MeshConfig>())
            {
                archive.Model(modelAsset);
            }

            if (skeletonSpawn)
            {
                archive.SkeletonModel(skeletonAsset);
            }
            archive.RenderConfig(emitter->m_renderConfig);
            archive.EmitterEnd();
        }

        assetValue->m_lods = m_lods;
        assetValue->m_distribution = m_distribution;

        if (particleAsset)
        {
            particleAsset.Get()->SetReady();
            return AZ::Success(particleAsset);
        }
        
        return AZ::Failure(CreateParticleAssetFailure::Generic);
    }

    void ParticleAssetData::Reset()
    {
        for (auto emitter : m_emitters)
        {
            delete emitter;
            emitter = nullptr;
        }
        m_emitters.clear();
        m_lods.clear();
        m_distribution.curves.clear();
        m_distribution.randoms.clear();
    }
} // namespace OpenParticle
