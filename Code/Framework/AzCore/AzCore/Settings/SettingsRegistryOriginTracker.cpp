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
        const AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        if (m_settingsOriginPrefixTree.ValueAtPath(jsonPath, AZ::Dom::PrefixTreeMatch::ExactPath) == nullptr)
        {
            m_settingsOriginPrefixTree.SetValue(jsonPath, SettingsRegistryOriginStack());
        }
       // adds a new origin to origin stack of a dom tree node
        auto addOriginToStack = [&](const AZ::Dom::Path& path, SettingsRegistryOriginStack& stack) {
            
            // checks if there is a node in the dom tree for a given path and creates one if it doesn't exist
            SettingsRegistryOriginStack* stackPtr =
                m_settingsOriginPrefixTree.ValueAtPath(path, AZ::Dom::PrefixTreeMatch::ExactPath);
            if (stackPtr == nullptr)
            {
                m_settingsOriginPrefixTree.SetValue(path, SettingsRegistryOriginStack());
            }
            bool hasChildren = false;
            m_settingsOriginPrefixTree.VisitPath(
                path,
                [&hasChildren](const AZ::Dom::Path&, SettingsRegistryOriginStack&)
                {
                    hasChildren = true;
                    return false;
                },
                AZ::Dom::PrefixTreeTraversalFlags::ExcludeExactPath | AZ::Dom::PrefixTreeTraversalFlags::ExcludeParentPaths);
            if (stack.empty() || !hasChildren)
            {

                AZStd::string pathStr = path.ToString();
                SettingsRegistryOriginTracker::SettingsRegistryOrigin origin{ originPath, pathStr };
                // retrieves the settings registry value for a key if it exists
                if (m_settingsRegistry.GetType(path.ToString()) != AZ::SettingsRegistryInterface::Type::NoType)
                {
                    AZStd::string settingJsonStr;
                    AZ::IO::ByteContainerStream stringWriter(&settingJsonStr);
                    AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                    AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(
                        this->m_settingsRegistry, pathStr, stringWriter, dumperSettings);
                    origin.m_settingsValue.emplace(settingJsonStr);
                }
                stack.emplace_back(origin);
            };
            return true;
        };
        // Add origin does not add an origin to any child paths, so children are excluded
        // Parent paths will also have an origin added if they do not currently contain one
        AZ::Dom::PrefixTreeTraversalFlags traversalFlags =
            AZ::Dom::PrefixTreeTraversalFlags::ExcludeChildPaths | AZ::Dom::PrefixTreeTraversalFlags::TraverseMostToLeastSpecific;
        m_settingsOriginPrefixTree.VisitPath(jsonPath, addOriginToStack, traversalFlags);
    };

    void SettingsRegistryOriginTracker::RemoveOrigin(AZStd::string_view key, AZ::IO::PathView originPath, bool recurseChildren)
    {
        const AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        // Remove origin does not remove any ancestor settings paths, so ExcludeParentPaths is set
        // If recurseChildren is set, then a depth first search traversal is done
        // to allow deletion of children settings paths entries safely.
        const AZ::Dom::PrefixTreeTraversalFlags traversalFlags = AZ::Dom::PrefixTreeTraversalFlags::ExcludeParentPaths |
            (recurseChildren ? AZ::Dom::PrefixTreeTraversalFlags::TraverseMostToLeastSpecific
                             : AZ::Dom::PrefixTreeTraversalFlags::ExcludeChildPaths);

        //remove a setting origin at a given Dom path.
        auto removeOriginAtKey = [this, &originPath](const AZ::Dom::Path& path, SettingsRegistryOriginStack& originStack)
        {
            //return if a given origin is the origin to remove
            auto removeOriginCondition = [&originPath](const SettingsRegistryOrigin& origin)
            {
                return origin.m_originFilePath == originPath;
            };
            AZStd::erase_if(originStack, removeOriginCondition);

            // if the originStack is empty, remove associated node from prefix tree
            if (originStack.empty() && path.size() > 0)
            {
                m_settingsOriginPrefixTree.EraseValue(path, true);
            };
            return true;
        };
        m_settingsOriginPrefixTree.VisitPath(jsonPath, removeOriginAtKey, traversalFlags);
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
        AZ::Dom::PrefixTreeTraversalFlags traversalFlags =
            AZ::Dom::PrefixTreeTraversalFlags::TraverseLeastToMostSpecific | AZ::Dom::PrefixTreeTraversalFlags::ExcludeParentPaths;
        m_settingsOriginPrefixTree.VisitPath(jsonPath, visitOriginStackCallback, traversalFlags);
    };


};
