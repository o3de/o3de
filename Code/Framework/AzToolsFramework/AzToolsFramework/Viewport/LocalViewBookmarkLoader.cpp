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
    static constexpr const char* ViewBookmarksRegistryPath = "/O3DE/ViewBookmarks";
    static constexpr const char* LocalBookmarksKey = "LocalBookmarks";
    static constexpr const char* LastKnownLocationKey = "LastKnownLocation";

    // temporary value until there is UI to expose the fields
    static constexpr int DefaultViewBookmarkCount = 12;

    static AZ::IO::FixedMaxPath LocalBookmarkFilePath(const AZStd::string& bookmarkFileName)
    {
        return AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks/" / bookmarkFileName;
    }

    // returns true if the file already existed, false otherwise
    static bool EnsureLocalBookmarkFileExists(const AZStd::string& bookmarkFileName)
    {
        const auto localBookmarkFilePath = LocalBookmarkFilePath(bookmarkFileName);

        AZ::IO::SystemFile outputFile;
        // if the file doesn't exist but we got a valid filename from source control then let's create the file
        const auto configurationMode = !outputFile.Exists(localBookmarkFilePath.c_str())
            ? AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_READ_WRITE
            : AZ::IO::SystemFile::SF_OPEN_READ_ONLY;

        outputFile.Open(localBookmarkFilePath.c_str(), configurationMode);

        return outputFile.Length() > 0;
    }

    static AZStd::string BookmarkSetRegPath(const AZStd::string& bookmarkFileName)
    {
        return AZStd::string::format("/%s/LocalBookmarks", bookmarkFileName.c_str());
    }

    static AZStd::string BookmarkSetRegPathAtIndex(const AZStd::string& bookmarkFileName, const int index)
    {
        return AZStd::string::format("%s/%i", BookmarkSetRegPath(bookmarkFileName).c_str(), index);
    }

    void LocalViewBookmarkLoader::RegisterViewBookmarkLoaderInterface()
    {
        AZ::Interface<ViewBookmarkLoaderInterface>::Register(this);
    }

    void LocalViewBookmarkLoader::UnregisterViewBookmarkLoaderInterface()
    {
        AZ::Interface<ViewBookmarkLoaderInterface>::Unregister(this);
    }

    void LocalViewBookmarkLoader::WriteBookmarksSettingsRegistryToFile()
    {
        auto registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
            return;
        }

        // resolve path to user project folder
        const auto localBookmarkFilePath = LocalBookmarkFilePath(m_bookmarkFileName);

        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;

        const AZStd::string bookmarkKey = "/" + m_bookmarkFileName;
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
                localBookmarkFilePath.c_str());

            return;
        }

        [[maybe_unused]] bool saved = false;
        constexpr auto configurationMode =
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
        if (AZ::IO::SystemFile outputFile; outputFile.Open(localBookmarkFilePath.c_str(), configurationMode))
        {
            saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
        }

        // once written to the desired file, remove the key from the settings registry
        registry->Remove(bookmarkKey + "/");

        AZ_Warning(
            "LocalViewBookmarkLoader", saved, R"(Unable to save Local View Bookmark file to path "%s"\n)", localBookmarkFilePath.c_str());
    }

    bool LocalViewBookmarkLoader::SaveBookmark(const ViewBookmark& bookmark)
    {
        return SaveLocalBookmark(bookmark, ViewBookmarkType::Standard);
    }

    bool LocalViewBookmarkLoader::ModifyBookmarkAtIndex(const ViewBookmark& bookmark, int index)
    {
        SetupLocalViewBookmarkComponent();

        ReadViewBookmarksFromSettingsRegistry();

        if (index < 0 || index >= DefaultViewBookmarkCount)
        {
            // save default values to view bookmark file
            WriteBookmarksSettingsRegistryToFile();
            return false;
        }

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            // write to the settings registry
            success = registry->SetObject(BookmarkSetRegPathAtIndex(m_bookmarkFileName, index), bookmark);
        }

        if (!success)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "couldn't remove View Bookmark at index %d", index);
            return false;
        }

        // if we managed to add the bookmark
        WriteBookmarksSettingsRegistryToFile();

        return success;
    }

    bool LocalViewBookmarkLoader::SaveLastKnownLocation(const ViewBookmark& bookmark)
    {
        return SaveLocalBookmark(bookmark, ViewBookmarkType::LastKnownLocation);
    }

    bool LocalViewBookmarkLoader::ReadViewBookmarksFromSettingsRegistry()
    {
        struct ViewBookmarkVisitor : AZ::SettingsRegistryInterface::Visitor
        {
            ViewBookmarkVisitor()
                : m_viewBookmarksKey{ ViewBookmarksRegistryPath } {};

            AZ::SettingsRegistryInterface::VisitResponse Traverse(
                AZStd::string_view path,
                AZStd::string_view,
                AZ::SettingsRegistryInterface::VisitAction action,
                AZ::SettingsRegistryInterface::Type) override
            {
                if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
                {
                    // strip off the last json pointer key from the path and if it matches the view bookmark key then add an entry
                    // to the view bookmark map
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

                if (AZStd::optional<AZStd::string_view> localBookmarksId = AZ::StringFunc::TokenizeLast(path, "/");
                    localBookmarksId.has_value() && path == m_viewBookmarksKey && !localBookmarksId->empty())
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
                        auto existingBookmarkEntry = m_bookmarkMap.find(localBookmarksId.value());
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
                                const int bookmarkIndex = stoi(AZStd::string(bookmarkIndexStr->data()));
                                AZ_Assert(bookmarkIndex < bookmarks.size(), "Bookmark index is out of bounds");
                                ViewBookmark& bookmark = bookmarks[bookmarkIndex];
                                const int currentIndex = stoi(AZStd::string(valueIndex));

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

            AZ::SettingsRegistryInterface::FixedValueString m_viewBookmarksKey;
            AZStd::unordered_map<AZStd::string, AZStd::vector<ViewBookmark>> m_bookmarkMap;
            ViewBookmark m_lastKnownLocation;
        };

        if (LocalViewBookmarkComponent* bookmarkComponent = RetrieveLocalViewBookmarkComponent())
        {
            // get the file we want to merge into the settings registry
            if (const AZStd::string localBookmarkFileName = bookmarkComponent->GetLocalBookmarksFileName(); !localBookmarkFileName.empty())
            {
                auto registry = AZ::SettingsRegistry::Get();
                if (!registry)
                {
                    AZ_Warning(
                        "LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
                    return false;
                }

                if (EnsureLocalBookmarkFileExists(localBookmarkFileName))
                {
                    // merge the current view bookmark file into the settings registry
                    bool isMerged = registry->MergeSettingsFile(
                        LocalBookmarkFilePath(localBookmarkFileName).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch,
                        ViewBookmarksRegistryPath);

                    if (isMerged)
                    {
                        const AZStd::string bookmarkKey =
                            AZStd::string::format("%s/%s", ViewBookmarksRegistryPath, localBookmarkFileName.c_str());

                        ViewBookmarkVisitor viewBookmarkVisitor;
                        const bool visitedViewBookmarks = registry->Visit(viewBookmarkVisitor, bookmarkKey);

                        if (visitedViewBookmarks)
                        {
                            m_localBookmarks = viewBookmarkVisitor.m_bookmarkMap.at(localBookmarkFileName);
                            m_lastKnownLocation = viewBookmarkVisitor.m_lastKnownLocation;
                        }

                        return visitedViewBookmarks;
                    }
                }
                else
                {
                    // initialize default locations to zero, this is a temporary solution to match the 12 locations of the legacy system
                    // once there is a UI for the view bookmarks these lines should be removed
                    if (const auto setRegPath = BookmarkSetRegPath(m_bookmarkFileName);
                        !registry->SetObject(setRegPath, AZStd::vector<ViewBookmark>(DefaultViewBookmarkCount, ViewBookmark())))
                    {
                        AZ_Warning("LocalViewBookmarkLoader", false, "SetObject for \"%s\" failed", setRegPath.c_str());
                    }
                }
            }
        }

        // remove cached local bookmarks if a view bookmark file could not be loaded
        m_localBookmarks.clear();

        return false;
    }

    AZStd::optional<ViewBookmark> LocalViewBookmarkLoader::LoadBookmarkAtIndex(int index)
    {
        ReadViewBookmarksFromSettingsRegistry();

        if (index >= 0 && index < m_localBookmarks.size())
        {
            return m_localBookmarks[index];
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

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            success = registry->Remove(BookmarkSetRegPathAtIndex(m_bookmarkFileName, index));
        }

        if (!success)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "Couldn't remove View Bookmark at index %d", index);
            return false;
        }

        // if we managed to remove the bookmark
        WriteBookmarksSettingsRegistryToFile();

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

        // if we didn't find a component then we add it and return it.
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

        const auto prefabTemplate = prefabSystemComponent->FindTemplate(rootPrefabTemplateId);
        const AZStd::string prefabTemplateName = prefabTemplate->get().GetFilePath().Filename().Stem().Native();

        // to generate the file name in which we will store the view Bookmarks we use the name of the prefab + the timestamp
        // e.g. LevelName_1639763579377.setreg
        const AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
        const AZStd::string timeSinceEpoch = AZStd::string::format("%llu", now.time_since_epoch().count());
        return prefabTemplateName.data() + AZStd::string("_") + timeSinceEpoch + ".setreg";
    }

    void LocalViewBookmarkLoader::SetupLocalViewBookmarkComponent()
    {
        LocalViewBookmarkComponent* bookmarkComponent = RetrieveLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Warning("LocalViewBookmarkLoader", false, "Couldn't find a LocalViewBookmarkComponent");
            return;
        }

        // record an undo step if a local view bookmark component is added and configured
        ScopedUndoBatch undoBatch("SetupLocalViewBookmarks");
        undoBatch.MarkEntityDirty(bookmarkComponent->GetEntityId());

        // if the field is empty, we don't have a file linked to the prefab, so we create one and we save it in the component
        if (bookmarkComponent->GetLocalBookmarksFileName().empty())
        {
            bookmarkComponent->SetLocalBookmarksFileName(GenerateBookmarkFileName());
        }

        m_bookmarkFileName = bookmarkComponent->GetLocalBookmarksFileName();
    }

    bool LocalViewBookmarkLoader::SaveLocalBookmark(const ViewBookmark& bookmark, const ViewBookmarkType bookmarkType)
    {
        SetupLocalViewBookmarkComponent();

        const AZStd::string setRegPath = [this, bookmarkType]
        {
            switch (bookmarkType)
            {
            case ViewBookmarkType::Standard:
                // note: this function is not currently used and will need to be updated when DefaultViewBookmarkCount is removed
                return BookmarkSetRegPathAtIndex(m_bookmarkFileName, aznumeric_cast<int>(m_localBookmarks.size()) + 1);
            case ViewBookmarkType::LastKnownLocation:
                return "/" + m_bookmarkFileName + "/LastKnownLocation";
            default:
                AZ_Assert(false, "Invalid ViewBookmarkType found: %d", bookmarkType);
                return AZStd::string{};
            }
        }();

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            // write to the settings registry
            success = registry->SetObject(setRegPath, bookmark);
        }

        if (!success)
        {
            AZ_Warning(
                "LocalViewBookmarkLoader", false, "View Bookmark x=%.4f, y=%.4f, z=%.4f couldn't be saved", bookmark.m_position.GetX(),
                bookmark.m_position.GetY(), bookmark.m_position.GetZ());
            return false;
        }

        // if we managed to add the bookmark
        WriteBookmarksSettingsRegistryToFile();

        return success;
    }
} // namespace AzToolsFramework
