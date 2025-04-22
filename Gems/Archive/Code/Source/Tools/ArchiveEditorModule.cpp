/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Archive/ArchiveTypeIds.h>
#include <ArchiveModuleInterface.h>
#include "ArchiveEditorSystemComponent.h"
#include "ArchiveWriterFactory.h"

namespace Archive
{
    class ArchiveEditorModule
        : public ArchiveModuleInterface
    {
    public:
        AZ_RTTI(ArchiveEditorModule, ArchiveEditorModuleTypeId, ArchiveModuleInterface);
        AZ_CLASS_ALLOCATOR(ArchiveEditorModule, AZ::SystemAllocator);

        ArchiveEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ArchiveEditorSystemComponent::CreateDescriptor(),
                });

            m_archiveWriterFactory = AZStd::make_unique<ArchiveWriterFactory>();
            ArchiveWriterFactoryInterface::Register(m_archiveWriterFactory.get());
        }

        ~ArchiveEditorModule()
        {
            ArchiveWriterFactoryInterface::Unregister(m_archiveWriterFactory.get());
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<ArchiveEditorSystemComponent>(),
            };
        }

    private:
        // ArchiveWriterFactory instance for the Tools module that allows
        // external gem modules to create ArchiveWriter instances
        AZStd::unique_ptr<IArchiveWriterFactory> m_archiveWriterFactory;
    };
}// namespace Archive

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), Archive::ArchiveEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Archive_Editor, Archive::ArchiveEditorModule)
#endif
