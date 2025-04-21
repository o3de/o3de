/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "native/utilities/PlatformConfiguration.h"
#include "native/AssetManager/FileStateCache.h"
#include "native/assetprocessor.h"

#include <QDirIterator>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Gem/GemInfo.h>
#include <AzToolsFramework/Asset/AssetUtils.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>


namespace AssetProcessor
{
    struct AssetImporterPathsVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        AssetImporterPathsVisitor(AZ::SettingsRegistryInterface* settingsRegistry, AZStd::vector<AZStd::string>& supportedExtension)
            : m_settingsRegistry(settingsRegistry)
            , m_supportedFileExtensions(supportedExtension)
        {
        }

        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit(const AZ::SettingsRegistryInterface::VisitArgs&, AZStd::string_view value) override
        {
            if (auto found = value.find('.'); found != AZStd::string::npos)
            {
                m_supportedFileExtensions.emplace_back(value.substr(found + 1));
            }
            else
            {
                m_supportedFileExtensions.emplace_back(value);
            }
        }

        AZ::SettingsRegistryInterface* m_settingsRegistry;
        AZStd::vector<AZStd::string> m_supportedFileExtensions;
    };

    //! Visitor for reading the "/Amazon/AssetProcessor/Settings/ScanFolder *" entries from the Settings Registry
    //! Expects the key to path to the visitor to be "/Amazon/AssetProcessor/Settings"
    struct ScanFolderVisitor
        : AZ::SettingsRegistryVisitorUtils::ObjectVisitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        AZ::SettingsRegistryInterface::VisitResponse Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs) override;

        struct ScanFolderInfo
        {
            AZStd::string m_scanFolderIdentifier;
            AZStd::string m_scanFolderDisplayName;
            AZ::IO::Path m_watchPath{ AZ::IO::PosixPathSeparator };
            AZStd::vector<AZStd::string> m_includeIdentifiers;
            AZStd::vector<AZStd::string> m_excludeIdentifiers;
            int m_scanOrder{};
            bool m_isRecursive{};
        };
        AZStd::vector<ScanFolderInfo> m_scanFolderInfos;
    };

    struct ExcludeVisitor
        : AZ::SettingsRegistryVisitorUtils::ObjectVisitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        AZ::SettingsRegistryInterface::VisitResponse Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs) override;

        AZStd::vector<ExcludeAssetRecognizer> m_excludeAssetRecognizers;
    };

    struct SimpleJobVisitor
        : AZ::SettingsRegistryVisitorUtils::ObjectVisitor
    {
        SimpleJobVisitor(const AZStd::vector<AssetBuilderSDK::PlatformInfo>& enabledPlatforms)
            : m_enabledPlatforms(enabledPlatforms)
        {
        }
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        AZ::SettingsRegistryInterface::VisitResponse Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs) override;

        struct SimpleJobAssetRecognizer
        {
            AssetRecognizer m_recognizer;
            AZStd::string m_defaultParams;
            bool m_ignore{};
        };
        AZStd::vector<SimpleJobAssetRecognizer> m_assetRecognizers;
    private:
        void ApplyParamsOverrides(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs,
            SimpleJobAssetRecognizer& assetRecognizer);

        const AZStd::vector<AssetBuilderSDK::PlatformInfo>& m_enabledPlatforms;
    };

    //! This vistor reads in the Asset Cache Server configuration elements from the settings registry
    struct ACSVisitor
        : AZ::SettingsRegistryVisitorUtils::ObjectVisitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        AZ::SettingsRegistryInterface::VisitResponse Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs) override;

        AZStd::vector<AssetRecognizer> m_assetRecognizers;
    };

    struct PlatformsInfoVisitor
        : AZ::SettingsRegistryVisitorUtils::ObjectVisitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        AZ::SettingsRegistryInterface::VisitResponse Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs) override
        {
            // Visit any each "Platform *" field that is a direct child of the object at the AssetProcessorSettingsKey
            constexpr AZStd::string_view PlatformInfoPrefix = "Platform ";
            if (!visitArgs.m_fieldName.starts_with(PlatformInfoPrefix))
            {
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }

            // Retrieve the platform name from the rest of valueName portion of the key "Platform (.*)"
            AZStd::string platformIdentifier = visitArgs.m_fieldName.substr(PlatformInfoPrefix.size());
            // Lowercase the platformIdentifier
            AZStd::to_lower(platformIdentifier.begin(), platformIdentifier.end());

            // Look up the "tags" field that is child of the "Platform (.*)" field
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            const auto tagKeyPath = FixedValueString(visitArgs.m_jsonKeyPath) + "/tags";
            if (AZStd::string tagValue; visitArgs.m_registry.Get(tagValue, tagKeyPath))
            {
                AZStd::unordered_set<AZStd::string> platformTags;
                auto JoinTags = [&platformTags](AZStd::string_view token)
                {
                    AZStd::string cleanedTag{ token };
                    AZStd::to_lower(cleanedTag.begin(), cleanedTag.end());
                    platformTags.insert(AZStd::move(cleanedTag));
                };
                AZ::StringFunc::TokenizeVisitor(tagValue, JoinTags, ',');
                m_platformInfos.emplace_back(platformIdentifier, platformTags);
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        AZStd::vector<AssetBuilderSDK::PlatformInfo> m_platformInfos;
    };

    struct MetaDataTypesVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZStd::string_view value) override
        {
            m_metaDataTypes.push_back({ AZ::IO::PathView(visitArgs.m_fieldName, AZ::IO::PosixPathSeparator).LexicallyNormal().String(), value });
        }

        struct MetaDataType
        {
            AZStd::string m_fileType;
            AZStd::string m_extensionType;
        };
        AZStd::vector<MetaDataType> m_metaDataTypes;
    };


    AZ::SettingsRegistryInterface::VisitResponse ScanFolderVisitor::Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
    {
        constexpr AZStd::string_view ScanFolderInfoPrefix = "ScanFolder ";
        // Check if a "ScanFolder *" element is being traversed
        if (!visitArgs.m_fieldName.starts_with(ScanFolderInfoPrefix))
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        AZStd::string_view currentScanFolderIdentifier = visitArgs.m_fieldName.substr(ScanFolderInfoPrefix.size());

        ScanFolderInfo& scanFolderInfo = m_scanFolderInfos.emplace_back();
        scanFolderInfo.m_scanFolderIdentifier = currentScanFolderIdentifier;
        scanFolderInfo.m_scanFolderDisplayName = currentScanFolderIdentifier;

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        if (AZ::s64 value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/recursive"))
        {
            scanFolderInfo.m_isRecursive = value != 0;
        }
        if (AZ::s64 value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/order"))
        {
            scanFolderInfo.m_scanOrder = static_cast<int>(value);
        }

        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/watch"))
        {
            scanFolderInfo.m_watchPath = value;
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/display")
            && !value.empty())
        {
            scanFolderInfo.m_scanFolderDisplayName = value;
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/include"))
        {
            auto JoinTags = [&scanFolderInfo](AZStd::string_view token)
            {
                scanFolderInfo.m_includeIdentifiers.push_back(token);
            };
            AZ::StringFunc::TokenizeVisitor(value, JoinTags, ',');
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/exclude"))
        {
            auto JoinTags = [&scanFolderInfo](AZStd::string_view token)
            {
                scanFolderInfo.m_excludeIdentifiers.push_back(token);
            };
            AZ::StringFunc::TokenizeVisitor(value, JoinTags, ',');
        }

        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }

    AZ::SettingsRegistryInterface::VisitResponse ExcludeVisitor::Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
    {
        constexpr AZStd::string_view ExcludeNamePrefix = "Exclude ";
        if (!visitArgs.m_fieldName.starts_with(ExcludeNamePrefix))
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        AZStd::string_view excludeName = visitArgs.m_fieldName.substr(ExcludeNamePrefix.size());
        ExcludeAssetRecognizer& excludeAssetRecognizer = m_excludeAssetRecognizers.emplace_back();
        excludeAssetRecognizer.m_name = QString::fromUtf8(excludeName.data(), aznumeric_cast<int>(excludeName.size()));


        // The "pattern" and "glob" entries were previously parsed by QSettings which un-escapes the values
        // To compensate for it the AssetProcessorPlatformConfig.ini was escaping the
        // backslash character used to escape other characters, therefore causing a "double escape"
        // situation
        auto UnescapePattern = [](AZStd::string_view pattern)
        {
            constexpr AZStd::string_view backslashEscape = R"(\\)";
            AZStd::string unescapedResult;
            while (!pattern.empty())
            {
                size_t pos = pattern.find(backslashEscape);
                if (pos != AZStd::string_view::npos)
                {
                    unescapedResult += pattern.substr(0, pos);
                    unescapedResult += '\\';
                    // Move the pattern string after the double backslash characters
                    pattern = pattern.substr(pos + backslashEscape.size());
                }
                else
                {
                    unescapedResult += pattern;
                    pattern = {};
                }
            }

            return unescapedResult;
        };

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/pattern"))
        {
            if (!value.empty())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                excludeAssetRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/glob"))
        {
            if (!excludeAssetRecognizer.m_patternMatcher.IsValid())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                excludeAssetRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }

        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }

    AZ::SettingsRegistryInterface::VisitResponse SimpleJobVisitor::Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
    {
        constexpr AZStd::string_view RCNamePrefix = "RC "; // RC = Resource Compiler
        constexpr AZStd::string_view SJNamePrefix = "SJ "; // SJ = Simple Job

        if (!visitArgs.m_fieldName.starts_with(RCNamePrefix) && !visitArgs.m_fieldName.starts_with(SJNamePrefix))
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        AZStd::string_view sjNameView = visitArgs.m_fieldName.starts_with(SJNamePrefix)
            ? visitArgs.m_fieldName.substr(SJNamePrefix.size())
            : visitArgs.m_fieldName.substr(RCNamePrefix.size());

        auto& assetRecognizer = m_assetRecognizers.emplace_back();
        assetRecognizer.m_recognizer.m_name = sjNameView;

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/ignore"))
        {
            assetRecognizer.m_ignore = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/lockSource"))
        {
            assetRecognizer.m_recognizer.m_testLockSource = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/critical"))
        {
            assetRecognizer.m_recognizer.m_isCritical = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/checkServer"))
        {
            assetRecognizer.m_recognizer.m_checkServer = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/supportsCreateJobs"))
        {
            assetRecognizer.m_recognizer.m_supportsCreateJobs = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/outputProductDependencies"))
        {
            assetRecognizer.m_recognizer.m_outputProductDependencies = value;
        }
        if (AZ::s64 value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/priority"))
        {
            assetRecognizer.m_recognizer.m_priority = static_cast<int>(value);
        }

        // The "pattern" and "glob" entries were previously parsed by QSettings which un-escapes the values
        // To compensate for it the AssetProcessorPlatformConfig.ini was escaping the
        // backslash character used to escape other characters, therefore causing a "double escape"
        // situation
        auto UnescapePattern = [](AZStd::string_view pattern)
        {
            constexpr AZStd::string_view backslashEscape = R"(\\)";
            AZStd::string unescapedResult;
            while (!pattern.empty())
            {
                size_t pos = pattern.find(backslashEscape);
                if (pos != AZStd::string_view::npos)
                {
                    unescapedResult += pattern.substr(0, pos);
                    unescapedResult += '\\';
                    // Move the pattern string after the double backslash characters
                    pattern = pattern.substr(pos + backslashEscape.size());
                }
                else
                {
                    unescapedResult += pattern;
                    pattern = {};
                }
            }

            return unescapedResult;
        };

        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/pattern"))
        {
            if (!value.empty())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                assetRecognizer.m_recognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/glob"))
        {
            // Add the glob pattern if it the matter matcher doesn't already contain a valid regex pattern
            if (!assetRecognizer.m_recognizer.m_patternMatcher.IsValid())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                assetRecognizer.m_recognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/version"))
        {
            assetRecognizer.m_recognizer.m_version = value;
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/productAssetType"))
        {
            if (!value.empty())
            {
                AZ::Uuid productAssetType{ value.data(), value.size() };
                if (!productAssetType.IsNull())
                {
                    assetRecognizer.m_recognizer.m_productAssetType = productAssetType;
                }
            }
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/params"))
        {
            assetRecognizer.m_defaultParams = value;
        }

        ApplyParamsOverrides(visitArgs, assetRecognizer);
        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }

    void SimpleJobVisitor::ApplyParamsOverrides(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, SimpleJobAssetRecognizer& assetRecognizer)
    {
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
            [SJ blahblah]
            pattern=whatever
            params=abc
            mac=skip
            mobile=hijklmnop
            android=1234

            then the outcome should be a recognizer which has:
            pattern=whatever
            pc=abc        -- no tags or platforms matched but we do have a default params
            android=1234  -- because even though it matched the mobile tag, platforms explicitly specified take precedence
                (and no mac)  -- because it matched a skip rule

            So the strategy will be to read the default params
            - if present, we pre-populate all the platforms with it
            - If missing, we pre-populate nothing

            Then loop over the other params and
                if the key matches a tag, if it does we add/change that platform
                    (if its 'skip' we remove it)
                if the key matches a platform, if it does we add/change that platform
                    (if its 'skip' we remove it)
        */
        for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
        {
            // Exclude the common platform from the internal copy builder, we don't support it as an output for assets currently
            if(platform.m_identifier == AssetBuilderSDK::CommonPlatformName)
            {
                continue;
            }

            AZStd::string_view currentParams = assetRecognizer.m_defaultParams;
            // The "/Amazon/AssetProcessor/Settings/SJ */<platform>" entry will be queried
            AZ::IO::Path overrideParamsKey = AZ::IO::Path(AZ::IO::PosixPathSeparator);
            overrideParamsKey /= visitArgs.m_jsonKeyPath;
            overrideParamsKey /= platform.m_identifier;

            AZ::SettingsRegistryInterface::FixedValueString overrideParamsValue;
            // Check if the enabled platform identifier matches a key within the "SJ *" object
            if (visitArgs.m_registry.Get(overrideParamsValue, overrideParamsKey.Native()))
            {
                currentParams = overrideParamsValue;
            }
            else
            {
                // otherwise check for tags associated with the platform
                for (const AZStd::string& tag : platform.m_tags)
                {
                    overrideParamsKey.ReplaceFilename(AZ::IO::PathView(tag));
                    if (visitArgs.m_registry.Get(overrideParamsValue, overrideParamsKey.Native()))
                    {
                        // if we get here it means we found a tag that applies to this platform
                        currentParams = overrideParamsValue;
                        break;
                    }
                }
            }

            // now generate a platform spec as long as we're not skipping
            if (!AZ::StringFunc::Equal(currentParams, "skip"))
            {
                assetRecognizer.m_recognizer.m_platformSpecs[platform.m_identifier] = AssetInternalSpec::Copy;
            }
        }
    }

    AZ::SettingsRegistryInterface::VisitResponse ACSVisitor::Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
    {
        constexpr AZStd::string_view ACSNamePrefix = "ACS ";
        if (!visitArgs.m_fieldName.starts_with(ACSNamePrefix))
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        AZStd::string name = visitArgs.m_fieldName.substr(ACSNamePrefix.size());

        AssetRecognizer& assetRecognizer = m_assetRecognizers.emplace_back();
        assetRecognizer.m_name = name;

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/lockSource"))
        {
            assetRecognizer.m_testLockSource = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/critical"))
        {
            assetRecognizer.m_isCritical = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/checkServer"))
        {
            assetRecognizer.m_checkServer = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/supportsCreateJobs"))
        {
            assetRecognizer.m_supportsCreateJobs = value;
        }
        if (bool value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/outputProductDependencies"))
        {
            assetRecognizer.m_outputProductDependencies = value;
        }

        if (AZ::s64 value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/priority"))
        {
            assetRecognizer.m_priority = aznumeric_cast<int>(value);
        }

        // The "pattern" and "glob" entries were previously parsed by QSettings which un-escapes the values
        // To compensate for it the AssetProcessorPlatformConfig.ini was escaping the
        // backslash character used to escape other characters, therefore causing a "double escape"
        // situation
        auto UnescapePattern = [](AZStd::string_view pattern)
        {
            constexpr AZStd::string_view backslashEscape = R"(\\)";
            AZStd::string unescapedResult;
            while (!pattern.empty())
            {
                size_t pos = pattern.find(backslashEscape);
                if (pos != AZStd::string_view::npos)
                {
                    unescapedResult += pattern.substr(0, pos);
                    unescapedResult += '\\';
                    // Move the pattern string after the double backslash characters
                    pattern = pattern.substr(pos + backslashEscape.size());
                }
                else
                {
                    unescapedResult += pattern;
                    pattern = {};
                }
            }

            return unescapedResult;
        };

        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/pattern"))
        {
            if (!value.empty())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                assetRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/glob"))
        {
            // Add the glob pattern if it the matter matcher doesn't already contain a valid regex pattern
            if (!assetRecognizer.m_patternMatcher.IsValid())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                assetRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/version"))
        {
            assetRecognizer.m_version = value;
        }
        if (AZStd::string value;
            visitArgs.m_registry.Get(value, FixedValueString(visitArgs.m_jsonKeyPath) + "/productAssetType"))
        {
            if (!value.empty())
            {
                AZ::Uuid productAssetType{ value.data(), value.size() };
                if (!productAssetType.IsNull())
                {
                    assetRecognizer.m_productAssetType = productAssetType;
                }
            }
        }

        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }

    const char AssetConfigPlatformDir[] = "AssetProcessorConfig/";
    const char AssetProcessorPlatformConfigFileName[] = "AssetProcessorPlatformConfig.ini";
    constexpr const char* ProjectScanFolderKey = "Project/Assets";
    constexpr const char* GemStartingPriorityOrderKey = "/GemScanFolderStartingPriorityOrder";
    constexpr const char* ProjectRelativeGemPriorityKey = "/ProjectRelativeGemsScanFolderPriority";

    PlatformConfiguration::PlatformConfiguration(QObject* pParent)
        : QObject(pParent)
        , m_minJobs(1)
        , m_maxJobs(8)
    {
    }

    bool PlatformConfiguration::AddPlatformConfigFilePaths(AZStd::vector<AZ::IO::Path>& configFilePaths)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Global Settings Registry is not available, the "
                "Engine Root folder cannot be queried")
                return false;
        }
        AZ::IO::FixedMaxPath engineRoot;
        if (!settingsRegistry->Get(engineRoot.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to find Engine Root in Settings Registry");
            return false;
        }

        return AzToolsFramework::AssetUtils::AddPlatformConfigFilePaths(engineRoot.Native(), configFilePaths);
    }

    bool PlatformConfiguration::InitializeFromConfigFiles(const QString& absoluteSystemRoot, const QString& absoluteAssetRoot,
        const QString& projectPath, bool addPlatformConfigs, bool addGemsConfigs)
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
        if (commandLine && commandLine->HasSwitch(ScanFolderPatternOption))
        {
            for (size_t idx = 0; idx < commandLine->GetNumSwitchValues(ScanFolderPatternOption); idx++)
            {
                scanFolderPatterns.push_back(commandLine->GetSwitchValue(ScanFolderPatternOption, idx).c_str());
            }
        }

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "There is no Global Settings Registry set."
                " Unable to merge AssetProcessor config files(*.ini) and Asset processor settings registry files(*.setreg)");
            return false;
        }

        AZStd::vector<AZ::IO::Path> configFiles = AzToolsFramework::AssetUtils::GetConfigFiles(absoluteSystemRoot.toUtf8().constData(),
            projectPath.toUtf8().constData(),
            addPlatformConfigs, addGemsConfigs && !noGemScanFolders, settingsRegistry);

        // First Merge all Engine, Gem and Project specific AssetProcessor*Config.setreg/.inifiles
        for (const AZ::IO::Path& configFile : configFiles)
        {
            if (AZ::IO::SystemFile::Exists(configFile.c_str()))
            {
                MergeConfigFileToSettingsRegistry(*settingsRegistry, configFile);
            }
        }

        // Merge the Command Line to the Settings Registry after merging the AssetProcessor*Config.setreg/ini files
        // to allow the command line to override the settings
    #if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        if (commandLine)
        {
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*settingsRegistry, *commandLine, {});
        }
    #endif

        // first, read the platform informations.
        ReadPlatformInfosFromSettingsRegistry();

        // now read which platforms are currently enabled - this may alter the platform infos array and eradicate
        // the ones that are not suitable and currently enabled, leaving only the ones enabled either on command line
        // or config files.
        // the command line can always takes precedence - but can only turn on and off platforms, it cannot describe them.

        PopulateEnabledPlatforms();

        FinalizeEnabledPlatforms();

        if(!m_enabledPlatforms.empty())
        {
            // Add the common platform if we have some other platforms enabled.  For now, this is only intended for intermediate assets
            // So we don't want to enable it unless at least one actual platform is available, to avoid hiding an error state of no real platforms being active
            EnableCommonPlatform();
        }

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
                    false,
                    true,
                    platforms,
                    aznumeric_caster(idx),
                    /*scanFolderId*/ 0,
                    true));
            }
        }

        // Then read recognizers (which depend on platforms)
        if (!ReadRecognizersFromSettingsRegistry(absoluteAssetRoot, noConfigScanFolders, scanFolderPatterns))
        {
            if (m_fatalError.empty())
            {
                m_fatalError = "Unable to read recognizers specified in the configuration files during load.  Please check the Asset Processor platform ini files for errors.";
            }
            return IsValid();
        }

        if(!m_scanFolders.empty())
        {
            // Enable the intermediate scanfolder if we have some other scanfolders.  Since this is hardcoded we don't want to hide an error state
            // where no other scanfolders are enabled besides this one.  It wouldn't make sense for the intermediate scanfolder to be the only enabled scanfolder
            AddIntermediateScanFolder();
        }

        if (!noGemScanFolders && addGemsConfigs)
        {
            if (settingsRegistry == nullptr || !AzFramework::GetGemsInfo(m_gemInfoList, *settingsRegistry))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to Get Gems Info for the project (%s).", projectPath.toUtf8().constData());
                return false;
            }

            // now add all the scan folders of gems.
            AddGemScanFolders(m_gemInfoList);
        }
        // Then read metadata (which depends on scan folders)
        ReadMetaDataFromSettingsRegistry();

        // at this point there should be at least some watch folders besides gems.
        if (m_scanFolders.empty())
        {
            m_fatalError = "Unable to find any scan folders specified in the configuration files during load.  Please check the Asset Processor platform ini files for errors.";
            return IsValid();
        }

        return IsValid();
    }

    void PlatformConfiguration::PopulateEnabledPlatforms()
    {
        // if there are no platform informations inside the ini file, there's no point in proceeding
        // since we are unaware of the existence of the platform at all
        if (m_enabledPlatforms.empty())
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "There are no \"%s/Platform xxxxxx\" entries present in the settings registry. We cannot proceed.",
                AssetProcessorSettingsKey);
            return;
        }

        // the command line can always takes precedence - but can only turn on and off platforms, it cannot describe them.
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
                    AZStd::string platformOverride{ platform.toUtf8().data() };
                    if (auto foundIt = AZStd::find(m_tempEnabledPlatforms.begin(), m_tempEnabledPlatforms.end(), platformOverride);
                        foundIt == m_tempEnabledPlatforms.end())
                    {
                        m_tempEnabledPlatforms.push_back(AZStd::move(platformOverride));
                    }
                }
            }

            return; // command line wins!
        }
        // command line isn't active, read from settings registry instead.
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, R"(Global Settings Registry is not available, unable to read the "%s/Platforms")"
                " settings paths", AssetProcessorSettingsKey);
            return;
        }
        AZStd::vector<AZStd::string> enabledPlatforms;
        AzToolsFramework::AssetUtils::ReadEnabledPlatformsFromSettingsRegistry(*settingsRegistry, enabledPlatforms);

        m_tempEnabledPlatforms.insert(m_tempEnabledPlatforms.end(), AZStd::make_move_iterator(enabledPlatforms.begin()),
            AZStd::make_move_iterator(enabledPlatforms.end()));
    }

    void PlatformConfiguration::FinalizeEnabledPlatforms()
    {
#if defined(AZ_ENABLE_TRACING)
        // verify command line platforms are valid:
        for (const auto& enabledPlatformFromConfigs : m_tempEnabledPlatforms)
        {
            bool found = false;
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : m_enabledPlatforms)
            {
                if (platformInfo.m_identifier == enabledPlatformFromConfigs)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {

                m_fatalError = AZStd::string::format(R"(The list of enabled platforms in the settings registry does not contain platform "%s")"
                    " entries - check command line and settings registry files for errors!", enabledPlatformFromConfigs.c_str());
                return;
            }
        }
#endif // defined(AZ_ENABLE_TRACING)

        // over here, we want to eliminate any platforms in the m_enabledPlatforms array that are not in the m_tempEnabledPlatforms
        for (int enabledPlatformIdx = static_cast<int>(m_enabledPlatforms.size() - 1); enabledPlatformIdx >= 0; --enabledPlatformIdx)
        {
            const AssetBuilderSDK::PlatformInfo& platformInfo = m_enabledPlatforms[enabledPlatformIdx];

            if (auto foundIt = AZStd::find(m_tempEnabledPlatforms.begin(), m_tempEnabledPlatforms.end(), platformInfo.m_identifier);
                foundIt == m_tempEnabledPlatforms.end())
            {
                m_enabledPlatforms.erase(m_enabledPlatforms.cbegin() + enabledPlatformIdx);
            }
        }

        if (m_enabledPlatforms.empty())
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "There are no \"%s/Platform xxxxxx\" entry present in the settings registry. We cannot proceed.",
                AssetProcessorSettingsKey);
            return;
        }
        m_tempEnabledPlatforms.clear();
    }


    void PlatformConfiguration::ReadPlatformInfosFromSettingsRegistry()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, R"(Global Settings Registry is not available, unable to read the "%s/Platform *")"
                " settings paths", AssetProcessorSettingsKey);
            return;
        }
        PlatformsInfoVisitor visitor;
        settingsRegistry->Visit(visitor, AssetProcessorSettingsKey);
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : visitor.m_platformInfos)
        {
            EnablePlatform(platformInfo, true);
        }
    }

    void PlatformConfiguration::ReadEnabledPlatformsFromSettingsRegistry()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, R"(Global Settings Registry is not available, unable to read the "%s/Platforms")"
                " settings paths", AssetProcessorSettingsKey);
            return;
        }
        AzToolsFramework::AssetUtils::ReadEnabledPlatformsFromSettingsRegistry(*settingsRegistry, m_tempEnabledPlatforms);
    }

    void PlatformConfiguration::PopulatePlatformsForScanFolder(AZStd::vector<AssetBuilderSDK::PlatformInfo>& platformsList, QStringList includeTagsList, QStringList excludeTagsList)
    {
        if (includeTagsList.isEmpty())
        {
            // Add all enabled platforms
            for (const AssetBuilderSDK::PlatformInfo& platform : m_enabledPlatforms)
            {
                if(platform.m_identifier == AssetBuilderSDK::CommonPlatformName)
                {
                    // The common platform is not included in any scanfolder to avoid builders by-default producing jobs for it
                    continue;
                }

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
                    if(platform.m_identifier == AssetBuilderSDK::CommonPlatformName)
                    {
                        // The common platform is not included in any scanfolder to avoid builders by-default producing jobs for it
                        continue;
                    }

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

    void PlatformConfiguration::CacheIntermediateAssetsScanFolderId() const
    {
        for (const auto& scanfolder : m_scanFolders)
        {
            if (scanfolder.GetPortableKey() == AssetProcessor::IntermediateAssetsFolderName)
            {
                m_intermediateAssetScanFolderId = scanfolder.ScanFolderID();
                return;
            }
        }

        AZ_Error(
            "PlatformConfiguration", false,
            "CacheIntermediateAssetsScanFolderId: Failed to find Intermediate Assets folder in scanfolder list");
    }

    AZStd::optional<AZ::s64> PlatformConfiguration::GetIntermediateAssetsScanFolderId() const
    {
        if (m_intermediateAssetScanFolderId >= 0)
        {
            return m_intermediateAssetScanFolderId;
        }

        return AZStd::nullopt;
    }

    // used to save our the AssetCacheServer settings to a remote location
    struct AssetCacheServerMatcher
    {
        AZ_CLASS_ALLOCATOR(AssetCacheServerMatcher, AZ::SystemAllocator);
        AZ_TYPE_INFO(AssetCacheServerMatcher, "{329A59C9-755E-4FA9-AADB-05C50AC62FD5}");

        static void Reflect(AZ::SerializeContext* serializeContext)
        {
            serializeContext->Class<AssetCacheServerMatcher>()->Version(0)
                ->Field("name", &AssetCacheServerMatcher::m_name)
                ->Field("glob", &AssetCacheServerMatcher::m_glob)
                ->Field("pattern", &AssetCacheServerMatcher::m_pattern)
                ->Field("productAssetType", &AssetCacheServerMatcher::m_productAssetType)
                ->Field("checkServer", &AssetCacheServerMatcher::m_checkServer);

            serializeContext->RegisterGenericType<AZStd::unordered_map<AZStd::string, AssetCacheServerMatcher>>();
        }

        AZStd::string m_name;
        AZStd::string m_glob;
        AZStd::string m_pattern;
        AZ::Uuid m_productAssetType = AZ::Uuid::CreateNull();
        bool m_checkServer = false;
    };

    bool PlatformConfiguration::ConvertToJson(const RecognizerContainer& recognizerContainer, AZStd::string& jsonText)
    {
        AZStd::unordered_map<AZStd::string, AssetCacheServerMatcher> assetCacheServerMatcherMap;

        for (const auto& recognizer : recognizerContainer)
        {
            AssetCacheServerMatcher matcher;
            matcher.m_name = recognizer.first;
            matcher.m_checkServer = recognizer.second.m_checkServer;
            matcher.m_productAssetType = recognizer.second.m_productAssetType;

            if (recognizer.second.m_patternMatcher.GetBuilderPattern().m_type == AssetBuilderSDK::AssetBuilderPattern::Wildcard)
            {
                matcher.m_glob = recognizer.second.m_patternMatcher.GetBuilderPattern().m_pattern;
            }
            else if (recognizer.second.m_patternMatcher.GetBuilderPattern().m_type == AssetBuilderSDK::AssetBuilderPattern::Regex)
            {
                matcher.m_pattern = recognizer.second.m_patternMatcher.GetBuilderPattern().m_pattern;
            }
            assetCacheServerMatcherMap.insert({"ACS " + matcher.m_name, matcher});
        }

        AZ::JsonSerializerSettings settings;
        AZ::ComponentApplicationBus::BroadcastResult(settings.m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        settings.m_registrationContext = nullptr;

        rapidjson::Document jsonDocument;
        auto jsonResult = AZ::JsonSerialization::Store(jsonDocument, jsonDocument.GetAllocator(), assetCacheServerMatcherMap, settings);
        if (jsonResult.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            return false;
        }

        auto saveToFileOutcome = AZ::JsonSerializationUtils::WriteJsonString(jsonDocument, jsonText);
        return saveToFileOutcome.IsSuccess();
    }

    bool PlatformConfiguration::ConvertFromJson(const AZStd::string& jsonText, RecognizerContainer& recognizerContainer)
    {
        rapidjson::Document assetCacheServerMatcherDoc;
        assetCacheServerMatcherDoc.Parse(jsonText.c_str());
        if (assetCacheServerMatcherDoc.HasParseError())
        {
            return false;
        }

        AZ::JsonDeserializerSettings settings;
        AZ::ComponentApplicationBus::BroadcastResult(settings.m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        settings.m_registrationContext = nullptr;

        AZStd::unordered_map<AZStd::string, AssetCacheServerMatcher> assetCacheServerMatcherMap;
        auto resultCode = AZ::JsonSerialization::Load(assetCacheServerMatcherMap, assetCacheServerMatcherDoc, settings);
        if (!resultCode.HasDoneWork())
        {
            return false;
        }

        recognizerContainer.clear();
        for (const auto& matcher : assetCacheServerMatcherMap)
        {
            AssetRecognizer assetRecognizer;
            assetRecognizer.m_checkServer = matcher.second.m_checkServer;
            assetRecognizer.m_name = matcher.second.m_name;
            assetRecognizer.m_productAssetType = matcher.second.m_productAssetType;

            if (!matcher.second.m_glob.empty())
            {
                assetRecognizer.m_patternMatcher = { matcher.second.m_glob , AssetBuilderSDK::AssetBuilderPattern::Wildcard };
            }
            else if (!matcher.second.m_pattern.empty())
            {
                assetRecognizer.m_patternMatcher = { matcher.second.m_pattern , AssetBuilderSDK::AssetBuilderPattern::Regex };
            }
            recognizerContainer.insert({ "ACS " + matcher.second.m_name, assetRecognizer });
        }

        return !recognizerContainer.empty();
    }

    void PlatformConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            AssetCacheServerMatcher::Reflect(serializeContext);
        }
    }

    bool PlatformConfiguration::ReadRecognizersFromSettingsRegistry(const QString& assetRoot, bool skipScanFolders, QStringList scanFolderPatterns)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Global Settings Registry is not set."
                " Unable to read recognizers Asset Processor Settings");
            return false;
        }

        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        AZ::IO::FixedMaxPathString projectName = AZ::Utils::GetProjectName();
        AZ::IO::FixedMaxPathString executableDirectory = AZ::Utils::GetExecutableDirectory();

        AZ::IO::FixedMaxPath engineRoot(AZ::IO::PosixPathSeparator);
        settingsRegistry->Get(engineRoot.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        engineRoot = engineRoot.LexicallyNormal(); // Normalize the path to use posix slashes

        AZ::s64 jobCount = m_minJobs;
        if (settingsRegistry->Get(jobCount, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorSettingsKey) + "/Jobs/minJobs"))
        {
            m_minJobs = aznumeric_cast<int>(jobCount);
        }

        jobCount = m_maxJobs;
        if (settingsRegistry->Get(jobCount, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorSettingsKey) + "/Jobs/maxJobs"))
        {
            m_maxJobs = aznumeric_cast<int>(jobCount);
        }

        if (!skipScanFolders)
        {
            AZStd::unordered_map<AZStd::string, AZ::IO::Path> gemNameToPathMap;
            auto MakeGemNameToPathMap = [&gemNameToPathMap, &projectPath, &engineRoot]
            (AZStd::string_view gemName, AZ::IO::PathView gemPath)
            {
                AZ::IO::FixedMaxPath gemAbsPath = gemPath;
                if (gemPath.IsRelative())
                {
                    gemAbsPath = projectPath / gemPath;
                    if (!AZ::IO::SystemFile::Exists(gemAbsPath.c_str()))
                    {
                        gemAbsPath = engineRoot / gemPath;
                    }

                    // convert the relative path to an absolute path
                    if (!AZ::IO::SystemFile::Exists(gemAbsPath.c_str()))
                    {
                        if (auto gemAbsPathOpt = AZ::Utils::ConvertToAbsolutePath(gemPath.Native());
                            gemAbsPathOpt.has_value())
                        {
                            gemAbsPath = AZStd::move(*gemAbsPathOpt);
                        }
                    }
                }
                if (AZ::IO::SystemFile::Exists(gemAbsPath.c_str()))
                {
                    gemNameToPathMap.try_emplace(AZStd::string::format("@GEMROOT:%.*s@", AZ_STRING_ARG(gemName)), gemAbsPath.AsPosix());
                }
            };

            AZ::SettingsRegistryMergeUtils::VisitActiveGems(*settingsRegistry, MakeGemNameToPathMap);
            ScanFolderVisitor visitor;
            settingsRegistry->Visit(visitor, AssetProcessorSettingsKey);
            for (auto& scanFolderEntry : visitor.m_scanFolderInfos)
            {
                if (scanFolderEntry.m_watchPath.empty())
                {
                    continue;
                }

                auto scanFolderMatch = [watchFolderQt = QString::fromUtf8(scanFolderEntry.m_watchPath.c_str(),
                    aznumeric_cast<int>(scanFolderEntry.m_watchPath.Native().size()))](const QString& scanFolderPattern)
                {
                    QRegExp nameMatch(scanFolderPattern, Qt::CaseInsensitive, QRegExp::Wildcard);
                    return nameMatch.exactMatch(watchFolderQt);
                };
                if (!scanFolderPatterns.empty() && AZStd::none_of(scanFolderPatterns.begin(), scanFolderPatterns.end(), scanFolderMatch))
                {
                    // Continue to the next iteration if the watch folder doesn't match any of the supplied patterns
                    continue;
                }

                // Substitute macro values into the watch path and the scan folder display name
                AZStd::string assetRootPath = assetRoot.toUtf8().data();
                AZ::StringFunc::Replace(scanFolderEntry.m_watchPath.Native(), "@ROOT@", assetRootPath.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_watchPath.Native(), "@PROJECTROOT@", projectPath.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_watchPath.Native(), "@ENGINEROOT@", engineRoot.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_watchPath.Native(), "@EXEFOLDER@", executableDirectory.c_str());
                // Normalize path make sure it is using posix slashes
                scanFolderEntry.m_watchPath = scanFolderEntry.m_watchPath.LexicallyNormal();

                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@ROOT@", assetRootPath.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@PROJECTROOT@", projectPath.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@PROJECTNAME@", projectName.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@ENGINEROOT@", engineRoot.c_str());

                // Substitute gem root path if applicable
                if (scanFolderEntry.m_watchPath.Native().contains("@GEMROOT")
                    || scanFolderEntry.m_scanFolderDisplayName.contains("@GEMROOT"))
                {
                    for (const auto& [gemAlias, gemPath] : gemNameToPathMap)
                    {
                        AZ::StringFunc::Replace(scanFolderEntry.m_watchPath.Native(), gemAlias.c_str(), gemPath.c_str());
                        AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, gemAlias.c_str(), gemPath.c_str());
                    }
                }

                QStringList includeIdentifiers;
                for (AZStd::string_view includeIdentifier : scanFolderEntry.m_includeIdentifiers)
                {
                    includeIdentifiers.push_back(QString::fromUtf8(includeIdentifier.data(), aznumeric_cast<int>(includeIdentifier.size())));
                }
                QStringList excludeIdentifiers;
                for (AZStd::string_view excludeIdentifier : scanFolderEntry.m_excludeIdentifiers)
                {
                    excludeIdentifiers.push_back(QString::fromUtf8(excludeIdentifier.data(), aznumeric_cast<int>(excludeIdentifier.size())));
                }

                AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
                PopulatePlatformsForScanFolder(platforms, includeIdentifiers, excludeIdentifiers);

                const bool isEngineRoot = scanFolderEntry.m_watchPath == engineRoot;
                // If the scan folder happens to be the engine root, it is not recursive
                scanFolderEntry.m_isRecursive = scanFolderEntry.m_isRecursive && !isEngineRoot;

                // New assets can be saved in any scan folder defined except for the engine root.
                const bool canSaveNewAssets = !isEngineRoot;

                QString watchFolderPath = QString::fromUtf8(scanFolderEntry.m_watchPath.c_str(), static_cast<int>(scanFolderEntry.m_watchPath.Native().size()));
                watchFolderPath = AssetUtilities::NormalizeDirectoryPath(watchFolderPath);
                AddScanFolder(ScanFolderInfo(
                    watchFolderPath,
                    QString::fromUtf8(scanFolderEntry.m_scanFolderDisplayName.c_str(), aznumeric_cast<int>(scanFolderEntry.m_scanFolderDisplayName.size())),
                    QString::fromUtf8(scanFolderEntry.m_scanFolderIdentifier.c_str(), aznumeric_cast<int>(scanFolderEntry.m_scanFolderIdentifier.size())),
                    isEngineRoot,
                    scanFolderEntry.m_isRecursive,
                    platforms,
                    scanFolderEntry.m_scanOrder,
                    0,
                    canSaveNewAssets
                ));
            }
        }

        ExcludeVisitor excludeVisitor;
        settingsRegistry->Visit(excludeVisitor, AssetProcessorSettingsKey);
        for (auto&& excludeRecognizer: excludeVisitor.m_excludeAssetRecognizers)
        {
            m_excludeAssetRecognizers[excludeRecognizer.m_name] = AZStd::move(excludeRecognizer);
        }

        SimpleJobVisitor simpleJobVisitor(m_enabledPlatforms);
        settingsRegistry->Visit(simpleJobVisitor, AssetProcessorSettingsKey);
        for (auto&& sjRecognizer : simpleJobVisitor.m_assetRecognizers)
        {
            if (!sjRecognizer.m_recognizer.m_platformSpecs.empty() && !sjRecognizer.m_ignore)
            {
                m_assetRecognizers[sjRecognizer.m_recognizer.m_name] = AZStd::move(sjRecognizer.m_recognizer);
            }
        }

        ACSVisitor acsVistor;
        settingsRegistry->Visit(acsVistor, AssetProcessorServerKey);
        for (auto&& acsRecognizer : acsVistor.m_assetRecognizers)
        {
            m_assetCacheServerRecognizers[acsRecognizer.m_name] = AZStd::move(acsRecognizer);
        }

        return true;
    }

    void PlatformConfiguration::ReadMetaDataFromSettingsRegistry()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Global Settings Registry is not set."
                " MetaDataTypes entries cannot be read from Asset Processor Settings");
            return;
        }

        MetaDataTypesVisitor visitor;
        settingsRegistry->Visit(visitor, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorSettingsKey) + "/MetaDataTypes");

        using namespace AzToolsFramework::AssetUtils;
        AZStd::vector<AZStd::string> supportedFileExtensions;
        AssetImporterPathsVisitor assetImporterVisitor{ settingsRegistry, supportedFileExtensions };
        settingsRegistry->Visit(assetImporterVisitor, AZ::SettingsRegistryInterface::FixedValueString(AssetImporterSettingsKey) + "/" + AssetImporterSupportedFileTypeKey);

        for (auto& entry : assetImporterVisitor.m_supportedFileExtensions)
        {
            visitor.m_metaDataTypes.push_back({ AZStd::string::format("%s.assetinfo", entry.c_str()), entry });
        }

        AddMetaDataType(AzToolsFramework::MetadataManager::MetadataFileExtensionNoDot, "");

        for (const auto& metaDataType : visitor.m_metaDataTypes)
        {
            QString fileType = AssetUtilities::NormalizeFilePath(QString::fromUtf8(metaDataType.m_fileType.c_str(),
                aznumeric_cast<int>(metaDataType.m_fileType.size())));
            auto extensionType = QString::fromUtf8(metaDataType.m_extensionType.c_str(),
                aznumeric_cast<int>(metaDataType.m_extensionType.size()));

            AddMetaDataType(fileType, extensionType);

            // Check if the Metadata 'file type' is a real file
            QString fullPath = FindFirstMatchingFile(fileType);
            if (!fullPath.isEmpty())
            {
                m_metaDataRealFiles.insert(fileType.toLower());
            }
        }
    }

    int PlatformConfiguration::GetProjectScanFolderOrder() const
    {
        auto mainProjectScanFolder = FindScanFolder([](const AssetProcessor::ScanFolderInfo& scanFolderInfo) -> bool
            {
                return scanFolderInfo.GetPortableKey() == ProjectScanFolderKey;
            });
        if (mainProjectScanFolder)
        {
            return mainProjectScanFolder->GetOrder();
        }
        return 0;
    }

    bool PlatformConfiguration::MergeConfigFileToSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry, const AZ::IO::PathView& configFile)
    {
        // If the config file is a settings registry file use the SettingsRegistryInterface MergeSettingsFile function
        // otherwise use the SettingsRegistryMergeUtils MergeSettingsToRegistry_ConfigFile function to merge an INI-style
        // file to the settings registry
        if (configFile.Extension() == ".setreg")
        {
            return static_cast<bool>(settingsRegistry.MergeSettingsFile(configFile.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        }

        AZ::SettingsRegistryMergeUtils::ConfigParserSettings configParserSettings;
        configParserSettings.m_registryRootPointerPath = AssetProcessorSettingsKey;
        return AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ConfigFile(settingsRegistry, configFile.Native(), configParserSettings);
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
        for (const auto& assetRecognizer : m_assetRecognizers)
        {
            const AssetRecognizer& recognizer = assetRecognizer.second;
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

    AZStd::vector<AzFramework::GemInfo> PlatformConfiguration::GetGemsInformation() const
    {
        return m_gemInfoList;
    }

    AssetProcessor::ScanFolderInfo& PlatformConfiguration::GetScanFolderAt(int index)
    {
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < m_scanFolders.size());
        return m_scanFolders[index];
    }

    const AssetProcessor::ScanFolderInfo& PlatformConfiguration::GetScanFolderAt(int index) const
    {
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < m_scanFolders.size());
        return m_scanFolders[index];
    }

    const AssetProcessor::ScanFolderInfo* PlatformConfiguration::FindScanFolder(
        AZStd::function<bool(const AssetProcessor::ScanFolderInfo&)> predicate) const
    {
        auto resultIt = AZStd::ranges::find_if(m_scanFolders, predicate);
        return resultIt != m_scanFolders.end() ? &(*resultIt) : nullptr;
    }

    const AssetProcessor::ScanFolderInfo* PlatformConfiguration::GetScanFolderById(AZ::s64 id) const
    {
        return FindScanFolder([id](const ScanFolderInfo& scanFolder)
            {
                return scanFolder.ScanFolderID() == id;
            });
    }

    const AZ::s64 PlatformConfiguration::GetIntermediateAssetScanFolderId() const
    {
        return m_intermediateAssetScanFolderId;
    }

    void PlatformConfiguration::AddScanFolder(const AssetProcessor::ScanFolderInfo& source, bool isUnitTesting)
    {
        if (isUnitTesting)
        {
            //using a bool instead of using #define UNIT_TEST because the user can also run batch processing in unittest
            m_scanFolders.push_back(source);

            // since we're synthesizing folder adds, assign ascending folder ids if not provided.
            if (source.ScanFolderID() == 0)
            {
                m_scanFolders.back().SetScanFolderID(m_scanFolders.size() - 1);
            }
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
        m_assetRecognizers.insert({source.m_name, source});
    }

    void PlatformConfiguration::RemoveRecognizer(QString name)
    {
        auto found = m_assetRecognizers.find(name.toUtf8().data());
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

    bool PlatformConfiguration::ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const
    {
        const ScanFolderInfo* info = GetScanFolderForFile(fullFileName);

        if (info)
        {
            scanFolderName = info->ScanPath();
            scanFolderName.replace(AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

            return ConvertToRelativePath(fullFileName, info, databaseSourceName);
        }
        // did not find it.
        return false;
    }

    bool PlatformConfiguration::ConvertToRelativePath(const QString& fullFileName, const ScanFolderInfo* scanFolderInfo, QString& databaseSourceName)
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

        databaseSourceName = relPath;

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

    // This function is one of the most frequently called ones in the entire application
    // and is invoked several times per file.  It can frequently become a bottleneck, so
    // avoid doing expensive operations here, especially memory or IO operations.
    QString PlatformConfiguration::FindFirstMatchingFile(QString relativeName, bool skipIntermediateScanFolder, const ScanFolderInfo** outScanFolderInfo) const
    {
        if (relativeName.isEmpty())
        {
            return QString();
        }

        auto* fileStateInterface = AZ::Interface<AssetProcessor::IFileStateRequests>::Get();

        // Only compute the intermediate assets folder path if we are going to search for and skip it.

        if (skipIntermediateScanFolder)
        {
            if (m_intermediateAssetScanFolderId == -1)
            {
                CacheIntermediateAssetsScanFolderId();
            }
        }

        QString absolutePath; // avoid allocating memory repeatedly here by reusing absolutePath each scan folder.
        absolutePath.reserve(AZ_MAX_PATH_LEN);

        QFileInfo details(relativeName); // note that this does not actually hit the actual storage medium until you query something
        bool isAbsolute = details.isAbsolute(); // note that this looks at the file name string only, it does not hit storage.

        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            const AssetProcessor::ScanFolderInfo& scanFolderInfo = m_scanFolders[pathIdx];

            if ((skipIntermediateScanFolder) && (scanFolderInfo.ScanFolderID() == m_intermediateAssetScanFolderId))
            {
                // There's only 1 intermediate assets folder, if we've skipped it, theres no point continuing to check every folder afterwards
                skipIntermediateScanFolder = false;
                continue;
            }

            if ((!scanFolderInfo.RecurseSubFolders()) && (relativeName.contains('/')))
            {
                // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
                continue;
            }

            if (isAbsolute)
            {
                if (!relativeName.startsWith(scanFolderInfo.ScanPath()))
                {
                    continue; // its not this scanfolder.
                }
                absolutePath = relativeName;
            }
            else
            {
                // scanfolders are always absolute paths and already normalized.  We can just concatenate.
                // Do so with minimal allocation by using resize/append, instead of operator+
                absolutePath.resize(0);
                absolutePath.append(scanFolderInfo.ScanPath());
                absolutePath.append('/');
                absolutePath.append(relativeName);
            }
            AssetProcessor::FileStateInfo fileStateInfo;

            if (fileStateInterface)
            {
                if (fileStateInterface->GetFileInfo(absolutePath, &fileStateInfo))
                {
                    if (outScanFolderInfo)
                    {
                        *outScanFolderInfo = &scanFolderInfo;
                    }

                    return AssetUtilities::NormalizeFilePath(fileStateInfo.m_absolutePath);
                }
            }
        }
        return QString();
    }

    QStringList PlatformConfiguration::FindWildcardMatches(
        const QString& sourceFolder,
        QString relativeName,
        bool includeFolders,
        bool recursiveSearch) const
    {
        if (relativeName.isEmpty())
        {
            return QStringList();
        }

        QDir sourceFolderDir(sourceFolder);

        QString posixRelativeName = QDir::fromNativeSeparators(relativeName);

        QStringList returnList;
        QRegExp nameMatch{ posixRelativeName, Qt::CaseInsensitive, QRegExp::Wildcard };
        QDirIterator dirIterator(
            sourceFolderDir.path(), QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot,
            recursiveSearch ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        QStringList files;
        while (dirIterator.hasNext())
        {
            dirIterator.next();
            if (!includeFolders && !dirIterator.fileInfo().isFile())
            {
                continue;
            }
            QString pathMatch{ sourceFolderDir.relativeFilePath(dirIterator.filePath()) };
            if (nameMatch.exactMatch(pathMatch))
            {
                returnList.append(QDir::fromNativeSeparators(dirIterator.filePath()));
            }
        }
        return returnList;
    }

    QStringList PlatformConfiguration::FindWildcardMatches(
        const QString& sourceFolder,
        QString relativeName,
        const AZStd::unordered_set<AZStd::string>& excludedFolders,
        bool includeFolders,
        bool recursiveSearch) const
    {
        if (relativeName.isEmpty())
        {
            return QStringList();
        }

        QDir sourceFolderDir(sourceFolder);

        QString posixRelativeName = QDir::fromNativeSeparators(relativeName);

        QStringList returnList;
        QRegExp nameMatch{ posixRelativeName, Qt::CaseInsensitive, QRegExp::Wildcard };
        AZStd::stack<QString> dirs;
        dirs.push(sourceFolderDir.absolutePath());

        while (!dirs.empty())
        {
            QString absolutePath = dirs.top();
            dirs.pop();

            if (excludedFolders.contains(absolutePath.toUtf8().constData()))
            {
                continue;
            }

            QDirIterator dirIterator(absolutePath, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot);

            while (dirIterator.hasNext())
            {
                dirIterator.next();

                if (!dirIterator.fileInfo().isFile())
                {
                    if (recursiveSearch)
                    {
                        dirs.push(dirIterator.filePath());
                    }

                    if (!includeFolders)
                    {
                        continue;
                    }
                }

                QString pathMatch{ sourceFolderDir.relativeFilePath(dirIterator.filePath()) };
                if (nameMatch.exactMatch(pathMatch))
                {
                    returnList.append(QDir::fromNativeSeparators(dirIterator.filePath()));
                }
            }
        }

        return returnList;
    }

    const AssetProcessor::ScanFolderInfo* PlatformConfiguration::GetScanFolderForFile(const QString& fullFileName) const
    {
        QString normalized = AssetUtilities::NormalizeFilePath(fullFileName);

        // first, check for an EXACT match.  If there's an exact match, this must be the one returned!
        // this is to catch the case where the actual path of a scan folder is fed in to this.

        // because exact matches are preferred over less exact, we first check exact matches:
        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            QString scanFolderName = m_scanFolders[pathIdx].ScanPath();
            if (scanFolderName.length() == normalized.length())
            {
                if (normalized.compare(scanFolderName, Qt::CaseInsensitive) == 0)
                {
                    // if its an exact match, we're basically done
                    return &m_scanFolders[pathIdx];
                }
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
        return FindScanFolder([&scanFolderPath](const AssetProcessor::ScanFolderInfo& scanFolder)
            {
                return scanFolder.ScanPath() == scanFolderPath;
            });
    }

    int PlatformConfiguration::GetMinJobs() const
    {
        return m_minJobs;
    }

    int PlatformConfiguration::GetMaxJobs() const
    {
        return m_maxJobs;
    }

    void PlatformConfiguration::EnableCommonPlatform()
    {
        EnablePlatform(AssetBuilderSDK::PlatformInfo{ AssetBuilderSDK::CommonPlatformName, AZStd::unordered_set<AZStd::string>{ "common" } });
    }

    void PlatformConfiguration::AddIntermediateScanFolder()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::SettingsRegistryInterface::FixedValueString cacheRootFolder;
        settingsRegistry->Get(cacheRootFolder, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);

        AZ::IO::Path scanfolderPath = cacheRootFolder.c_str();
        scanfolderPath /= AssetProcessor::IntermediateAssetsFolderName;

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        PopulatePlatformsForScanFolder(platforms);

        scanfolderPath = AssetUtilities::NormalizeDirectoryPath(QString::fromUtf8(scanfolderPath.c_str())).toUtf8().constData();

        // By default the project scanfolder is recursive with an order of 0
        // The intermediate assets folder needs to be higher priority since its a subfolder (otherwise GetScanFolderForFile won't pick the right scanfolder)
        constexpr int order = -1;

        AddScanFolder(ScanFolderInfo{
            scanfolderPath.c_str(),
            AssetProcessor::IntermediateAssetsFolderName,
            AssetProcessor::IntermediateAssetsFolderName,
            false,
            true,
            platforms,
            order
        });
    }

    void PlatformConfiguration::AddGemScanFolders(const AZStd::vector<AzFramework::GemInfo>& gemInfoList)
    {
        // If the gem is project-relative, make adjustments to its priority order based on registry settings:
        // /Amazon/AssetProcessor/Settings/GemScanFolderStartingPriorityOrder
        // /Amazon/AssetProcessor/Settings/ProjectRelativeGemsScanFolderPriority
        // See <o3de-root>/Registry/AssetProcessorPlatformConfig.setreg for more information.

        AZ::s64 gemStartingOrder = 100;
        AZStd::string projectGemPrioritySetting{};
        const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        int pathCount = 0;

        const int projectScanOrder = GetProjectScanFolderOrder();

        if (auto const settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(gemStartingOrder,
                AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorSettingsKey) + GemStartingPriorityOrderKey);

            settingsRegistry->Get(projectGemPrioritySetting,
                AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorSettingsKey) + ProjectRelativeGemPriorityKey);
            AZStd::to_lower(projectGemPrioritySetting.begin(), projectGemPrioritySetting.end());
        }

        auto GetGemFolderOrder = [&](bool isProjectRelativeGem) -> int
        {
            ++pathCount;
            int currentGemOrder = aznumeric_cast<int>(gemStartingOrder) + pathCount;
            if (isProjectRelativeGem)
            {
                if (projectGemPrioritySetting == "higher")
                {
                    currentGemOrder = projectScanOrder - pathCount;
                }
                else if (projectGemPrioritySetting == "lower")
                {
                    currentGemOrder = projectScanOrder + pathCount;
                }
            }
            return currentGemOrder;
        };

        int gemOrder = aznumeric_cast<int>(gemStartingOrder);

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        PopulatePlatformsForScanFolder(platforms);

        for (const AzFramework::GemInfo& gemElement : gemInfoList)
        {
            for (size_t sourcePathIndex{}; sourcePathIndex < gemElement.m_absoluteSourcePaths.size(); ++sourcePathIndex)
            {
                const AZ::IO::Path& absoluteSourcePath = gemElement.m_absoluteSourcePaths[sourcePathIndex];
                QString gemAbsolutePath = QString::fromUtf8(absoluteSourcePath.c_str(), aznumeric_cast<int>(absoluteSourcePath.Native().size())); // this is an absolute path!

                const bool isProjectGem = absoluteSourcePath.IsRelativeTo(projectPath);

                // Append the index of the source path array element to make a unique portable key is created for each path of a gem
                AZ::Uuid gemNameUuid = AZ::Uuid::CreateName((gemElement.m_gemName + AZStd::to_string(sourcePathIndex)).c_str());
                QString gemNameAsUuid(gemNameUuid.ToFixedString().c_str());

                QDir gemDir(gemAbsolutePath);

                // The gems /Assets/ folders are always added to the watch list, we want the following params
                //      Watched folder: (absolute path to the gem /Assets/ folder) MUST BE CORRECT CASE
                //      Display name:   "Gems/GemName/Assets" // uppercase, for human eyes
                //      portable Key:   "gemassets-(UUID Of Gem)"
                //      Is Root:        False
                //      Recursive:      True
                QString gemFolder = gemDir.absoluteFilePath(AzFramework::GemInfo::GetGemAssetFolder());

                // note that we normalize this gem path with slashes so that there's nothing special about it compared to other scan folders
                gemFolder = AssetUtilities::NormalizeDirectoryPath(gemFolder);

                QString assetBrowserDisplayName = AzFramework::GemInfo::GetGemAssetFolder(); // Gems always use assets folder as their displayname...
                QString portableKey = QString("gemassets-%1").arg(gemNameAsUuid);
                bool isRoot = false;
                bool isRecursive = true;
                gemOrder = GetGemFolderOrder(isProjectGem);

                AZ_TracePrintf(AssetProcessor::DebugChannel, "Adding GEM assets folder for monitoring / scanning: %s.\n", gemFolder.toUtf8().data());
                AddScanFolder(ScanFolderInfo(
                    gemFolder,
                    assetBrowserDisplayName,
                    portableKey,
                    isRoot,
                    isRecursive,
                    platforms,
                    gemOrder,
                    /*scanFolderId*/ 0,
                    /*canSaveNewAssets*/ true)); // Users can create assets like slices in Gem asset folders.

                // Now add another scan folder on Gem/GemName/Registry...
                gemFolder = gemDir.absoluteFilePath(AzFramework::GemInfo::GetGemRegistryFolder());
                gemFolder = AssetUtilities::NormalizeDirectoryPath(gemFolder);

                assetBrowserDisplayName = AzFramework::GemInfo::GetGemRegistryFolder();
                portableKey = QString("gemregistry-%1").arg(gemNameAsUuid);
                gemOrder = GetGemFolderOrder(isProjectGem);

                AZ_TracePrintf(AssetProcessor::DebugChannel, "Adding GEM registry folder for monitoring / scanning: %s.\n", gemFolder.toUtf8().data());
                AddScanFolder(ScanFolderInfo(
                    gemFolder,
                    assetBrowserDisplayName,
                    portableKey,
                    isRoot,
                    isRecursive,
                    platforms,
                    gemOrder));
            }
        }
    }

    const RecognizerContainer& PlatformConfiguration::GetAssetRecognizerContainer() const
    {
        return m_assetRecognizers;
    }

    const RecognizerContainer& PlatformConfiguration::GetAssetCacheRecognizerContainer() const
    {
        return m_assetCacheServerRecognizers;
    }

    const ExcludeRecognizerContainer& PlatformConfiguration::GetExcludeAssetRecognizerContainer() const
    {
        return m_excludeAssetRecognizers;
    }

    bool PlatformConfiguration::AddAssetCacheRecognizerContainer(const RecognizerContainer& recognizerContainer)
    {
        bool addedEntries = false;
        for (const auto& recognizer : recognizerContainer)
        {
            auto entryIter = m_assetCacheServerRecognizers.find(recognizer.first);
            if (entryIter != m_assetCacheServerRecognizers.end())
            {
                m_assetCacheServerRecognizers.insert(recognizer);
                addedEntries = true;
            }
        }
        return addedEntries;
    }

    // AssetProcessor

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
        QString relPath, scanFolderName;
        if (ConvertToRelativePath(fileName, relPath, scanFolderName))
        {
            return IsFileExcludedRelPath(relPath);
        }

        return false;
    }

    bool AssetProcessor::PlatformConfiguration::IsFileExcludedRelPath(QString relPath) const
    {
        AZ::IO::FixedMaxPathString encoded = relPath.toUtf8().constData();
        for (const ExcludeAssetRecognizer& excludeRecognizer : m_excludeAssetRecognizers)
        {
            if (excludeRecognizer.m_patternMatcher.MatchesPath(encoded.c_str()))
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
                m_fatalError = "The configuration is invalid - no platforms appear to be enabled. Check to make sure that the AssetProcessorPlatformConfig.setreg file(s) are present and correct.";
            }
            else if (m_assetRecognizers.empty())
            {
                m_fatalError = "The configuration is invalid - no matching asset recognizers appear valid.  Check to make sure that the AssetProcessorPlatformConfig.setreg file(s) are present and correct.";
            }
            else if (m_scanFolders.empty())
            {
                m_fatalError = "The configuration is invalid - no scan folders defined.  Check to make sure that the AssetProcessorPlatformConfig.setreg file(s) are present and correct.";
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

