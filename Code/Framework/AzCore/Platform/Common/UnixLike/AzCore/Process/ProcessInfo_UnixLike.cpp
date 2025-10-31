/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Process/ProcessInfo.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/utility/charconv.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <stdio.h>
#include <sys/resource.h>

namespace AZ
{
    inline namespace Process
    {
        bool QueryMemInfo(ProcessMemInfo& meminfo)
        {
            memset(&meminfo, 0, sizeof(ProcessMemInfo));

            auto parseSize = [](AZStd::string_view target, AZStd::string_view line, int64_t& value)
            {
                if (line.starts_with(target))
                {
                    line = line.substr(target.size());
                    // Skip leading whitespace
                    size_t index = 0;
                    for (; index < line.size(); ++index)
                    {
                        if (!AZStd::isspace(line[index]))
                        {
                            break;
                        }
                    }

                    line = line.substr(index);

                    if (!line.empty())
                    {
                        AZStd::from_chars(line.begin(), line.end(), value);
                        constexpr int64_t KiB_to_bytes = 1024;
                        value *= KiB_to_bytes;
                        return true;
                    }
                }
                return false;
            };

            AZ::IO::SystemFile file("/proc/self/status", AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
            if (!file.IsOpen())
            {
                return false;
            }
            constexpr size_t MaxReadSize = 4096;
            char buffer[MaxReadSize];
            AZ::IO::SystemFile::SizeType readSize = file.Read(MaxReadSize, buffer);
            while (readSize > 0)
            {
                // Find first non-whitespace character for line
                size_t frontOffset{};
                for (; frontOffset < readSize;)
                {
                    if (AZStd::isspace(buffer[frontOffset]))
                    {
                        ++frontOffset;
                        continue;
                    }

                    // find end of line
                    auto lineEndIter = AZStd::find(buffer + frontOffset, buffer + readSize, '\n');
                    bool foundNewLine = lineEndIter != (buffer + readSize);

                    // If less than MaxReadSize bytes have been read,
                    // assume the file is missing a final newline
                    // and take `buffer + readSize` as the end of the file
                    if (!foundNewLine && readSize == MaxReadSize)
                    {
                        break;
                    }

                    // Create a view of the current line
                    const AZStd::string_view line(buffer + frontOffset, lineEndIter);

                    // Skip pass the newline character
                    // If a newline was found, otherwise the end of the file has been found
                    frontOffset = (lineEndIter - buffer) + (foundNewLine ? 1 : 0);

                    int64_t size = 0;
                    if (parseSize("VmSize:", line, size))
                    {
                        meminfo.m_pagefileUsage = size;
                    }

                    if (parseSize("VmRSS:", line, size))
                    {
                        meminfo.m_workingSet = size;
                    }

                    if (parseSize("VmPeak:", line, size))
                    {
                        meminfo.m_peakWorkingSet = size;
                    }
                }

                // Read up to MaxReadSize bytes
                // Any partial lines are taken out of the read count
                size_t partialLineSize = MaxReadSize - frontOffset;
                if (partialLineSize > 0)
                {
                    memmove(buffer, buffer + frontOffset, partialLineSize);
                }
                readSize = file.Read(partialLineSize, buffer);
            }

            file.Close();

            AZ::IO::SystemFile statFile("/proc/self/stat", AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
            if (!statFile.IsOpen())
            {
                return false;
            }

            if (readSize = statFile.Read(MaxReadSize, buffer); readSize > 0)
            {
                // Create a view of the status information fields
                AZStd::string_view line(buffer, buffer + readSize);

                // Parsing the /proc/self/stat file to read the minor and major process faults
                // The format is documented at https://man7.org/linux/man-pages/man5/proc.5.html
                constexpr size_t minorFaultOffset = 10;
                constexpr size_t majorFaultOffset = 12;
                // The offsets are 1-based in the proc man file
                AZStd::optional<AZStd::string_view> fieldValue;
                size_t fieldOffset = 1;
                for(; fieldOffset < minorFaultOffset; ++fieldOffset)
                {
                    fieldValue = AZ::StringFunc::TokenizeNext(line, ' ');
                }

                // Read the minflt field from the /proc/self/stat file
                fieldValue = AZ::StringFunc::TokenizeNext(line, ' ');
                if (size_t minfltValue;
                    fieldValue.has_value() && AZStd::from_chars(fieldValue->begin(), fieldValue->end(), minfltValue).ec == AZStd::errc{})
                {
                    meminfo.m_pageFaultCount += minfltValue;
                }

                ++fieldOffset;

                for(; fieldOffset < majorFaultOffset; ++fieldOffset)
                {
                    fieldValue = AZ::StringFunc::TokenizeNext(line, ' ');
                }

                fieldValue = AZ::StringFunc::TokenizeNext(line, ' ');
                if (size_t maxfltValue;
                    fieldValue.has_value() && AZStd::from_chars(fieldValue->begin(), fieldValue->end(), maxfltValue).ec == AZStd::errc{})
                {
                    meminfo.m_pageFaultCount += maxfltValue;
                }
            }

            return true;
        }
    } // inline namespace Process
} // namespace AZ
