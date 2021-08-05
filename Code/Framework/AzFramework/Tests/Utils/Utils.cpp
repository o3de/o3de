/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Utils.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/functional.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace UnitTest
{
    void DeleteFolderRecursive(const AZ::IO::PathView& path)
    {
        auto callback = [&path](AZStd::string_view filename, bool isFile) -> bool
        {
            if (isFile)
            {
                auto filePath = AZ::IO::FixedMaxPath(path) / filename;
                AZ::IO::SystemFile::Delete(filePath.c_str());
            }
            else
            {
                if (filename != "." && filename != "..")
                {
                    auto folderPath = AZ::IO::FixedMaxPath(path) / filename;
                    DeleteFolderRecursive(folderPath);
                }
            }
            return true;
        };
        auto searchPath = AZ::IO::FixedMaxPath(path) / "*";
        AZ::IO::SystemFile::FindFiles(searchPath.c_str(), callback);
        AZ::IO::SystemFile::DeleteDir(AZ::IO::FixedMaxPathString(path.Native()).c_str());
    }


    ScopedTemporaryDirectory::ScopedTemporaryDirectory()
    {
        constexpr int MaxAttempts = 255;
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER     
        const auto userTempFolder = std::filesystem::temp_directory_path();
#else
        AZ::IO::Path userTempFolder("/tmp");
#endif

        for (int i = 0; i < MaxAttempts; ++i)
        {
            auto randomFolder = AZ::Uuid::CreateRandom().ToString<AZStd::fixed_string<512>>(false, false);
            AZ::IO::FixedMaxPath testPath;
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER 
            auto path = userTempFolder / ("UnitTest-" + randomFolder).c_str();
            testPath = path.string().c_str();
#else
            userTempFolder /= ("UnitTest-" + randomFolder).c_str();
            testPath = userTempFolder.c_str();
#endif
            if (!AZ::IO::SystemFile::Exists(testPath.c_str()))
            {
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER   
                m_path = path;
                m_tempDirectory = m_path.string().c_str();
#else
                m_tempDirectory = testPath;
#endif
                m_directoryExists = AZ::IO::SystemFile::CreateDir(m_tempDirectory.c_str());
                break;
            }
        }

        AZ_Error("ScopedTemporaryDirectory", !m_tempDirectory.empty(), "Failed to create unique temporary directory after attempting %d random folder names", MaxAttempts);
    }

    ScopedTemporaryDirectory::~ScopedTemporaryDirectory()
    {
        if (m_directoryExists)
        {
            DeleteFolderRecursive(m_tempDirectory);
        }
    }

    bool ScopedTemporaryDirectory::IsValid() const
    {
        return m_directoryExists;
    }

    const char* ScopedTemporaryDirectory::GetDirectory() const
    {
        return m_tempDirectory.c_str();
    }


#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER     
    const std::filesystem::path& ScopedTemporaryDirectory::GetPath() const
    {
        return m_path;
    }

    std::filesystem::path ScopedTemporaryDirectory::operator/(const std::filesystem::path& rhs) const
    {
        return m_path / rhs;
    }
#endif // !AZ_TRAIT_USE_POSIX_TEMP_FOLDER
}
