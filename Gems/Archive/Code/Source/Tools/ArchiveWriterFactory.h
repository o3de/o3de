/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Archive/Tools/ArchiveWriterAPI.h>

namespace Archive
{
    // Implements a factory that is registered with an
    // AZ::Interface<IArchiveWriterFactory> in the Archive.Tools
    // gem module ArchiveEditorModule class
    // This allows users of the Archive.Tools.API to create an ArchiveWriter
    class ArchiveWriterFactory
        : public IArchiveWriterFactory
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ArchiveWriterFactory);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        AZStd::unique_ptr<IArchiveWriter> Create() const override;
        AZStd::unique_ptr<IArchiveWriter> Create(const ArchiveWriterSettings& writerSettings) const override;
        AZStd::unique_ptr<IArchiveWriter> Create(AZ::IO::PathView archivePath,
            const ArchiveWriterSettings& writerSettings) const override;
        AZStd::unique_ptr<IArchiveWriter> Create(IArchiveWriter::ArchiveStreamPtr archiveStream,
            const ArchiveWriterSettings& writerSettings) const override;
   };
} // namespace Archive
