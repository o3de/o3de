/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveWriterFactory.h"
#include <Tools/ArchiveWriter.h>

#include <AzCore/Memory/SystemAllocator.h>

#include <Archive/ArchiveTypeIds.h>


namespace Archive
{
    // Implement TypeInfo, Rtti and Allocator support
    AZ_TYPE_INFO_WITH_NAME_IMPL(ArchiveWriterFactory, "ArchiveWriterFactory", ArchiveWriterFactoryTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(ArchiveWriterFactory, IArchiveWriterFactory);
    AZ_CLASS_ALLOCATOR_IMPL(ArchiveWriterFactory, AZ::SystemAllocator);

    // ArchiveWriter forwarding functions
    // This is used to create an ArchiveWriter instance that is pointed
    // to from an IArchiveWriter* to allow use of the ArchiveWriter in modules outside of the Archive Gem
    AZStd::unique_ptr<IArchiveWriter> ArchiveWriterFactory::Create() const
    {
        return AZStd::make_unique<ArchiveWriter>();
    }

    AZStd::unique_ptr<IArchiveWriter> ArchiveWriterFactory::Create(const ArchiveWriterSettings& writerSettings) const
    {
        return AZStd::make_unique<ArchiveWriter>(writerSettings);
    }

    AZStd::unique_ptr<IArchiveWriter> ArchiveWriterFactory::Create(AZ::IO::PathView archivePath,
        const ArchiveWriterSettings& writerSettings) const
    {
        return AZStd::make_unique<ArchiveWriter>(archivePath, writerSettings);
    }

    AZStd::unique_ptr<IArchiveWriter> ArchiveWriterFactory::Create(IArchiveWriter::ArchiveStreamPtr archiveStream,
        const ArchiveWriterSettings& writerSettings) const
    {
        return AZStd::make_unique<ArchiveWriter>(AZStd::move(archiveStream), writerSettings);
    }
} // namespace Archive
