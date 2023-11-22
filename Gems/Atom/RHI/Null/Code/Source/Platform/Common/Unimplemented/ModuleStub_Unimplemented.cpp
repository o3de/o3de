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
    namespace Null
    {
        class PlatformModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{feed05ac-e346-436a-bf70-7689c9f2c393}", Module);

            PlatformModule() = default;
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::Null::PlatformModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Null_Private, AZ::Null::PlatformModule)
#endif
