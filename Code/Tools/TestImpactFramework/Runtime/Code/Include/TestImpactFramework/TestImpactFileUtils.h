/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactException.h>
#include <TestImpactFramework/TestImpactRuntime.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#pragma once

namespace TestImpact
{
    //! Attempts to read the contents of the specified file into a string.
    //! @tparam ExceptionType The exception type to throw upon failure.
    //! @param path The path to the file to read the contents of.
    //! @returns The contents of the file.
    template<typename ExceptionType>
    AZStd::string ReadFileContents(const RepoPath& path)
    {
        const auto fileSize = AZ::IO::SystemFile::Length(path.c_str());
        AZ_TestImpact_Eval(fileSize > 0, ExceptionType, AZStd::string::format("File %s does not exist", path.c_str()));

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = '\0';
        AZ_TestImpact_Eval(
            AZ::IO::SystemFile::Read(path.c_str(), buffer.data()),
            ExceptionType,
            AZStd::string::format("Could not read contents of file %s", path.c_str()));

        return AZStd::string(buffer.begin(), buffer.end());
    }

    //! Attempts to write the contents of the specified string to a file.
    //! @tparam ExceptionType The exception type to throw upon failure.
    //! @param contents The contents to write to the file.
    //! @param path The path to the file to write the contents to.
    template<typename ExceptionType>
    void WriteFileContents(const AZStd::string& contents, const RepoPath& path)
    {
        AZ::IO::SystemFile file;
        const AZStd::vector<char> bytes(contents.begin(), contents.end());
        AZ_TestImpact_Eval(
            file.Open(path.c_str(),
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY),
            ExceptionType,
            AZStd::string::format("Couldn't open file %s for writing", path.c_str()));

        AZ_TestImpact_Eval(
            file.Write(bytes.data(), bytes.size()), ExceptionType, AZStd::string::format("Couldn't write contents for file %s", path.c_str()));
    }

    //! Delete the files that match the pattern from the specified directory.
    //! @param path The path to the directory to pattern match the files for deletion.
    //! @param pattern The pattern to match files for deletion.
    inline void DeleteFiles(const RepoPath& path, const AZStd::string& pattern)
    {
        AZ::IO::SystemFile::FindFiles(AZStd::string::format("%s/%s", path.c_str(), pattern.c_str()).c_str(),
            [&path](const char* file, bool isFile)
        {
            if (isFile)
            {
                AZ::IO::SystemFile::Delete(AZStd::string::format("%s/%s", path.c_str(), file).c_str());
            }

            return true;
        });
    }

    //! Deletes the specified file.
    inline void DeleteFile(const RepoPath& file)
    {
        DeleteFiles(file.ParentPath(), file.Filename().Native());
    }
} // namespace TestImpact
