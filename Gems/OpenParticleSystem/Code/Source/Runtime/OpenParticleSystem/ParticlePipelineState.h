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
        SimuCore::ParticleCore::VariantKey variantKey = { UINT64_MAX };
        AZ::RHI::RenderStates states;
        AZ::RPI::ShaderOptionGroup optionGroup;
        // set geometryView per draw packet builing process
        AZ::RHI::GeometryView m_geometryView{ AZ::RHI::MultiDevice::AllDevices };
    };

    struct EmitterInstance
    {
        AZStd::vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>> m_perDrawSrgs;
        AZStd::vector<AZStd::pair<AZ::RHI::DrawListTag, AZ::Data::Instance<AZ::RPI::Shader>>> m_shaders;
        AZStd::unordered_map<AZ::RPI::Shader*, EmitterForDraw> m_emitterForDrawPair;
        AZ::Data::Instance<AZ::RPI::Material> m_material;
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_objSrg;
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_matSrg;
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
        ParticleModel m_model;
        AZ::RPI::Material::ChangeId m_materialChangeId = AZ::RPI::Material::DEFAULT_CHANGE_ID;
        AZ::RPI::Scene* m_scene = nullptr;
        void Setup(AZ::Data::Asset<AZ::RPI::MaterialAsset>& mat);
        void ReBuildPipeline();
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
