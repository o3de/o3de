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
        AZ_CLASS_ALLOCATOR(PythonAssetBuilderModule, AZ::SystemAllocator);

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
            return AZ::ComponentTypeList{};
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), PythonAssetBuilder::PythonAssetBuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_PythonAssetBuilder_Editor, PythonAssetBuilder::PythonAssetBuilderModule)
#endif
