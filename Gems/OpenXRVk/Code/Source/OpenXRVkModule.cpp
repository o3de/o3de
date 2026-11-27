/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <OpenXRVk/OpenXRVkSystemComponent.h>
#include <XRCameraMovementComponent.h>
#include "Devices/Common/XRControllerComponent.h"
#include "Devices/Common/XRControllerAnimationsComponent.h"
#include <XRRayInteractorComponent.h>
#include <XRInteractableComponent.h>

#if defined (OPENXRVK_BUILDERS)
#include "Builders/OpenXRVkAssetsBuilderSystemComponent.h"
#endif

namespace OpenXRVk
{   
    //! This module is in charge of loading system components related to Openxrvk. 
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(Module, "{C34AA64E-0983-4D30-A33C-0D7C7676A20E}", AZ::Module);
        AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator);

        Module()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                SystemComponent::CreateDescriptor(),
                XRCameraMovementComponent::CreateDescriptor(),
                XRControllerComponent::CreateDescriptor(),
				XRRayInteractorComponent::CreateDescriptor(),
				XRInteractableComponent::CreateDescriptor(),
                XRControllerAnimationsComponent::CreateDescriptor(),
                #if defined (OPENXRVK_BUILDERS)
                OpenXRVkBuilders::OpenXRAssetsBuilderSystemComponent::CreateDescriptor(),
                #endif
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return
            {
                azrtti_typeid<OpenXRVk::SystemComponent>(),
            };
        }
    };
}


#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), OpenXRVk::Module)
#else
AZ_DECLARE_MODULE_CLASS(Gem_OpenXRVk, OpenXRVk::Module)
#endif
