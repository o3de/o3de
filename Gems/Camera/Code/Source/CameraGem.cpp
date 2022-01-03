/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>

#include "CameraComponent.h"
#include "CameraSystemComponent.h"

#if defined(CAMERA_EDITOR)
#include "CameraEditorSystemComponent.h"
#include "EditorCameraComponent.h"
#endif // CAMERA_EDITOR

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace Camera
{
    class CameraModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CameraModule, "{C2E72B0D-BCEF-452A-9BFA-03833015258C}", AZ::Module);

        CameraModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                Camera::CameraComponent::CreateDescriptor(),
                Camera::CameraSystemComponent::CreateDescriptor(),

#if defined(CAMERA_EDITOR)
                CameraEditorSystemComponent::CreateDescriptor(),
                EditorCameraComponent::CreateDescriptor(),
#endif // CAMERA_EDITOR
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

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<Camera::CameraSystemComponent>(),
#if defined(CAMERA_EDITOR)
                azrtti_typeid<CameraEditorSystemComponent>(),
#endif // CAMERA_EDITOR
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Camera, Camera::CameraModule)
