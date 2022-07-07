/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AZCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/ByteContainerStream.h>

namespace AZ
{
    SettingsRegistryOriginTracker::SettingsRegistryOriginTracker(AZ::SettingsRegistryInterface& registry)
        : m_settingsRegistry(registry)
    {
        registry.SetNotifyForMergeOperations(true);
        auto preMergeCallback =
            [this](AZStd::string_view filePath, [[maybe_unused]] AZStd::string_view rootKey)
        {
            m_currentOriginStack.emplace(filePath);
        };
        auto postMergeCallback =
            [this]([[maybe_unused]] AZStd::string_view path, [[maybe_unused]] AZStd::string_view rootKey)
        {
            m_currentOriginStack.pop();
        };
        m_preMergeEventHandler = m_settingsRegistry.RegisterPreMergeEvent(preMergeCallback);
        m_postMergeEventHandler = m_settingsRegistry.RegisterPostMergeEvent(postMergeCallback);
        m_notifyHandler = m_settingsRegistry.RegisterNotifier(SettingsNotificationHandler(*this));
    };

    SettingsRegistryOriginTracker::~SettingsRegistryOriginTracker()
    {
        AZ_Assert(
            m_currentOriginStack.empty(),
            "The Settings Registry Origin Tracker origin stack should be empty on destruction."
            " This implies that it is being destroyed during the middle of merging a file");
    };

    SettingsRegistryOriginTracker::SettingsNotificationHandler::SettingsNotificationHandler(AZ::SettingsRegistryOriginTracker &originTracker) : m_originTracker(originTracker){
    };

    SettingsRegistryOriginTracker::SettingsNotificationHandler::~SettingsNotificationHandler() = default;

    bool SettingsRegistryOriginTracker::SettingsRegistryOrigin::operator==(const SettingsRegistryOrigin& origin) const
    {
        return (
            m_originFilePath == origin.m_originFilePath && m_settingsKey == origin.m_settingsKey &&
            m_settingsValue == origin.m_settingsValue);
    }


    void AZ::SettingsRegistryOriginTracker::SettingsNotificationHandler::operator()(AZStd::string_view path, AZ::SettingsRegistryInterface::Type){
        AZ::IO::Path originPath =
            !m_originTracker.m_currentOriginStack.empty() ? m_originTracker.m_currentOriginStack.top() : "<in-memory>";
        m_originTracker.AddOrigin(path, originPath);
    };

    bool SettingsRegistryOriginTracker::FindLastOrigin(AZ::IO::Path &originPath, AZStd::string_view key) {
        auto populateOriginPath = [&](const AZ::Dom::Path&, const SettingsRegistryOriginStack& stack)
        {
            if (originPath.empty() && !stack.empty())
            {
                originPath = stack.back().m_originFilePath;
                return false;
            };
            return true;
        };
        AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        m_settingsOriginPrefixTree.VisitPath(
            jsonPath, populateOriginPath, AZ::Dom::PrefixTreeTraversalFlags::TraverseLeastToMostSpecific);
        return !originPath.empty();
    }

    void SettingsRegistryOriginTracker::AddOrigin(AZStd::string_view key, AZ::IO::PathView originPath)
    {
        AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        m_settingsOriginPrefixTree.ValueAtPathOrDefault(
            jsonPath, SettingsRegistryOriginStack(), AZ::Dom::PrefixTreeMatch::PathAndParents);
        auto removeExistingOrigin = [&](const AZ::Dom::Path&, SettingsRegistryOriginStack& stack)
        {
            for (auto it = stack.begin(); it != stack.end(); ++it)
            {
                if (it->m_originFilePath == originPath)
                {
                    stack.erase(it);
                    break;
                };
            };
            return true;
        };
        m_settingsOriginPrefixTree.VisitPath(jsonPath, removeExistingOrigin, AZ::Dom::PrefixTreeTraversalFlags::TraverseLeastToMostSpecific);
        auto addOriginToStack = [&](const AZ::Dom::Path& path, SettingsRegistryOriginStack& stack) {
            if (stack.empty() || path == jsonPath)
            {
                AZStd::string_view pathStr = AZStd::string_view(path.ToString());
                AZStd::string settingJsonStr;
                AZ::IO::ByteContainerStream stringWriter(&settingJsonStr);
                AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(this->m_settingsRegistry, key, stringWriter, dumperSettings);
                SettingsRegistryOriginTracker::SettingsRegistryOrigin origin{ originPath, pathStr, settingJsonStr };
                stack.emplace_back(origin);
            };
            return true;
        };
        m_settingsOriginPrefixTree.VisitPath(jsonPath, addOriginToStack, AZ::Dom::PrefixTreeTraversalFlags::TraverseLeastToMostSpecific);
    };

    void RemoveOrigin(AZStd::string_view key, AZ::IO::PathView originPath, bool recurseChildren)
    {
        (void)key;
        (void)originPath;
        (void)recurseChildren;
    };

    void SettingsRegistryOriginTracker::VisitOrigins(AZStd::string_view key, const OriginVisitorCallback& visitCallback)
    {
        auto visitOriginStackCallback = [&](const AZ::Dom::Path&, SettingsRegistryOriginStack& stack)
        {
            for (auto& origin : stack)
            {
                if (visitCallback(origin))
                {
                    break;
                }
            }
            AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
            return true;
        };
        AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        m_settingsOriginPrefixTree.VisitPath(jsonPath, visitOriginStackCallback, AZ::Dom::PrefixTreeTraversalFlags::TraverseLeastToMostSpecific);
    };


};
