/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/FileRange.h>

namespace AZ::IO
{
    FileRange FileRange::CreateRange(u64 offset, u64 size)
    {
        FileRange result;
        result.m_hasOffsetEndSet = true;
        result.m_isEntireFile = false;
        result.m_offsetBegin = offset;
        result.m_offsetEnd = offset + size;
        return result;
    }

    FileRange FileRange::CreateRangeForEntireFile()
    {
        FileRange result;
        result.m_hasOffsetEndSet = false;
        result.m_isEntireFile = true;
        result.m_offsetBegin = 0;
        result.m_offsetEnd = (static_cast<u64>(1) << 63) - 1;
        return result;
    }

    FileRange FileRange::CreateRangeForEntireFile(u64 fileSize)
    {
        FileRange result;
        result.m_hasOffsetEndSet = true;
        result.m_isEntireFile = true;
        result.m_offsetBegin = 0;
        result.m_offsetEnd = fileSize;
        return result;
    }

    FileRange::FileRange()
        : m_isEntireFile(false)
        , m_offsetBegin(0)
        , m_hasOffsetEndSet(false)
        , m_offsetEnd(0)
    {
    }

    bool FileRange::operator==(const FileRange& rhs) const
    {
        if (m_isEntireFile)
        {
            return rhs.m_isEntireFile && m_offsetBegin == rhs.m_offsetBegin;
        }
        else
        {
            return m_offsetBegin == rhs.m_offsetBegin && m_offsetEnd == rhs.m_offsetEnd;
        }
    }

    bool FileRange::operator!=(const FileRange& rhs) const
    {
        if (m_isEntireFile)
        {
            return !rhs.m_isEntireFile || m_offsetBegin != rhs.m_offsetBegin;
        }
        else
        {
            return m_offsetBegin != rhs.m_offsetBegin || m_offsetEnd != rhs.m_offsetEnd;
        }
    }

    bool FileRange::IsEntireFile() const
    {
        return m_isEntireFile != 0;
    }

    bool FileRange::IsSizeKnown() const
    {
        // m_hasOffsetEndSet being zero has the special meaning that the file size has not
        // specifically been set yet.
        return m_hasOffsetEndSet != 0;
    }

    bool FileRange::IsInRange(u64 offset) const
    {
        return m_offsetBegin <= offset && offset < m_offsetEnd;
    }

    u64 FileRange::GetOffset() const
    {
        return m_offsetBegin;
    }

    u64 FileRange::GetSize() const
    {
        AZ_Assert(m_hasOffsetEndSet, "Calling GetSize on a FileRange that doesn't have a size specified.");
        return m_offsetEnd - m_offsetBegin;
    }

    u64 FileRange::GetEndPoint() const
    {
        AZ_Assert(m_hasOffsetEndSet, "Calling GetEndPoint on a FileRange that doesn't have an end offset specified.");
        return m_offsetEnd;
    }
} // namespace AZ::IO
