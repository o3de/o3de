/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OpenParticleSystemModuleInterface.h"
#include "OpenParticleSystemSystemComponent.h"
#include <OpenParticleSystem/SystemComponent.h>
#include <OpenParticleSystem/ParticleComponent.h>
namespace OpenParticleSystem
{
    class OpenParticleSystemModule
        : public OpenParticleSystemModuleInterface
    {
    public:
        AZ_RTTI(OpenParticleSystemModule, "{dbac9a67-ec13-4371-8a43-3f889fb1b8c6}", OpenParticleSystemModuleInterface);
        AZ_CLASS_ALLOCATOR(OpenParticleSystemModule, AZ::SystemAllocator, 0);

        OpenParticleSystemModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and
            // EditContext. This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OpenParticle::SystemComponent::CreateDescriptor(),
                OpenParticle::ParticleComponent::CreateDescriptor(),
            });
        }

        ~OpenParticleSystemModule() = default;

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<OpenParticle::SystemComponent>(),
            };
        }
    };
} // namespace OpenParticleSystem

AZ_DECLARE_MODULE_CLASS(Gem_OpenParticleSystem, OpenParticleSystem::OpenParticleSystemModule)
