/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Material/MaterialShaderParameter.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ::RPI
{
    struct MaterialInstanceData
    {
        int32_t m_materialTypeId{ -1 };
        int32_t m_materialInstanceId{ -1 };
        bool m_usesSceneMaterialSrg{ false };
        Data::Instance<ShaderResourceGroup> m_shaderResourceGroup{ nullptr };
        Data::Instance<MaterialShaderParameter> m_materialShaderParameter{ nullptr };
    };
} // namespace AZ::RPI