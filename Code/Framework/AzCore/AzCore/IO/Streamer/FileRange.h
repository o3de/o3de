/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace IO
    {
        //! FileRange represents a subsection within a file.
        class FileRange
        {
        public:
            //! Creates a file range for the subsection of the file.
            static FileRange CreateRange(u64 offset, u64 size);
            //! Creates a file range the represents the entire file.
            static FileRange CreateRangeForEntireFile();
            //! Creates a file range the represents the entire file and includes the file size.
            static FileRange CreateRangeForEntireFile(u64 fileSize);

            FileRange();

            FileRange(const FileRange& rhs) = default;
            FileRange& operator=(const FileRange& rhs) = default;
            FileRange(FileRange&& rhs) = default;
            FileRange& operator=(FileRange&& rhs) = default;

            bool operator==(const FileRange& rhs) const;
            bool operator!=(const FileRange& rhs) const;

            //! Whether or not the range covers the entire file.
            bool IsEntireFile() const;
            //! Whether or not this range has specified a known size. This is always true if the file contains a
            //! subsection, but is optionally filled in when the entire file is represented. If a size isn't
            //! specified GetSize() and GetEndPoint() can not be safely called.
            bool IsSizeKnown() const;
            //! Checks whether or not the given offset is within the file range.
            bool IsInRange(u64 offset) const;

            //! Gets the offset inside the target file where this range starts. If this range
            //! represents the entire file this function will always return 0.
            u64 GetOffset() const;
            //! Gets the size of the range. If this range represents the entire file this function
            //! will always return an arbitrarily large size.
            u64 GetSize() const;
            //! Gets the offset in the file where this range ends. If this range represents the entire 
            //! file this function will always return an arbitrarily large offset.
            u64 GetEndPoint() const;

        private:
            //! Whether or not this range represents the entire file.
            u64 m_isEntireFile : 1;
            //! Offset into the file where the range begins. If the range represents the entire file this
            //! will be zero.
            u64 m_offsetBegin : 63;
            //! Whether or not m_offsetSet is an arbitrarily large number of contains the specific offset
            //! for the range.
            u64 m_hasOffsetEndSet : 1;
            //! Offset into the file where the range end. If the range represents the entire file this
            //! will be an arbitrarily large offset.
            u64 m_offsetEnd : 63;
        };
    } // namespace IO
} // namesapce AZ
