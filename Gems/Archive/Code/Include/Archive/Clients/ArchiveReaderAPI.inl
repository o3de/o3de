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

#include <Archive/ArchiveTypeIds.h>

namespace Archive
{
    inline ArchiveReaderError::operator bool() const
    {
        return m_errorCode != ArchiveReaderErrorCode{};
    }

    // A file path token that isn't Invalid indicates
    // that the file exist in the archive
    inline ArchiveExtractFileResult::operator bool() const
    {
        return m_filePathToken != InvalidArchiveFileToken
            && m_resultOutcome.has_value();
    }

    // As for the case with ther ArchiveExtractFileResult
    // a valid file path token is used to indicate success of the result
    inline ArchiveListFileResult::operator bool() const
    {
        return m_filePathToken != InvalidArchiveFileToken
            && m_resultOutcome.has_value();
    }

    inline constexpr EnumerateArchiveResult::operator bool() const
    {
        return m_resultOutcome.has_value();
    }

    // Archive Writer interface TypeInfo, rtti and allocator macros
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(IArchiveReader, "IArchiveReader", IArchiveReaderTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(IArchiveReader);
    AZ_CLASS_ALLOCATOR_IMPL_INLINE(IArchiveReader, AZ::SystemAllocator);

    // ArchiveStreamDeleter for the GenericStream unique_ptr
    inline IArchiveReader::ArchiveStreamDeleter::ArchiveStreamDeleter() = default;
    inline IArchiveReader::ArchiveStreamDeleter::ArchiveStreamDeleter(bool shouldDelete)
        : m_delete(shouldDelete)
    {}

    inline void IArchiveReader::ArchiveStreamDeleter::operator()(AZ::IO::GenericStream* ptr) const
    {
        if (m_delete && ptr)
        {
            delete ptr;
        }
    }

    // IArchiveReader impl
    inline IArchiveReader::~IArchiveReader() = default;
} // namespace Archive

