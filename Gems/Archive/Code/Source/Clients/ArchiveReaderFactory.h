/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Archive/Clients/ArchiveReaderAPI.h>

namespace Archive
{
    // Implements a factory that is registered with an
    // AZ::Interface<IArchiveReaderFactory> in the Archive.Clients
    // gem module ArchiveModule class
    // This allows users of the Archive.Clients.API to create an ArchiveReader
    class ArchiveReaderFactory
        : public IArchiveReaderFactory
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ArchiveReaderFactory);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        AZStd::unique_ptr<IArchiveReader> Create() const override;
        AZStd::unique_ptr<IArchiveReader> Create(const ArchiveReaderSettings& readerSettings) const override;
        AZStd::unique_ptr<IArchiveReader> Create(AZ::IO::PathView archivePath,
            const ArchiveReaderSettings& readerSettings) const override;
        AZStd::unique_ptr<IArchiveReader> Create(IArchiveReader::ArchiveStreamPtr archiveStream,
            const ArchiveReaderSettings& readerSettings) const override;
   };
} // namespace Archive
