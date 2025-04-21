/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtlasBuilderWorker.h"

#include <AzCore/Math/MathIntrinsics.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Atom/ImageProcessing/ImageObject.h>
#include <Atom/ImageProcessing/ImageProcessingBus.h>
#include <Atom/ImageProcessing/PixelFormats.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <qimage.h>
#include <QString>
#include <QDir>
#include <qfileinfo.h>

namespace TextureAtlasBuilder
{
    //! Used for sorting ImageDimensions
    bool operator<(ImageDimension a, ImageDimension b);

    //! Used to expose the ImageDimension in a pair to AZStd::Sort
    bool operator<(IndexImageDimension a, IndexImageDimension b);

    //! Returns true if two coordinate sets overlap
    bool Collides(AtlasCoordinates a, AtlasCoordinates b);

    //! Returns true if item collides with any object in list
    bool Collides(AtlasCoordinates item, AZStd::vector<AtlasCoordinates> list);

    //! Returns the portion of the second item that overlaps with the first
    AtlasCoordinates GetOverlap(AtlasCoordinates a, AtlasCoordinates b);

    //! Performs an operation that copies a pixel to the output
    void SetPixels(AZ::u8* dest, const AZ::u8* source, int destBytes);

    //! Checks if we can insert an image into a slot
    bool CanInsert(AtlasCoordinates slot, ImageDimension image, int padding, int farRight, int farBot);

    //! Adds the necessary padding to an Atlas Coordinate 
    void AddPadding(AtlasCoordinates& slot, int padding, int farRight, int farBot);

    //! Counts leading zeros
    uint32_t CountLeadingZeros32(uint32_t x)
    {
        return x == 0 ? 32 : az_clz_u32(x);
    }

    //! Integer log2
    uint32_t IntegerLog2(uint32_t x)
    {
        return 31 - CountLeadingZeros32(x);
    }

    bool IsFolderPath(const AZStd::string& path)
    {
        bool hasExtension = AzFramework::StringFunc::Path::HasExtension(path.c_str());
        return !hasExtension;
    }

    bool HasTrailingSlash(const AZStd::string& path)
    {
        size_t pathLength = path.size();
        return (pathLength > 0 && (path.at(pathLength - 1) == '/' || path.at(pathLength - 1) == '\\'));
    }

    bool GetCanonicalPathFromFullPath(const AZStd::string& fullPath, AZStd::string& canonicalPathOut)
    {
        AZStd::string curPath = fullPath;

        // We avoid using LocalFileIO::ConvertToAbsolutePath for this because it does not behave consistently across platforms.
        // On non-Windows platforms, LocalFileIO::ConvertToAbsolutePath requires that the path exist, otherwise the path
        // remains unchanged. This won't work for paths that include wildcards.
        // Also, on non-Windows platforms, if the path is already a full path, it will remain unchanged even if it contains
        // "./" or "../" somewhere other than the beginning of the path

        // Normalize path
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, curPath);

        const AZStd::string slash("/");

        // Replace "/./" occurrances with "/"
        const AZStd::string slashDotSlash("/./");
        bool replaced = false;
        do
        {
            // Replace first occurrance
            replaced = AzFramework::StringFunc::Replace(curPath, slashDotSlash.c_str(), slash.c_str(), false, true, false);
        } while (replaced);

        // Replace "/xxx/../" with "/"
        const AZStd::regex slashDotDotSlash("\\/[^/.]*\\/\\.\\.\\/");
        AZStd::string prevPath;
        while (prevPath != curPath)
        {
            prevPath = curPath;
            curPath = AZStd::regex_replace(prevPath, slashDotDotSlash, slash, AZStd::regex_constants::match_flag_type::format_first_only);
        }

        if ((curPath.find("..") != AZStd::string::npos) || (curPath.find("./") != AZStd::string::npos) || (curPath.find("/.") != AZStd::string::npos))
        {
            return false;
        }

        canonicalPathOut = curPath;
        return true;
    }

    bool ResolveRelativePath(const AZStd::string& relativePath, const AZStd::string& watchDirectory, AZStd::string& resolvedFullPathOut)
    {
        bool resolved = false;

        if (relativePath[0] == '@')
        {
            // Get full path by resolving the alias at the front of the path
            char resolvedPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(relativePath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
            resolvedFullPathOut = resolvedPath;
            resolved = true;
        }
        else
        {
            // Get full path by appending the relative path to the watch directory
            AZStd::string fullPath = watchDirectory;
            fullPath.append("/");
            fullPath.append(relativePath);

            // Resolve to canonical path (remove "./" and "../") 
            resolved = GetCanonicalPathFromFullPath(fullPath, resolvedFullPathOut);
        }

        return resolved;
    }

    bool GetAbsoluteSourcePathFromRelativePath(const AZStd::string& relativeSourcePath, AZStd::string& absoluteSourcePathOut)
    {
        bool result = false;
        AZ::Data::AssetInfo info;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, relativeSourcePath.c_str(), info, watchFolder);
        if (result)
        {
            absoluteSourcePathOut = AZStd::string::format("%s/%s", watchFolder.c_str(), info.m_relativePath.c_str());

            // Normalize path
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, absoluteSourcePathOut);
        }
        return result;
    }

    // Reflect the input parameters
    void AtlasBuilderInput::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtlasBuilderInput>()
                ->Version(1)
                ->Field("Force Square", &AtlasBuilderInput::m_forceSquare)
                ->Field("Force Power of Two", &AtlasBuilderInput::m_forcePowerOf2)
                ->Field("Include White Texture", &AtlasBuilderInput::m_includeWhiteTexture)
                ->Field("Maximum Dimension", &AtlasBuilderInput::m_maxDimension)
                ->Field("Padding", &AtlasBuilderInput::m_padding)
                ->Field("UnusedColor", &AtlasBuilderInput::m_unusedColor)
                ->Field("PresetName", &AtlasBuilderInput::m_presetName)
                ->Field("Textures to Add", &AtlasBuilderInput::m_filePaths);
        }
    }

    // Supports a custom parser format
    AtlasBuilderInput AtlasBuilderInput::ReadFromFile(const AZStd::string& path, const AZStd::string& directory, bool& valid)
    {
        // Open the file
        AZ::IO::FileIOBase* input = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::HandleType handle;
        input->Open(path.c_str(), AZ::IO::OpenMode::ModeRead, handle);

        // Read the file
        AZ::u64 size;
        input->Size(handle, size);
        char* buffer = new char[size + 1];
        input->Read(handle, buffer, size);
        buffer[size] = 0;

        // Close the file
        input->Close(handle);

        // Prepare the output
        AtlasBuilderInput data;

        // Parse the input into lines
        AZStd::vector<AZStd::string> lines;
        AzFramework::StringFunc::Tokenize(buffer, lines, "\n\t");
        delete[] buffer;

        // Parse the individual lines
        for (auto line : lines)
        {
            line = AzFramework::StringFunc::TrimWhiteSpace(line, true, true);
            // Check for comments and empty lines
            if ((line.length() >= 2 && line[0] == '/' && line[1] == '/') || line.length() < 1)
            {
                continue;
            }
            else if (line.find('=') != -1)
            {
                AZStd::vector<AZStd::string> args;
                AzFramework::StringFunc::Tokenize(line.c_str(), args, '=', true, true);

                if (args.size() > 2)
                {
                    AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to parse line: Excessive '=' symbols were found: \"%s\"", line.c_str()).c_str());
                    valid = false;
                }

                // Trim whitespace
                args[0] = AzFramework::StringFunc::TrimWhiteSpace(args[0], true, true);
                args[1] = AzFramework::StringFunc::TrimWhiteSpace(args[1], true, true);

                // No case sensitivity for property names
                AZStd::to_lower(args[0].begin(), args[0].end());

                // Keep track of if the value is rejected
                bool accepted = false;

                if (args[0] == "square")
                {
                    accepted = AzFramework::StringFunc::LooksLikeBool(args[1].c_str());
                    if (accepted)
                    {
                        data.m_forceSquare = AzFramework::StringFunc::ToBool(args[1].c_str());
                    }
                }
                else if (args[0] == "poweroftwo")
                {
                    accepted = AzFramework::StringFunc::LooksLikeBool(args[1].c_str());
                    if (accepted)
                    {
                        data.m_forcePowerOf2 = AzFramework::StringFunc::ToBool(args[1].c_str());
                    }
                }
                else if (args[0] == "whitetexture")
                {
                    accepted = AzFramework::StringFunc::LooksLikeBool(args[1].c_str());
                    if (accepted)
                    {
                        data.m_includeWhiteTexture = AzFramework::StringFunc::ToBool(args[1].c_str());
                    }
                }
                else if (args[0] == "maxdimension")
                {
                    accepted = AzFramework::StringFunc::LooksLikeInt(args[1].c_str());
                    if (accepted)
                    {
                        data.m_maxDimension = AzFramework::StringFunc::ToInt(args[1].c_str());
                    }
                }
                else if (args[0] == "padding")
                {
                    accepted = AzFramework::StringFunc::LooksLikeInt(args[1].c_str());
                    if (accepted)
                    {
                        data.m_padding = AzFramework::StringFunc::ToInt(args[1].c_str());
                    }
                }
                else if (args[0] == "unusedcolor")
                {
                    accepted = args[1].at(0) == '#' && args[1].length() == 9;
                    if (accepted)
                    {
                        AZStd::string color = AZStd::string::format("%s%s%s%s", args[1].substr(7).c_str(), args[1].substr(5, 2).c_str(),
                                                                    args[1].substr(3, 2).c_str(), args[1].substr(1, 2).c_str());
                        data.m_unusedColor.FromU32(static_cast<AZ::u32>(AZStd::stoul(color, nullptr, 16)));
                    }
                }
                else if (args[0] == "presetname")
                {
                    accepted = true;
                    data.m_presetName = args[1];
                }
                else
                {
                    // Supress accepted error because this error superceeds it
                    accepted = true;
                    valid = false;
                    AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to parse line: Unrecognized property: \"%s\"", args[0].c_str()).c_str());
                }

                // If the property is recognized but the value is rejected, fail the job
                if (!accepted)
                {
                    valid = false;
                    AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to parse line: Invalid value assigned to property: Property: \"%s\" Value: \"%s\"", args[0].c_str(), args[1].c_str()).c_str());
                }
            }
            else if ((line[0] == '-'))
            {
                // Remove image files
                AZStd::string remove = line.substr(1);
                remove = AzFramework::StringFunc::TrimWhiteSpace(remove, true, true);
                if (remove.find('*') != -1)
                {
                    AZStd::string resolvedAbsolutePath;
                    bool resolved = ResolveRelativePath(remove, directory, resolvedAbsolutePath);
                    if (resolved)
                    {
                        RemoveFilesUsingWildCard(data.m_filePaths, resolvedAbsolutePath);
                    }
                    else
                    {
                        valid = false;
                        AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to resolve relative path: %s", remove.c_str()).c_str());
                    }
                }
                else if (IsFolderPath(remove))
                {
                    AZStd::string resolvedAbsolutePath;
                    bool resolved = ResolveRelativePath(remove, directory, resolvedAbsolutePath);
                    if (resolved)
                    {
                        RemoveFolderContents(data.m_filePaths, resolvedAbsolutePath);
                    }
                    else
                    {
                        valid = false;
                        AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to resolve relative path: %s", remove.c_str()).c_str());
                    }
                }
                else
                {
                    // Get the full path to the source image from the relative source path
                    AZStd::string fullSourceAssetPathName;
                    bool fullPathFound = GetAbsoluteSourcePathFromRelativePath(remove, fullSourceAssetPathName);

                    if (!fullPathFound)
                    {
                        // Try to resolve relative path as it might be using "./" or "../"
                        fullPathFound = ResolveRelativePath(remove, directory, fullSourceAssetPathName);
                    }

                    if (fullPathFound)
                    {
                        for (size_t i = 0; i < data.m_filePaths.size(); ++i)
                        {
                            if (data.m_filePaths[i] == fullSourceAssetPathName)
                            {
                                data.m_filePaths.erase(data.m_filePaths.begin() + i);
                            }
                        }
                    }
                    else
                    {
                        valid = false;
                        AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to get source asset path for image: %s", remove.c_str()).c_str());
                    }
                }
            }
            else
            {
                // Add image files
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, line);
                bool duplicate = false;
                if (line.find('*') != -1)
                {
                    AZStd::string resolvedAbsolutePath;
                    bool resolved = ResolveRelativePath(line, directory, resolvedAbsolutePath);
                    if (resolved)
                    {
                        AddFilesUsingWildCard(data.m_filePaths, resolvedAbsolutePath);
                    }
                    else
                    {
                        valid = false;
                        AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to resolve relative path: %s", line.c_str()).c_str());
                    }
                }
                else if (IsFolderPath(line))
                {
                    AZStd::string resolvedAbsolutePath;
                    bool resolved = ResolveRelativePath(line, directory, resolvedAbsolutePath);
                    if (resolved)
                    {
                        AddFolderContents(data.m_filePaths, resolvedAbsolutePath, valid);
                    }
                    else
                    {
                        valid = false;
                        AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to resolve relative path: %s", line.c_str()).c_str());
                    }
                }
                else
                {
                    // Get the full path to the source image from the relative source path
                    AZStd::string fullSourceAssetPathName;
                    bool fullPathFound = GetAbsoluteSourcePathFromRelativePath(line, fullSourceAssetPathName);

                    if (!fullPathFound)
                    {
                        // Try to resolve relative path as it might be using "./" or "../"
                        fullPathFound = ResolveRelativePath(line, directory, fullSourceAssetPathName);
                    }

                    if (fullPathFound)
                    {
                        // Prevent duplicates
                        for (size_t i = 0; i < data.m_filePaths.size() && !duplicate; ++i)
                        {
                            duplicate = data.m_filePaths[i] == fullSourceAssetPathName;
                        }
                        if (!duplicate)
                        {
                            data.m_filePaths.push_back(fullSourceAssetPathName);
                        }
                    }
                    else
                    {
                        valid = false;
                        AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to get source asset path for image: %s", line.c_str()).c_str());
                    }
                }
            }
        }

        return data;
    }

    void AtlasBuilderInput::AddFilesUsingWildCard(AZStd::vector<AZStd::string>& paths, const AZStd::string& insert)
    {
        const AZStd::string& fullPath = insert;

        AZStd::vector<AZStd::string> candidates;
        AZStd::string fixedPath = fullPath.substr(0, fullPath.find('*'));
        fixedPath = fixedPath.substr(0, fixedPath.find_last_of('/'));
        candidates.push_back(fixedPath);

        AZStd::vector<AZStd::string> wildPath;
        AzFramework::StringFunc::Tokenize(fullPath.substr(fixedPath.length()).c_str(), wildPath, "/");

        for (size_t i = 0; i < wildPath.size() && candidates.size() > 0; ++i)
        {
            AZStd::vector<AZStd::string> nextCandidates;
            for (size_t j = 0; j < candidates.size(); ++j)
            {
                AZStd::string compare = AZStd::string::format("%s/%s", candidates[j].c_str(), wildPath[i].c_str());
                QDir inputFolder(candidates[j].c_str());
                if (inputFolder.exists())
                {
                    QFileInfoList entries = inputFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);
                    for (const QFileInfo& entry : entries)
                    {
                        AZStd::string child = (entry.filePath().toStdString()).c_str();
                        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, child);
                        if (DoesPathnameMatchWildCard(compare, child))
                        {
                            nextCandidates.push_back(child);
                        }
                    }
                }
            }
            candidates = nextCandidates;
        }

        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (!IsFolderPath(candidates[i]) && !HasTrailingSlash(fullPath))
            {
                AZStd::string ext;
                AzFramework::StringFunc::Path::GetExtension(candidates[i].c_str(), ext, false);

                bool extensionSupported = false;
                ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(extensionSupported, &ImageProcessingAtom::ImageBuilderRequestBus::Events::IsExtensionSupported, ext.c_str());
                if (extensionSupported)
                {
                    bool duplicate = false;
                    for (size_t j = 0; j < paths.size() && !duplicate; ++j)
                    {
                        duplicate = paths[j] == candidates[i];
                    }
                    if (!duplicate)
                    {
                        paths.push_back(candidates[i]);
                    }
                }
            }
            else if (IsFolderPath(candidates[i]) && HasTrailingSlash(fullPath))
            {
                bool waste = true;
                AddFolderContents(paths, candidates[i], waste);
            }
        }
    }

    void AtlasBuilderInput::RemoveFilesUsingWildCard(AZStd::vector<AZStd::string>& paths, const AZStd::string& remove)
    {
        bool isDir = (remove.at(remove.length() - 1) == '/');
        for (size_t i = 0; i < paths.size(); ++i)
        {
            if (isDir ? DoesWildCardDirectoryIncludePathname(remove, paths[i]) : DoesPathnameMatchWildCard(remove, paths[i]))
            {
                paths.erase(paths.begin() + i);
                --i;
            }
        }
    }

    // Tells us if the child follows the rule
    bool AtlasBuilderInput::DoesPathnameMatchWildCard(const AZStd::string& rule, const AZStd::string& child)
    {
        AZStd::vector<AZStd::string> rulePathTokens;
        AzFramework::StringFunc::Tokenize(rule.c_str(), rulePathTokens, "/");
        AZStd::vector<AZStd::string> pathTokens;
        AzFramework::StringFunc::Tokenize(child.c_str(), pathTokens, "/");
        if (rulePathTokens.size() != pathTokens.size())
        {
            return false;
        }
        for (size_t i = 0; i < rulePathTokens.size(); ++i)
        {
            if (!TokenMatchesWildcard(rulePathTokens[i], pathTokens[i]))
            {
                return false;
            }
        }
        return true;
    }

    bool AtlasBuilderInput::DoesWildCardDirectoryIncludePathname(const AZStd::string& rule, const AZStd::string& child)
    {
        AZStd::vector<AZStd::string> rulePathTokens;
        AzFramework::StringFunc::Tokenize(rule.c_str(), rulePathTokens, "/");
        AZStd::vector<AZStd::string> pathTokens;
        AzFramework::StringFunc::Tokenize(child.c_str(), pathTokens, "/");
        if (rulePathTokens.size() >= pathTokens.size())
        {
            return false;
        }
        for (size_t i = 0; i < rulePathTokens.size(); ++i)
        {
            if (!TokenMatchesWildcard(rulePathTokens[i], pathTokens[i]))
            {
                return false;
            }
        }
        return true;
    }

    bool AtlasBuilderInput::TokenMatchesWildcard(const AZStd::string& rule, const AZStd::string& child)
    {
        AZStd::vector<AZStd::string> ruleTokens;
        AzFramework::StringFunc::Tokenize(rule.c_str(), ruleTokens, "*");
        size_t pos = 0;
        int token = 0;
        if (rule.at(0) != '*' && child.find(ruleTokens[0]) != 0)
        {
            return false;
        }

        while (pos != AZStd::string::npos && token < ruleTokens.size())
        {
            pos = child.find(ruleTokens[token], pos);
            if (pos != AZStd::string::npos)
            {
                pos += ruleTokens[token].size();
            }
            ++token;
        }
        return pos == child.size() || (pos != AZStd::string::npos && rule.at(rule.length() - 1) == '*');
    }

    // Replaces all folder paths with the files they contain
    void AtlasBuilderInput::AddFolderContents(AZStd::vector<AZStd::string>& paths, const AZStd::string& insert, bool& valid)
    {
        QDir inputFolder(insert.c_str());

        if (inputFolder.exists())
        {
            QFileInfoList entries = inputFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);
            for (const QFileInfo& entry : entries)
            {
                AZStd::string child = (entry.filePath().toStdString()).c_str();
                AZStd::string ext;

                bool isDir = !AzFramework::StringFunc::Path::GetExtension(child.c_str(), ext, false);
                if (isDir)
                {
                    AddFolderContents(paths, child, valid);
                    continue;
                }

                // Not a directory - add the file if it supports an image extension
                bool extensionSupported = false;
                ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(extensionSupported, &ImageProcessingAtom::ImageBuilderRequestBus::Events::IsExtensionSupported, ext.c_str());

                if (extensionSupported)    
                {
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, child);
                    bool duplicate = false;
                    for (size_t i = 0; i < paths.size() && !duplicate; ++i)
                    {
                        duplicate = paths[i] == child;
                    }
                    if (!duplicate)
                    {
                        paths.push_back(child);
                    }
                }
            }
        }
        else
        {
            valid = false;
            AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to find requested directory: %s", insert.c_str()).c_str());
        }
    }

    // Removes all of the contents of a folder
    void AtlasBuilderInput::RemoveFolderContents(AZStd::vector<AZStd::string>& paths, const AZStd::string& remove)
    {
        AZStd::string folder = remove;
        AzFramework::StringFunc::Strip(folder, "/", false, false, true);
        folder.append("/");
        for (size_t i = 0; i < paths.size(); ++i)
        {
            if (paths[i].find(folder) == 0)
            {
                paths.erase(paths.begin() + i);
                --i;
            }
        }
    }

    // Note - Shutdown will be called on a different thread than your process job thread
    void AtlasBuilderWorker::ShutDown() { m_isShuttingDown = true; }

    void AtlasBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request,
        AssetBuilderSDK::CreateJobsResponse& response)
    {
        // Read in settings/filepaths to set dependencies
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(
            request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, true, true);
        // Check if input is valid
        bool valid = true;
        AtlasBuilderInput input = AtlasBuilderInput::ReadFromFile(fullPath, request.m_watchFolder, valid);

        // Set dependencies
        for (int i = 0; i < input.m_filePaths.size(); ++i)
        {
            AssetBuilderSDK::SourceFileDependency dependency;
            dependency.m_sourceFileDependencyPath = input.m_filePaths[i].c_str();
            response.m_sourceFileDependencyList.push_back(dependency);
        }

        // We process the same file for all platforms
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            bool doesSupportPlatform = false;
            ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(doesSupportPlatform,
                &ImageProcessingAtom::ImageBuilderRequests::DoesSupportPlatform,
                info.m_identifier);
            if (doesSupportPlatform)
            {
                AssetBuilderSDK::JobDescriptor descriptor = GetJobDescriptor(request.m_sourceFile, input);
                descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                response.m_createJobOutputs.push_back(descriptor);
            }
        }

        if (valid)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        return;
    }

    AssetBuilderSDK::JobDescriptor AtlasBuilderWorker::GetJobDescriptor(const AZStd::string& sourceFile, const AtlasBuilderInput& input)
    {
        // Get the extension of the file
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(sourceFile.c_str(), ext, false);
        AZStd::to_upper(ext.begin(), ext.end());

        AssetBuilderSDK::JobDescriptor descriptor;
        descriptor.m_jobKey = ext + " Atlas";
        descriptor.m_critical = false;
        descriptor.m_jobParameters[AZ_CRC_CE("forceSquare")] = input.m_forceSquare ? "true" : "false";
        descriptor.m_jobParameters[AZ_CRC_CE("forcePowerOf2")] = input.m_forcePowerOf2 ? "true" : "false";
        descriptor.m_jobParameters[AZ_CRC_CE("includeWhiteTexture")] = input.m_includeWhiteTexture ? "true" : "false";
        descriptor.m_jobParameters[AZ_CRC_CE("padding")] = AZStd::to_string(input.m_padding);
        descriptor.m_jobParameters[AZ_CRC_CE("maxDimension")] = AZStd::to_string(input.m_maxDimension);
        descriptor.m_jobParameters[AZ_CRC_CE("filePaths")] = AZStd::to_string(input.m_filePaths.size());

        AZ::u32 col = input.m_unusedColor.ToU32();
        descriptor.m_jobParameters[AZ_CRC_CE("unusedColor")] = AZStd::to_string(*reinterpret_cast<int*>(&col));
        descriptor.m_jobParameters[AZ_CRC_CE("presetName")] = input.m_presetName;

        // The starting point for the list
        const int start = static_cast<int>(descriptor.m_jobParameters.size()) + 1;
        descriptor.m_jobParameters[AZ_CRC_CE("startPoint")] = AZStd::to_string(start);

        for (int i = 0; i < input.m_filePaths.size(); ++i)
        {
            descriptor.m_jobParameters[start + i] = input.m_filePaths[i];
        }

        return descriptor;
    }

    void AtlasBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Before we begin, let's make sure we are not meant to abort.
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        const AZStd::string path = request.m_fullPath;

        // read in settings/filepaths
        AtlasBuilderInput input;
        input.m_forceSquare = AzFramework::StringFunc::ToBool(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("forceSquare"))->second.c_str());
        input.m_forcePowerOf2 = AzFramework::StringFunc::ToBool(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("forcePowerOf2"))->second.c_str());
        input.m_includeWhiteTexture = AzFramework::StringFunc::ToBool(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("includeWhiteTexture"))->second.c_str());
        input.m_padding = AzFramework::StringFunc::ToInt(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("padding"))->second.c_str());
        input.m_maxDimension = AzFramework::StringFunc::ToInt(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("maxDimension"))->second.c_str());
        int startAsInt = AzFramework::StringFunc::ToInt(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("startPoint"))->second.c_str());
        int sizeAsInt = AzFramework::StringFunc::ToInt(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("filePaths"))->second.c_str());
        AZ::u32 start = static_cast<AZ::u32>(AZStd::max(0, startAsInt));
        AZ::u32 size = static_cast<AZ::u32>(AZStd::max(0, sizeAsInt));

        int col = AzFramework::StringFunc::ToInt(request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("unusedColor"))->second.c_str());
        input.m_unusedColor.FromU32(*reinterpret_cast<AZ::u32*>(&col));

        input.m_presetName = request.m_jobDescription.m_jobParameters.find(AZ_CRC_CE("presetName"))->second;

        for (AZ::u32 i = 0; i < size; ++i)
        {
            input.m_filePaths.push_back(request.m_jobDescription.m_jobParameters.find(start + i)->second);
        }

        if (input.m_filePaths.empty())
        {
            AZ_Error("AtlasBuilder", false, "No image files specified. Cannot create an empty atlas.");
            return;
        }

        // Don't allow padding to be less than zero
        if (input.m_padding < 0)
        {
            input.m_padding = 0;
        }

        if (input.m_presetName.empty())
        {
            // Default to the TextureAtlas preset which is currently set to use compression
            const AZStd::string defaultPresetName = "UserInterface_Compressed";
            input.m_presetName = defaultPresetName;
        }

        bool isFormatSquarePow2 = false;
        ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(isFormatSquarePow2,
            &ImageProcessingAtom::ImageBuilderRequests::IsPresetFormatSquarePow2,
            input.m_presetName, request.m_platformInfo.m_identifier);

        if (isFormatSquarePow2)
        {
            // Override the user config settings to force square and power of 2.
            // Otherwise the image conversion process will stretch the image to satisfy these requirements
            input.m_forceSquare = true;
            input.m_forcePowerOf2 = true;
        }

        // Read in images
        AZStd::vector<ImageProcessingAtom::IImageObjectPtr> images;
        AZ::u64 totalArea = 0;
        int maxArea = input.m_maxDimension * input.m_maxDimension;
        bool sizeFailure = false;
        for (int i = 0; i < input.m_filePaths.size() && !jobCancelListener.IsCancelled(); ++i)
        {
            ImageProcessingAtom::IImageObjectPtr inputImage;
            ImageProcessingAtom::ImageProcessingRequestBus::BroadcastResult(inputImage, &ImageProcessingAtom::ImageProcessingRequests::LoadImage, input.m_filePaths[i]);

            // Check if we were able to load the image
            if (inputImage)
            {
                images.push_back(inputImage);
                totalArea += inputImage->GetWidth(0) * inputImage->GetHeight(0);
            }
            else
            {
                AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to load file: %s", input.m_filePaths[i].c_str()).c_str());
                return;
            }
            if (maxArea < totalArea)
            {
                sizeFailure = true;
            }
        }
        // If we get cancelled, return
        if (jobCancelListener.IsCancelled())
        {
            return;
        }

        if (sizeFailure)
        {
            AZ_Error("AtlasBuilder", false, AZStd::string::format("Total image area exceeds maximum alotted area. %llu > %d", totalArea, maxArea).c_str());
            return;
        }

        // Convert all image paths to their output format referenced at runtime
        for (auto& filePath : input.m_filePaths)
        {
            // Get path relative to the watch folder
            bool result = false;
            AZ::Data::AssetInfo info;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, filePath.c_str(), info, watchFolder);
            if (!result)
            {
                AZ_Error("AtlasBuilder", false, AZStd::string::format("Atlas Builder unable to get relative source path for image: %s", filePath.c_str()).c_str());
                return;
            }

            // Remove extension
            filePath = info.m_relativePath.substr(0, info.m_relativePath.find_last_of('.'));

            // Normalize path
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, filePath);
        }

        // Add white texture if we need to
        if (input.m_includeWhiteTexture)
        {
            ImageProcessingAtom::IImageObjectPtr texture;
            ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(texture,
                &ImageProcessingAtom::ImageBuilderRequests::CreateImage,
                aznumeric_cast<AZ::u32>(cellSize),
                aznumeric_cast<AZ::u32>(cellSize),
                1,
                ImageProcessingAtom::EPixelFormat::ePixelFormat_R8G8B8A8);

            // Make the texture white
            texture->ClearColor(1, 1, 1, 1);
            images.push_back(texture);
            input.m_filePaths.push_back("WhiteTexture");
        }

        // Generate algorithm inputs
        ImageDimensionData data;
        for (int i = 0; i < images.size(); ++i)
        {
            data.push_back(IndexImageDimension(i,
                ImageDimension(images[i]->GetWidth(0),
                images[i]->GetHeight(0))));
        }
        AZStd::sort(data.begin(), data.end());

        // Run algorithm

        // Variables that keep track of the optimal solution
        int resultWidth = -1;
        int resultHeight = -1;

        // Check that the max dimension is not large enough for the area to loop past the maximum integer
        // This is important because we do not want the area to be calculated negative
        if (input.m_maxDimension > 65535)
        {
            input.m_maxDimension = 65535;
        }

        // Get the optimal mappings based on the input settings
        AZStd::vector<AtlasCoordinates> paddedMap;
        size_t amountFit = 0;
        if (!TryTightening(
            input, data, GetWidest(data), GetTallest(data), aznumeric_cast<int>(totalArea), input.m_padding, resultWidth, resultHeight, amountFit, paddedMap))
        {
            AZ_Error("AtlasBuilder", false, AZStd::string::format("Cannot fit images into given maximum atlas size (%dx%d). Only %zu out of %zu images fit.", input.m_maxDimension, input.m_maxDimension, amountFit, input.m_filePaths.size()).c_str());
            // For some reason, failing the assert isn't enough to stop the Asset builder. It will still fail further
            // down when it tries to assemble the atlas, but returning here is cleaner.
            return;
        }

        // Move coordinates from algorithm space to padded result space
        TextureAtlasNamespace::AtlasCoordinateSets output;
        resultWidth = 0;
        resultHeight = 0;
        AZStd::vector<AtlasCoordinates> map;
        for (int i = 0; i < paddedMap.size(); ++i)
        {
            map.push_back(AtlasCoordinates(paddedMap[i].GetLeft(), paddedMap[i].GetLeft() + images[data[i].first]->GetWidth(0), paddedMap[i].GetTop(), paddedMap[i].GetTop() + images[data[i].first]->GetHeight(0)));
            resultHeight = resultHeight > map[i].GetBottom() ? resultHeight : map[i].GetBottom();
            resultWidth = resultWidth > map[i].GetRight() ? resultWidth : map[i].GetRight();

            const AZStd::string& outputFilePath = input.m_filePaths[data[i].first];
            output.push_back(AZStd::pair<AZStd::string, AtlasCoordinates>(outputFilePath, map[i]));
        }
        if (input.m_forcePowerOf2)
        {
            resultWidth = aznumeric_cast<int>(pow(2, 1 + IntegerLog2(static_cast<uint32_t>(resultWidth - 1))));
            resultHeight = aznumeric_cast<int>(pow(2, 1 + IntegerLog2(static_cast<uint32_t>(resultHeight - 1))));
        }
        else
        {
            resultWidth = (resultWidth + (cellSize - 1)) / cellSize * cellSize;
            resultHeight = (resultHeight + (cellSize - 1)) / cellSize * cellSize;
        }
        if (input.m_forceSquare)
        {
            if (resultWidth > resultHeight)
            {
                resultHeight = resultWidth;
            }
            else
            {
                resultWidth = resultHeight;
            }
        }

        // Process texture sheet
        ImageProcessingAtom::IImageObjectPtr outImage;
        ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(outImage,
            &ImageProcessingAtom::ImageBuilderRequests::CreateImage,
            aznumeric_cast<AZ::u32>(resultWidth),
            aznumeric_cast<AZ::u32>(resultHeight),
            1,
            ImageProcessingAtom::EPixelFormat::ePixelFormat_R8G8B8A8);

        // Clear the sheet
        outImage->ClearColor(input.m_unusedColor.GetR(), input.m_unusedColor.GetG(), input.m_unusedColor.GetB(), input.m_unusedColor.GetA());

        AZ::u8* outBuffer = nullptr;
        AZ::u32 outPitch;
        outImage->GetImagePointer(0, outBuffer, outPitch);

        // Copy images over
        for (int i = 0; i < map.size() && !jobCancelListener.IsCancelled(); ++i)
        {
            AZ::u8* inBuffer = nullptr;
            AZ::u32 inPitch;
            images[data[i].first]->GetImagePointer(0, inBuffer, inPitch);
            int j = 0;

            // The padding calculated here is the amount of excess horizontal space measured in bytes that are in each
            // row of the destination space AFTER the placement of the source row.
            int rightPadding = (paddedMap[i].GetRight() - map[i].GetRight() - input.m_padding);
            if (map[i].GetRight() + rightPadding > resultWidth)
            {
                rightPadding = resultWidth - map[i].GetRight();
            }
            rightPadding *= bytesPerPixel;
            int bottomPadding = (paddedMap[i].GetBottom() - map[i].GetBottom() - input.m_padding);
            if (map[i].GetBottom() + bottomPadding > resultHeight)
            {
                bottomPadding = resultHeight - map[i].GetBottom();
            }

            int leftPadding = 0;
            if (map[i].GetLeft() - input.m_padding >= 0)
            {
                leftPadding = input.m_padding * bytesPerPixel;
            }

            int topPadding = 0;
            if (map[i].GetTop() - input.m_padding >= 0)
            {
                topPadding = input.m_padding;
            }

            for (j = 0; j < map[i].GetHeight(); ++j)
            {
                // When we multiply `map[i].GetLeft()` by 4, we are changing the measure from atlas space, to byte array
                // space. The number is 4 because in this format, each pixel is 4 bytes long.
                memcpy(outBuffer + (map[i].GetTop() + j) * outPitch + (map[i].GetLeft() * bytesPerPixel),
                    inBuffer + inPitch * j,
                    inPitch);
                // Fill in the last bit of the row in the destination space with the same colors
                SetPixels(outBuffer + (map[i].GetTop() + j) * outPitch + (map[i].GetLeft() * bytesPerPixel) + inPitch,
                    outBuffer + (map[i].GetTop() + j) * outPitch + (map[i].GetLeft() * bytesPerPixel) + inPitch - bytesPerPixel,
                    rightPadding);
                // Fill in the first bit of the row in the destination space with the same colors
                SetPixels(outBuffer + (map[i].GetTop() + j) * outPitch + (map[i].GetLeft() * bytesPerPixel) - leftPadding,
                    outBuffer + (map[i].GetTop() + j) * outPitch + (map[i].GetLeft() * bytesPerPixel),
                    leftPadding);
            }
            // Fill in the last few rows of the buffer with the same colors
            for (; j < map[i].GetHeight() + bottomPadding; ++j)
            {
                memcpy(outBuffer + (map[i].GetTop() + j) * outPitch + (map[i].GetLeft() * bytesPerPixel) - leftPadding,
                    outBuffer + (map[i].GetBottom() - 1) * outPitch + (map[i].GetLeft() * bytesPerPixel) - leftPadding,
                    inPitch + leftPadding + rightPadding);
            }
            for (j = 1; j <= topPadding; ++j)
            {
                memcpy(outBuffer + (map[i].GetTop() - j) * outPitch + (map[i].GetLeft() * bytesPerPixel) - leftPadding,
                    outBuffer + map[i].GetTop() * outPitch + (map[i].GetLeft() * bytesPerPixel) - leftPadding,
                    inPitch + rightPadding + leftPadding);
            }
        }

        // If we get cancelled, return
        if (jobCancelListener.IsCancelled())
        {
            return;
        }

        // Output Atlas Coordinates
        AZStd::string fileName;
        AZStd::string outputPath;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileName);
        fileName = fileName.append("idx");
        AzFramework::StringFunc::Path::Join(
            request.m_tempDirPath.c_str(), fileName.c_str(), outputPath, true, true);

        // Output texture sheet
        AZStd::string imageFileName, imageOutputPath;
        AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), imageFileName);
        imageFileName += ".texatlas";
        AzFramework::StringFunc::Path::Join(
            request.m_tempDirPath.c_str(), imageFileName.c_str(), imageOutputPath, true, true);

        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;
        ImageProcessingAtom::ImageBuilderRequestBus::BroadcastResult(outProducts,
            &ImageProcessingAtom::ImageBuilderRequests::ConvertImageObject,
            outImage,
            input.m_presetName,
            request.m_platformInfo.m_identifier,
            imageOutputPath,
            request.m_sourceFileUUID,
            request.m_sourceFile);

        if (!outProducts.empty())
        {
            TextureAtlasNamespace::TextureAtlasRequestBus::Broadcast(
                &TextureAtlasNamespace::TextureAtlasRequests::SaveAtlasToFile, outputPath, output, resultWidth, resultHeight);
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(outputPath));
            response.m_outputProducts[static_cast<int>(Product::TexatlasidxProduct)].m_productAssetType = azrtti_typeid<TextureAtlasNamespace::TextureAtlasAsset>();
            response.m_outputProducts[static_cast<int>(Product::TexatlasidxProduct)].m_productSubID = 0;

            // The Image Processing Gem can produce multiple output files under certain
            // circumstances, but the texture atlas is not expected to produce such output
            // There should only be the texture atlas and its abdata file
            if (outProducts.size() > 2)
            {
                AZ_Error("AtlasBuilder", false, "Image processing resulted in multiple output files. Texture atlas is expected to produce one output.");
                response.m_outputProducts.clear();
                return;
            }

            response.m_outputProducts.insert(response.m_outputProducts.end(), outProducts.begin(), outProducts.end());

            // The texatlasidx file is a data file that indicates where the original parts are inside the atlas, 
            // and this would usually imply that it refers to its dds file in some way or needs it to function.
            // The texatlasidx file should be the one that depends on the DDS because it's possible to use the DDS
            // without the texatlasid, but not the other way around
            AZ::Data::AssetId productAssetId(request.m_sourceFileUUID, response.m_outputProducts.back().m_productSubID);
            response.m_outputProducts[static_cast<int>(Product::TexatlasidxProduct)].m_dependencies.push_back(AssetBuilderSDK::ProductDependency(productAssetId, 0));
            response.m_outputProducts[static_cast<int>(Product::TexatlasidxProduct)].m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
    }

    bool AtlasBuilderWorker::TryPack(const ImageDimensionData& images,
        int targetWidth,
        int targetHeight,
        int padding,
        size_t& amountFit,
        AZStd::vector<AtlasCoordinates>& out)
    {
        // Start with one open slot and initialize a vector to store the closed products
        AZStd::vector<AtlasCoordinates> open;
        AZStd::vector<AtlasCoordinates> closed;
        open.push_back(AtlasCoordinates(0, targetWidth, 0, targetHeight));
        bool slotNotFound = false;
        for (size_t i = 0; i < images.size() && !slotNotFound; ++i)
        {
            slotNotFound = true;
            // Try to place the image in every open slot
            for (size_t j = 0; j < open.size(); ++j)
            {
                if (CanInsert(open[j], images[i].second, padding, targetWidth, targetHeight))
                {
                    // if it fits, subdivide the excess space in the slot, add it back to the open list and place the
                    // filled space into the closed vector
                    slotNotFound = false;
                    AtlasCoordinates spent(open[j].GetLeft(),
                        open[j].GetLeft() + images[i].second.m_width,
                        open[j].GetTop(),
                        open[j].GetTop() + images[i].second.m_height);

                    // We are going to try pushing the object up / left to try to avoid creating tight open spaces.
                    bool needTrim = false;
                    AtlasCoordinates coords = spent;
                    // Modifying left will preserve width
                    coords.SetLeft(coords.GetLeft() - 1);
                    AddPadding(coords, padding, targetWidth, targetHeight);
                    while (spent.GetLeft() > 0 && !Collides(coords, closed))
                    {
                        spent.SetLeft(coords.GetLeft());
                        coords = spent;
                        coords.SetLeft(coords.GetLeft() - 1);
                        AddPadding(coords, padding, targetWidth, targetHeight);
                        needTrim = true;
                    }
                    // Refocus the search to see if we can push up
                    coords = spent;
                    coords.SetTop(coords.GetTop() - 1);
                    AddPadding(coords, padding, targetWidth, targetHeight);
                    while (spent.GetTop() > 0 && !Collides(coords, closed))
                    {
                        spent.SetTop(coords.GetTop());
                        coords = spent;
                        coords.SetTop(coords.GetTop() - 1);
                        AddPadding(coords, padding, targetWidth, targetHeight);
                        needTrim = true;
                    }
                    AddPadding(spent, padding, targetWidth, targetHeight);
                    if (needTrim)
                    {
                        TrimOverlap(open, spent);
                        closed.push_back(spent);
                        break;
                    }
                    AtlasCoordinates bigCoords;
                    AtlasCoordinates smallCoords;

                    // Create the largest possible subdivision and another subdivision that uses the left over space
                    if (open[j].GetBottom() - spent.GetBottom() < open[j].GetRight() - spent.GetRight())
                    {
                        smallCoords = AtlasCoordinates(
                            open[j].GetLeft(), spent.GetRight(), spent.GetBottom(), open[j].GetBottom());
                        bigCoords = AtlasCoordinates(spent.GetRight(), open[j].GetRight(), open[j].GetTop(), smallCoords.GetBottom());
                    }
                    else
                    {
                        bigCoords = AtlasCoordinates(
                            open[j].GetLeft(), open[j].GetRight(), spent.GetBottom(), open[j].GetBottom());
                        smallCoords = AtlasCoordinates(spent.GetRight(), open[j].GetRight(), open[j].GetTop(), bigCoords.GetTop());
                    }

                    open.erase(open.begin() + j, open.begin() + j + 1);
                    if (bigCoords.GetHeight() > 0 && bigCoords.GetHeight() > 0)
                    {
                        InsertInOrder(open, bigCoords);
                    }
                    if (smallCoords.GetHeight() > 0 && smallCoords.GetHeight() > 0)
                    {
                        InsertInOrder(open, smallCoords);
                    }

                    closed.push_back(spent);
                    break;
                }
            }
            if (slotNotFound)
            {
                // If no single open slot can fit the object, do one last check to see if we can fit it in at any open
                // corner. The reason we perform this check is in case the object can be fit across multiple different
                // open spaces. If there is a space that an object can be fit in, it will probably involve the top left
                // corner of that object in the top left corner of an open slot. This may miss some odd fits, but due to
                // the nature of the packing algorithm, such solutions are highly unlikely to exist. If we wanted to
                // expand the algorithm, we could theoretically base it on edges instead of corners to find all results,
                // but it would not be time efficient.
                for (size_t j = 0; j < open.size(); ++j)
                {
                    AtlasCoordinates insert = AtlasCoordinates(open[j].GetLeft(),
                        open[j].GetLeft() + images[i].second.m_width,
                        open[j].GetTop(),
                        open[j].GetTop() + images[i].second.m_height);
                    AddPadding(insert, padding, targetWidth, targetHeight);
                    if (insert.GetRight() <= targetWidth && insert.GetBottom() <= targetHeight)
                    {
                        bool collision = Collides(insert, closed);
                        if (!collision)
                        {
                            closed.push_back(insert);
                            // Trim overlapping open slots
                            TrimOverlap(open, insert);
                            slotNotFound = false;
                            break;
                        }
                    }
                }
            }
        }
        // If we succeeded, update the output
        if (!slotNotFound)
        {
            out = closed;
        }
        amountFit = amountFit > closed.size() ? amountFit : closed.size();
        return !slotNotFound;
    }

    // Modifies slotList so that no items in slotList overlap with item
    void AtlasBuilderWorker::TrimOverlap(AZStd::vector<AtlasCoordinates>& slotList, AtlasCoordinates item)
    {
        for (size_t i = 0; i < slotList.size(); ++i)
        {
            if (Collides(slotList[i], item))
            {
                // Subdivide the overlapping slot to seperate overlapping and non overlapping portions
                AtlasCoordinates overlap = GetOverlap(item, slotList[i]);
                AZStd::vector<AtlasCoordinates> excess;
                excess.push_back(AtlasCoordinates(
                    slotList[i].GetLeft(), overlap.GetRight(), slotList[i].GetTop(), overlap.GetTop()));
                excess.push_back(AtlasCoordinates(
                    slotList[i].GetLeft(), overlap.GetLeft(), overlap.GetTop(), slotList[i].GetBottom()));
                excess.push_back(AtlasCoordinates(
                    overlap.GetRight(), slotList[i].GetRight(), slotList[i].GetTop(), overlap.GetBottom()));
                excess.push_back(AtlasCoordinates(
                    overlap.GetLeft(), slotList[i].GetRight(), overlap.GetBottom(), slotList[i].GetBottom()));
                slotList.erase(slotList.begin() + i);
                for (size_t j = 0; j < excess.size(); ++j)
                {
                    if (excess[j].GetWidth() > 0 && excess[j].GetHeight() > 0)
                    {
                        InsertInOrder(slotList, excess[j]);
                    }
                }
                --i;
            }
        }
    }

    // This function interprets input and performs the proper tightening option
    bool AtlasBuilderWorker::TryTightening(AtlasBuilderInput input,
        const ImageDimensionData& images,
        int smallestWidth,
        int smallestHeight,
        int targetArea,
        int padding,
        int& resultWidth,
        int& resultHeight,
        size_t& amountFit,
        AZStd::vector<AtlasCoordinates>& out)
    {
        if (input.m_forceSquare)
        {
            return TryTighteningSquare(images,
                smallestWidth > smallestHeight ? smallestWidth : smallestHeight,
                input.m_maxDimension,
                targetArea,
                input.m_forcePowerOf2,
                padding,
                resultWidth,
                resultHeight,
                amountFit,
                out);
        }
        else
        {
            return TryTighteningOptimal(images,
                smallestWidth,
                smallestHeight,
                input.m_maxDimension,
                targetArea,
                input.m_forcePowerOf2,
                padding,
                resultWidth,
                resultHeight,
                amountFit,
                out);
        }
    }

    // Finds the optimal square solution by starting with the ideal solution and expanding the size of the space until everything fits
    bool AtlasBuilderWorker::TryTighteningSquare(const ImageDimensionData& images,
        int lowerBound,
        int maxDimension,
        int targetArea,
        bool powerOfTwo,
        int padding,
        int& resultWidth,
        int& resultHeight,
        size_t& amountFit,
        AZStd::vector<AtlasCoordinates>& out)
    {
        // Square solution cannot be smaller than the target area
        int dimension = aznumeric_cast<int>(sqrt(static_cast<float>(targetArea)));
        // Solution cannot be smaller than the smallest side
        dimension = dimension > lowerBound ? dimension : lowerBound;
        if (powerOfTwo)
        {
            // Starting dimension needs to be rounded up to the nearest power of two
            dimension = aznumeric_cast<int>(pow(2, 1 + IntegerLog2(static_cast<uint32_t>(dimension - 1))));
        }

        AZStd::vector<AtlasCoordinates> track;
        // Expand the square until the contents fit
        while (!TryPack(images, dimension, dimension, padding, amountFit, track) && dimension <= maxDimension)
        {
            // Step to the next valid value
            dimension = powerOfTwo ? dimension * 2 : dimension + cellSize;
        }
        // Make sure we found a solution
        if (dimension > maxDimension)
        {
            return false;
        }

        resultHeight = dimension;
        resultWidth = dimension;
        out = track;
        return true;
    }

    // Finds the optimal solution by starting with a somewhat optimal solution and searching for better solutions
    bool AtlasBuilderWorker::TryTighteningOptimal(const ImageDimensionData& images,
        int smallestWidth,
        int smallestHeight,
        int maxDimension,
        int targetArea,
        bool powerOfTwo,
        int padding,
        int& resultWidth,
        int& resultHeight,
        size_t& amountFit,
        AZStd::vector<AtlasCoordinates>& out)
    {
        AZStd::vector<AtlasCoordinates> track;

        // round max dimension down to a multiple of cellSize
        AZ::u32 maxDimensionRounded = maxDimension - (maxDimension % cellSize);

        // The starting width is the larger of the widest individual texture and the width required
        // to fit the total texture area given the max dimension
        AZ::u32 smallestWidthDueToArea = targetArea / maxDimensionRounded;
        AZ::u32 minWidth = AZStd::max(static_cast<AZ::u32>(smallestWidth), smallestWidthDueToArea);

        if (powerOfTwo)
        {
            // Starting dimension needs to be rounded up to the nearest power of two
            minWidth = aznumeric_cast<AZ::u32>(pow(2, 1 + IntegerLog2(static_cast<uint32_t>(minWidth - 1))));
        }

        // Round min width up to the nearest compression unit
        minWidth = (minWidth + (cellSize - 1)) / cellSize * cellSize;

        AZ::u32 height = 0;
        // Finds the optimal thin solution
        // This uses a standard binary search to find the smallest width that can pack everything
        AZ::u32 lower = minWidth;
        AZ::u32 upper = maxDimensionRounded;
        AZ::u32 width = 0;
        while (lower <= upper)
        {
            AZ::u32 testWidth = (lower + upper) / 2;    // must be divisible by cellSize because lower and upper are
            bool canPack = TryPack(images, testWidth, maxDimension, padding, amountFit, track);
            if (canPack)
            {
                // it packed, continue looking for smaller widths that pack
                width = testWidth;  // best fit so far
                upper = testWidth - cellSize;
            }
            else
            {
                // it failed to pack, don't try any widths smaller than this
                lower = testWidth + cellSize;
            }
        }
        // Make sure we found a solution
        if (width == 0)
        {
            return false;
        }

        // Find the height of the solution
        for (int i = 0; i < track.size(); ++i)
        {
            uint32_t bottom = static_cast<uint32_t>(AZStd::max(0, track[i].GetBottom()));
            if (height < bottom)
            {
                height = bottom;
            }
        }

        // Fix height for power of two when applicable
        if (powerOfTwo)
        {
            // Starting dimensions need to be rounded up to the nearest power of two
            height = aznumeric_cast<AZ::u32>(pow(2, 1 + IntegerLog2(static_cast<uint32_t>(height - 1))));
        }

        AZ::u32 resultArea = height * width;
        // This for loop starts with the optimal thin width and makes it wider at each step. For each width, it
        // calculates what height would be neccesary to have a more optimal solution than the stored solution. If the
        // more optimal solution is valid, it tries shrinking the height until the solution fails. The loop ends when it
        // is determined that a valid solution cannot exist at further steps
        for (AZ::u32 testWidth = width; testWidth <= maxDimensionRounded && resultArea / testWidth >= static_cast<AZ::u32>(smallestHeight);
            testWidth = powerOfTwo ? testWidth * 2 : testWidth + cellSize)
        {
            // The area of test height and width should be equal or less than resultArea
            // Note: We don't need to force powers of two here because the Area and the width are already powers of two
            int testHeight = resultArea / testWidth * cellSize / cellSize;
            // Try the tighter pack
            while (TryPack(images, static_cast<int>(testWidth), testHeight, padding, amountFit, track))
            {
                // Loop and continue to shrink the height until you cannot do so any further
                width = testWidth;
                height = testHeight;
                resultArea = height * width;
                // Try to step down a level
                testHeight = powerOfTwo ? testHeight / 2 : testHeight - cellSize;
            }
        }
        // Output the results of the function
        out = track;
        resultHeight = height;
        resultWidth = width;
        return true;
    }

    // Allows us to keep the list of open spaces in order from lowest to highest area
    void AtlasBuilderWorker::InsertInOrder(AZStd::vector<AtlasCoordinates>& slotList, AtlasCoordinates item)
    {
        int area = item.GetWidth() * item.GetHeight();
        for (size_t i = 0; i < slotList.size(); ++i)
        {
            if (area < slotList[i].GetWidth() * slotList[i].GetHeight())
            {
                slotList.insert(slotList.begin() + i, item);
                return;
            }
        }
        slotList.push_back(item);
    }

    // Defines priority so that sorting can be meaningful. It may seem odd that larger items are "less than" smaller
    // ones, but as this is a deduction of priority, not value, it is correct.
    bool operator<(ImageDimension a, ImageDimension b)
    {
        // Prioritize first by longest size
        if ((a.m_width > a.m_height ? a.m_width : a.m_height) != (b.m_width > b.m_height ? b.m_width : b.m_height))
        {
            return (a.m_width > a.m_height ? a.m_width : a.m_height) > (b.m_width > b.m_height ? b.m_width : b.m_height);
        }
        // Prioritize second by the length of the smaller side
        if (a.m_width * a.m_height != b.m_width * b.m_height)
        {
            return a.m_width * a.m_height > b.m_width * b.m_height;
        }
        // Prioritize wider objects over taller objects for objects of the same size
        else
        {
            return a.m_width > b.m_width;
        }
    }

    // Exposes priority logic to the sorting algorithm
    bool operator<(IndexImageDimension a, IndexImageDimension b) { return a.second < b.second; }

    // Tests if two coordinate sets intersect
    bool Collides(AtlasCoordinates a, AtlasCoordinates b)
    {
        return !((a.GetRight() <= b.GetLeft()) || (a.GetBottom() <= b.GetTop()) || (b.GetRight() <= a.GetLeft())
            || (b.GetBottom() <= a.GetTop()));
    }

    // Tests if an item collides with any items in a list
    bool Collides(AtlasCoordinates item, AZStd::vector<AtlasCoordinates> list)
    {
        for (size_t i = 0; i < list.size(); ++i)
        {
            if (Collides(list[i], item))
            {
                return true;
            }
        }
        return false;
    }

    // Returns the overlap of two intersecting coordinate sets
    AtlasCoordinates GetOverlap(AtlasCoordinates a, AtlasCoordinates b)
    {
        return AtlasCoordinates(b.GetLeft() > a.GetLeft() ? b.GetLeft() : a.GetLeft(),
            b.GetRight() < a.GetRight() ? b.GetRight() : a.GetRight(),
            b.GetTop() > a.GetTop() ? b.GetTop() : a.GetTop(),
            b.GetBottom() < a.GetBottom() ? b.GetBottom() : a.GetBottom());
    }

    // Returns the width of the widest element in imageList
    int AtlasBuilderWorker::GetWidest(const ImageDimensionData& imageList)
    {
        int max = 0;
        for (size_t i = 0; i < imageList.size(); ++i)
        {
            if (max < imageList[i].second.m_width)
            {
                max = imageList[i].second.m_width;
            }
        }
        return max;
    }

    // Returns the height of the tallest element in imageList
    int AtlasBuilderWorker::GetTallest(const ImageDimensionData& imageList)
    {
        int max = 0;
        for (size_t i = 0; i < imageList.size(); ++i)
        {
            if (max < imageList[i].second.m_height)
            {
                max = imageList[i].second.m_height;
            }
        }
        return max;
    }

    // Performs an operation that copies a pixel to the output
    void SetPixels(AZ::u8* dest, const AZ::u8* source, int destBytes)
    {
        if (destBytes >= bytesPerPixel)
        {
            memcpy(dest, source, bytesPerPixel);
            int bytesCopied = bytesPerPixel;
            while (bytesCopied * 2 < destBytes)
            {
                memcpy(dest + bytesCopied, dest, bytesCopied);
                bytesCopied *= 2;
            }
            memcpy(dest + bytesCopied, dest, destBytes - bytesCopied);
        }
    }

    // Checks if we can insert an image into a slot
    bool CanInsert(AtlasCoordinates slot, ImageDimension image, int padding, int farRight, int farBot)
    {
        int right = slot.GetLeft() + image.m_width;
        if (slot.GetRight() < farRight)
        {
            // Add padding for my right border
            right += padding;
            // Round up to the nearest compression unit
            right = (right + (cellSize - 1)) / cellSize * cellSize;
            // Add padding for an adjacent unit's left border
            right += padding;
        }

        int bot = slot.GetTop() + image.m_height;
        if (slot.GetBottom() < farBot)
        {
            // Add padding for my right border
            bot += padding;
            // Round up to the nearest compression unit
            bot = (bot + (cellSize - 1)) / cellSize * cellSize;
            // Add padding for an adjacent unit's left border
            bot += padding;
        }

        return slot.GetRight() >= right && slot.GetBottom() >= bot;
    }

    // Adds the necessary padding to an Atlas Coordinate
    void AddPadding(AtlasCoordinates& slot, int padding, [[maybe_unused]] int farRight, [[maybe_unused]] int farBot)
    {
        // Add padding for my right border
        int right = slot.GetRight() + padding;
        // Round up to the nearest compression unit
        right = (right + (cellSize - 1)) / cellSize * cellSize;
        // Add padding for an adjacent unit's left border
        right += padding;

        // Add padding for my right border
        int bot = slot.GetBottom() + padding;
        // Round up to the nearest compression unit
        bot = (bot + (cellSize - 1)) / cellSize * cellSize;
        // Add padding for an adjacent unit's left border
        bot += padding;

        slot.SetRight(right);
        slot.SetBottom(bot);
    }

}
