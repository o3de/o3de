/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/ranges/filter_view.h>
#include <AzCore/IO/ByteContainerStream.h>

namespace AZ
{
    SettingsRegistryOriginTracker::SettingsRegistryOriginTracker(AZ::SettingsRegistryInterface& registry)
        : m_settingsRegistry(registry)
    {
        registry.SetNotifyForMergeOperations(true);
        m_notifyHandler = m_settingsRegistry.RegisterNotifier(SettingsNotificationHandler(*this));
    }

    SettingsRegistryOriginTracker::~SettingsRegistryOriginTracker() = default;

    SettingsRegistryOriginTracker::SettingsNotificationHandler::SettingsNotificationHandler(
        AZ::SettingsRegistryOriginTracker& originTracker)
        : m_originTracker(originTracker)
    {
    }

    void SettingsRegistryOriginTracker::SetTrackingFilter(TrackingFilterCallback filterCallback)
    {
        m_trackingFilter = AZStd::move(filterCallback);
    }

    AZ::SettingsRegistryInterface& SettingsRegistryOriginTracker::GetSettingsRegistry()
    {
        return m_settingsRegistry;
    }

    const AZ::SettingsRegistryInterface& SettingsRegistryOriginTracker::GetSettingsRegistry() const
    {
        return m_settingsRegistry;
    }

    SettingsRegistryOriginTracker::SettingsNotificationHandler::~SettingsNotificationHandler() = default;

    bool SettingsRegistryOriginTracker::SettingsRegistryOrigin::operator==(const SettingsRegistryOrigin& origin) const
    {
        if (m_originFilePath == origin.m_originFilePath && m_settingsKey == origin.m_settingsKey)
        {
            if (m_settingsValue.has_value() == origin.m_settingsValue.has_value())
            {
                if (m_settingsValue.has_value())
                {
                    auto TrimWhitespace = [](char element) -> bool
                    {
                        return !::isspace(element);
                    };
                    return AZStd::ranges::equal(*m_settingsValue | AZStd::views::filter(TrimWhitespace),
                        *origin.m_settingsValue | AZStd::views::filter(TrimWhitespace));
                }
                else
                {
                    return true;
                }
            }
        }

        return false;
    }

    void AZ::SettingsRegistryOriginTracker::SettingsNotificationHandler::operator()(
        const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
    {
        m_originTracker.AddOrigin(notifyEventArgs.m_jsonKeyPath, notifyEventArgs.m_mergeFilePath);
    }

    bool SettingsRegistryOriginTracker::FindLastOrigin(AZ::IO::Path& originPath, AZStd::string_view key) const
    {
        auto populateOriginPath = [&originPath](const AZ::Dom::Path&, const SettingsRegistryOriginStack& stack)
        {
            if (!stack.empty())
            {
                originPath = stack.front().m_originFilePath;
                return false;
            }
            return true;
        };
        AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        constexpr AZ::Dom::PrefixTreeTraversalFlags traversalFlags = AZ::Dom::PrefixTreeTraversalFlags::ExcludeChildPaths | AZ::Dom::PrefixTreeTraversalFlags::TraverseMostToLeastSpecific;
        m_settingsOriginPrefixTree.VisitPath(jsonPath, populateOriginPath, traversalFlags);
        return !originPath.empty();
    }

    void SettingsRegistryOriginTracker::AddOrigin(AZStd::string_view key, AZ::IO::PathView originPath)
    {
        //! Scratch Buffer for storing json parent path
        AZStd::string domPathBuffer;

        // Remove any entries lower in the stack for this setting that has the same file origin
        // This could occur if the settings file is merged multiple times
        // i.e foo.setreg -> bar.setreg -> ... -> foo.setreg
        // This makes sure the origin stack only contains one entry
        // for any settings key, file origin pair
        RemoveOrigin(key, originPath);

        AZ::Dom::Path domPath(key);
        do
        {
            // If a tracking filter callback exist, invoke it
            // and if it returns false skip tracking the json entry
            if (m_trackingFilter && !m_trackingFilter(key))
            {
                return;
            }

            // Check to see if the settings is within the settings registry
            // If the setting has been removed, then the SettingsRegistryInterface::GetType function will return no type
            SettingsRegistryOrigin settingsRegistryOrigin{ originPath, key };
            if (auto type = m_settingsRegistry.GetType(key);
                type != AZ::SettingsRegistryInterface::Type::NoType)
            {
                // If the Type is an array or object just add a marker value instead of dumping the entire object
                AZStd::string settingsValue;
                if (type == AZ::SettingsRegistryInterface::Type::Object)
                {
                    constexpr AZStd::string_view ObjectMarkerValue = "<object>";
                    settingsValue = ObjectMarkerValue;
                }
                else if (type == AZ::SettingsRegistryInterface::Type::Array)
                {
                    constexpr AZStd::string_view ArrayMarkerValue = "<array>";
                    settingsValue = ArrayMarkerValue;
                }
                else
                {
                    // Retrieve the JSON value as a string using the SettingsRegistryMergeUtils::DumpSettingsRegistryToStream function
                    AZ::IO::ByteContainerStream stringWriter(&settingsValue);
                    AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                    dumperSettings.m_prettifyOutput = false;
                    AZ_Verify(AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(m_settingsRegistry, key, stringWriter, dumperSettings),
                        R"(Failed to Dump SettingsRegistry at key "%.*s")", AZ_STRING_ARG(key));
                }
                // The settings at the json pointer path has been written to the string writer as JSON
                // So emplace it into the settings value of SettingsRegistryOrigin
                settingsRegistryOrigin.m_settingsValue.emplace(settingsValue);
            }

            // Push the new entry settings registry origin entry for this DOM path to origin stack
            if (SettingsRegistryOriginStack* settingsOriginStack = m_settingsOriginPrefixTree.ValueAtPath(
                domPath, AZ::Dom::PrefixTreeMatch::ExactPath);
                settingsOriginStack != nullptr)
            {
                settingsOriginStack->emplace_front(AZStd::move(settingsRegistryOrigin));
            }
            else
            {
                SettingsRegistryOriginStack newOriginStack;
                newOriginStack.emplace_front(AZStd::move(settingsRegistryOrigin));
                m_settingsOriginPrefixTree.SetValue(domPath, AZStd::move(newOriginStack));
            }

            // Recursively walk up the json path fields to make sure that ancestor keys have their file origin set as well
            // The scenario where this occurs is if a setting is set at a hierarchy multiple levels below an existing key
            // i.e SettingsRegistry->MergeSettings("/O3DE", ...),
            // SettingsRegistry->MergeSettings("/O3DE/Two/Levels", ...)
            // When the "/O3DE/Two/Levels" key origin is being added, there is no origin being tracked for "/O3DE/Two"
            if (domPath.IsEmpty())
            {
                break;
            }

            // Pop of the current field entry and examine the parent JSON path
            domPath.Pop();
            domPathBuffer = domPath.ToString();
            key = domPathBuffer;

            // If the parent DOM path isn't in the settings registry origin prefix tree
            // Then loop again the supplied file path origin of the child entry
        } while (m_settingsOriginPrefixTree.ValueAtPath(domPath, AZ::Dom::PrefixTreeMatch::ExactPath) == nullptr);
    }

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
            if (originStack.empty() && !path.IsEmpty())
            {
                m_settingsOriginPrefixTree.EraseValue(path);
            }
            return true;
        };
        m_settingsOriginPrefixTree.VisitPath(jsonPath, removeOriginAtKey, traversalFlags);
    }

    void SettingsRegistryOriginTracker::VisitOrigins(AZStd::string_view key, const OriginVisitorCallback& visitCallback) const
    {
        auto visitOriginStackCallback = [&](const AZ::Dom::Path&, const SettingsRegistryOriginStack& stack)
        {
            for (const auto& origin : stack)
            {
                if (!visitCallback(origin))
                {
                    break;
                }
            }
            return true;
        };
        AZ::Dom::Path jsonPath = AZ::Dom::Path(key);
        constexpr AZ::Dom::PrefixTreeTraversalFlags traversalFlags =
            AZ::Dom::PrefixTreeTraversalFlags::TraverseLeastToMostSpecific | AZ::Dom::PrefixTreeTraversalFlags::ExcludeParentPaths;
        m_settingsOriginPrefixTree.VisitPath(jsonPath, visitOriginStackCallback, traversalFlags);
    }
}
