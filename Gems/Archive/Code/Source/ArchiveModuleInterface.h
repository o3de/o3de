/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace Archive
{
    class IArchiveReaderFactory;

    class ArchiveModuleInterface
        : public AZ::Module
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ArchiveModuleInterface)
        AZ_RTTI_NO_TYPE_INFO_DECL()
        AZ_CLASS_ALLOCATOR_DECL

        ArchiveModuleInterface();
        ~ArchiveModuleInterface();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    private:
        // Archive Reader factory registered with the ArchiveReaderFactoryInterface
        // This allows external gem modules to create ArchiveReader instances
        // via the CreateArchiveReader functions in the ArchiveReaderAPI.h
        AZStd::unique_ptr<IArchiveReaderFactory> m_archiveReaderFactory;
    };
}// namespace Archive
