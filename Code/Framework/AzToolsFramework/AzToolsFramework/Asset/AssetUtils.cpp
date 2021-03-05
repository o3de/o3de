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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
AZ_PUSH_DISABLE_WARNING(4127 4251 4800, "-Wunknown-warning-option")
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QDirIterator>
AZ_POP_DISABLE_WARNING


namespace AzToolsFramework
{
    namespace AssetUtils
    {
        namespace Internal
        {

            const char AssetConfigPlatformDir[] = "AssetProcessorConfig";
            const char RestrictedPlatformDir[] = "restricted";

            QStringList FindWildcardMatches(const QString& sourceFolder, QString relativeName)
            {
                if (relativeName.isEmpty())
                {
                    return QStringList();
                }

                const int pathLen = sourceFolder.length() + 1;

                relativeName.replace('\\', '/');

                QStringList returnList;
                QRegExp nameMatch{ relativeName, Qt::CaseInsensitive, QRegExp::Wildcard };
                QDirIterator diretoryIterator(sourceFolder, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                QStringList files;
                while (diretoryIterator.hasNext())
                {
                    diretoryIterator.next();
                    if (!diretoryIterator.fileInfo().isFile())
                    {
                        continue;
                    }
                    QString pathMatch{ diretoryIterator.filePath().mid(pathLen) };
                    if (nameMatch.exactMatch(pathMatch))
                    {
                        returnList.append(diretoryIterator.filePath());
                    }
                }
                return returnList;
            }

            void ReadPlatformInfosFromConfigFile(QString configFile, QStringList& enabledPlatforms)
            {
                // in the inifile the platform can be missing (commented out)
                // in which case it is disabled implicitly by not being there
                // or it can be 'disabled' which means that it is explicitly disabled.
                // or it can be 'enabled' which means that it is explicitly enabled.

                if (!QFile::exists(configFile))
                {
                    return;
                }

                QSettings loader(configFile, QSettings::IniFormat);

                // Read in enabled platforms
                loader.beginGroup("Platforms");
                QStringList keys = loader.allKeys();
                for (int idx = 0; idx < keys.count(); idx++)
                {
                    QString val = loader.value(keys[idx]).toString();
                    QString platform = keys[idx].toLower().trimmed();

                    val = val.toLower().trimmed();

                    if (val == "enabled")
                    {
                        if (!enabledPlatforms.contains(val))
                        {
                            enabledPlatforms.push_back(platform);
                        }
                    }
                    else if (val == "disabled")
                    {
                        // disable platform explicitly.
                        int index = enabledPlatforms.indexOf(platform);
                        if (index != -1)
                        {
                            enabledPlatforms.removeAt(index);
                        }
                    }
                }
                loader.endGroup();
            }

            void AddGemConfigFiles(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, QStringList& configFiles)
            {
                // there can only be one gam gem per project, so if we find one, we cache the name of it so that
                // later we can add it to the very end of the list, giving it the ability to override all other config files.
                QString gameConfigPath;

                for (const GemInfo& gemElement : gemInfoList)
                {
                    QString gemAbsolutePath(gemElement.m_absoluteFilePath.c_str());

                    QDir gemDir(gemAbsolutePath);
                    QString absPathToConfigFile = gemDir.absoluteFilePath("AssetProcessorGemConfig.ini");
                    if (gemElement.m_isGameGem)
                    {
                        gameConfigPath = absPathToConfigFile;
                    }
                    else
                    {
                        configFiles.push_back(absPathToConfigFile);
                    }
                }

                // if a 'game gem' was discovered during the above loop, we want to append it to the END of the list
                if (!gameConfigPath.isEmpty())
                {
                    configFiles.push_back(gameConfigPath);
                }
            }

            bool AddPlatformConfigFilePaths(const char* root, QStringList& configFilePaths)
            {
                QString configWildcardName{ "*" };
                configWildcardName.append(AssetProcessorPlatformConfigFileName);
                QDir sourceRoot(root);

                // first collect public platform configs 
                QStringList platformList = FindWildcardMatches(sourceRoot.filePath(AssetConfigPlatformDir), configWildcardName);

                // then collect restricted platform configs
                QDirIterator it(sourceRoot.filePath(RestrictedPlatformDir), QDir::NoDotAndDotDot | QDir::Dirs);
                while (it.hasNext())
                {
                    QDir platformDir(it.next());
                    platformList << FindWildcardMatches(platformDir.filePath(AssetConfigPlatformDir), configWildcardName);
                }

                for (const auto& thisConfig : platformList)
                {
                    configFilePaths.append(thisConfig);
                }
                return (platformList.size() > 0);
            }
        }

        const char* AssetProcessorPlatformConfigFileName = "AssetProcessorPlatformConfig.ini";
        const char* AssetProcessorGamePlatformConfigFileName = "AssetProcessorGamePlatformConfig.ini";
        const char GemsDirectoryName[] = "Gems";

        GemInfo::GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath, AZStd::string identifier, bool isGameGem, bool assetOnlyGem)
            : m_gemName(name)
            , m_relativeFilePath(relativeFilePath)
            , m_absoluteFilePath(absoluteFilePath)
            , m_identifier(identifier)
            , m_isGameGem(isGameGem)
            , m_assetOnly(assetOnlyGem)
        {
        }

        QStringList GetEnabledPlatforms(QStringList configFiles)
        {
            QStringList enabledPlatforms;

            // note that the current host platform is enabled by default.
            enabledPlatforms.push_back(AzToolsFramework::AssetSystem::GetHostAssetPlatform());

            for (const QString& configFile : configFiles)
            {
                Internal::ReadPlatformInfosFromConfigFile(configFile, enabledPlatforms);
            }
            return enabledPlatforms;
        }

        QStringList GetConfigFiles(const char* root, const char* assetRoot, const char* gameName, bool addPlatformConfigs, bool addGemsConfigs)
        {
            QStringList configFiles;
            QDir configRoot(root);

            QString rootConfigFile = configRoot.filePath(AssetProcessorPlatformConfigFileName);

            configFiles.push_back(rootConfigFile);
            if (addPlatformConfigs)
            {
                Internal::AddPlatformConfigFilePaths(root, configFiles);
            }

            if (addGemsConfigs)
            {
                AZStd::vector<AzToolsFramework::AssetUtils::GemInfo> gemInfoList;
                if (!GetGemsInfo(root, assetRoot, gameName, gemInfoList))
                {
                    AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to read gems for game project (%s).\n", gameName);
                    return {};
                }

                Internal::AddGemConfigFiles(gemInfoList, configFiles);
            }

            QDir assetRootDir(assetRoot);
            assetRootDir.cd(gameName);

            QString projectConfigFile = assetRootDir.filePath(AssetProcessorGamePlatformConfigFileName);

            configFiles.push_back(projectConfigFile);

            return configFiles;
        }

        static GemInfo ParseGemInfo(const rapidjson::Document& gemJsonDocument)
        {
            constexpr AZStd::fixed_string<32> NameKey = "Name";
            constexpr AZStd::fixed_string<32> UuidKey = "Uuid";
            constexpr AZStd::fixed_string<32> LinkTypeKey = "LinkType";
            constexpr AZStd::fixed_string<32> IsGameGemKey = "IsGameGem";

            GemInfo gemInfo;
            auto memberIter = gemJsonDocument.FindMember(NameKey.c_str());
            if (memberIter != gemJsonDocument.MemberEnd())
            {
                gemInfo.m_gemName.assign(memberIter->value.GetString(), memberIter->value.GetStringLength());
            }

            memberIter = gemJsonDocument.FindMember(UuidKey.c_str());
            if (memberIter != gemJsonDocument.MemberEnd())
            {
                gemInfo.m_identifier.assign(memberIter->value.GetString(), memberIter->value.GetStringLength());
            }

            memberIter = gemJsonDocument.FindMember(IsGameGemKey.c_str());
            if (memberIter != gemJsonDocument.MemberEnd())
            {
                gemInfo.m_isGameGem = memberIter->value.GetBool();
            }


            AZStd::string_view linkTypeString;
            memberIter = gemJsonDocument.FindMember(LinkTypeKey.c_str());
            if (memberIter != gemJsonDocument.MemberEnd())
            {
                linkTypeString = AZStd::string_view{ memberIter->value.GetString(), memberIter->value.GetStringLength() };
            }

            gemInfo.m_assetOnly = linkTypeString == "NoCode";

            return gemInfo;
        }

        bool GetGemsInfo([[maybe_unused]] const char* root, [[maybe_unused]] const char* assetRoot, [[maybe_unused]] const char* gameName, AZStd::vector<GemInfo>& gemInfoList)
        {
            AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry == nullptr)
            {
                AZ_Error("AzToolsFramework::AssetUtils", false, "Settings Registry does not exist, cannot retrieve information about loaded Gem modules");
                return false;
            }

            auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
            if (fileIoBase == nullptr)
            {
                AZ_Error("AzToolsFramework::AssetUtils", false, "File IO, cannot retrieve information about loaded Gem modules");
                return false;
            }

            constexpr AZStd::string_view GemJsonFilename{ "gem.json" };
            AZStd::vector<AZ::IO::FixedMaxPath> gemModuleSourcePaths;

            struct GemSourcePathsVisitor
                : AZ::SettingsRegistryInterface::Visitor
            {
                GemSourcePathsVisitor(AZStd::vector<AZ::IO::FixedMaxPath>& gemSourcePaths)
                    : m_gemSourcePaths(gemSourcePaths)
                {}

                void Visit(AZStd::string_view path, [[maybe_unused]] AZStd::string_view valueName, [[maybe_unused]] AZ::SettingsRegistryInterface::Type type,
                    AZStd::string_view value) override
                {
                    if (path.find("SourcePaths") != AZStd::string_view::npos
                        && AZStd::find(m_gemSourcePaths.begin(), m_gemSourcePaths.end(), value) == m_gemSourcePaths.end())
                    {
                        m_gemSourcePaths.emplace_back(value);
                    }
                }
                AZStd::vector<AZ::IO::FixedMaxPath>& m_gemSourcePaths;
            };

            GemSourcePathsVisitor visitor{ gemModuleSourcePaths };
            const auto gemListKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
            AZ::SettingsRegistry::Get()->Visit(visitor, gemListKey);

            AZ::IO::FixedMaxPath engineRootPath;
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            if (engineRootPath.empty())
            {
                AZ_TracePrintfOnce("AzToolsFramework::AssetUtils", "Engine Root Path is empty. The Gem Module Source Paths will have the @engroot@ alias prepended");
                engineRootPath = "@engroot@";
            }

            AZStd::vector<char> gemJsonFileData;
            for (const AZ::IO::FixedMaxPath& gemSourcePath : gemModuleSourcePaths)
            {
                AZ::IO::FixedMaxPath gemJsonPath = engineRootPath / gemSourcePath / GemJsonFilename;
                if (AZ::IO::HandleType gemJsonHandle;  fileIoBase->Open(gemJsonPath.c_str(), AZ::IO::OpenMode::ModeRead, gemJsonHandle))
                {
                    AZ::IO::SizeType gemJsonFileSize;
                    if (AZ::IO::Result ioResult = fileIoBase->Size(gemJsonHandle, gemJsonFileSize); !ioResult)
                    {
                        AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to query file size of gem json at path '%s'.\nResult code %u returned",
                            gemJsonPath.c_str(), aznumeric_cast<uint32_t>(ioResult.GetResultCode()));
                        fileIoBase->Close(gemJsonHandle);
                        continue;
                    }

                    gemJsonFileData.resize_no_construct(gemJsonFileSize);
                    AZ::IO::SizeType bytesRead{};
                    if (AZ::IO::Result ioResult = fileIoBase->Read(gemJsonHandle, gemJsonFileData.data(), gemJsonFileData.size(), false, &bytesRead); !ioResult)
                    {
                        AZ_Error("AzToolsFramework::AssetUtils", false, "Reading from gem json at path '%s' has failed with result code %u.\n%zu has been read",
                            gemJsonPath.c_str(), aznumeric_cast<uint32_t>(ioResult.GetResultCode()), bytesRead);
                        fileIoBase->Close(gemJsonHandle);
                        continue;
                    }

                    rapidjson::Document gemJsonDocument;
                    gemJsonDocument.Parse(gemJsonFileData.data(), gemJsonFileData.size());
                    if (gemJsonDocument.HasParseError())
                    {
                        AZ_Error("AzToolsFramework::AssetUtils", false, "Parsing gem json at path '%s' has json Parse Error: %s",
                            gemJsonPath.c_str(), rapidjson::GetParseError_En(gemJsonDocument.GetParseError()))
                    }

                    GemInfo gemInfo = ParseGemInfo(gemJsonDocument);


                    char gemJsonResolvePath[AZ::IO::MaxPathLength];
                    const bool foundFilename = fileIoBase->GetFilename(gemJsonHandle, AZStd::data(gemJsonResolvePath), AZStd::size(gemJsonResolvePath));
                    if (foundFilename)
                    {
                        // Set the Absolute path to the folder containing the gem.json file
                        gemInfo.m_absoluteFilePath = AZ::IO::PathView(gemJsonResolvePath).ParentPath().Native();
                    }
                    else
                    {
                        // If unable to retrieve the Filename from the File IO handle the gemJsonPath is used instead
                        gemInfo.m_absoluteFilePath = gemJsonPath.ParentPath().Native();
                    }

                    // The gemSourcePath is the relative path
                    gemInfo.m_relativeFilePath = static_cast<AZStd::string_view>(gemSourcePath.Native());
                    gemInfoList.push_back(gemInfo);
                    fileIoBase->Close(gemJsonHandle);
                }
            }

            return true;
        }

        bool UpdateFilePathToCorrectCase(const QString& root, QString& relativePathFromRoot)
        {
            AZStd::string rootPath(root.toUtf8().data());
            AZStd::string relPathFromRoot(relativePathFromRoot.toUtf8().data());
            AZ::StringFunc::Path::Normalize(relPathFromRoot);
            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(relPathFromRoot.c_str(), tokens, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

            AZStd::string validatedPath;
            if (rootPath.empty())
            {
                const char* appRoot = nullptr;
                AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
                validatedPath = AZStd::string(appRoot);
            }
            else
            {
                validatedPath = rootPath;
            }

            bool success = true;

            for (int idx = 0; idx < tokens.size(); idx++)
            {
                AZStd::string element = tokens[idx];
                bool foundAMatch = false;
                AZ::IO::FileIOBase::GetInstance()->FindFiles(validatedPath.c_str(), "*", [&](const char* file)
                    {
                        if ( idx != tokens.size() - 1 && !AZ::IO::FileIOBase::GetInstance()->IsDirectory(file))
                        {
                            // only the last token is supposed to be a filename, we can skip filenames before that
                            return true;
                        }

                        AZStd::string absFilePath(file);
                        AZ::StringFunc::Path::Normalize(absFilePath);
                        auto found = absFilePath.rfind(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                        size_t startingPos = found + 1;
                        if (found != AZStd::string::npos && absFilePath.size() > startingPos)
                        {
                            AZStd::string componentName = AZStd::string(absFilePath.begin() + startingPos, absFilePath.end());
                            if (AZ::StringFunc::Equal(componentName.c_str(), tokens[idx].c_str()))
                            {
                                tokens[idx] = componentName;
                                foundAMatch = true;
                                return false;
                            }
                        }

                        return true;
                    });

                if (!foundAMatch)
                {
                    success = false;
                    break;
                }

                AZStd::string absoluteFilePath;
                AZ::StringFunc::Path::ConstructFull(validatedPath.c_str(), element.c_str(), absoluteFilePath);

                validatedPath = absoluteFilePath; // go one step deeper.
            }

            if (success)
            {
                relPathFromRoot.clear();
                AZ::StringFunc::Join(relPathFromRoot, tokens.begin(), tokens.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                relativePathFromRoot = relPathFromRoot.c_str();
            }

            return success;
        }
    } //namespace AssetUtils
} //namespace AzToolsFramework
