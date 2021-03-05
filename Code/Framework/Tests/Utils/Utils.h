/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/IO/LocalFileIO.h>
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER
#include <filesystem>
#endif

namespace UnitTest
{
    // Creates a randomly named folder inside the user's temporary directory.
    // The folder and all contents will be destroyed when the object goes out of scope
    struct ScopedTemporaryDirectory
    {
        ScopedTemporaryDirectory();
        ~ScopedTemporaryDirectory();

        bool IsValid() const;
        const char* GetDirectory() const;

#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER      
        const std::filesystem::path& GetPath() const;
        std::filesystem::path operator/(const std::filesystem::path& rhs) const;
#endif
        AZ_DISABLE_COPY_MOVE(ScopedTemporaryDirectory);

    private:
        bool m_directoryExists{ false };
        AZStd::string m_tempDirectory;
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER     
        std::filesystem::path m_path;
#endif
    };
}
