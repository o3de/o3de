/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairBuilderModule : public AZ::Module
            {
            public:
                AZ_RTTI(HairBuilderModule, "{44440BE8-48AC-46AA-9643-2BD866709E27}", AZ::Module);
                AZ_CLASS_ALLOCATOR(HairBuilderModule, AZ::SystemAllocator);

                HairBuilderModule();

                //! Add required SystemComponents to the SystemEntity.
                AZ::ComponentTypeList GetRequiredSystemComponents() const override;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::Render::Hair::HairBuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomTressFX_Builders, AZ::Render::Hair::HairBuilderModule)
#endif
