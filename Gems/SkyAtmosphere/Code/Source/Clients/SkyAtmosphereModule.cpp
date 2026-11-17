/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereTypeIds.h>
#include <SkyAtmosphereModuleInterface.h>

#include <AzCore/RTTI/RTTI.h>

#include <Components/SkyAtmosphereComponent.h>

namespace SkyAtmosphere
{
    class SkyAtmosphereModule
        : public SkyAtmosphereModuleInterface
    {
    public:
        AZ_RTTI(SkyAtmosphereModule, SkyAtmosphereModuleTypeId, SkyAtmosphereModuleInterface);
        AZ_CLASS_ALLOCATOR(SkyAtmosphereModule, AZ::SystemAllocator);

        SkyAtmosphereModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    SkyAtmosphereComponent::CreateDescriptor(),
                });
        }
    };
}// namespace SkyAtmosphere

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), SkyAtmosphere::SkyAtmosphereModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SkyAtmosphere, SkyAtmosphere::SkyAtmosphereModule)
#endif
