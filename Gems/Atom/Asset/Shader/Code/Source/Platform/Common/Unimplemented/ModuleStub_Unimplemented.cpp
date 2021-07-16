/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        class AzslShaderBuilderModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(AzslShaderBuilderModule, "{18F6276E-09B7-4C1D-B658-FADA2DB22148}", AZ::Module);

            AzslShaderBuilderModule() = default;
            ~AzslShaderBuilderModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AzslShaderBuilder, AZ::ShaderBuilder::AzslShaderBuilderModule)
