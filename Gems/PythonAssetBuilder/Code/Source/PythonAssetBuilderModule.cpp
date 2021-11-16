/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AzToolsFramework/API/PythonLoader.h>

#include <PythonAssetBuilderSystemComponent.h>

namespace PythonAssetBuilder
{
    class PythonAssetBuilderModule
        : public AZ::Module
        , public AzToolsFramework::EmbeddedPython::PythonLoader
    {
    public:
        AZ_RTTI(PythonAssetBuilderModule, "{35C9457E-54C2-474C-AEBE-5A70CC1D435D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PythonAssetBuilderModule, AZ::SystemAllocator, 0);

        PythonAssetBuilderModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                PythonAssetBuilderSystemComponent::CreateDescriptor(),
            });
        }

        // Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<PythonAssetBuilderSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(PythonAssetBuilder_0a5fda05323649009444bb7c3ee2b9c4, PythonAssetBuilder::PythonAssetBuilderModule)
