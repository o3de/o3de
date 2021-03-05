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
#include "native/utilities/PlatformConfiguration.h"
#include "native/AssetManager/FileStateCache.h"

#include <QDirIterator>

#include <QSettings>

#include <AzFramework/API/ApplicationAPI.h>

namespace
{
    // the starting order in the file for gems.
    const int g_gemStartingOrder = 100;

    unsigned int GetCrc(const char* name)
    {
        return AssetUtilities::ComputeCRC32(name);
    }
}

namespace AssetProcessor
{
    const char AssetConfigPlatformDir[] = "AssetProcessorConfig/";
    const char AssetProcessorPlatformConfigFileName[] = "AssetProcessorPlatformConfig.ini";
    const char RestrictedPlatformDir[] = "restricted";

    PlatformConfiguration::PlatformConfiguration(QObject* pParent)
        : QObject(pParent)
        , m_minJobs(1)
        , m_maxJobs(8)
    {
    }

    bool PlatformConfiguration::AddPlatformConfigFilePaths(QStringList& configFilePaths)
    {
        QDir engineRoot;
        AssetUtilities::ComputeEngineRoot(engineRoot);
        QString configWildcardName{ "*" };
        configWildcardName.append(AssetProcessorPlatformConfigFileName);

        // first collect public platform configs 
        QStringList platformList = FindWildcardMatches(engineRoot.filePath(AssetConfigPlatformDir), configWildcardName);

        // then collect restricted platform configs
        QDirIterator it(engineRoot.filePath(RestrictedPlatformDir), QDir::NoDotAndDotDot | QDir::Dirs);
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

    bool PlatformConfiguration::InitializeFromConfigFiles(QString absoluteSystemRoot, QString absoluteAssetRoot, QString gameName, bool addPlatformConfigs, bool addGemsConfigs)
    {
        // this function may look strange, but the point here is that each section in the config file
        // can depend on entries from the prior section, but also, each section can be overridden by
        // the other config files.
        // so we have to read each section one at a time, in order of config file priority (most important one last)

        static const char ScanFolderOption[] = "scanfolders";
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
        const bool scanFolderOverride = commandLine ? commandLine->HasSwitch(ScanFolderOption) : false;

        static const char NoConfigScanFolderOption[] = "noConfigScanFolders";
        const bool noConfigScanFolders = commandLine ? commandLine->HasSwitch(NoConfigScanFolderOption) : false;

        static const char NoGemScanFolderOption[] = "noGemScanFolders";
        const bool noGemScanFolders = commandLine ? commandLine->HasSwitch(NoGemScanFolderOption) : false;

        static const char ScanFolderPatternOption[] = "scanfolderpattern";
        QStringList scanFolderPatterns;
        if (commandLine->HasSwitch(ScanFolderPatternOption))
        {
            for (size_t idx = 0; idx < commandLine->GetNumSwitchValues(ScanFolderPatternOption); idx++)
            {
                scanFolderPatterns.push_back(commandLine->GetSwitchValue(ScanFolderPatternOption, idx).c_str());
            }
        }
        QStringList configFiles = AzToolsFramework::AssetUtils::GetConfigFiles(absoluteSystemRoot.toUtf8().constData(), absoluteAssetRoot.toUtf8().constData(), gameName.toUtf8().constData(), addPlatformConfigs, addGemsConfigs && !noGemScanFolders);

        // first, read the platform informations.
        for (QString configFile : configFiles)
        {
            ReadPlatformInfosFromConfigFile(configFile);
        }

        // now read which platforms are currently enabled - this may alter the platform infos array and eradicate
        // the ones that are not suitable and currently enabled, leaving only the ones enabled either on command line
        // or config files.
        // the command line can always takes precidence - but can only turn on and off platforms, it cannot describe them.

        PopulateEnabledPlatforms(configFiles);

        FinalizeEnabledPlatforms();

        if (scanFolderOverride)
        {
            AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
            PopulatePlatformsForScanFolder(platforms);
            for (size_t idx = 0; idx < commandLine->GetNumSwitchValues(ScanFolderOption); idx++)
            {
                QString scanFolder{ commandLine->GetSwitchValue(ScanFolderOption, idx).c_str() };
                scanFolder = AssetUtilities::NormalizeFilePath(scanFolder);
                AddScanFolder(ScanFolderInfo(
                    scanFolder,
                    AZStd::string::format("ScanFolderParam %zu", idx).c_str(),
                    AZStd::string::format("SF%zu", idx).c_str(),
                    "",
                    false,
                    true,
                    platforms,
                    aznumeric_caster(idx),
                    /*scanFolderId*/ 0,
                    true));
            }
        }
        // Then read recognizers (which depend on platforms)
        for (QString configFile : configFiles)
        {
            if (!ReadRecognizersFromConfigFile(configFile, noConfigScanFolders, scanFolderPatterns))
            {
                if (m_fatalError.empty())
                {
                    m_fatalError = "Unable to read recognizers specified in the configuration files during load.  Please check the Asset Processor platform ini files for errors.";
                }
                return IsValid();
            }
        }
        if (!noGemScanFolders && addGemsConfigs)
        {
            if (!AzToolsFramework::AssetUtils::GetGemsInfo(absoluteSystemRoot.toUtf8().constData(), absoluteAssetRoot.toUtf8().constData(), gameName.toUtf8().constData(), m_gemInfoList))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to Get Gems Info for the project (%s).", gameName.toUtf8().constData());
                return false;
            }

            // now add all the scan folders of gems.
            AddGemScanFolders(m_gemInfoList);
        }
        // Then read metadata (which depends on scan folders)
        for (QString configFile : configFiles)
        {
            ReadMetaDataFromConfigFile(configFile);
        }

        // at this point there should be at least some watch folders besides gems.
        if (m_scanFolders.empty())
        {
            m_fatalError = "Unable to find any scan folders specified in the configuration files during load.  Please check the Asset Processor platform ini files for errors.";
            return IsValid();
        }

        return IsValid();
    }

    void PlatformConfiguration::PopulateEnabledPlatforms(QStringList configFiles)
    {
        // if there are no platform informations inside the ini file, there's no point in proceeding
        // since we are unaware of the existence of the platform at all
        if (m_enabledPlatforms.empty())
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "There are no [Platform xxxxxx] entries present in the config file(s).  We cannot proceed.");
            return;
        }

        // the command line can always takes precidence - but can only turn on and off platforms, it cannot describe them.
        QStringList commandLinePlatforms = AssetUtilities::ReadPlatformsFromCommandLine();

        if (!commandLinePlatforms.isEmpty())
        {
            // command line overrides everything.
            m_tempEnabledPlatforms.clear();

            for (const QString& platformFromCommandLine : commandLinePlatforms)
            {
                QString platform = platformFromCommandLine.toLower().trimmed();
                if (!platform.isEmpty())
                {
                    if (!m_tempEnabledPlatforms.contains(platform))
                    {
                        m_tempEnabledPlatforms.push_back(platform);
                    }
                }
            }

            return; // command line wins!
        }
        // command line isn't active, read from config files instead.
        QStringList enabledPlatforms = AzToolsFramework::AssetUtils::GetEnabledPlatforms(configFiles);

        for (const QString& enabledPlatform : enabledPlatforms)
        {
            m_tempEnabledPlatforms.push_back(enabledPlatform);
        }
    }

    void PlatformConfiguration::FinalizeEnabledPlatforms()
    {
#if defined(AZ_ENABLE_TRACING)
        // verify command line platforms are valid:
        for (const QString& enabledPlatformFromConfigs : m_tempEnabledPlatforms)
        {
            bool found = false;
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : m_enabledPlatforms)
            {
                if (platformInfo.m_identifier == enabledPlatformFromConfigs.toUtf8().data())
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                m_fatalError = AZStd::string::format("Platform in config file or command line '%s' matches no [Platform xxxxxx] entries present in the config file(s) - check command line and config files for errors!", enabledPlatformFromConfigs.toUtf8().constData());
                return;
            }
        }
#endif // defined(AZ_ENABLE_TRACING)

        // over here, we want to eliminate any platforms in the m_enabledPlatforms array that are not in the m_tempEnabledPlatforms
        for (int enabledPlatformIdx = static_cast<int>(m_enabledPlatforms.size() - 1); enabledPlatformIdx >= 0; --enabledPlatformIdx)
        {
            const AssetBuilderSDK::PlatformInfo& platformInfo = m_enabledPlatforms[enabledPlatformIdx];

            if (!m_tempEnabledPlatforms.contains(platformInfo.m_identifier.c_str()))
            {
                m_enabledPlatforms.erase(m_enabledPlatforms.cbegin() + enabledPlatformIdx);
            }
        }

        if (m_enabledPlatforms.empty())
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "There are no [Platform xxxxxx] entries present in the config file(s) after parsing files and command line.  We cannot proceed.");
            return;
        }
        m_tempEnabledPlatforms.clear();
    }


    void PlatformConfiguration::ReadPlatformInfosFromConfigFile(QString iniPath)
    {
        if (QFile::exists(iniPath))
        {
            QSettings loader(iniPath, QSettings::IniFormat);

            QStringList groups = loader.childGroups();
            for (QString group : groups)
            {
                if (!group.startsWith("Platform ", Qt::CaseInsensitive))
                {
                    continue;
                }

                loader.beginGroup(group);
                QString platformIdentifier = group.mid(9); // chop off the "Platform " and you're left with the remainder name
                platformIdentifier = platformIdentifier.trimmed().toLower();

                QVariant paramValue = loader.value("tags", QString());
                QString platformTagString = paramValue.type() == QVariant::StringList ? paramValue.toStringList().join(",") : paramValue.toString();

                QStringList platformTagsQt = platformTagString.split(QChar(','), Qt::SkipEmptyParts);
                AZStd::unordered_set<AZStd::string> platformTagsAZ;

                for (const QString& tag : platformTagsQt)
                {
                    QString cleaned = tag.trimmed().toLower();
                    if (!cleaned.isEmpty())
                    {
                        QByteArray utf8Encoded = tag.toUtf8();
                        platformTagsAZ.insert(utf8Encoded.constData());
                    }
                }

                EnablePlatform(AssetBuilderSDK::PlatformInfo(platformIdentifier.toUtf8().constData(), platformTagsAZ), true);

                loader.endGroup();
            }
        }
    }

    void PlatformConfiguration::ReadEnabledPlatformsFromConfigFile(QString iniPath)
    {
        // in the inifile the platform can be missing (commented out)
        // in which case it is disabled implicitly by not being there
        // or it can be 'disabled' which means that it is explicitly enabled.
        // or it can be 'enabled' which means that it is explicitly enabled.

        if (QFile::exists(iniPath))
        {
            QSettings loader(iniPath, QSettings::IniFormat);

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
                    if (!m_tempEnabledPlatforms.contains(val))
                    {
                        m_tempEnabledPlatforms.push_back(platform);
                    }
                }
                else if (val == "disabled")
                {
                    // disable platform explicitly.
                    int index = m_tempEnabledPlatforms.indexOf(platform);
                    if (index != -1)
                    {
                        m_tempEnabledPlatforms.removeAt(index);
                    }
                }
            }
            loader.endGroup();
        }
    }

    void PlatformConfiguration::PopulatePlatformsForScanFolder(AZStd::vector<AssetBuilderSDK::PlatformInfo>& platformsList, QStringList includeTagsList, QStringList excludeTagsList)
    {
        if (includeTagsList.isEmpty())
        {
            // Add all enabled platforms
            for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
            {
                if (AZStd::find(platformsList.begin(), platformsList.end(), platform) == platformsList.end())
                {
                    platformsList.push_back(platform);
                }
            }
        }
        else
        {
            for (QString identifier : includeTagsList)
            {
                for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
                {
                    bool addPlatform = (QString::compare(identifier, platform.m_identifier.c_str(), Qt::CaseInsensitive) == 0) ||
                        platform.m_tags.find(identifier.toLower().toUtf8().data()) != platform.m_tags.end();

                    if (addPlatform)
                    {
                        if (AZStd::find(platformsList.begin(), platformsList.end(), platform) == platformsList.end())
                        {
                            platformsList.push_back(platform);
                        }
                    }
                }
            }
        }

        for (QString identifier : excludeTagsList)
        {
            for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
            {
                bool removePlatform = (QString::compare(identifier, platform.m_identifier.c_str(), Qt::CaseInsensitive) == 0) ||
                     platform.m_tags.find(identifier.toLower().toUtf8().data()) != platform.m_tags.end();

                if (removePlatform)
                {
                    platformsList.erase(AZStd::remove(platformsList.begin(), platformsList.end(), platform), platformsList.end());
                }
            }
        }
    }

    bool PlatformConfiguration::ReadRecognizersFromConfigFile(QString iniPath, bool skipScanFolders, QStringList scanFolderPatterns)
    {
        QDir assetRoot;
        AssetUtilities::ComputeAssetRoot(assetRoot);
        const QString normalizedRoot = AssetUtilities::NormalizeDirectoryPath(assetRoot.absolutePath());
        const QString gameName = AssetUtilities::ComputeGameName();

        QDir engineRoot;
        AssetUtilities::ComputeEngineRoot(engineRoot);
        const QString normalizedEngineRoot = AssetUtilities::NormalizeDirectoryPath(engineRoot.absolutePath());


        if (QFile::exists(iniPath))
        {
            QSettings loader(iniPath, QSettings::IniFormat);

            loader.beginGroup("Jobs");
            m_minJobs = loader.value("minJobs", m_minJobs).toInt();
            m_maxJobs = loader.value("maxJobs", m_maxJobs).toInt();
            loader.endGroup();

            // Read in scan folders and RC flags per asset/platform
            QStringList groups = loader.childGroups();
            for (QString group : groups)
            {
                if (!skipScanFolders && group.startsWith("ScanFolder ", Qt::CaseInsensitive))
                {
                    // its a scan folder!
                    loader.beginGroup(group);
                    QString watchFolder = loader.value("watch", QString()).toString();
                    if (!scanFolderPatterns.empty())
                    {
                        bool foundPattern{ false };
                        for (auto& thisPattern : scanFolderPatterns)
                        {
                            QRegExp nameMatch{ thisPattern, Qt::CaseInsensitive, QRegExp::Wildcard };
                            if (nameMatch.exactMatch(watchFolder))
                            {
                                foundPattern = true;
                                break;
                            }
                        }
                        if (foundPattern == false)
                        {
                            continue;
                        }
                    }
                    int order = loader.value("order", 0).toInt();

                    watchFolder.replace("@root@", normalizedRoot, Qt::CaseInsensitive);
                    watchFolder.replace("@GAMENAME@", gameName, Qt::CaseInsensitive);
                    watchFolder.replace("@engineroot@", normalizedEngineRoot, Qt::CaseInsensitive);
                    watchFolder = AssetUtilities::NormalizeDirectoryPath(watchFolder);
                    
                    QVariant includeParams = loader.value("include", QString());
                    QVariant excludeParams = loader.value("exclude", QString());
                    
                    QString includeTagString = includeParams.type() == QVariant::StringList ? includeParams.toStringList().join(",") : includeParams.toString();
                    QString excludeTagString = excludeParams.type() == QVariant::StringList ? excludeParams.toStringList().join(",") : excludeParams.toString();

                    QStringList includeIdentifiers = includeTagString.split(QChar(','), Qt::SkipEmptyParts);
                    QStringList excludeIdentifiers = excludeTagString.split(QChar(','), Qt::SkipEmptyParts);

                    AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
                    PopulatePlatformsForScanFolder(platforms, includeIdentifiers, excludeIdentifiers);

                    QString outputPrefix = loader.value("output", QString()).toString();

                    // note that the old way of computing the scan folder portable key is retained below
                    // for purposes of database compatibility - so that assets are not recompiled unnecessarily
                    // this does mean that for old compatibility you may not have a watch folder configuration that has non-unique first part
                    // after the ScanFolder section here, meaning the following would be in conflict:
                    // [ScanFolder My Game]
                    // [ScanFolder My Gem]
                    // the portable key should absolutely not include the outputprefix because you can map
                    // multiple folders into the same output prefix, it is not a suitable unique identifier.
                    QString oldDisplayName = group.split(" ", Qt::SkipEmptyParts)[1];
                    QString scanFolderPortableKey = QString("from-ini-file-%1").arg(oldDisplayName);

                    // the new way of computing the display name involves taking everything after the "[ScanFolder " section name.
                    // mid(11) is everything after "ScanFolder ", so it can include spaces in its display name.
                    QString scanFolderDisplayName = group.mid(11);

                    // Override display name and normalize outputPrefix only after the portable key has been generated.
                    // Since the portable key is built up using the display name,
                    // existing databases would see recompile on all assets within this watch folder if these aleady exist
                    outputPrefix = AssetUtilities::NormalizeDirectoryPath(outputPrefix);
                    QString overrideDisplayName = loader.value("display", QString()).toString();
                    if (!overrideDisplayName.isEmpty())
                    {
                        scanFolderDisplayName = overrideDisplayName;
                    }

                    // you are allowed to use macros in your display name.
                    scanFolderDisplayName.replace("@GAMENAME@", gameName, Qt::CaseInsensitive);
                    scanFolderDisplayName.replace("@root@", normalizedRoot, Qt::CaseInsensitive);
                    scanFolderDisplayName.replace("@engineroot@", normalizedEngineRoot, Qt::CaseInsensitive);

                    bool recurse = (loader.value("recursive", 1).toInt() == 1);
                    bool isRoot = (watchFolder.compare(normalizedRoot, Qt::CaseInsensitive) == 0);
                    recurse &= !isRoot; // root never recurses

                    // New assets can be saved in any scan folder defined except for the engine root.
                    const bool canSaveNewAssets = !isRoot;

                    if (!watchFolder.isEmpty())
                    {
                        AddScanFolder(ScanFolderInfo(
                            watchFolder,
                            scanFolderDisplayName,
                            scanFolderPortableKey,
                            outputPrefix,
                            isRoot,
                            recurse,
                            platforms,
                            order,
                            /*scanFolderId*/ 0,
                            canSaveNewAssets));
                    }

                    loader.endGroup();
                }

                if (group.startsWith("Exclude ", Qt::CaseInsensitive))
                {
                    loader.beginGroup(group);

                    ExcludeAssetRecognizer excludeRecognizer;
                    excludeRecognizer.m_name = group.mid(8); // chop off the "Exclude " and you're left with the remainder name
                    QString pattern = loader.value("pattern", QString()).toString();
                    AssetBuilderSDK::AssetBuilderPattern::PatternType patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                    if (pattern.isEmpty())
                    {
                        patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                        pattern = loader.value("glob", QString()).toString();
                    }
                    AZStd::string excludePattern(pattern.toUtf8().data());
                    excludeRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(excludePattern, patternType);
                    m_excludeAssetRecognizers[excludeRecognizer.m_name] = excludeRecognizer;
                    loader.endGroup();
                }

                if (group.startsWith("RC ", Qt::CaseInsensitive))
                {
                    loader.beginGroup(group);

                    AssetRecognizer rec;
                    rec.m_name = group.mid(3); // chop off the "RC " and you're left with the remainder name

                    if (loader.value("ignore", false).toBool())
                    {
                        // This allows a game-specific configuration to remove an AssetRecognizer that exists in
                        // the default configuration. For example, some projects may provide an AssetBuilder for 
                        // a file type that normally uses RC by default.
                        m_assetRecognizers.remove(rec.m_name);
                    }
                    else
                    {
                        if (m_assetRecognizers.find(rec.m_name) != m_assetRecognizers.end())
                        {
                            rec = m_assetRecognizers[rec.m_name];
                        }

                        if (!ReadRecognizerFromConfig(rec, loader))
                        {
                            return false;
                        }
                        if (!rec.m_platformSpecs.empty())
                        {
                            m_assetRecognizers[rec.m_name] = rec;
                        }
                    }

                    loader.endGroup();
                }
            }
        }
        return true;
    }

    void PlatformConfiguration::ReadMetaDataFromConfigFile(QString iniPath)
    {
        if (QFile::exists(iniPath))
        {
            QSettings loader(iniPath, QSettings::IniFormat);

            //Read in Metadata types
            loader.beginGroup("MetaDataTypes");
            QStringList fileExtensions = loader.allKeys();
            for (int idx = 0; idx < fileExtensions.count(); idx++)
            {
                QString fileType = AssetUtilities::NormalizeFilePath(fileExtensions[idx]);
                QString extensionType = loader.value(fileType, QString()).toString();

                AddMetaDataType(fileType, extensionType);

                // Check if the Metadata 'file type' is a real file
                QString fullPath = FindFirstMatchingFile(fileType);
                if (!fullPath.isEmpty())
                {
                    m_metaDataRealFiles.insert(fileType.toLower());
                }
            }

            loader.endGroup();
        }
    }

    const AZStd::vector<AssetBuilderSDK::PlatformInfo>& PlatformConfiguration::GetEnabledPlatforms() const
    {
        return m_enabledPlatforms;
    }

    const AssetBuilderSDK::PlatformInfo* const PlatformConfiguration::GetPlatformByIdentifier(const char* identifier) const
    {
        for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
        {
            if (platform.m_identifier == identifier)
            {
                // this may seem odd - returning a pointer into a vector, but this vector is initialized once during startup and then remains static thereafter.
                return &platform;
            }
        }
        return nullptr;
    }

    QPair<QString, QString> PlatformConfiguration::GetMetaDataFileTypeAt(int pos) const
    {
        return m_metaDataFileTypes[pos];
    }

    bool PlatformConfiguration::IsMetaDataTypeRealFile(QString relativeName) const
    {
        return m_metaDataRealFiles.find(relativeName.toLower()) != m_metaDataRealFiles.end();
    }

    void PlatformConfiguration::EnablePlatform(const AssetBuilderSDK::PlatformInfo& platform, bool enable)
    {
        // remove it if present.
        auto platformIt = std::find_if(m_enabledPlatforms.begin(), m_enabledPlatforms.end(), [&](const AssetBuilderSDK::PlatformInfo& info)
                {
                    return info.m_identifier == platform.m_identifier;
                });


        if (platformIt != m_enabledPlatforms.end())
        {
            // already present - replace or remove it.
            if (enable)
            {
                *platformIt = platform;
            }
            else
            {
                m_enabledPlatforms.erase(platformIt);
            }
        }
        else
        {
            // it is not already present.  we only add it if we're enabling.
            // if we're disabling, there's nothing to do.
            if (enable)
            {
                m_enabledPlatforms.push_back(platform);
            }
        }
    }

    bool PlatformConfiguration::GetMatchingRecognizers(QString fileName, RecognizerPointerContainer& output) const
    {
        bool foundAny = false;
        if (IsFileExcluded(fileName))
        {
            //if the file is excluded than return false;
            return false;
        }
        for (const AssetRecognizer& recognizer : m_assetRecognizers)
        {
            if (recognizer.m_patternMatcher.MatchesPath(fileName.toUtf8().constData()))
            {
                // found a match
                output.push_back(&recognizer);
                foundAny = true;
            }
        }
        return foundAny;
    }

    int PlatformConfiguration::GetScanFolderCount() const
    {
        return aznumeric_caster(m_scanFolders.size());
    }

    AZStd::vector<AzToolsFramework::AssetUtils::GemInfo> PlatformConfiguration::GetGemsInformation() const
    {
        return m_gemInfoList;
    }

    AssetProcessor::ScanFolderInfo& PlatformConfiguration::GetScanFolderAt(int index)
    {
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < m_scanFolders.size());
        return m_scanFolders[index];
    }

    void PlatformConfiguration::AddScanFolder(const AssetProcessor::ScanFolderInfo& source, bool isUnitTesting)
    {
        if (isUnitTesting)
        {
            //using a bool instead of using #define UNIT_TEST because the user can also run batch processing in unittest
            m_scanFolders.push_back(source);
            return;
        }

        // Find and remove any previous matching entry, last entry wins
        auto it = std::find_if(m_scanFolders.begin(), m_scanFolders.end(), [&source](const ScanFolderInfo& info)
                {
                    return info.GetPortableKey().toLower() == source.GetPortableKey().toLower();
                });
        if (it != m_scanFolders.end())
        {
            m_scanFolders.erase(it);
        }

        m_scanFolders.push_back(source);

        std::stable_sort(m_scanFolders.begin(), m_scanFolders.end(), [](const ScanFolderInfo& a, const ScanFolderInfo& b)
            {
                return a.GetOrder() < b.GetOrder();
            }
        );
    }

    void PlatformConfiguration::AddRecognizer(const AssetRecognizer& source)
    {
        m_assetRecognizers.insert(source.m_name, source);
    }

    void PlatformConfiguration::RemoveRecognizer(QString name)
    {
        auto found = m_assetRecognizers.find(name);
        m_assetRecognizers.erase(found);
    }

    void PlatformConfiguration::AddMetaDataType(const QString& type, const QString& extension)
    {
        QPair<QString, QString> key = qMakePair(type.toLower(), extension.toLower());
        if (!m_metaDataFileTypes.contains(key))
        {
            m_metaDataFileTypes.push_back(key);
        }
    }

    bool PlatformConfiguration::ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName, bool includeOutputPrefix) const
    {
        const ScanFolderInfo* info = GetScanFolderForFile(fullFileName);

        if (info)
        {
            scanFolderName = info->ScanPath();
            scanFolderName.replace(AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

            return ConvertToRelativePath(fullFileName, info, databaseSourceName, includeOutputPrefix);
        }
        // did not find it.
        return false;
    }

    bool PlatformConfiguration::ConvertToRelativePath(const QString& fullFileName, const ScanFolderInfo* scanFolderInfo, QString& databaseSourceName, bool includeOutputPrefix)
    {
        if(!scanFolderInfo)
        {
            return false;
        }

        QString relPath; // empty string.
        if (fullFileName.length() > scanFolderInfo->ScanPath().length())
        {
            relPath = fullFileName.right(fullFileName.length() - scanFolderInfo->ScanPath().length() - 1); // also eat the slash, hence -1
        }

        if ((scanFolderInfo->GetOutputPrefix().isEmpty()) || (!includeOutputPrefix))
        {
            databaseSourceName = relPath;
        }
        else
        {
            databaseSourceName = scanFolderInfo->GetOutputPrefix();

            if (!relPath.isEmpty())
            {
                databaseSourceName += '/';
                databaseSourceName += relPath;
            }
        }

        databaseSourceName.replace(AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

        return true;
    }

    QString PlatformConfiguration::GetOverridingFile(QString relativeName, QString scanFolderName) const
    {
        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            AssetProcessor::ScanFolderInfo scanFolderInfo = m_scanFolders[pathIdx];

            if (scanFolderName.compare(scanFolderInfo.ScanPath(), Qt::CaseInsensitive) == 0)
            {
                // we have found the actual folder containing the file we started with
                // since all other folders "deeper" in the override vector are lower priority than this one
                // (they are sorted in priority order, most priority first).
                return QString();
            }
            QString tempRelativeName(relativeName);
            //if relative path starts with the output prefix than remove it first
            if (!scanFolderInfo.GetOutputPrefix().isEmpty() && tempRelativeName.startsWith(scanFolderInfo.GetOutputPrefix(), Qt::CaseInsensitive))
            {
                tempRelativeName = tempRelativeName.right(tempRelativeName.length() - scanFolderInfo.GetOutputPrefix().length() - 1); // also eat the slash, hence -1
            }

            if ((!scanFolderInfo.RecurseSubFolders()) && (tempRelativeName.contains('/')))
            {
                // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
                continue;
            }

            // note that we only Update To Correct Case here, because this is one of the few situations where
            // a file with the same relative path may be overridden but different case.
            if (AssetUtilities::UpdateToCorrectCase(scanFolderInfo.ScanPath(), tempRelativeName))
            {
                // we have found a file in an earlier scan folder that would override this file
                return QDir(scanFolderInfo.ScanPath()).absoluteFilePath(tempRelativeName);
            }
        }

        // we found it nowhere.
        return QString();
    }

    QString PlatformConfiguration::FindFirstMatchingFile(QString relativeName) const
    {
        if (relativeName.isEmpty())
        {
            return QString();
        }

        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            AssetProcessor::ScanFolderInfo scanFolderInfo = m_scanFolders[pathIdx];

            QString tempRelativeName(relativeName);

            //if relative path starts with the output prefix than remove it first
            if (!scanFolderInfo.GetOutputPrefix().isEmpty() && tempRelativeName.startsWith(scanFolderInfo.GetOutputPrefix(), Qt::CaseInsensitive))
            {
                tempRelativeName = tempRelativeName.right(tempRelativeName.length() - scanFolderInfo.GetOutputPrefix().length() - 1); // also eat the slash, hence -1
            }
            if ((!scanFolderInfo.RecurseSubFolders()) && (tempRelativeName.contains('/')))
            {
                // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
                continue;
            }
            QDir rooted(scanFolderInfo.ScanPath());
            QString absolutePath = rooted.absoluteFilePath(tempRelativeName);
            AssetProcessor::FileStateInfo fileStateInfo;
            auto* fileStateInterface = AZ::Interface<AssetProcessor::IFileStateRequests>::Get();
            if (fileStateInterface)
            {
                if (fileStateInterface->GetFileInfo(absolutePath, &fileStateInfo))
                {
                    return AssetUtilities::NormalizeFilePath(fileStateInfo.m_absolutePath);
                }
            }
        }
        return QString();
    }

    QStringList PlatformConfiguration::FindWildcardMatches(const QString& sourceFolder, QString relativeName, bool includeFolders, bool recursiveSearch) const
    {
        if (relativeName.isEmpty())
        {
            return QStringList();
        }

        const int pathLen = sourceFolder.length() + 1;

        relativeName.replace('\\', '/');

        QStringList returnList;
        QRegExp nameMatch{ relativeName, Qt::CaseInsensitive, QRegExp::Wildcard };
        QDirIterator diretoryIterator(sourceFolder, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, recursiveSearch ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        QStringList files;
        while (diretoryIterator.hasNext())
        {
            diretoryIterator.next();
            if (!includeFolders && !diretoryIterator.fileInfo().isFile())
            {
                continue;
            }
            QString pathMatch{ diretoryIterator.filePath().mid(pathLen) };
            if (nameMatch.exactMatch(pathMatch))
            {
                returnList.append(AssetUtilities::NormalizeFilePath(diretoryIterator.filePath()));
            }
        }
        return returnList;
    }

    const AssetProcessor::ScanFolderInfo* PlatformConfiguration::GetScanFolderForFile(const QString& fullFileName) const
    {
        QString normalized = AssetUtilities::NormalizeFilePath(fullFileName);

        // first, check for an EXACT match.  If there's an exact match, this must be the one returned!
        // this is to catch the case where the actual path of a scan folder is fed in to this.
        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            QString scanFolderName = m_scanFolders[pathIdx].ScanPath();
            if (normalized.compare(scanFolderName, Qt::CaseInsensitive) == 0)
            {
                // if its an exact match, we're basically done
                return &m_scanFolders[pathIdx];
            }
        }

        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            QString scanFolderName = m_scanFolders[pathIdx].ScanPath();
            if (normalized.length() > scanFolderName.length())
            {
                if (normalized.startsWith(scanFolderName, Qt::CaseInsensitive))
                {
                    QChar examineChar = normalized[scanFolderName.length()]; // it must be a slash or its just a scan folder that starts with the same thing by coincidence.
                    if (examineChar != QChar('/'))
                    {
                        continue;
                    }
                    QString relPath = normalized.right(normalized.length() - scanFolderName.length() - 1); // also eat the slash, hence -1
                    if (!m_scanFolders[pathIdx].RecurseSubFolders())
                    {
                        // we only allow things that are in the root for nonrecursive folders
                        if (relPath.contains('/'))
                        {
                            continue;
                        }
                    }
                    return &m_scanFolders[pathIdx];
                }
            }
        }
        return nullptr; // not found.
    }

    //! Given a scan folder path, get its complete info
    const AssetProcessor::ScanFolderInfo* PlatformConfiguration::GetScanFolderByPath(const QString& scanFolderPath) const
    {
        QString normalized = AssetUtilities::NormalizeFilePath(scanFolderPath);
        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            if (QString::compare(m_scanFolders[pathIdx].ScanPath(), normalized, Qt::CaseSensitive) == 0)
            {
                return &m_scanFolders[pathIdx];
            }
        }
        return nullptr;
    }

    int PlatformConfiguration::GetMinJobs() const
    {
        return m_minJobs;
    }

    int PlatformConfiguration::GetMaxJobs() const
    {
        return m_maxJobs;
    }


    bool PlatformConfiguration::ReadRecognizerFromConfig(AssetRecognizer& target, QSettings& loader)
    {
        AssetBuilderSDK::AssetBuilderPattern::PatternType patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
        QString pattern = loader.value("pattern", QString()).toString();
        if (pattern.isEmpty())
        {
            patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
            pattern = loader.value("glob", QString()).toString();
        }

        if (pattern.isEmpty())
        {
            if (!target.m_patternMatcher.IsValid())
            {
                // no pattern present and we are NOT overriding an existing one.
                AZ_Warning(AssetProcessor::ConsoleChannel, false, "No pattern was found in config %s while reading platform config.\n", target.m_name.toUtf8().data());
                return false;
            }
        }
        else
        {
            // we have provided a non-empty one, override the existing one.
            target.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(AZStd::string(pattern.toUtf8().data()), patternType);

            if (!target.m_patternMatcher.IsValid())
            {
                // no pattern present
                AZ_Warning(AssetProcessor::ConsoleChannel, false, "Invalid pattern in %s while reading platform config : %s \n",
                    target.m_name.toUtf8().data(),
                    target.m_patternMatcher.GetErrorString().c_str());
                return false;
            }
        }

        /* so in this particular case we want to end up with an AssetPlatformSpec struct that
            has only got the platforms that 'matter' in it
            so for example, if you have the following enabled platforms
            [Platform PC]
            tags=blah
            [Platform Mac]
            tags=whatever
            [Platform android]
            tags=mobile

            and you encounter a recognizer like:
            [RC blahblah]
            pattern=whatever
            params=abc
            mac=skip
            mobile=hijklmnop
            android=1234

            then the outcome should be a recognizer which has:
            pattern=whatever
            pc=abc        -- no tags or platforms matched but we do have a default params
            android=1234  -- because even though it matched the mobile tag, platforms explicitly specified take precidence
                (and no mac)  -- because it matched a skip rule

            So the stragegy will be to read the default params
            - if present, we prepopulate all th platforms with it
            - If missing, we prepopulate nothing

            Then loop over the other params and
                if the key matches a tag, if it does we add/change that platform
                    (if its 'skip' we remove it)
                if the key matches a platform, if it does we add/change that platform
                    (if its 'skip' we remove it)
        */

        // Qt actually has a custom INI parse which treats commas as string lists.
        // so if we want the original (with commas) we have to check for that
        QVariant paramValue = loader.value("params", QString());
        QString defaultParams = paramValue.type() == QVariant::StringList ? paramValue.toStringList().join(",") : paramValue.toString();

        for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
        {
            QString platformIdentifier = QString::fromUtf8(platform.m_identifier.c_str());
            QString currentRCParams = defaultParams; // blank default is okay.

            // first, iterate over tags
            for (const AZStd::string& tag : platform.m_tags)
            {
                paramValue = loader.value(tag.c_str(), QString());
                QString tagParams = paramValue.type() == QVariant::StringList ? paramValue.toStringList().join(",") : paramValue.toString();

                if (!tagParams.isEmpty())
                {
                    // if we get here it means we found a tag that applies to this platform int he rc block.
                    currentRCParams = tagParams;
                }
            }

            // now check if there's a specific platform tag (like "Pc"=whatever)
            paramValue = loader.value(platformIdentifier, QString());
            QString platformSpecificParams = paramValue.type() == QVariant::StringList ? paramValue.toStringList().join(",") : paramValue.toString();

            if (!platformSpecificParams.isEmpty())
            {
                currentRCParams = platformSpecificParams;
            }

            // now generate a platform spec as long as we're not skipping
            if (currentRCParams.trimmed().compare("skip", Qt::CaseInsensitive) != 0)
            {
                AssetPlatformSpec spec;
                // a special case exists where this is "overriding" an underlying version.
                // in this case, unless some string was specified for the overrider, we use the underlying one
                if (!currentRCParams.isEmpty())
                {
                    spec.m_extraRCParams = currentRCParams;
                }
                else
                {
                    if (target.m_platformSpecs.contains(platformIdentifier))
                    {
                        // carry over the prior
                        spec.m_extraRCParams = target.m_platformSpecs[platformIdentifier].m_extraRCParams;
                    }
                }
                target.m_platformSpecs[platformIdentifier] = spec;
            }
        }

        target.m_version = loader.value("version", QString()).toString();
        target.m_testLockSource = loader.value("lockSource", false).toBool();
        target.m_isCritical = loader.value("critical", false).toBool();
        target.m_checkServer = loader.value("checkServer", false).toBool();
        target.m_supportsCreateJobs = loader.value("supportsCreateJobs", false).toBool();
        target.m_priority = loader.value("priority", 0).toInt();
        QString assetTypeString = loader.value("productAssetType", QString()).toString();
        target.m_outputProductDependencies = loader.value("outputProductDependencies", false).toBool();
        if (!assetTypeString.isEmpty())
        {
            target.m_productAssetType = AZ::Uuid(assetTypeString.toUtf8().data());
            if (target.m_productAssetType.IsNull())
            {
                // you specified a UUID and it did not read.  A warning will have already been issued
                return false;
            }
        }


        return true;
    }

    void PlatformConfiguration::AddGemScanFolders(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList)
    {
        int gemOrder = g_gemStartingOrder;
        int gemGameOrder = 1; // game gems are very high priority, start at 1 onwards.
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        PopulatePlatformsForScanFolder(platforms);

        for (const AzToolsFramework::AssetUtils::GemInfo& gemElement : gemInfoList)
        {
            QString gemGuid = gemElement.m_identifier.c_str();
            QString gemAbsolutePath = gemElement.m_absoluteFilePath.c_str(); // this is an absolute path!
            QString gemDisplayName = gemElement.m_gemName.c_str();
            QString gemRelativePath = gemElement.m_relativeFilePath.c_str();

            QDir gemDir(gemAbsolutePath);

            // The gems /Assets/ folders are always added to the watch list, we want the following params
            //      Watched folder: (absolute path to the gem /Assets/ folder) MUST BE CORRECT CASE
            //      Display name:   "Gems/GemName/Assets" // uppercase, for human eyes
            //      portable Key:   "gemassets-(UUID Of Gem)"
            //      Output Prefix:  "" // empty string - this means put it in @assets@ as per default
            //      Is Root:        False
            //      Recursive:      True
            QString gemFolder = gemDir.absoluteFilePath(AzToolsFramework::AssetUtils::GemInfo::GetGemAssetFolder().c_str());

            // note that we normalize this gem path with slashes so that there's nothing special about it compared to other scan folders
            gemFolder = AssetUtilities::NormalizeDirectoryPath(gemFolder);

            QString assetBrowserDisplayName = AzToolsFramework::AssetUtils::GemInfo::GetGemAssetFolder().c_str(); // Gems always use assets folder as their displayname...
            QString portableKey = QString("gemassets-%1").arg(gemGuid);
            QString outputPrefix; // empty intentionally here
            bool isRoot = false;
            bool isRecursive = true;
            int order = gemElement.m_isGameGem ? gemGameOrder++ : gemOrder++;

            AZ_TracePrintf(AssetProcessor::DebugChannel, "Adding GEM assets folder for monitoring / scanning: %s.\n", gemFolder.toUtf8().data());
            AddScanFolder(ScanFolderInfo(
                gemFolder,
                assetBrowserDisplayName,
                portableKey,
                outputPrefix,
                isRoot,
                isRecursive,
                platforms,
                order,
                /*scanFolderId*/ 0,
                /*canSaveNewAssets*/ true)); // Users can create assets like slices in Gem asset folders.
        }
    }

    const RecognizerContainer& PlatformConfiguration::GetAssetRecognizerContainer() const
    {
        return this->m_assetRecognizers;
    }

    const ExcludeRecognizerContainer& PlatformConfiguration::GetExcludeAssetRecognizerContainer() const
    {
        return this->m_excludeAssetRecognizers;
    }

    void AssetProcessor::PlatformConfiguration::AddExcludeRecognizer(const ExcludeAssetRecognizer& recogniser)
    {
        m_excludeAssetRecognizers.insert(recogniser.m_name, recogniser);
    }

    void AssetProcessor::PlatformConfiguration::RemoveExcludeRecognizer(QString name)
    {
        auto found = m_excludeAssetRecognizers.find(name);
        if (found != m_excludeAssetRecognizers.end())
        {
            m_excludeAssetRecognizers.erase(found);
        }
    }

    bool AssetProcessor::PlatformConfiguration::IsFileExcluded(QString fileName) const
    {
        for (const ExcludeAssetRecognizer& excludeRecognizer : m_excludeAssetRecognizers)
        {
            if (excludeRecognizer.m_patternMatcher.MatchesPath(fileName.toUtf8().constData()))
            {
                return true;
            }
        }
        return false;
    }

    bool AssetProcessor::PlatformConfiguration::IsValid() const
    {
        if (m_fatalError.empty())
        {
            if (m_enabledPlatforms.empty())
            {
                m_fatalError = "The configuration is invalid - no platforms appear to be enabled. Check to make sure that the AssetProcessorPlatformConfig.ini file(s) are present and correct.";
            }
            else if (m_assetRecognizers.empty())
            {
                m_fatalError = "The configuration is invalid - no matching asset recognizers appear valid.  Check to make sure that the AssetProcessorPlatformConfig.ini file(s) are present and correct.";
            }
            else if (m_scanFolders.empty())
            {
                m_fatalError = "The configuration is invalid - no scan folders defined.  Check to make sure that the AssetProcessorPlatformConfig.ini file(s) are present and correct.";
            }
        }

        if (!m_fatalError.empty())
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Error: %s", m_fatalError.c_str());
            return false;
        }

        return true;
    }

    const AZStd::string& AssetProcessor::PlatformConfiguration::GetError() const
    {
        return m_fatalError;
    }
} // namespace assetProcessor

