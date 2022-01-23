/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/string/string.h>

namespace AZ::RPI
{
    class ShaderFunction
    {
    public:
        AZ_TYPE_INFO(ShaderFunction, "{6BA160A4-10B2-4C2E-9E3F-E2E58964C032}");
        static void Reflect(AZ::ReflectContext* context);

    private:
        //! File reference to srgi header containing SRG declaration
        AZStd::string m_srgSource;
        //! File reference to azlsi header containing function declaration
        AZStd::string m_source;
    };

    class ShaderFunctionCollection
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        ShaderFunction* LookupShaderFunction(AZStd::string name);

    private:
        AZStd::unordered_map<AZStd::string, ShaderFunction> m_shaderFunctions;
    };
} // namespace AZ::RPI

