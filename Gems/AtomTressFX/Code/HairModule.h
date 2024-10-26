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
            class HairModule
                : public AZ::Module
            {
            public:
                AZ_RTTI(HairModule, "{0EF06CF0-8011-4668-A31F-A6851583EBDC}", AZ::Module);
                AZ_CLASS_ALLOCATOR(HairModule, AZ::SystemAllocator);

                HairModule();

                //! Add required SystemComponents to the SystemEntity.
                AZ::ComponentTypeList GetRequiredSystemComponents() const override;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::Hair::HairModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomTressFX, AZ::Render::Hair::HairModule)
#endif
