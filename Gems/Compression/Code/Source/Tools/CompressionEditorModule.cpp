/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionRegistrarImpl.h"
#include "CompressionEditorSystemComponent.h"

#include <CompressionModuleInterface.h>
#include <Compression/CompressionTypeIds.h>

namespace Compression
{
    class CompressionEditorModule
        : public CompressionModuleInterface
    {
    public:
        AZ_RTTI(CompressionEditorModule, CompressionEditorModuleTypeId, CompressionModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionEditorModule, AZ::SystemAllocator);

        CompressionEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                CompressionEditorSystemComponent::CreateDescriptor(),
            });

            m_compressionRegistrarInterface = AZStd::make_unique<CompressionRegistrarImpl>();
            if (CompressionRegistrar::Get() == nullptr)
            {
                CompressionRegistrar::Register(m_compressionRegistrarInterface.get());
            }
        }

        ~CompressionEditorModule()
        {
            if (CompressionRegistrar::Get() == m_compressionRegistrarInterface.get())
            {
                CompressionRegistrar::Unregister(m_compressionRegistrarInterface.get());
            }
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<CompressionEditorSystemComponent>(),
            };
        }

    private:
        // CompressionRegistrar interface used to register Compression interfaces
        // Available in tooling applications to allow compression algorithms to run
        AZStd::unique_ptr<CompressionRegistrarInterface> m_compressionRegistrarInterface;
    };
}// namespace Compression

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Compression::CompressionEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Compression, Compression::CompressionEditorModule)
#endif
