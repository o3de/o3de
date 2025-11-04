/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZStd::string FileUtilities::CreateOutputFileName(
                const AZStd::string& groupName, const AZStd::string& outputDirectory, const AZStd::string& extension,
                const AZStd::string& sourceFileExtension)
            {
                // Create an initial name that looks like 'directory/groupName'
                AZ::IO::Path result(outputDirectory);
                result /= groupName;

                // Either add an extension or replace the existing one with the source file extension. This will typically
                // add the extension since most group names don't have an extension already.
                // This will ensure that final file name is unique based on the source file extension.
                // For example, 'model.fbx' and 'model.stl' would both have a groupName of 'model', and would then produce
                // 'model.azmodel' for example as the product asset for both. By appending the original source extension here,
                // we end up with the unique names of 'model.fbx.azmodel' and 'model.stl.azmodel'.
                // This could theoretically be fixed in the groupName generation, but that requires changes in many more places to the
                // code, along with knock-off effects that come from preserving the '.' which is also used by the SceneAPI to
                // separate node names. By adding it to the end file name, we avoid all of the negative effects and can centralize
                // the change to this one place.
                result.ReplaceExtension(AZ::IO::PathView(sourceFileExtension));

                // Append product extension to the file path by manipulating the string directly
                if (!extension.starts_with('.'))
                {
                    result.Native() += '.';
                }
                result.Native() += extension;

                // Return the final file name.
                return result.LexicallyNormal().Native();
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
