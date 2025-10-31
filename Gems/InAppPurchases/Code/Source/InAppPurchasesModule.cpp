/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InAppPurchasesModule.h"
#include "InAppPurchasesSystemComponent.h"

namespace InAppPurchases
{
    InAppPurchasesModule::InAppPurchasesModule()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            SystemComponent::CreateDescriptor(),
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList InAppPurchasesModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<SystemComponent>(),
        };
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), InAppPurchases::InAppPurchasesModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_InAppPurchases, InAppPurchases::InAppPurchasesModule)
#endif
