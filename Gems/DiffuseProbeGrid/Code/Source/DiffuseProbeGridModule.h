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
        class DiffuseProbeGridModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(DiffuseProbeGridModule, "{72F3860A-0EA6-4C61-9EE0-DF0D690FD53B}", AZ::Module);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridModule, AZ::SystemAllocator);

            DiffuseProbeGridModule();

            //! Add required SystemComponents to the SystemEntity.
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::DiffuseProbeGridModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_DiffuseProbeGrid, AZ::Render::DiffuseProbeGridModule)
#endif
