/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <RHI.Profiler/RenderDoc/RenderDocSystemComponent.h>

namespace AZ::RHI
{
    //! This module is in charge of loading the RenderDoc system component
    class RenderDocModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(RenderDocModule, "{E977EF87-E738-4E15-9D01-C7663867E959}", Module);

        RenderDocModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                RenderDocSystemComponent::CreateDescriptor()
            });
        }
        ~RenderDocModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
                azrtti_typeid<RenderDocSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Profiler_RenderDoc, AZ::RHI::RenderDocModule)
