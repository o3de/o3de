/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActorSystemComponent.h>
#include <AtomBackend.h>
#include <Integration/Rendering/RenderBackendManager.h>

#include <AzCore/Serialization/SerializeContext.h>


namespace AZ
{
    namespace Render
    {
        ActorSystemComponent::ActorSystemComponent() = default;
        ActorSystemComponent::~ActorSystemComponent() = default;

        void ActorSystemComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<ActorSystemComponent, Component>()
                    ->Version(0)
                    ;
            }
        }

        void ActorSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ActorSystemService"));
        }

        void ActorSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ActorSystemService"));
        }

        void ActorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("SkinnedMeshService"));
            required.push_back(AZ_CRC_CE("EMotionFXAnimationService"));
        }

        void ActorSystemComponent::Activate()
        {
            AZ_Assert(AZ::Interface<EMotionFX::Integration::RenderBackendManager>::Get(), "The EMotionFX RenderBackendManger must be initialized before a render backend can register itself.");

            // The RenderBackendManager will manage the lifetime of the AtomBackend
            AZ::Interface<EMotionFX::Integration::RenderBackendManager>::Get()->SetRenderBackend(aznew AtomBackend());
        }

        void ActorSystemComponent::Deactivate()
        {
        }

    } // End Render namespace
} // End AZ namespace

