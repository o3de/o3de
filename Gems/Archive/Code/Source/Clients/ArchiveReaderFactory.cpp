/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveReaderFactory.h"
#include <Clients/ArchiveReader.h>

#include <AzCore/Memory/SystemAllocator.h>

#include <Archive/ArchiveTypeIds.h>


namespace Archive
{
    // Implement TypeInfo, Rtti and Allocator support
    AZ_TYPE_INFO_WITH_NAME_IMPL(ArchiveReaderFactory, "ArchiveReaderFactory", ArchiveReaderFactoryTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(ArchiveReaderFactory, IArchiveReaderFactory);
    AZ_CLASS_ALLOCATOR_IMPL(ArchiveReaderFactory, AZ::SystemAllocator);

    // ArchiveReader forwarding functions
    // This is used to create an ArchiveReader instance that is pointed
    // to from an IArchiveReader* to allow use of the ArchiveReader in modules outside of the Archive Gem
    AZStd::unique_ptr<IArchiveReader> ArchiveReaderFactory::Create() const
    {
        return AZStd::make_unique<ArchiveReader>();
    }

    AZStd::unique_ptr<IArchiveReader> ArchiveReaderFactory::Create(const ArchiveReaderSettings& readerSettings) const
    {
        return AZStd::make_unique<ArchiveReader>(readerSettings);
    }

    AZStd::unique_ptr<IArchiveReader> ArchiveReaderFactory::Create(AZ::IO::PathView archivePath,
        const ArchiveReaderSettings& readerSettings) const
    {
        return AZStd::make_unique<ArchiveReader>(archivePath, readerSettings);
    }

    AZStd::unique_ptr<IArchiveReader> ArchiveReaderFactory::Create(IArchiveReader::ArchiveStreamPtr archiveStream,
        const ArchiveReaderSettings& readerSettings) const
    {
        return AZStd::make_unique<ArchiveReader>(AZStd::move(archiveStream), readerSettings);
    }
} // namespace Archive
