/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTIMacros.h>

namespace Archive
{
    inline ArchiveWriterError::operator bool() const
    {
        return m_errorCode != ArchiveWriterErrorCode{};
    }

    inline ArchiveWriterSettings::ArchiveWriterSettings() = default;

    // bool operator which converts an ArchiveAddToFileResult to true
    // of the file path token isn't invalid
    inline ArchiveAddToFileResult::operator bool() const
    {
        return m_filePathToken == InvalidArchiveFileToken;
    }

    // Archive Writer interface TypeInfo, rtti and allocator macros
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(IArchiveWriter, "IArchiveWriter", "{6C966C29-8D98-4FCD-AEE5-CFFF80EEB561}");
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
} // namespace Archive

