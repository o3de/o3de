/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomCore/Instance/Instance.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace RPI
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

        class IMaterialInstanceHandler
        {
        public:
            AZ_RTTI(AZ::RPI::IMaterialInstanceHandler, "{C683CF51-4859-4E8E-802B-115A2364BCAF}");

            virtual ~IMaterialInstanceHandler(){};

            virtual MaterialInstanceData RegisterMaterialInstance([[maybe_unused]] const Data::Instance<Material> material) = 0;
            virtual void ReleaseMaterialInstance([[maybe_unused]] const MaterialInstanceData& materialInstance) = 0;
            virtual void Compile() = 0;
        };

        using MaterialInstanceHandlerInterface = Interface<IMaterialInstanceHandler>;

    } // namespace RPI

} // namespace AZ