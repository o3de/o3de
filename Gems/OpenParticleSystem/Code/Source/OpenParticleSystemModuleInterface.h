/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include "OpenParticleSystemSystemComponent.h"
#endif

namespace OpenParticleSystem
{
    class OpenParticleSystemModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(OpenParticleSystemModuleInterface, "{509897C4-451C-47F5-9AAA-B3363679801F}", AZ::Module);
        AZ_CLASS_ALLOCATOR(OpenParticleSystemModuleInterface, AZ::SystemAllocator, 0);

        OpenParticleSystemModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OpenParticleSystemSystemComponent::CreateDescriptor(),
                });
        }

        ~OpenParticleSystemModuleInterface() = default;

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<OpenParticleSystemSystemComponent>(),
            };
        }
    };
}// namespace OpenParticleSystem
