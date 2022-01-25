/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/Gem/GemInfo.h>

namespace AzFramework
{
    GemInfo::GemInfo(AZStd::string name)
        : m_gemName(AZStd::move(name))
    {
    }

    bool GetGemsInfo(AZStd::vector<GemInfo>& gemInfoList, AZ::SettingsRegistryInterface& settingsRegistry)
    {
        AZStd::vector<AZ::IO::FixedMaxPath> gemModuleSourcePaths;

        struct GemSourcePathsVisitor
            : AZ::SettingsRegistryInterface::Visitor
        {
            GemSourcePathsVisitor(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::vector<GemInfo>& gemInfoList)
                : m_settingsRegistry(settingsRegistry)
                , m_gemInfoList(gemInfoList)
            {
            }

            using AZ::SettingsRegistryInterface::Visitor::Visit;
            void Visit(AZStd::string_view path, AZStd::string_view, AZ::SettingsRegistryInterface::Type,
                AZStd::string_view value) override
            {
                AZStd::string_view jsonPointerPath{ path };
                // Remove the array index from the path and check if the JSON path ends with "/SourcePaths"
                AZ::StringFunc::TokenizeLast(jsonPointerPath, '/');
                if (jsonPointerPath.ends_with("/SourcePaths"))
                {
                    AZStd::string_view gemName;
                    // The parent key of the "SourcePaths" field is the gem name
                    AZ::StringFunc::TokenizeLast(jsonPointerPath, '/'); // Peel off "/SourcePaths"
                    // Retrieve Gem name
                    if (auto gemNameToken = AZ::StringFunc::TokenizeLast(jsonPointerPath, '/'); gemNameToken.has_value())
                    {
                        gemName = *gemNameToken;
                    }

                    auto FindGemInfoByName = [gemName](const GemInfo& gemInfo)
                    {
                        return gemName == gemInfo.m_gemName;
                    };
                    auto gemInfoFoundIter = AZStd::find_if(m_gemInfoList.begin(), m_gemInfoList.end(), FindGemInfoByName);
                    GemInfo& gemInfo = gemInfoFoundIter != m_gemInfoList.end() ? *gemInfoFoundIter : m_gemInfoList.emplace_back(gemName);

                    AZ::IO::Path& gemAbsPath = gemInfo.m_absoluteSourcePaths.emplace_back(value);
                    // Resolve any file aliases first - Do not use ResolvePath() as that assumes
                    // any relative path is underneath the @products@ alias
                    if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
                    {
                        AZ::IO::FixedMaxPath replacedAliasPath;
                        if (fileIoBase->ReplaceAlias(replacedAliasPath, value))
                        {
                            gemAbsPath = AZ::IO::PathView(replacedAliasPath);
                        }
                    }

                    // The current assumption is that the gem source path is the relative to the engine root
                    AZ::IO::FixedMaxPath engineRootPath;
                    m_settingsRegistry.Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
                    gemAbsPath = (engineRootPath / gemAbsPath).LexicallyNormal();
                }
            }

            AZ::SettingsRegistryInterface& m_settingsRegistry;
            AZStd::vector<GemInfo>& m_gemInfoList;
        };

        GemSourcePathsVisitor visitor{ settingsRegistry, gemInfoList };
        constexpr auto gemListKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::OrganizationRootKey)
            + "/Gems";
        settingsRegistry.Visit(visitor, gemListKey);

        return true;
    }
}
