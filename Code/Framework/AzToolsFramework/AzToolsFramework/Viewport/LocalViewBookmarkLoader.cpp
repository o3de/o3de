/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <API/ToolsApplicationAPI.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <Entity/PrefabEditorEntityOwnershipInterface.h>
#include <Prefab/PrefabSystemComponentInterface.h>
#include <Viewport/LocalViewBookmarkLoader.h>

namespace AzToolsFramework
{
    static constexpr const char* ViewBookmarksRegistryPath = "/O3DE/ViewBookmarks/";
    static constexpr const char* LocalBookmarksKey = "LocalBookmarks";
    static constexpr const char* LastKnownLocationKey = "LastKnownLocation";

    // temporary value until there is UI to expose the fields
    static constexpr int DefaultViewBookmarkCount = 12;

    void LocalViewBookmarkLoader::RegisterViewBookmarkLoaderInterface()
    {
        AZ::Interface<ViewBookmarkLoaderInterface>::Register(this);
    }

    void LocalViewBookmarkLoader::UnregisterViewBookmarkLoaderInterface()
    {
        AZ::Interface<ViewBookmarkLoaderInterface>::Unregister(this);
    }

    void LocalViewBookmarkLoader::SaveBookmarkSettingsFile()
    {
        auto registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
            return;
        }

        // Resolve path to user project folder
        AZ::IO::FixedMaxPath editorBookmarkFilePath = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks/";

        editorBookmarkFilePath /= m_bookmarkfileName;

        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;

        AZStd::string bookmarkKey = "/" + m_bookmarkfileName;

        dumperSettings.m_includeFilter = [&bookmarkKey](AZStd::string_view path)
        {
            AZStd::string_view o3dePrefixPath(bookmarkKey);
            return o3dePrefixPath.starts_with(path.substr(0, o3dePrefixPath.size()));
        };

        AZStd::string stringBuffer;
        AZ::IO::ByteContainerStream stringStream(&stringBuffer);
        if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, "", stringStream, dumperSettings))
        {
            AZ_Warning(
                "LocalViewBookmarkLoader", false, R"(Unable to save changes to the Editor Preferences registry file at "%s"\n)",
                editorBookmarkFilePath.c_str());
            return;
        }

        bool saved = false;
        constexpr auto configurationMode =
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
        if (AZ::IO::SystemFile outputFile; outputFile.Open(editorBookmarkFilePath.c_str(), configurationMode))
        {
            saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
        }

        // Once written to the desired file remove the key from the settings registry
        registry->Remove(bookmarkKey + "/");
        AZ_Warning(
            "LocalViewBookmarkLoader", saved, R"(Unable to save Local View Bookmark file to path "%s"\n)", editorBookmarkFilePath.c_str());
    }

    bool LocalViewBookmarkLoader::SaveBookmark(const ViewBookmark& bookmark)
    {
        return SaveLocalBookmark(bookmark, ViewBookmarkType::Standard);
    }

    bool LocalViewBookmarkLoader::ModifyBookmarkAtIndex(const ViewBookmark& bookmark, int index)
    {
        if (index < 0 || index >= DefaultViewBookmarkCount)
        {
            return false;
        }

        LoadDefaultLocalViewBookmarks();

        AZStd::string finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(index);

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            success = registry->SetObject(finalPath, bookmark);
        }

        // If we managed to add the bookmark
        if (success)
        {
            SaveBookmarkSettingsFile();
            LoadViewBookmarks();
        }
        return success;
    }

    bool LocalViewBookmarkLoader::SaveLastKnownLocation(const ViewBookmark& bookmark)
    {
        return SaveLocalBookmark(bookmark, ViewBookmarkType::LastKnownLocation);
    }

    bool LocalViewBookmarkLoader::LoadViewBookmarks()
    {
        struct ViewBookmarkVisitor : AZ::SettingsRegistryInterface::Visitor
        {
            ViewBookmarkVisitor()
                : m_viewBookmarksKey{ "/O3DE/ViewBookmarks" } {};

            AZ::SettingsRegistryInterface::VisitResponse Traverse(
                AZStd::string_view path,
                AZStd::string_view,
                AZ::SettingsRegistryInterface::VisitAction action,
                AZ::SettingsRegistryInterface::Type) override
            {
                if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
                {
                    // Strip off the last JSON pointer key from the path and if it matches the view bookmark key then add an entry
                    // to the ViewBookmark Map
                    AZStd::optional<AZStd::string_view> localBookmarksID = AZ::StringFunc::TokenizeLast(path, "/");
                    if (path == m_viewBookmarksKey && localBookmarksID && !localBookmarksID->empty())
                    {
                        AZStd::string_view bookmarkKey = localBookmarksID.value();

                        if (auto existingBookmarkEntry = m_bookmarkMap.find(bookmarkKey) == m_bookmarkMap.end())
                        {
                            m_bookmarkMap.insert(AZStd::make_pair(bookmarkKey, AZStd::vector<ViewBookmark>{}));
                        }
                    }
                }

                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            }

            using AZ::SettingsRegistryInterface::Visitor::Visit;
            void Visit(AZStd::string_view path, AZStd::string_view valueIndex, AZ::SettingsRegistryInterface::Type, double value) override
            {
                AZ::StringFunc::TokenizeLast(path, "/");
                AZStd::optional<AZStd::string_view> dataType = AZ::StringFunc::TokenizeLast(path, "/");
                AZStd::optional<AZStd::string_view> bookmarkIndexStr = AZ::StringFunc::TokenizeLast(path, "/");
                AZStd::optional<AZStd::string_view> bookmarkType;
                if (bookmarkIndexStr == LastKnownLocationKey)
                {
                    bookmarkType = bookmarkIndexStr;
                }
                else
                {
                    // differentiate between local Bookmarks and LastKnownLocation
                    bookmarkType = AZ::StringFunc::TokenizeLast(path, "/");
                }
                AZStd::optional<AZStd::string_view> localBookmarksID = AZ::StringFunc::TokenizeLast(path, "/");

                if (path == m_viewBookmarksKey && localBookmarksID && !localBookmarksID->empty())
                {
                    auto setVec3Fn = [value](AZ::Vector3& inout, int currentIndex)
                    {
                        switch (currentIndex)
                        {
                        case 0:
                            inout.SetX(aznumeric_cast<float>(value));
                            break;
                        case 1:
                            inout.SetY(aznumeric_cast<float>(value));
                            break;
                        case 2:
                            inout.SetZ(aznumeric_cast<float>(value));
                            break;
                        default:
                            AZ_Warning(
                                "LocalViewBookmarkLoader", false, "Trying to set an invalid index in a Vector3, index = %d", currentIndex);
                            break;
                        }
                    };

                    if (bookmarkType == LastKnownLocationKey)
                    {
                        int currentIndex = stoi(AZStd::string(valueIndex));
                        if (dataType == "Position")
                        {
                            setVec3Fn(m_lastKnownLocation.m_position, currentIndex);
                        }
                        else if (dataType == "Rotation")
                        {
                            setVec3Fn(m_lastKnownLocation.m_rotation, currentIndex);
                        }
                    }
                    else if (bookmarkType == LocalBookmarksKey)
                    {
                        auto existingBookmarkEntry = m_bookmarkMap.find(localBookmarksID.value());
                        if (existingBookmarkEntry != m_bookmarkMap.end())
                        {
                            AZStd::vector<ViewBookmark>& bookmarks = existingBookmarkEntry->second;
                            // if it is the first bookmark and it is the Position data it means it is the first one
                            // and we have to create the Bookmark.
                            if (valueIndex == "0" && dataType == "Position")
                            {
                                ViewBookmark bookmark;
                                bookmark.m_position.SetX(aznumeric_cast<float>(value));
                                bookmarks.push_back(bookmark);
                            }
                            else
                            {
                                int bookmarkIndex = stoi(AZStd::string(bookmarkIndexStr->data()));
                                ViewBookmark& bookmark = bookmarks.at(bookmarkIndex);
                                int currentIndex = stoi(AZStd::string(valueIndex));

                                if (dataType == "Position")
                                {
                                    setVec3Fn(bookmark.m_position, currentIndex);
                                }
                                else if (dataType == "Rotation")
                                {
                                    setVec3Fn(bookmark.m_rotation, currentIndex);
                                }
                            }
                        }
                    }
                }
            };

            const AZ::SettingsRegistryInterface::FixedValueString m_viewBookmarksKey;
            AZStd::unordered_map<AZStd::string, AZStd::vector<ViewBookmark>> m_bookmarkMap;
            ViewBookmark m_lastKnownLocation;
        };

        if (LocalViewBookmarkComponent* bookmarkComponent = RetrieveLocalViewBookmarkComponent())
        {
            // Get the file we want to merge into the settings registry.
            if (!bookmarkComponent->GetLocalBookmarksFileName().empty())
            {
                auto registry = AZ::SettingsRegistry::Get();
                if (!registry)
                {
                    AZ_Warning(
                        "LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
                    return false;
                }

                auto projectUserRegistryPath = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks";
                projectUserRegistryPath /= AZ::IO::FixedMaxPathString(bookmarkComponent->GetLocalBookmarksFileName());

                // Merge the current viewBookmark file into the settings registry.
                bool isMerged = registry->MergeSettingsFile(
                    projectUserRegistryPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "/O3DE/ViewBookmarks");

                if (isMerged)
                {
                    m_bookmarkfileName = bookmarkComponent->GetLocalBookmarksFileName();
                    using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
                    AZStd::string bookmarkKey = ViewBookmarksRegistryPath + m_bookmarkfileName;
                    auto viewBookmarkSettingsKey = FixedValueString::format("%s", bookmarkKey.c_str());
                    ViewBookmarkVisitor viewBookmarkVisitor;

                    const bool visitedViewBookmarks = registry->Visit(viewBookmarkVisitor, viewBookmarkSettingsKey);

                    if (visitedViewBookmarks)
                    {
                        m_localBookmarks = viewBookmarkVisitor.m_bookmarkMap.at(m_bookmarkfileName);
                        m_lastKnownLocation = viewBookmarkVisitor.m_lastKnownLocation;
                    }

                    // once loaded we can remove the data from the settings registry.
                    registry->Remove(bookmarkKey + "/");

                    return visitedViewBookmarks;
                }
            }
        }

        return false;
    }

    AZStd::optional<ViewBookmark> LocalViewBookmarkLoader::LoadBookmarkAtIndex(int index)
    {
        LoadViewBookmarks();
        if (index >= 0 && index < m_localBookmarks.size())
        {
            return AZStd::optional<ViewBookmark>(m_localBookmarks.at(index));
        }
        AZ_Warning("LocalViewBookmarkLoader", false, "Couldn't load View Bookmark from file.");
        return AZStd::nullopt;
    }

    bool LocalViewBookmarkLoader::RemoveBookmarkAtIndex(int index)
    {
        if (index < 0 || index >= DefaultViewBookmarkCount)
        {
            return false;
        }

        AZStd::string finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(index);

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            success = registry->Remove(finalPath);
        }

        if (!success)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "couldn't remove View Bookmark at index %d", index);
            return false;
        }

        // If we managed to remove the bookmark
        SaveBookmarkSettingsFile();
        LoadViewBookmarks();

        return success;
    }

    AZStd::optional<ViewBookmark> LocalViewBookmarkLoader::LoadLastKnownLocation() const
    {
        return m_lastKnownLocation;
    }

    LocalViewBookmarkComponent* LocalViewBookmarkLoader::RetrieveLocalViewBookmarkComponent()
    {
        auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        const AZ::EntityId containerEntityId = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetContainerEntityId();

        if (!containerEntityId.IsValid())
        {
            return nullptr;
        }

        AZ::Entity* containerEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(containerEntity, &AZ::ComponentApplicationBus::Events::FindEntity, containerEntityId);

        if (!containerEntity)
        {
            return nullptr;
        }

        LocalViewBookmarkComponent* bookmarkComponent = containerEntity->FindComponent<LocalViewBookmarkComponent>();
        if (bookmarkComponent)
        {
            return bookmarkComponent;
        }

        ScopedUndoBatch undoBatch("AddLocalViewBookmark");
        undoBatch.MarkEntityDirty(containerEntityId);

        // If we didn't find a component then we add it and return it.
        containerEntity->Deactivate();
        bookmarkComponent = containerEntity->CreateComponent<LocalViewBookmarkComponent>();
        containerEntity->Activate();

        AZ_Assert(bookmarkComponent, "Couldn't create LocalViewBookmarkComponent.");
        return bookmarkComponent;
    }

    AZStd::string LocalViewBookmarkLoader::GenerateBookmarkFileName() const
    {
        auto* prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(prefabEditorEntityOwnershipInterface != nullptr, "PrefabEditorEntityOwnershipInterface is not found.");
        AzToolsFramework::Prefab::TemplateId rootPrefabTemplateId = prefabEditorEntityOwnershipInterface->GetRootPrefabTemplateId();

        if (rootPrefabTemplateId == AzToolsFramework::Prefab::InvalidTemplateId)
        {
            return AZStd::string();
        }

        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(
            prefabSystemComponent != nullptr,
            "Prefab System Component Interface could not be found. "
            "It is a requirement for the LocalViewBookmarkLoader class. "
            "Check that it is being correctly initialized.");

        auto prefabTemplate = prefabSystemComponent->FindTemplate(rootPrefabTemplateId);
        AZStd::string prefabTemplateName = prefabTemplate->get().GetFilePath().Filename().Stem().Native();

        // To generate the file name in which we will store the view Bookmarks we use the name of the prefab + the timestamp
        // e.g. LevelName_1639763579377.setreg
        AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
        AZStd::string sTime = AZStd::string::format("%llu", now.time_since_epoch().count());
        return prefabTemplateName.data() + AZStd::string("_") + sTime + ".setreg";
    }

    bool LocalViewBookmarkLoader::LoadDefaultLocalViewBookmarks()
    {
        // Write to the settings registry
        auto registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "Couldn't load Settings Registry");
            return false;
        }

        LocalViewBookmarkComponent* bookmarkComponent = RetrieveLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "Couldn't find a LocalViewBookmarkComponent");
            return false;
        }

        // if the field is not empty then we have a file linked to the prefab
        if (!bookmarkComponent->GetLocalBookmarksFileName().empty())
        {
            m_bookmarkfileName = bookmarkComponent->GetLocalBookmarksFileName();

            AZ::IO::FixedMaxPath editorBookmarkFilePath =
                AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks/";
            editorBookmarkFilePath /= m_bookmarkfileName;
            AZ::IO::SystemFile outputFile;

            // if the file doesn't exist but we got a valid filename from source control then let's create the file
            if (!outputFile.Exists(editorBookmarkFilePath.c_str()))
            {
                constexpr auto configurationMode =
                    AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;

                outputFile.Open(editorBookmarkFilePath.c_str(), configurationMode);

                // initialize default locations to 0 - this is a temporary solution to match the 12 locations of the legacy system
                // once there is a UI for the view bookmarks these lines will be removed
                for (int i = 0; i < DefaultViewBookmarkCount; i++)
                {
                    AZStd::string finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(i);
                    registry->SetObject(finalPath, ViewBookmark());
                }

                LoadViewBookmarks();

                return true;
            }
            else
            {
                LoadViewBookmarks();

                for (int i = 0; i < m_localBookmarks.size(); i++)
                {
                    AZStd::string finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(i);
                    registry->SetObject(finalPath, m_localBookmarks.at(i));
                }

                return true;
            }
        }
        else // if the field is empty, we don't have a file linked to the prefab, so we create one and we save it in the component
        {
            m_bookmarkfileName = GenerateBookmarkFileName();

            // initialize default locations to 0
            for (int i = 0; i < DefaultViewBookmarkCount; i++)
            {
                AZStd::string finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(i);
                registry->SetObject(finalPath, ViewBookmark());
            }

            bookmarkComponent->SetLocalBookmarksFileName(m_bookmarkfileName);

            return true;
        }
    }

    bool LocalViewBookmarkLoader::SaveLocalBookmark(const ViewBookmark& bookmark, ViewBookmarkType bookmarkType)
    {
        LoadDefaultLocalViewBookmarks();

        AZStd::string finalPath;

        switch (bookmarkType)
        {
        case ViewBookmarkType::Standard:
            // note: this function is not used currently and will need to be updated when DefaultViewBookmarkCount is removed
            finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(m_localBookmarks.size() + 1);
            break;
        case ViewBookmarkType::LastKnownLocation:
            finalPath = "/" + m_bookmarkfileName + "/LastKnownLocation";
            break;
        }

        bool success = false;
        // Write to the settings registry
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            success = registry->SetObject(finalPath, bookmark);
        }

        if (!success)
        {
            AZ_Warning(
                "LocalViewBookmarkLoader", false, "View Bookmark x=%.4f, y=%.4f, z=%.4f couldn't be saved", bookmark.m_position.GetX(),
                bookmark.m_position.GetY(), bookmark.m_position.GetZ());
            return false;
        }

        // if we managed to add the bookmark
        SaveBookmarkSettingsFile();

        // once we have written into our view bookmark file let's load the new values.
        LoadViewBookmarks();

        return success;
    }
} // namespace AzToolsFramework
