/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTIMacros.h>

#include <Archive/ArchiveTypeIds.h>

namespace Archive
{
    inline ArchiveWriterError::operator bool() const
    {
        return m_errorCode != ArchiveWriterErrorCode{};
    }

    inline ArchiveWriterSettings::ArchiveWriterSettings() = default;

    // bool operator which converts an ArchiveAddFileResult to true
    // if the file path token isn't invalid
    inline ArchiveAddFileResult::operator bool() const
    {
        return m_filePathToken != InvalidArchiveFileToken
            && m_resultOutcome.has_value();
    }

    // If the archive file was successfully removed
    // the relative file path will not be empty
    inline ArchiveRemoveFileResult::operator bool() const
    {
        return !m_relativeFilePath.empty();
    }

    // Archive Writer interface TypeInfo, rtti and allocator macros
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(IArchiveWriter, "IArchiveWriter", IArchiveWriterTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(IArchiveWriter);
    AZ_CLASS_ALLOCATOR_IMPL_INLINE(IArchiveWriter, AZ::SystemAllocator);

    // ArchiveStreamDeleter for the GenericStream unique_ptr
    inline IArchiveWriter::ArchiveStreamDeleter::ArchiveStreamDeleter() = default;
    inline IArchiveWriter::ArchiveStreamDeleter::ArchiveStreamDeleter(bool shouldDelete)
        : m_delete(shouldDelete)
    {}

    inline void IArchiveWriter::ArchiveStreamDeleter::operator()(AZ::IO::GenericStream* ptr) const
    {
        if (m_delete && ptr)
        {
            delete ptr;
        }
    }

    // IArchiveWriter impl
    inline IArchiveWriter::~IArchiveWriter() = default;

    // ArchiveWriterFactory functions
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(IArchiveWriterFactory, "IArchiveWriterFactory", IArchiveWriterFactoryTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(IArchiveWriterFactory);
    AZ_CLASS_ALLOCATOR_IMPL_INLINE(IArchiveWriterFactory, AZ::SystemAllocator);

    inline IArchiveWriterFactory::~IArchiveWriterFactory() = default;

    inline CreateArchiveWriterResult CreateArchiveWriter()
    {
        auto archiveWriterFactory = AZ::Interface<IArchiveWriterFactory>::Get();
        if (archiveWriterFactory == nullptr)
        {
            return AZStd::unexpected(ResultString("ArchiveWriterFactory is not registered with an"
                " AZ::Interface<IArchiveWriterFactory>. Has the Archive Gem been set as active?"));
        }

        return archiveWriterFactory->Create();
    }
    inline CreateArchiveWriterResult CreateArchiveWriter(const ArchiveWriterSettings& writerSettings)
    {
        auto archiveWriterFactory = AZ::Interface<IArchiveWriterFactory>::Get();
        if (archiveWriterFactory == nullptr)
        {
            return AZStd::unexpected(ResultString("ArchiveWriterFactory is not registered with an"
                " AZ::Interface<IArchiveWriterFactory>. Has the Archive Gem been set as active?"));
        }

        return archiveWriterFactory->Create(writerSettings);
    }

    inline CreateArchiveWriterResult CreateArchiveWriter(AZ::IO::PathView archivePath,
        const ArchiveWriterSettings& writerSettings)
    {
        auto archiveWriterFactory = AZ::Interface<IArchiveWriterFactory>::Get();
        if (archiveWriterFactory == nullptr)
        {
            return AZStd::unexpected(ResultString("ArchiveWriterFactory is not registered with an"
                " AZ::Interface<IArchiveWriterFactory>. Has the Archive Gem been set as active?"));
        }

        return archiveWriterFactory->Create(archivePath, writerSettings);
    }
    inline CreateArchiveWriterResult CreateArchiveWriter(IArchiveWriter::ArchiveStreamPtr archiveStream,
        const ArchiveWriterSettings& writerSettings)
    {
        auto archiveWriterFactory = AZ::Interface<IArchiveWriterFactory>::Get();
        if (archiveWriterFactory == nullptr)
        {
            return AZStd::unexpected(ResultString("ArchiveWriterFactory is not registered with an"
                " AZ::Interface<IArchiveWriterFactory>. Has the Archive Gem been set as active?"));
        }

        return archiveWriterFactory->Create(AZStd::move(archiveStream), writerSettings);
    }
} // namespace Archive

