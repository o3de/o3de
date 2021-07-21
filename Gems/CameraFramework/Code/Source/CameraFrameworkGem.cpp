/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraFramework_precompiled.h"

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
            EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_CameraFramework, CameraFramework::CameraFrameworkModule)
