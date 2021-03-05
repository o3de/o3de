/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            provided.push_back(AZ_CRC("ActorSystemService", 0x5e493d6c));
        }

        void ActorSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ActorSystemService", 0x5e493d6c));
        }

        void ActorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
            required.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
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

