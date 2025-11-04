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
    namespace DX12
    {
        class PlatformModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{6AA25C53-C451-42DD-B2CA-08623869EBDD}", AZ::Module);

            PlatformModule() = default;
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    } // namespace DX12
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::DX12::PlatformModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_DX12_Private, AZ::DX12::PlatformModule)
#endif
