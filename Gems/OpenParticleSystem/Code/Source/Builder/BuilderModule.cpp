/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BuilderComponent.h"
#include <OpenParticleSystem/ParticleComponent.h>
#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/RTTI.h>
#include <Editor/EditorParticleComponent.h>

namespace OpenParticle
{
    class BuilderModule final
        : public AZ::Module
    {
    public:
        AZ_RTTI(BuilderModule, "{cb03b874-09c5-47c3-aee0-9fef719eadff}", AZ::Module);

        BuilderModule()
        {
            m_descriptors.push_back(BuilderComponent::CreateDescriptor());
            m_descriptors.push_back(ParticleComponent::CreateDescriptor());
            m_descriptors.push_back(EditorParticleComponent::CreateDescriptor());
        }

        ~BuilderModule() = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList();
        }
    };
} // namespace OpenParticle
AZ_DECLARE_MODULE_CLASS(Gem_OpenParticleSystem_Builder, OpenParticle::BuilderModule)
