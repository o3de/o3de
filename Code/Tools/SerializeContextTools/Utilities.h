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

#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

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

            static bool InspectSerializedFile(const AZStd::string& filePath, SerializeContext* sc, const ObjectStream::ClassReadyCB& classCallback);

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
