/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ShaderManagementConsole
{
    struct ShaderVariantInfo
    {
        AZ_TYPE_INFO(ShaderVariantInfo, "{FF831BC7-CEA3-4525-B55E-6204D3B75ADD}");
        AZ_CLASS_ALLOCATOR(ShaderVariantInfo, AZ::SystemAllocator);

        AZ::RPI::ShaderOptionGroup m_shaderOptionGroup;
        int m_count;
    };
    struct ShaderVariantStatisticData
    {
        AZ_TYPE_INFO(ShaderVariantStatisticData, "{1A36910A-5FB5-4001-9C4E-FD6238A133E2}");
        AZ_CLASS_ALLOCATOR(ShaderVariantStatisticData, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::map<AZ::RPI::ShaderVariantId, ShaderVariantInfo> m_shaderVariantUsage;
        AZStd::unordered_map<AZ::Name, AZStd::unordered_map<AZ::Name, int>> m_shaderOptionUsage;
    };
}
