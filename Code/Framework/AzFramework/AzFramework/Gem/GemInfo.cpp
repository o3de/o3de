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
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Gem/GemInfo.h>

namespace AzFramework
{
    GemInfo::GemInfo(AZStd::string name)
        : m_gemName(AZStd::move(name))
    {
    }

    bool GetGemsInfo(AZStd::vector<GemInfo>& gemInfoList, AZ::SettingsRegistryInterface& settingsRegistry)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        auto GemSettingsVisitor = [&settingsRegistry, &gemInfoList]
        (const AZ::SettingsRegistryInterface::VisitArgs& gemVisitArgs)
        {
            auto FindGemInfoByName = [&gemVisitArgs](const GemInfo& gemInfo)
            {
                return gemVisitArgs.m_fieldName == gemInfo.m_gemName;
            };
            auto gemInfoFoundIter = AZStd::ranges::find_if(gemInfoList, FindGemInfoByName);
            GemInfo& gemInfo = gemInfoFoundIter != gemInfoList.end() ? *gemInfoFoundIter : gemInfoList.emplace_back(gemVisitArgs.m_fieldName);

            // Read the Gem Target Name into target Name field
            auto VisitGemTargets = [&gemInfo](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                // Assume the fieldName is the name of the target in this case
                gemInfo.m_gemTargetNames.emplace_back(visitArgs.m_fieldName);
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };
            AZ::SettingsRegistryVisitorUtils::VisitObject(settingsRegistry, VisitGemTargets,
                FixedValueString::format("%.*s/Targets", AZ_STRING_ARG(gemVisitArgs.m_jsonKeyPath)));

            // Visit the "SourcePath" array fields of the gem to
            // populate the Gem Absolute Source Paths array
            const auto gemPathKey = FixedValueString::format("%s/%.*s/Path",
                AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey, AZ_STRING_ARG(gemVisitArgs.m_fieldName));
            if (AZ::IO::Path gemRootPath; settingsRegistry.Get(gemRootPath.Native(), gemPathKey))
            {
                gemInfo.m_absoluteSourcePaths.emplace_back(gemRootPath);
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(settingsRegistry, GemSettingsVisitor,
            AZ::SettingsRegistryMergeUtils::ActiveGemsRootKey);

        return true;
    }
}
