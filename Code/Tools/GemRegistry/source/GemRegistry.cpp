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
#include "ProjectSettings.h"
#include "GemRegistry.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace Gems
{
    AZ_CLASS_ALLOCATOR_IMPL(GemRegistry, AZ::SystemAllocator, 0)

    AZ::Outcome<void, AZStd::string> GemRegistry::AddSearchPath(const SearchPath& searchPathIn, bool loadGemsNow)
    {
        SearchPath searchPath = searchPathIn;

        // Remove trailing slash if present
        char lastChar = *(searchPath.m_path.end() - 1);
        if (lastChar == '/' ||
            lastChar == '\\')
        {
            AzFramework::StringFunc::RChop(searchPath.m_path, 1);
        }

        if (AZStd::find(m_searchPaths.begin(), m_searchPaths.end(), searchPath) == m_searchPaths.end())
        {
            m_searchPaths.emplace_back(searchPath);
        }

        if (loadGemsNow)
        {
            return LoadGemsFromDir(searchPath);
        }
        else
        {
            return AZ::Success();
        }
    }

    AZ::Outcome<void, AZStd::string> GemRegistry::LoadAllGemsFromDisk()
    {
        AZStd::string errorString;
        for (const auto& searchPath : m_searchPaths)
        {
            auto pathOutcome = LoadGemsFromDir(searchPath);
            if (!pathOutcome.IsSuccess())
            {
                errorString += pathOutcome.GetError() + "\n";
            }
        }

        if (errorString.empty())
        {
            return AZ::Success();
        }
        else
        {
            // Remove trailing \n
            return AZ::Failure(errorString.substr(0, errorString.length() - 1));
        }
    }

    AZ::Outcome<void, AZStd::string> GemRegistry::LoadProject(const IProjectSettings& settings, bool resetPreviousProjects)
    {
        if (resetPreviousProjects)
        {
            m_gemDescs.clear();
        }
        for (const auto& pair : settings.GetGems())
        {
            const char* absolutePath = nullptr;

            // First priority goes to the project root's folder
            AZStd::string testGemPath = settings.GetProjectRootPath();
            if (AzFramework::StringFunc::Path::ConstructFull(settings.GetProjectRootPath().c_str(), pair.second.m_path.c_str(), testGemPath, true))
            {
                if (AzFramework::StringFunc::Path::Join(testGemPath.c_str(), GEM_DEF_FILE, testGemPath))
                {
                    if (AZ::IO::SystemFile::Exists(testGemPath.c_str()))
                    {
                        absolutePath = testGemPath.c_str();
                    }
                }
            }

            auto loadOutcome = LoadGemDescription(pair.second.m_path, absolutePath);
            if (!loadOutcome.IsSuccess())
            {
                return AZ::Failure(loadOutcome.GetError());
            }
        }
        return AZ::Success();
    }

    IGemDescriptionConstPtr GemRegistry::GetGemDescription(const GemSpecifier& spec) const
    {
        IGemDescriptionConstPtr result;

        auto idIt = m_gemDescs.find(spec.m_id);
        if (idIt != m_gemDescs.end())
        {
            auto versionIt = idIt->second.find(spec.m_version);
            if (versionIt != idIt->second.end())
            {
                result = AZStd::static_pointer_cast<IGemDescriptionConstPtr::element_type>(versionIt->second);
            }
        }

        return result;
    }

    IGemDescriptionConstPtr GemRegistry::GetLatestGem(const AZ::Uuid& uuid) const
    {
        IGemDescriptionConstPtr result;
        auto idIt = m_gemDescs.find(uuid);
        if (idIt != m_gemDescs.end())
        {
            GemVersion latestVersion;
            GemDescriptionPtr desc;

            for (const auto& pair : idIt->second)
            {
                if (pair.first > latestVersion)
                {
                    latestVersion = pair.first;
                    desc = pair.second;
                }
            }

            if (!latestVersion.IsZero())
            {
                result = AZStd::static_pointer_cast<IGemDescriptionConstPtr::element_type>(desc);
            }
        }

        return result;
    }

    AZStd::vector<IGemDescriptionConstPtr> GemRegistry::GetAllGemDescriptions() const
    {
        AZStd::vector<IGemDescriptionConstPtr> results;

        for (auto && idIt : m_gemDescs)
        {
            for (auto && versionIt : idIt.second)
            {
                results.push_back(AZStd::static_pointer_cast<IGemDescriptionConstPtr::element_type>(versionIt.second));
            }
        }

        return results;
    }

    AZStd::vector<IGemDescriptionConstPtr> GemRegistry::GetAllRequiredGemDescriptions() const
    {
        AZStd::vector<IGemDescriptionConstPtr> results;

        for (auto && idIt : m_gemDescs)
        {
            for (auto && versionIt : idIt.second)
            {
                if (versionIt.second->IsRequired())
                {
                    results.push_back(AZStd::static_pointer_cast<IGemDescriptionConstPtr::element_type>(versionIt.second));
                }
            }
        }
        return results;
    }

    IGemDescriptionConstPtr GemRegistry::GetProjectGemDescription(const AZStd::string& projectName) const
    {
        IGemDescriptionConstPtr result;

        // We know searchPaths[0] is the old engine root, so we'll just use that to avoid a search
        AZStd::string gemFolderPath = projectName + "/Gem";
        auto descOutcome = ParseToGemDescription(gemFolderPath, nullptr);
        if (descOutcome)
        {
            result = AZStd::make_shared<GemDescription>(descOutcome.TakeValue());
        }

        return result;
    }


    IProjectSettings* GemRegistry::CreateProjectSettings()
    {
        return aznew ProjectSettings(this);
    }

    void GemRegistry::DestroyProjectSettings(IProjectSettings* settings)
    {
        delete settings;
    }

    AZ::Outcome<void, AZStd::string> GemRegistry::LoadGemsFromDir(const SearchPath& searchPath)
    {
        AZ::IO::LocalFileIO fileIo;
        AZStd::string errorString;

        // Handles each file and directory found
        // Safe to capture all by reference because the find will run sync
        AZ::IO::LocalFileIO::FindFilesCallbackType fileFinderCb;
        fileFinderCb = [&](const char* fullPath) -> bool
        {
            if (fileIo.IsDirectory(fullPath))
            {
                // recurse into subdirectory
                // "*" filter will match all files/directories except specials ('.', '..', etc.) with the fewest compares
                fileIo.FindFiles(fullPath, "*", fileFinderCb);
            }
            else
            {
                AZStd::string fileName;
                AzFramework::StringFunc::Path::GetFullFileName(fullPath, fileName);
                if (0 == azstricmp(fileName.c_str(), GEM_DEF_FILE))
                {
                    // need relative path to gem folder, so strip searchPath from front and Gem.json from back
                    AZStd::string gemFolderRelPath = fullPath + searchPath.m_path.length();
                    AzFramework::StringFunc::Path::StripFullName(gemFolderRelPath);
                    AzFramework::StringFunc::RChop(gemFolderRelPath, 1); // Remove trailing '/'
                    auto skipPathFirstSepIndex = gemFolderRelPath.find_first_not_of("/\\");
                    gemFolderRelPath = gemFolderRelPath.substr(skipPathFirstSepIndex);

                    auto loadOutcome = LoadGemDescription(gemFolderRelPath, fullPath);
                    if (loadOutcome.IsSuccess() == false)
                    {
                        errorString += AZStd::string::format("Fail to load Gems from path %s disk. %s\n", searchPath.m_path.c_str(), loadOutcome.GetError().c_str());
                    }

                    // We found the Gem.json file but we have to keep looking to support nested gems
                }
            }

            return true; // keep searching
        };

        // Scans subdirectories
        fileIo.FindFiles(searchPath.m_path.c_str(), searchPath.m_filter.c_str(), fileFinderCb);

        if (errorString.empty())
        {
            return AZ::Success();
        }
        else
        {
            // Remove trailing \n
            return AZ::Failure(errorString.substr(0, errorString.length() - 1));
        }
    }

    AZ::Outcome<IGemDescriptionConstPtr, AZStd::string> GemRegistry::LoadGemDescription(const AZStd::string& gemFolderPath, const char* absoluteFilePath)
    {
        auto descOutcome = ParseToGemDescription(gemFolderPath, absoluteFilePath);
        if (!descOutcome)
        {
            return AZ::Failure(AZStd::string::format("An error occurred while parsing %s: %s", gemFolderPath.c_str(), descOutcome.GetError().c_str()));
        }

        auto desc = AZStd::make_shared<GemDescription>(descOutcome.TakeValue());

        // If the Gem hasn't been loaded yet, add it's id to the root map
        auto idIt = m_gemDescs.find(desc->GetID());
        if (idIt == m_gemDescs.end())
        {
            idIt = m_gemDescs.emplace(desc->GetID(), AZStd::unordered_map<GemVersion, GemDescriptionPtr>()).first;
        }

        // If the Gem's version doesn't exist, add it, otherwise update it
        auto versionIt = idIt->second.find(desc->GetVersion());
        if (versionIt == idIt->second.end())
        {
            idIt->second.emplace(desc->GetVersion(), AZStd::move(desc));
        }
        else
        {
            versionIt->second = desc;
        }

        return AZ::Success(AZStd::static_pointer_cast<IGemDescriptionConstPtr::element_type>(desc));
    }

    AZ::Outcome<IGemDescriptionConstPtr, AZStd::string> GemRegistry::ParseToGemDescriptionPtr(const AZStd::string& gemFolderRelPath, const char* absoluteFilePath)
    {
        auto descOutcome = ParseToGemDescription(gemFolderRelPath, absoluteFilePath);
        if (!descOutcome)
        {
            return AZ::Failure(AZStd::string::format("An error occurred while parsing %s: %s", gemFolderRelPath.c_str(), descOutcome.GetError().c_str()));
        }

        auto desc = AZStd::make_shared<GemDescription>(descOutcome.TakeValue());

        return  AZ::Success(AZStd::static_pointer_cast<IGemDescriptionConstPtr::element_type>(desc));
    }

    AZ::Outcome<GemDescription, AZStd::string> GemRegistry::ParseToGemDescription(const AZStd::string& gemFolderPath, const char* absoluteFilePath) const
    {
        // do we have a pluggable, engine-compatible fileIO?  For things like tools, we may not be plugged
        // into a game engine, and thus, we may need to use raw file io.
        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();

        // build absolute path to gem file
        AZStd::string filePath;

        if (absoluteFilePath)
        {
            filePath = absoluteFilePath;
        }
        else
        {
            for (const auto& searchPath : m_searchPaths)
            {
                // Append relative path to search path
                AzFramework::StringFunc::Path::Join(searchPath.m_path.c_str(), gemFolderPath.c_str(), filePath);
                // Append file name to file path
                AzFramework::StringFunc::Path::Join(filePath.c_str(), GEM_DEF_FILE, filePath);
                // note that paths are case sensitive on some systems.

                if (fileReader)
                {
                    if (fileReader->Exists(filePath.c_str()))
                    {
                        break;
                    }
                }
                else
                {
                    if (AZ::IO::SystemFile::Exists(filePath.c_str()))
                    {
                        break;
                    }
                }
            }
        }

        // read json
        AZStd::string fileBuf;
        if (fileReader)
        {
            // an engine compatible file reader has been attached, so use that.
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            AZ::u64 fileSize = 0;
            if (!fileReader->Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
            {
                return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", filePath.c_str()));
            }

            if ((!fileReader->Size(fileHandle, fileSize)) || (fileSize == 0))
            {
                fileReader->Close(fileHandle);
                return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", filePath.c_str()));
            }

            fileBuf.resize(fileSize);
            if (!fileReader->Read(fileHandle, fileBuf.data(), fileSize, true))
            {
                fileReader->Close(fileHandle);
                return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", filePath.c_str()));
            }
            fileReader->Close(fileHandle);
        }
        else
        {
            // we don't have an engine file io, use raw file IO.
            AZ::IO::SystemFile rawFile;
            if (!rawFile.Open(filePath.c_str(), AZ::IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY))
            {
                return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", filePath.c_str()));
            }
            fileBuf.resize(rawFile.Length());
            if (fileBuf.size() == 0)
            {
                return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", filePath.c_str()));
            }
            if (rawFile.Read(fileBuf.size(), fileBuf.data()) != fileBuf.size())
            {
                return AZ::Failure(AZStd::string::format("Failed to read %s - file read failed.", filePath.c_str()));
            }
        }

        rapidjson::Document document;
        document.Parse(fileBuf.data());
        if (document.HasParseError())
        {
            const char* errorStr = rapidjson::GetParseError_En(document.GetParseError());
            return AZ::Failure(AZStd::string::format("Failed to parse %s: %s", filePath.c_str(), errorStr));
        }

        return GemDescription::CreateFromJson(document, gemFolderPath, filePath);
    }
} // namespace Gems

//////////////////////////////////////////////////////////////////////////
// DLL Exported Functions
//////////////////////////////////////////////////////////////////////////

#ifndef AZ_MONOLITHIC_BUILD // Module init functions, only required when building as a DLL.
AZ_DECLARE_MODULE_INITIALIZATION
#endif//AZ_MONOLITHIC_BUILD

extern "C" AZ_DLL_EXPORT Gems::IGemRegistry * CreateGemRegistry()
{
    return aznew Gems::GemRegistry();
}

extern "C" AZ_DLL_EXPORT void DestroyGemRegistry(Gems::IGemRegistry* reg)
{
    delete reg;
}
