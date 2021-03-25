/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Atom_RHI_Metal_precompiled.h"
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace Metal
    {
        class PlatformModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{91979A38-0EDF-48C0-8A23-C02A283FAE93}", Module);

            PlatformModule() = default;
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Metal_Private, AZ::Metal::PlatformModule)
