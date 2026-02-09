/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Serializer/DataConvertor.h>

#include <OpenParticleSystem/Asset/ParticleAsset.h>

namespace OpenParticle
{
    class ParticleAssetData final
    {
    public:
        AZ_TYPE_INFO(OpenParticle::ParticleAssetData, "{2ed76d7b-d961-4b82-a2f0-81edc5dcaeb0}");

        static void Reflect(AZ::ReflectContext* context);

        ParticleAssetData();
        ~ParticleAssetData();

        ParticleAssetData(const ParticleAssetData& other);
        ParticleAssetData& operator=(const ParticleAssetData& other);

        struct EmitterInfo
        {
            AZ_CLASS_ALLOCATOR(ParticleAssetData::EmitterInfo, AZ::SystemAllocator, 0);

            AZStd::string m_name;
            AZStd::any m_config;
            AZStd::any m_renderConfig;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_model;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_skeletonModel;
            AZStd::list<AZStd::any> m_emitModules;
            AZStd::list<AZStd::any> m_spawnModules;
            AZStd::list<AZStd::any> m_updateModules;
            AZStd::list<AZStd::any> m_eventModules;
            SimuCore::ParticleCore::MeshSampleType m_meshSampleType;
        };

        
        enum class CreateParticleAssetFailure
        {
            MaterialMissing,       // No material assigned for rendering
            RenderTypeMissing,     // No render type assigned
            IncorrectMaterialType, // Incorrect material type assigned for the render type (ie, "Sprite" but on mesh renderer)
            MeshAssetMissing,      // Mesh renderer used, but no mesh asset assigned
            Generic,               // Generic failure - check log for details.
        };

        using CreateParticleResult = AZ::Outcome<AZ::Data::Asset<ParticleAsset>, CreateParticleAssetFailure>;


        CreateParticleResult CreateParticleAsset(
            AZ::Data::AssetId assetId, [[maybe_unused]] AZStd::string_view sourceFilePath, bool elevateWarnings = true) const;

        void Reset();

        AZStd::string m_name = "ParticleSystem";
        AZStd::any m_config;
        AZStd::any m_preWarm;
        AZStd::vector<EmitterInfo*> m_emitters;
        AZStd::vector<ParticleLOD> m_lods;
        ParticleDistribution m_distribution;
        AZ::SerializeContext* m_serializeContext = nullptr;
    };
} // namespace OpenParticle

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleAssetData::EmitterInfo, "{f7ee0cf4-80ea-4c64-a5c3-32b9ee527896}");
}
