/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/IO/LocalFileIO.h>
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER
#include <filesystem>
#endif

namespace UnitTest
{
    //! Deletes a folder hierarchy from the supplied path
    void DeleteFolderRecursive(const AZ::IO::PathView& path);

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
        AZ::IO::FixedMaxPath m_tempDirectory;
#if !AZ_TRAIT_USE_POSIX_TEMP_FOLDER     
        std::filesystem::path m_path;
#endif
    };
}
