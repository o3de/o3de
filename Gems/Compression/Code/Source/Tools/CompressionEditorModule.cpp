/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionFactoryImpl.h"
#include <CompressionModuleInterface.h>
#include "CompressionEditorSystemComponent.h"

namespace Compression
{
    class CompressionEditorModule
        : public CompressionModuleInterface
    {
    public:
        AZ_RTTI(CompressionEditorModule, "{6D256D91-6F1F-4132-B78E-6C24BA9D688C}", CompressionModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionEditorModule, AZ::SystemAllocator, 0);

        CompressionEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                CompressionEditorSystemComponent::CreateDescriptor(),
            });

            m_compressionFactoryInterface = AZStd::make_unique<CompressionFactoryImpl>();
            if (CompressionFactory::Get() == nullptr)
            {
                CompressionFactory::Register(m_compressionFactoryInterface.get());
            }
        }

        ~CompressionEditorModule()
        {
            if (CompressionFactory::Get() == m_compressionFactoryInterface.get())
            {
                CompressionFactory::Unregister(m_compressionFactoryInterface.get());
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
        // CompressionFactory interface used to register Compression interfaces
        // Available in tooling applications to allow compression algorithms to run
        AZStd::unique_ptr<CompressionFactoryInterface> m_compressionFactoryInterface;
    };
}// namespace Compression

AZ_DECLARE_MODULE_CLASS(Gem_Compression, Compression::CompressionEditorModule)
