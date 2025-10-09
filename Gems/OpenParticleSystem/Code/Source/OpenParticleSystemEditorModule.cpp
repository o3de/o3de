/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystemModuleInterface.h>
#include <OpenParticleSystemEditorSystemComponent.h>

#include <Runtime/OpenParticleSystem/ParticleComponent.h>
#include <Runtime/OpenParticleSystem/SystemComponent.h>

#ifdef PARTICLE_EDITOR
#include <Editor/DistributionCacheComponent.h>
#include <Editor/EditorParticleComponent.h>
#include <Editor/EditorSystemComponent.h>
#include <OpenParticleSystemEditor/Viewport/OpenParticleViewportComponent.h>
#include <ParticleMaterialThumbnailSystemComponent.h>
#endif

namespace OpenParticleSystem
{
    class OpenParticleSystemEditorModule
        : public OpenParticleSystemModuleInterface
    {
    public:
        AZ_RTTI(OpenParticleSystemEditorModule, "{D5A4EE34-260B-40BB-888B-BD21537A5787}", OpenParticleSystemModuleInterface);
        AZ_CLASS_ALLOCATOR(OpenParticleSystemEditorModule, AZ::SystemAllocator, 0);

        OpenParticleSystemEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and
            // EditContext. This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OpenParticle::SystemComponent::CreateDescriptor(),
                OpenParticle::ParticleComponent::CreateDescriptor(),
#ifdef PARTICLE_EDITOR
                Thumbnails::ParticleMaterialThumbnailSystemComponent::CreateDescriptor(),
                OpenParticleSystemEditor::OpenParticleViewportComponent::CreateDescriptor(),
                OpenParticle::EditorParticleComponent::CreateDescriptor(),
                OpenParticle::EditorSystemComponent::CreateDescriptor(),
                OpenParticle::DistributionCacheComponent::CreateDescriptor(),
                OpenParticleSystemEditorSystemComponent::CreateDescriptor()
#endif
            });
        }

        ~OpenParticleSystemEditorModule() = default;

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<OpenParticle::SystemComponent>(),
#ifdef PARTICLE_EDITOR
                azrtti_typeid<OpenParticleSystemEditor::OpenParticleViewportComponent>(),
                azrtti_typeid<OpenParticle::EditorSystemComponent>(),
                azrtti_typeid<OpenParticle::DistributionCacheComponent>(),
                azrtti_typeid<OpenParticleSystemEditorSystemComponent>(),
                azrtti_typeid<Thumbnails::ParticleMaterialThumbnailSystemComponent>()
#endif
            };
        }
    };
} // namespace OpenParticleSystem

AZ_DECLARE_MODULE_CLASS(Gem_OpenParticleSystem, OpenParticleSystem::OpenParticleSystemEditorModule)
