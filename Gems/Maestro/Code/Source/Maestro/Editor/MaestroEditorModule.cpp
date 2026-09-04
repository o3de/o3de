/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IGem.h>

#include <Maestro/Components/EditorSequenceAgentComponent.h>
#include <Maestro/Components/EditorSequenceComponent.h>
#include <Maestro/Components/SequenceAgentComponent.h>
#include <Maestro/Components/SequenceComponent.h>
#include <Maestro/MaestroSystemComponent.h>

namespace Maestro
{
    class MaestroEditorModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(MaestroEditorModule, "{3E5B0789-9B53-45B6-8F36-60A7672BDBE1}", CryHooksModule);

        MaestroEditorModule()
            : CryHooksModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                MaestroAllocatorComponent::CreateDescriptor(),
                MaestroSystemComponent::CreateDescriptor(),
                SequenceComponent::CreateDescriptor(),
                SequenceAgentComponent::CreateDescriptor(),
                EditorSequenceComponent::CreateDescriptor(),
                EditorSequenceAgentComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MaestroAllocatorComponent>(),
                azrtti_typeid<MaestroSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Maestro::MaestroEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Maestro_Editor, Maestro::MaestroEditorModule)
#endif
