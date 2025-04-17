/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace StartingPointMovement
{

    // Dummy component to get reflect function
    class StartingPointMovementDummyComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(StartingPointMovementDummyComponent, "{6C9DA3DD-A0B3-4DCB-B77B-E93C4AF89134}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->ClassDeprecate("Event Action Bindings", AZ::Uuid("{2BB79CFC-7EBC-4EF4-A62E-5D64CB45CDBD}"));

                serializeContext->Class<StartingPointMovementDummyComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        void Activate() override { }
        void Deactivate() override { }
    };

    class StartingPointMovementModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(StartingPointMovementModule, "{AE406AE3-77AE-4CA6-84AD-842224EE2188}", AZ::Module);

        StartingPointMovementModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                StartingPointMovementDummyComponent::CreateDescriptor(),
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
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), StartingPointMovement::StartingPointMovementModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_StartingPointMovement, StartingPointMovement::StartingPointMovementModule)
#endif
