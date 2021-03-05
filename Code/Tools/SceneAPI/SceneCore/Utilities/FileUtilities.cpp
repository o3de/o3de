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

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            AZStd::string FileUtilities::CreateOutputFileName(const AZStd::string& groupName, const AZStd::string& outputDirectory, const AZStd::string& extension)
            {
                AZStd::string result;

                if (!AzFramework::StringFunc::Path::ConstructFull(outputDirectory.c_str(), groupName.c_str(), extension.c_str(), result, true))
                {
                    return "";
                }

                return result;
            }

            bool FileUtilities::EnsureTargetFolderExists(const AZStd::string& path)
            {
                AZStd::string folder;
                if (!AzFramework::StringFunc::Path::GetFullPath(path.c_str(), folder))
                {
                    return false;
                }
                if (!IO::SystemFile::Exists(folder.c_str()))
                {
                    if (!IO::SystemFile::CreateDir(folder.c_str()))
                    {
                        return false;
                    }
                }

                return true;
            }

            AZStd::string FileUtilities::GetRelativePath(const AZStd::string& path, const AZStd::string& rootPath) 
            {
                AZStd::string outputPath(path);
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, outputPath);
                if (outputPath.compare(0, rootPath.size(), rootPath) == 0)
                {
                    size_t offset = rootPath[rootPath.length() - 1] == '/' ? 1 : 0;
                    AzFramework::StringFunc::RKeep(outputPath, rootPath.size() - offset);
                }
                return outputPath;
            }
        }
    }
}