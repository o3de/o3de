/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <AtomCore/Instance/InstanceData.h>
#include <OpenParticleSystem/ParticleModel.h>

#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <particle/core/Particle.h>

namespace OpenParticle
{
    template<typename T>
    struct PtrHolder
    {
        AZ::RHI::Ptr<T> ptr;
    };

    struct EmitterForDraw
    {
        AZ::RPI::ShaderResourceGroup* m_drawSrg = nullptr;
        SimuCore::ParticleCore::VariantKey m_variantKey{ UINT64_MAX };
        AZ::RHI::RenderStates states;
        AZ::RPI::ShaderOptionGroup optionGroup;
        AZ::RHI::GeometryView m_geometryView{ AZ::RHI::MultiDevice::AllDevices };
    };

    struct EmitterDrawKey
    {
        AZ::RPI::Shader* m_shader = nullptr;
        AZ::RHI::DrawListTag m_drawListTag;
        AZ::Name m_materialPipelineName;
        size_t m_shaderOptionHash = 0;

        bool operator==(const EmitterDrawKey& rhs) const
        {
            return m_shader == rhs.m_shader &&
                m_drawListTag == rhs.m_drawListTag &&
                m_materialPipelineName == rhs.m_materialPipelineName &&
                m_shaderOptionHash == rhs.m_shaderOptionHash;
        }
    };

    struct EmitterDrawKeyHasher
    {
        size_t operator()(const EmitterDrawKey& key) const
        {
            size_t hash = AZStd::hash<AZ::RPI::Shader*>{}(key.m_shader);
            hash ^= AZStd::hash<AZ::u16>{}(key.m_drawListTag.GetIndex()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= AZStd::hash<AZ::Name>{}(key.m_materialPipelineName) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= key.m_shaderOptionHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    struct ShaderDrawRecord
    {
        AZ::RHI::DrawListTag m_drawListTag;
        AZ::Name m_materialPipelineName;
        AZ::Data::Instance<AZ::RPI::Shader> m_shader;
        EmitterDrawKey m_drawKey;
    };

    struct EmitterInstance
    {
        AZStd::vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>> m_perDrawSrgs;
        AZStd::vector<ShaderDrawRecord> m_shaders;
        AZStd::unordered_map<EmitterDrawKey, EmitterForDraw, EmitterDrawKeyHasher> m_emitterForDrawPair;
        AZ::Data::Instance<AZ::RPI::Material> m_material;
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_objSrg;
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_matSrg;
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
        ParticleModel m_model;
        AZ::RPI::Material::ChangeId m_materialChangeId = AZ::RPI::Material::DEFAULT_CHANGE_ID;
        AZ::RPI::Scene* m_scene = nullptr;
        AZ::u32 m_sortId = 0;
        bool m_needsPipelineRebuild = false;
        void Setup(AZ::Data::Asset<AZ::RPI::MaterialAsset>& mat);
        void ReBuildPipeline();
        bool TryRebuildPipeline();
        void Reset();
    };

    struct ParticlePipelineState
    {
        AZ::RHI::InputStreamLayout m_streamLayout;
        bool Setup(AZ::u32);

    private:
        void SetupSprite();
        void SetupMesh();
        void SetupRibbon();
    };
} // namespace OpenParticle
