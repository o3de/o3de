/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
