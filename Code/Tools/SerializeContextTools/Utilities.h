/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/IO/Path/Path_fwd.h>

namespace AZ
{
    class SerializeContext;

    namespace SerializeContextTools
    {
        class Application;

        class Utilities final
        {
        public:
            static AZStd::string ReadOutputTargetFromCommandLine(Application& application, const char* defaultFileOrFolder = "");
            static AZStd::vector<AZStd::string> ReadFileListFromCommandLine(Application& application, AZStd::string_view switchName);
            static AZStd::vector<AZStd::string> ExpandFileList(const char* root, const AZStd::vector<AZStd::string_view>& fileList);
            static bool HasWildCard(AZStd::string_view string);
            static void SanitizeFilePath(AZStd::string& filePath);

            static bool IsSerializationPrimitive(const AZ::Uuid& classId);

            static AZStd::vector<AZ::Uuid> GetSystemComponents(const Application& application);

            //! Converts path into a path that's relative to the highest-priority containing folder
            //! (the Assets folder for a Gem or the Projects folder for project's assets)
            static AZStd::string GenerateRelativePosixPath(const AZ::IO::PathView& projectPath, const AZ::IO::PathView& absolutePath);
            static AZStd::string_view GetStringAfterFirstOccurenceOf(const AZStd::string_view& toFind, const AZStd::string_view& string);

        private:
            Utilities() = delete;
            ~Utilities() = delete;
            Utilities(const Utilities&) = delete;
            Utilities(Utilities&&) = delete;
            Utilities& operator=(const Utilities&) = delete;
            Utilities& operator=(Utilities&&) = delete;
        };
    } // namespace SerializeContextTools
} // namespace AZ
