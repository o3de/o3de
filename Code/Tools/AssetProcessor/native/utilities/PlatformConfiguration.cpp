/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "native/utilities/PlatformConfiguration.h"
#include "native/AssetManager/FileStateCache.h"

#include <QDirIterator>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Gem/GemInfo.h>
#include <AzToolsFramework/Asset/AssetUtils.h>

namespace
{
    // the starting order in the file for gems.
    const int g_gemStartingOrder = 100;
}

namespace AssetProcessor
{

    void AssetImporterPathsVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view, AZ::SettingsRegistryInterface::Type,
        AZStd::string_view value)
    {
        auto found = value.find('.');
        if (found != AZStd::string::npos)
        {
            m_supportedFileExtensions.emplace_back(value.substr(found + 1));
        }
        else
        {
            m_supportedFileExtensions.emplace_back(value);
        }
    }

    struct PlatformsInfoVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        AZ::SettingsRegistryInterface::VisitResponse Traverse(AZStd::string_view jsonPath, AZStd::string_view valueName,
            AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type) override
        {
            constexpr AZStd::string_view PlatformInfoPrefix = "Platform ";
            switch (action)
            {
            case AZ::SettingsRegistryInterface::VisitAction::Begin:
            {
                // Only continue traversal if the path is exactly the AssetProcessorSettingsKey (which indicates the start of traversal)
                // or if a "Platform *" object and it's children are being traversed 
                if (jsonPath == AssetProcessorSettingsKey)
                {
                    return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                }
                if (valueName.starts_with(PlatformInfoPrefix))
                {
                    // Retrieve the platform name from the rest of valueName portion of the key "Platform (.*)"
                    AZStd::string platformIdentifier = valueName.substr(PlatformInfoPrefix.size());
                    // Lowercase the platformIdentifier and store it in the stack
                    AZStd::to_lower(platformIdentifier.begin(), platformIdentifier.end());

                    m_platformIdentifierStack.push(AZStd::move(platformIdentifier));
                }
            }
            break;
            case AZ::SettingsRegistryInterface::VisitAction::End:
            {
                if (valueName.starts_with(PlatformInfoPrefix))
                {
                    AZ_Assert(!m_platformIdentifierStack.empty(), "PlatformInfo stack should not be empty. More stack pops, than pushes");
                    m_platformIdentifierStack.pop();
                }
            }
            break;

            default:
                break;
            }

            return !m_platformIdentifierStack.empty() ? AZ::SettingsRegistryInterface::VisitResponse::Continue
                : AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override
        {
            if (m_platformIdentifierStack.empty())
            {
                return;
            }

            if (valueName == "tags")
            {
                AZStd::string_view platformIdentifier = m_platformIdentifierStack.top();
                AZStd::unordered_set<AZStd::string> platformTags;
                auto JoinTags = [&platformTags](AZStd::string_view token)
                {
                    AZStd::string cleanedTag{ token };
                    AZStd::to_lower(cleanedTag.begin(), cleanedTag.end());
                    platformTags.insert(AZStd::move(cleanedTag));
                };
                AZ::StringFunc::TokenizeVisitor(value, JoinTags, ',');
                m_platformInfos.emplace_back(platformIdentifier, platformTags);
            }
        }

        AZStd::vector<AssetBuilderSDK::PlatformInfo> m_platformInfos;
    private:
        AZStd::stack<AZStd::string> m_platformIdentifierStack;
    };

    struct MetaDataTypesVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override
        {
            m_metaDataTypes.push_back({ AZ::IO::PathView(valueName, AZ::IO::PosixPathSeparator).LexicallyNormal().String(), value });
        }

        struct MetaDataType
        {
            AZStd::string m_fileType;
            AZStd::string m_extensionType;
        };
        AZStd::vector<MetaDataType> m_metaDataTypes;
    };

    AZ::SettingsRegistryInterface::VisitResponse ScanFolderVisitor::Traverse(AZStd::string_view jsonPath, AZStd::string_view valueName,
        AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type)
    {
        constexpr AZStd::string_view ScanFolderInfoPrefix = "ScanFolder ";
        switch (action)
        {
        case AZ::SettingsRegistryInterface::VisitAction::Begin:
        {
            if (jsonPath == AssetProcessorSettingsKey)
            {
                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            }
            if (valueName.starts_with(ScanFolderInfoPrefix))
            {
                // Retrieve the ScanFolder identifier from "Scan Folder *"
                AZStd::string scanFolderDisplayName = valueName.substr(ScanFolderInfoPrefix.size());
                m_scanFolderStack.push(scanFolderDisplayName);

                ScanFolderInfo& scanFolderInfo = m_scanFolderInfos.emplace_back();
                scanFolderInfo.m_scanFolderIdentifier = scanFolderDisplayName;
                scanFolderInfo.m_scanFolderDisplayName = AZStd::move(scanFolderDisplayName);
            }
        }
        break;
        case AZ::SettingsRegistryInterface::VisitAction::End:
        {
            if (valueName.starts_with(ScanFolderInfoPrefix))
            {
                AZ_Assert(!m_scanFolderStack.empty(), "ScanFolder identifier stack should not be empty. More stack pops, than pushes");
                m_scanFolderStack.pop();
            }
        }
        break;

        default:
            break;
        }

        return !m_scanFolderStack.empty() ? AZ::SettingsRegistryInterface::VisitResponse::Continue
            : AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }
    void ScanFolderVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value)
    {
        // Check if a "ScanFolder *" element is being traversed
        if (m_scanFolderStack.empty())
        {
            return;
        }

        AZStd::string_view currentScanFolderIdentifier = m_scanFolderStack.top();

        // Find ScanFolder element being iterated over.
        // It should be the last element in ScanFolderInfo vector
        auto scanFolderEntryIt = AZStd::find_if(m_scanFolderInfos.rbegin(), m_scanFolderInfos.rend(),
            [&currentScanFolderIdentifier](const ScanFolderInfo& scanFolderInfo)
        {
            return scanFolderInfo.m_scanFolderIdentifier == currentScanFolderIdentifier;
        });
        if (scanFolderEntryIt == m_scanFolderInfos.rend())
        {
            return;
        }

        ScanFolderInfo& scanFolderEntry = *scanFolderEntryIt;
        if (valueName == "recursive")
        {
            scanFolderEntry.m_isRecursive = value != 0;
        }
        else if (valueName == "order")
        {
            scanFolderEntry.m_scanOrder = static_cast<int>(value);
        }
    }

    void ScanFolderVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
    {
        // Check if a "ScanFolder *" element is being traversed
        if (m_scanFolderStack.empty())
        {
            return;
        }

        AZStd::string_view currentScanFolderIdentifier = m_scanFolderStack.top();

        // Find ScanFolder element being iterated over.
        // It should be the last element in ScanFolderInfo vector
        auto scanFolderEntryIt = AZStd::find_if(m_scanFolderInfos.rbegin(), m_scanFolderInfos.rend(),
            [&currentScanFolderIdentifier](const ScanFolderInfo& scanFolderInfo)
        {
            return scanFolderInfo.m_scanFolderIdentifier == currentScanFolderIdentifier;
        });
        if (scanFolderEntryIt == m_scanFolderInfos.rend())
        {
            return;
        }

        ScanFolderInfo& scanFolderEntry = *scanFolderEntryIt;
        if (valueName == "watch")
        {
            scanFolderEntry.m_watchPath = value;
        }
        else if (valueName == "display" && !value.empty())
        {
            scanFolderEntry.m_scanFolderDisplayName = value;
        }
        else if (valueName == "include")
        {
            auto JoinTags = [&scanFolderEntry](AZStd::string_view token)
            {
                scanFolderEntry.m_includeIdentifiers.push_back(token);
            };
            AZ::StringFunc::TokenizeVisitor(value, JoinTags, ',');
        }
        else if (valueName == "exclude")
        {
            auto JoinTags = [&scanFolderEntry](AZStd::string_view token)
            {
                scanFolderEntry.m_excludeIdentifiers.push_back(token);
            };
            AZ::StringFunc::TokenizeVisitor(value, JoinTags, ',');
        }
    }

    AZ::SettingsRegistryInterface::VisitResponse ExcludeVisitor::Traverse(AZStd::string_view jsonPath, AZStd::string_view valueName,
        AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type)
    {
        constexpr AZStd::string_view ExcludeNamePrefix = "Exclude ";
        switch (action)
        {
        case AZ::SettingsRegistryInterface::VisitAction::Begin:
        {
            if (jsonPath == AssetProcessorSettingsKey)
            {
                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            }
            if (valueName.starts_with(ExcludeNamePrefix))
            {
                // Extract the substr that is part of the valueName "Exclude *"
                AZStd::string excludeName = valueName.substr(ExcludeNamePrefix.size());
                m_excludeNameStack.push(excludeName);

                ExcludeAssetRecognizer& excludeAssetRecognizer = m_excludeAssetRecognizers.emplace_back();
                excludeAssetRecognizer.m_name = QString::fromUtf8(excludeName.c_str(), aznumeric_cast<int>(excludeName.size()));
            }
        }
        break;
        case AZ::SettingsRegistryInterface::VisitAction::End:
        {
            if (valueName.starts_with(ExcludeNamePrefix))
            {
                AZ_Assert(!m_excludeNameStack.empty(), "Exclude stack should not be empty. More stack pops, than pushes");
                m_excludeNameStack.pop();
            }
        }
        break;

        default:
            break;
        }

        return !m_excludeNameStack.empty() ? AZ::SettingsRegistryInterface::VisitResponse::Continue
            : AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }

    void ExcludeVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
    {
        if (m_excludeNameStack.empty())
        {
            return;
        }

        AZStd::string_view excludeNameView = m_excludeNameStack.top();
        auto excludeName = QString::fromUtf8(excludeNameView.data(), aznumeric_cast<int>(excludeNameView.size()));

        // Find ScanFolder element being iterated over.
        // It should be the last element in ScanFolderInfo vector
        auto excludeAssetRecognizerEntryIt = AZStd::find_if(m_excludeAssetRecognizers.rbegin(), m_excludeAssetRecognizers.rend(),
            [&excludeName](const ExcludeAssetRecognizer& excludeAssetRecognizer)
        {
            return excludeAssetRecognizer.m_name == excludeName;
        });
        if (excludeAssetRecognizerEntryIt == m_excludeAssetRecognizers.rend())
        {
            return;
        }

        ExcludeAssetRecognizer& excludeAssetRecognizer = *excludeAssetRecognizerEntryIt;

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

        if (valueName == "pattern")
        {
            if (!value.empty())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                excludeAssetRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        else if (valueName == "glob")
        {
            if (!excludeAssetRecognizer.m_patternMatcher.IsValid())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                excludeAssetRecognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
    }

    AZ::SettingsRegistryInterface::VisitResponse RCVisitor::Traverse(AZStd::string_view jsonPath, AZStd::string_view valueName,
        AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type)
    {
        constexpr AZStd::string_view RCNamePrefix = "RC ";
        switch (action)
        {
        case AZ::SettingsRegistryInterface::VisitAction::Begin:
        {
            if (jsonPath == AssetProcessorSettingsKey)
            {
                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            }
            if (valueName.starts_with(RCNamePrefix))
            {
                // Extract the substr that is part of the valueName "Exclude *"
                AZStd::string rcName = valueName.substr(RCNamePrefix.size());
                m_rcNameStack.push(rcName);

                RCAssetRecognizer& assetRecognizer = m_assetRecognizers.emplace_back();
                assetRecognizer.m_recognizer.m_name = QString::fromUtf8(rcName.c_str(), aznumeric_cast<int>(rcName.size()));
            }
        }
        break;
        case AZ::SettingsRegistryInterface::VisitAction::End:
        {
            if (valueName.starts_with(RCNamePrefix))
            {
                AZ_Assert(!m_rcNameStack.empty(), "RC name stack should not be empty. More stack pops, than pushes");
                ApplyParamsOverrides(jsonPath);
                m_rcNameStack.pop();
            }
        }
        break;

        default:
            break;
        }

        return !m_rcNameStack.empty() ? AZ::SettingsRegistryInterface::VisitResponse::Continue
            : AZ::SettingsRegistryInterface::VisitResponse::Skip;
    }

    void RCVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value)
    {
        if (m_rcNameStack.empty())
        {
            return;
        }

        AZStd::string_view rcNameView = m_rcNameStack.top();
        auto rcName = QString::fromUtf8(rcNameView.data(), aznumeric_cast<int>(rcNameView.size()));

        // Find AssetRecognizer identified by the top entry in the name stack
        auto assetRecognizerEntryIt = AZStd::find_if(m_assetRecognizers.rbegin(), m_assetRecognizers.rend(),
            [&rcName](const RCAssetRecognizer& assetRecognizer)
        {
            return assetRecognizer.m_recognizer.m_name == rcName;
        });
        if (assetRecognizerEntryIt == m_assetRecognizers.rend())
        {
            return;
        }

        RCAssetRecognizer& assetRecognizer = *assetRecognizerEntryIt;
        if (valueName == "ignore")
        {
            assetRecognizer.m_ignore = value;
        }
        else if (valueName == "lockSource")
        {
            assetRecognizer.m_recognizer.m_testLockSource = value;
        }
        else if (valueName == "critical")
        {
            assetRecognizer.m_recognizer.m_isCritical = value;
        }
        else if (valueName == "checkServer")
        {
            assetRecognizer.m_recognizer.m_checkServer = value;
        }
        else if (valueName == "supportsCreateJobs")
        {
            assetRecognizer.m_recognizer.m_supportsCreateJobs = value;
        }
        else if (valueName == "outputProductDependencies")
        {
            assetRecognizer.m_recognizer.m_outputProductDependencies = value;
        }
    }

    void RCVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value)
    {
        if (m_rcNameStack.empty())
        {
            return;
        }

        AZStd::string_view rcNameView = m_rcNameStack.top();
        auto rcName = QString::fromUtf8(rcNameView.data(), aznumeric_cast<int>(rcNameView.size()));

        // Find AssetRecognizer identified by the top entry in the name stack
        auto assetRecognizerEntryIt = AZStd::find_if(m_assetRecognizers.rbegin(), m_assetRecognizers.rend(),
            [&rcName](const RCAssetRecognizer& assetRecognizer)
        {
            return assetRecognizer.m_recognizer.m_name == rcName;
        });
        if (assetRecognizerEntryIt == m_assetRecognizers.rend())
        {
            return;
        }

        RCAssetRecognizer& assetRecognizer = *assetRecognizerEntryIt;
        if (valueName == "priority")
        {
            assetRecognizer.m_recognizer.m_priority = static_cast<int>(value);
        }
    }

    void RCVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
    {
        if (m_rcNameStack.empty())
        {
            return;
        }

        // Find AssetRecognizer identified by the top entry in the name stack
        AZStd::string_view rcNameView = m_rcNameStack.top();
        auto rcName = QString::fromUtf8(rcNameView.data(), aznumeric_cast<int>(rcNameView.size()));

        auto assetRecognizerEntryIt = AZStd::find_if(m_assetRecognizers.rbegin(), m_assetRecognizers.rend(),
            [&rcName](const RCAssetRecognizer& assetRecognizer)
        {
            return assetRecognizer.m_recognizer.m_name == rcName;
        });
        if (assetRecognizerEntryIt == m_assetRecognizers.rend())
        {
            return;
        }

        RCAssetRecognizer& assetRecognizer = *assetRecognizerEntryIt;

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

        if (valueName == "pattern")
        {
            if (!value.empty())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                assetRecognizer.m_recognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        else if (valueName == "glob")
        {
            // Add the glob pattern if it the matter matcher doesn't already contain a valid regex pattern
            if (!assetRecognizer.m_recognizer.m_patternMatcher.IsValid())
            {
                const auto patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                assetRecognizer.m_recognizer.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(UnescapePattern(value), patternType);
            }
        }
        else if (valueName == "version")
        {
            assetRecognizer.m_recognizer.m_version = QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size()));
        }
        else if (valueName == "productAssetType")
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
        else if (valueName == "params")
        {
            assetRecognizer.m_defaultParams = value;
        }
    }

    void RCVisitor::ApplyParamsOverrides(AZStd::string_view path)
    {
        if (m_rcNameStack.empty())
        {
            return;
        }

        AZStd::string_view rcNameView = m_rcNameStack.top();
        auto rcName = QString::fromUtf8(rcNameView.data(), aznumeric_cast<int>(rcNameView.size()));

        // Find AssetRecognizer identified by the top entry in the name stack
        auto assetRecognizerEntryIt = AZStd::find_if(m_assetRecognizers.rbegin(), m_assetRecognizers.rend(),
            [&rcName](const RCAssetRecognizer& assetRecognizer)
        {
            return assetRecognizer.m_recognizer.m_name == rcName;
        });
        if (assetRecognizerEntryIt == m_assetRecognizers.rend())
        {
            return;
        }

        RCAssetRecognizer& assetRecognizer = *assetRecognizerEntryIt;

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
            AZStd::string_view currentRCParams = assetRecognizer.m_defaultParams;
            // The "/Amazon/AssetProcessor/Settings/RC */<platform>" entry will be queried 
            AZ::IO::Path overrideParamsKey = AZ::IO::Path(AZ::IO::PosixPathSeparator);
            overrideParamsKey /= path;
            overrideParamsKey /= platform.m_identifier;

            AZ::SettingsRegistryInterface::FixedValueString overrideParamsValue;
            // Check if the enabled platform identifier matches a key within the "RC *" object
            if (m_registry.Get(overrideParamsValue, overrideParamsKey.Native()))
            {
                currentRCParams = overrideParamsValue;
            }
            else
            {
                // otherwise check for tags associated with the platform 
                for (const AZStd::string& tag : platform.m_tags)
                {
                    overrideParamsKey.ReplaceFilename(AZ::IO::PathView(tag));
                    if (m_registry.Get(overrideParamsValue, overrideParamsKey.Native()))
                    {
                        // if we get here it means we found a tag that applies to this platform
                        currentRCParams = overrideParamsValue;
                        break;
                    }
                }
            }

            // now generate a platform spec as long as we're not skipping
            if (!AZ::StringFunc::Equal(currentRCParams, "skip"))
            {
                auto platformIdentifier = QString::fromUtf8(platform.m_identifier.data(),
                    aznumeric_cast<int>(platform.m_identifier.size()));
                AssetPlatformSpec spec;
                // a special case exists where this is "overriding" an underlying version.
                // in this case, unless some string was specified for the overrider, we use the underlying one
                if (!currentRCParams.empty())
                {
                    spec.m_extraRCParams = QString::fromUtf8(currentRCParams.data(), aznumeric_cast<int>(currentRCParams.size()));
                }
                else
                {
                    if (assetRecognizer.m_recognizer.m_platformSpecs.contains(platformIdentifier))
                    {
                        // carry over the prior
                        spec.m_extraRCParams = assetRecognizer.m_recognizer.m_platformSpecs[platformIdentifier].m_extraRCParams;
                    }
                }
                assetRecognizer.m_recognizer.m_platformSpecs[platformIdentifier] = spec;
            }
        }
    }

    const char AssetConfigPlatformDir[] = "AssetProcessorConfig/";
    const char AssetProcessorPlatformConfigFileName[] = "AssetProcessorPlatformConfig.ini";

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
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*settingsRegistry, *commandLine, true);
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

    bool PlatformConfiguration::ReadRecognizersFromSettingsRegistry(const QString& assetRoot, bool skipScanFolders, QStringList scanFolderPatterns)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Global Settings Registry is not set."
                " Unable to read recognizers Asset Processor Settings");
            return false;
        }

        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        AZ::IO::FixedMaxPathString projectName = AZ::Utils::GetProjectName();

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
                // Normalize path make sure it is using posix slashes
                scanFolderEntry.m_watchPath = scanFolderEntry.m_watchPath.LexicallyNormal();

                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@ROOT@", assetRootPath.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@PROJECTROOT@", projectPath.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@PROJECTNAME@", projectName.c_str());
                AZ::StringFunc::Replace(scanFolderEntry.m_scanFolderDisplayName, "@ENGINEROOT@", engineRoot.c_str());

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
                AddScanFolder(ScanFolderInfo(
                    QString::fromUtf8(scanFolderEntry.m_watchPath.c_str(), aznumeric_cast<int>(scanFolderEntry.m_watchPath.Native().size())),
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

        RCVisitor rcVisitor(*settingsRegistry, m_enabledPlatforms);
        settingsRegistry->Visit(rcVisitor, AssetProcessorSettingsKey);
        for (auto&& rcRecognizer : rcVisitor.m_assetRecognizers)
        {
            if (rcRecognizer.m_ignore)
            {
                m_assetRecognizers.remove(rcRecognizer.m_recognizer.m_name);
            }
            else if (!rcRecognizer.m_recognizer.m_platformSpecs.empty())
            {
                m_assetRecognizers[rcRecognizer.m_recognizer.m_name] = rcRecognizer.m_recognizer;
            }
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

    bool PlatformConfiguration::MergeConfigFileToSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry, const AZ::IO::PathView& configFile)
    {
        // If the config file is a settings registry file use the SettingsRegistryInterface MergeSettingsFile function
        // otherwise use the SettingsRegistryMergeUtils MergeSettingsToRegistry_ConfigFile function to merge an INI-style
        // file to the settings registry
        if (configFile.Extension() == ".setreg")
        {
            return settingsRegistry.MergeSettingsFile(configFile.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
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
        AZ::IO::Path scanFolderPathView(scanFolderPath.toUtf8().constData());
        for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
        {
            if (AZ::IO::PathView(m_scanFolders[pathIdx].ScanPath().toUtf8().constData()) == scanFolderPathView)
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

    void PlatformConfiguration::AddGemScanFolders(const AZStd::vector<AzFramework::GemInfo>& gemInfoList)
    {
        int gemOrder = g_gemStartingOrder;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        PopulatePlatformsForScanFolder(platforms);

        for (const AzFramework::GemInfo& gemElement : gemInfoList)
        {
            for (size_t sourcePathIndex{}; sourcePathIndex < gemElement.m_absoluteSourcePaths.size(); ++sourcePathIndex)
            {
                const AZ::IO::Path& absoluteSourcePath = gemElement.m_absoluteSourcePaths[sourcePathIndex];
                QString gemAbsolutePath = QString::fromUtf8(absoluteSourcePath.c_str(), aznumeric_cast<int>(absoluteSourcePath.Native().size())); // this is an absolute path!
                // Append the index of the source path array element to make a unique portable key is created for each path of a gem
                AZ::Uuid gemNameUuid = AZ::Uuid::CreateName((gemElement.m_gemName + AZStd::to_string(sourcePathIndex)).c_str());
                char gemNameToUuidBuffer[AZ::Uuid::MaxStringBuffer];
                gemNameUuid.ToString(gemNameToUuidBuffer);
                QString gemNameAsUuid(gemNameToUuidBuffer);

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
                gemOrder++;

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
                gemOrder++;

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
        QString relPath, scanFolderName;
        if (ConvertToRelativePath(fileName, relPath, scanFolderName))
        {
            for (const ExcludeAssetRecognizer& excludeRecognizer : m_excludeAssetRecognizers)
            {
                if (excludeRecognizer.m_patternMatcher.MatchesPath(relPath.toUtf8().constData()))
                {
                    return true;
                }
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

