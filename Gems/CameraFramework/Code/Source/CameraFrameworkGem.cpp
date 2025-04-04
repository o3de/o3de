/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraRigComponent.h"

#include <AzCore/Module/Module.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace CameraFramework
{
    class CameraFrameworkModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CameraFrameworkModule, "{F9223D55-1D4C-4746-8864-5E2075A4DE50}", AZ::Module);

        CameraFrameworkModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                Camera::CameraRigComponent::CreateDescriptor(),
            });



            // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
            // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
            AZStd::vector<AZ::Uuid> typeIds;
            typeIds.reserve(m_descriptors.size());
            for (AZ::ComponentDescriptor* descriptor : m_descriptors)
            {
                typeIds.emplace_back(descriptor->GetUuid());
            }
            AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(
                &AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, typeIds);
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), CameraFramework::CameraFrameworkModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_CameraFramework, CameraFramework::CameraFrameworkModule)
#endif
