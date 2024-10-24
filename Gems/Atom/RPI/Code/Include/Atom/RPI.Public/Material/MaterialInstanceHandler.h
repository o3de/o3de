/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <AtomCore/Instance/Instance.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::RPI
{
    class Material;
    class ShaderResourceGroup;
    class Buffer;
    class MaterialShaderParameter;

    struct MaterialInstanceData
    {
        int32_t m_materialTypeId{ -1 };
        int32_t m_materialInstanceId{ -1 };
        Data::Instance<ShaderResourceGroup> m_shaderResourceGroup{ nullptr };
        Data::Instance<MaterialShaderParameter> m_materialShaderParameter{ nullptr };
    };

    struct SharedSamplerState
    {
        uint32_t m_samplerIndex;
        RHI::SamplerState m_samplerState;
    };

    // Interface to hold and maintain the global SceneMaterialSrg. Each Material registers itself in the Init() function and
    // gets a 'MaterialInstanceData', which contains either the SceneMaterialSrg and the indices to access the right MaterialParameter -
    // buffer, or a unique MaterialSrg for the material. Also manages the TextureSamplers, and registers them in the appropriate
    // Sampler-Array in the Material SRGs.
    class IMaterialInstanceHandler
    {
    public:
        AZ_RTTI(AZ::RPI::IMaterialInstanceHandler, "{C683CF51-4859-4E8E-802B-115A2364BCAF}");

        virtual ~IMaterialInstanceHandler(){};

        virtual MaterialInstanceData RegisterMaterialInstance([[maybe_unused]] const Data::Instance<Material> material) = 0;
        virtual void ReleaseMaterialInstance([[maybe_unused]] const MaterialInstanceData& materialInstance) = 0;
        virtual int32_t RegisterMaterialTexture(
            [[maybe_unused]] const int materialTypeIndex,
            [[maybe_unused]] const int materialInstanceIndex,
            [[maybe_unused]] Data::Instance<Image> image) = 0;
        virtual AZStd::shared_ptr<SharedSamplerState> RegisterTextureSampler(
            [[maybe_unused]] const int materialTypeIndex,
            [[maybe_unused]] const int materialInstanceIndex,
            [[maybe_unused]] const RHI::SamplerState& samplerState) = 0;
        virtual const RHI::SamplerState GetRegisteredTextureSampler(
            const int materialTypeIndex, const int materialInstanceIndex, const uint32_t samplerIndex) = 0;

        virtual void Compile() = 0;
    };

    using MaterialInstanceHandlerInterface = Interface<IMaterialInstanceHandler>;

} // namespace AZ::RPI