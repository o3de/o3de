/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/RequestPath.h>

namespace AZ
{
    namespace IO
    {
        RequestPath::RequestPath(AZ::IO::PathView path)
            : m_path(path)
        {
            if (path.IsRelative())
            {
                m_relativePathOffset = FindAliasOffset(path.Native());
                m_absolutePathHash = s_emptyPathHash;
            }
            else
            {
                m_relativePathOffset = 0;
                m_absolutePathHash = AZStd::hash<decltype(m_path)>{}(m_path);
            }
        }

        RequestPath& RequestPath::operator=(AZ::IO::PathView path)
        {
            *this = RequestPath(path);
            return *this;
        }

        bool RequestPath::operator==(const RequestPath& rhs) const
        {
            if (m_path.empty() || rhs.m_path.empty())
            {
                return m_path.empty() == rhs.m_path.empty();
            }

            ResolvePath();
            rhs.ResolvePath();
            if (m_absolutePathHash != rhs.m_absolutePathHash)
            {
                return false;
            }
            else
            {
                return m_path == rhs.m_path;
            }
        }

        bool RequestPath::operator!=(const RequestPath& rhs) const
        {
            return !operator==(rhs);
        }

        bool RequestPath::IsValid() const
        {
            ResolvePath();
            return m_absolutePathHash != s_invalidPathHash;
        }

        void RequestPath::Clear()
        {
            m_path.clear();
            m_absolutePathHash = s_emptyPathHash;
            m_relativePathOffset = 0;
        }

        const char* RequestPath::GetAbsolutePathCStr() const
        {
            ResolvePath();
            return m_path.c_str();
        }

        const char* RequestPath::GetRelativePathCStr() const
        {
            return m_path.c_str() + m_relativePathOffset;
        }

        PathView RequestPath::GetAbsolutePath() const
        {
            ResolvePath();
            return m_path;
        }

        PathView RequestPath::GetRelativePath() const
        {
            // In case m_path has not been resolved it holds the relative path with the path offset set to exclude the alias.
            // If m_path has been resolved it holds the absolute path with the path offset set to the start of the start of the relative part.
            return PathView(m_path).Native().substr(m_relativePathOffset);
        }

        size_t RequestPath::GetHash() const
        {
            return m_absolutePathHash;
        }

        void RequestPath::ResolvePath() const
        {
            if (m_absolutePathHash == s_emptyPathHash)
            {
                AZ_Assert(FileIOBase::GetInstance(),
                    "Trying to resolve a path in RequestPath before the low level file system has been initialized.");

                size_t relativePathLength = m_path.Native().length() - m_relativePathOffset;
                if (FileIOBase::GetInstance()->ResolvePath(m_path, m_path))
                {
                    m_absolutePathHash = AZ::IO::hash_value(m_path);
                    if (m_path.Native().length() >= relativePathLength)
                    {
                        m_relativePathOffset = m_path.Native().length() - relativePathLength;
                        return;
                    }
                }
                m_absolutePathHash = s_invalidPathHash;
            }
        }

        size_t RequestPath::FindAliasOffset(AZStd::string_view path) const
        {
            const char* pathChar = path.data();
            const char* pathCharEnd = path.end();
            if (*pathChar == '@')
            {
                const char* alias = pathChar;
                alias++;
                while (*alias != '@' && alias < pathCharEnd)
                {
                    alias++;
                }
                if (*alias == '@')
                {
                    alias++;
                    return alias - pathChar;
                }
            }
            return 0;
        }
    } // namespace IO
} // namespace AZ
