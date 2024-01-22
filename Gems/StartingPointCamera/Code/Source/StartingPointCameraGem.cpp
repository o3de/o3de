/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraTargetAcquirers/AcquireByEntityId.h"
#include "CameraTargetAcquirers/AcquireByTag.h"
#include "CameraTransformBehaviors/FollowTargetFromDistance.h"
#include "CameraLookAtBehaviors/OffsetPosition.h"
#include "CameraTransformBehaviors/FollowTargetFromAngle.h"
#include "CameraTransformBehaviors/Rotate.h"
#include "CameraTransformBehaviors/OffsetCameraPosition.h"
#include "CameraLookAtBehaviors/SlideAlongAxisBasedOnAngle.h"
#include "CameraLookAtBehaviors/RotateCameraLookAt.h"
#include "CameraTransformBehaviors/FaceTarget.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Module/Module.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace StartingPointCamera
{
    struct StartingPointCameraGemComponent
        : public AZ::Component
    {
        ~StartingPointCameraGemComponent() override = default;
        AZ_COMPONENT(StartingPointCameraGemComponent, "{728DF62E-6787-4A16-8F07-8A45BECADAD7}");
        static void Reflect(AZ::ReflectContext* reflection)
        {
            Camera::AcquireByEntityId::Reflect(reflection);
            Camera::AcquireByTag::Reflect(reflection);
            Camera::FollowTargetFromDistance::Reflect(reflection);
            Camera::OffsetPosition::Reflect(reflection);
            Camera::FollowTargetFromAngle::Reflect(reflection);
            Camera::Rotate::Reflect(reflection);
            Camera::OffsetCameraPosition::Reflect(reflection);
            Camera::SlideAlongAxisBasedOnAngle::Reflect(reflection);
            Camera::RotateCameraLookAt::Reflect(reflection);
            Camera::FaceTarget::Reflect(reflection);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<StartingPointCameraGemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }
        void Activate() override {}
        void Deactivate() override {}
    };

    class StartingPointCameraModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(StartingPointCameraModule, "{87B6E891-9C64-4C5D-9FA1-4079BF6D902D}", AZ::Module);

        StartingPointCameraModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                StartingPointCameraGemComponent::CreateDescriptor(),
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
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), StartingPointCamera::StartingPointCameraModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_StartingPointCamera, StartingPointCamera::StartingPointCameraModule)
#endif
