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
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <Entity/PrefabEditorEntityOwnershipInterface.h>
#include <Prefab/PrefabSystemComponentInterface.h>
#include <Viewport/LocalViewBookmarkComponent.h>
#include <Viewport/LocalViewBookmarkLoader.h>

namespace AzToolsFramework
{
    static constexpr const char* ViewBookmarksRegistryPath = "/O3DE/ViewBookmarks";
    static constexpr const char* LocalBookmarksKey = "LocalBookmarks";
    static constexpr const char* LastKnownLocationKey = "LastKnownLocation";

    struct ViewBookmarkVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        ViewBookmarkVisitor()
            : m_viewBookmarksKey{ ViewBookmarksRegistryPath } {};

        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, double value) override
        {
            AZStd::string_view jsonKeyPath = visitArgs.m_jsonKeyPath;
            AZ::StringFunc::TokenizeLast(jsonKeyPath, "/");
            AZStd::optional<AZStd::string_view> dataType = AZ::StringFunc::TokenizeLast(jsonKeyPath, "/");
            AZStd::optional<AZStd::string_view> bookmarkIndexStr = AZ::StringFunc::TokenizeLast(jsonKeyPath, "/");
            AZStd::optional<AZStd::string_view> bookmarkType;
            if (bookmarkIndexStr == LastKnownLocationKey)
            {
                bookmarkType = bookmarkIndexStr;
            }
            else
            {
                // differentiate between local Bookmarks and LastKnownLocation
                bookmarkType = AZ::StringFunc::TokenizeLast(jsonKeyPath, "/");
            }

            if (AZStd::optional<AZStd::string_view> localBookmarksId = AZ::StringFunc::TokenizeLast(jsonKeyPath, "/");
                localBookmarksId.has_value() && jsonKeyPath == m_viewBookmarksKey && !localBookmarksId->empty())
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
                        AZ_Error(
                            "LocalViewBookmarkLoader", false, "Trying to set an invalid index in a Vector3, index = %d", currentIndex);
                        break;
                    }
                };

                if (bookmarkType == LastKnownLocationKey)
                {
                    if (!m_lastKnownLocation.has_value())
                    {
                        m_lastKnownLocation.emplace(ViewBookmark{});
                    }

                    const int currentIndex = AZStd::stoi(AZStd::string(visitArgs.m_fieldName));
                    if (dataType == "Position")
                    {
                        setVec3Fn(m_lastKnownLocation->m_position, currentIndex);
                    }
                    else if (dataType == "Rotation")
                    {
                        setVec3Fn(m_lastKnownLocation->m_rotation, currentIndex);
                    }
                }
                else if (bookmarkType == LocalBookmarksKey)
                {
                    if (auto existingBookmarkEntry = m_bookmarkMap.find(localBookmarksId.value());
                        existingBookmarkEntry != m_bookmarkMap.end())
                    {
                        AZStd::vector<ViewBookmark>& bookmarks = existingBookmarkEntry->second;
                        // if it is the first bookmark and it is the Position data it means it is the first one
                        // and we have to create the Bookmark.
                        if (visitArgs.m_fieldName == "0" && dataType == "Position")
                        {
                            ViewBookmark bookmark;
                            bookmark.m_position.SetX(aznumeric_cast<float>(value));
                            bookmarks.push_back(bookmark);
                        }
                        else
                        {
                            const int bookmarkIndex = AZStd::stoi(AZStd::string(bookmarkIndexStr->data()));
                            AZ_Assert(bookmarkIndex >= 0 && bookmarkIndex < bookmarks.size(), "Bookmark index is out of bounds");
                            ViewBookmark& bookmark = bookmarks[bookmarkIndex];
                            const int currentIndex = AZStd::stoi(AZStd::string(visitArgs.m_fieldName));

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
        }

        AZ::SettingsRegistryInterface::FixedValueString m_viewBookmarksKey;
        AZStd::unordered_map<AZStd::string, AZStd::vector<ViewBookmark>> m_bookmarkMap;
        AZStd::optional<ViewBookmark> m_lastKnownLocation;
    };

    static AZ::IO::FixedMaxPath LocalBookmarkFilePath(const AZ::IO::PathView& bookmarkFileName)
    {
        return AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks/" / bookmarkFileName;
    }

    // returns true if the file already existed, false otherwise
    static bool EnsureLocalBookmarkFileExists(const AZ::IO::PathView& bookmarkFileName)
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

    static AZStd::string LocalViewBookmarkSetRegPath(const AZ::IO::PathView& bookmarkFileName)
    {
        return AZStd::string::format("/%.*s/LocalBookmarks", AZ_STRING_ARG(bookmarkFileName.Native()));
    }

    static AZStd::string LocalViewBookmarkSetRegPathAtIndex(const AZ::IO::PathView& bookmarkFileName, const int index)
    {
        return AZStd::string::format("%s/%i", LocalViewBookmarkSetRegPath(bookmarkFileName).c_str(), index);
    }

    static AZStd::string LastKnownLocationSetRegPath(const AZ::IO::PathView& bookmarkFileName)
    {
        return AZStd::string::format("/%.*s/LastKnownLocation", AZ_STRING_ARG(bookmarkFileName.Native()));
    }

    static AZ::IO::Path GenerateBookmarkFileName()
    {
        auto* prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(prefabEditorEntityOwnershipInterface != nullptr, "PrefabEditorEntityOwnershipInterface is not found.");
        Prefab::TemplateId rootPrefabTemplateId = prefabEditorEntityOwnershipInterface->GetRootPrefabTemplateId();

        if (rootPrefabTemplateId == Prefab::InvalidTemplateId)
        {
            return {};
        }

        auto* prefabSystemComponent = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(
            prefabSystemComponent != nullptr,
            "Prefab System Component Interface could not be found. "
            "It is a requirement for the LocalViewBookmarkLoader class. "
            "Check that it is being correctly initialized.");

        const auto prefabTemplate = prefabSystemComponent->FindTemplate(rootPrefabTemplateId);
        const AZ::IO::Path prefabTemplateName = prefabTemplate->get().GetFilePath().Filename().Stem();

        // to generate the file name in which we will store the view Bookmarks we use the name of the prefab + the timestamp
        // e.g. LevelName_1639763579377.setreg
        const AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
        AZStd::fixed_string<32> wallClockMicroseconds;
        AZStd::to_string(wallClockMicroseconds, now.time_since_epoch().count());
        return AZStd::string::format("%s_%s.setreg", prefabTemplateName.c_str(), wallClockMicroseconds.c_str());
    }

    static AZ::SettingsRegistryMergeUtils::DumperSettings CreateDumperSettings(AZStd::string_view bookmarkKey)
    {
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;
        dumperSettings.m_includeFilter = [bookmarkKey = AZStd::string(bookmarkKey)](AZStd::string_view path)
        {
            AZStd::string_view o3dePrefixPath(bookmarkKey);
            return o3dePrefixPath.starts_with(path.substr(0, o3dePrefixPath.size()));
        };
        return dumperSettings;
    }

    void LocalViewBookmarkLoader::RegisterViewBookmarkInterface()
    {
        AZ::Interface<ViewBookmarkInterface>::Register(this);
        AZ::Interface<ViewBookmarkPersistInterface>::Register(this);

        m_streamWriteFn = [](AZ::IO::PathView localBookmarksFileName, AZStd::string_view stringBuffer,
                             AZStd::function<bool(AZ::IO::GenericStream&, AZStd::string_view)> writeFn)
        {
            constexpr auto configurationMode =
                AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite;

            // resolve path to user project folder
            const auto localBookmarkFilePath = LocalBookmarkFilePath(localBookmarksFileName);

            AZ::IO::SystemFileStream systemFileStream(localBookmarkFilePath.c_str(), configurationMode);
            const bool saved = writeFn(systemFileStream, stringBuffer);

            AZ_Warning(
                "LocalViewBookmarkLoader", saved, R"(Unable to save Local View Bookmark file to path "%s"\n)",
                localBookmarkFilePath.c_str());

            return saved;
        };

        m_streamReadFn = [](const AZ::IO::PathView localBookmarksFileName)
        {
            const auto localBookmarkFilePath = LocalBookmarkFilePath(localBookmarksFileName);
            const auto fileSize = AZ::IO::SystemFile::Length(localBookmarkFilePath.c_str());
            AZStd::vector<char> buffer(fileSize + 1);
            buffer[fileSize] = '\0';
            if (!AZ::IO::SystemFile::Read(localBookmarkFilePath.c_str(), buffer.data()))
            {
                return AZStd::vector<char>{};
            }
            return buffer;
        };

        m_fileExistsFn = [](const AZ::IO::PathView& localBookmarksFileName)
        {
            return EnsureLocalBookmarkFileExists(localBookmarksFileName);
        };
    }

    void LocalViewBookmarkLoader::UnregisterViewBookmarkInterface()
    {
        AZ::Interface<ViewBookmarkPersistInterface>::Unregister(this);
        AZ::Interface<ViewBookmarkInterface>::Unregister(this);
    }

    void LocalViewBookmarkLoader::WriteViewBookmarksSettingsRegistryToFile(const AZ::IO::PathView& localBookmarksFileName)
    {
        auto registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
            return;
        }

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        auto bookmarkKey = FixedValueString("/");
        bookmarkKey += localBookmarksFileName.Native();

        AZStd::string stringBuffer;
        AZ::IO::ByteContainerStream stringStream(&stringBuffer);
        if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, "", stringStream, CreateDumperSettings(bookmarkKey)))
        {
            AZ_Error(
                "LocalViewBookmarkLoader", false, R"(Unable to dump SettingsRegistry to stream "%.*s"\n)", AZ_STRING_ARG(localBookmarksFileName.Native()));
            return;
        }

        [[maybe_unused]] const bool saved = m_streamWriteFn(
            localBookmarksFileName, stringBuffer,
            [this](AZ::IO::GenericStream& genericStream, AZStd::string_view stringBuffer)
            {
                return Write(genericStream, stringBuffer);
            });

        AZ_Warning(
            "LocalViewBookmarkLoader", saved, R"(Unable to save Local View Bookmark file to path "%.*s"\n)", AZ_STRING_ARG(localBookmarksFileName.Native()));

        // once written to the desired file, remove the key from the settings registry
        registry->Remove(bookmarkKey);
    }

    bool LocalViewBookmarkLoader::Write(AZ::IO::GenericStream& genericStream, AZStd::string_view stringBuffer)
    {
        return genericStream.Write(stringBuffer.size(), stringBuffer.data()) == stringBuffer.size();
    }

    bool LocalViewBookmarkLoader::SaveBookmarkAtIndex(const ViewBookmark& bookmark, const int index)
    {
        if (index < 0 || index >= DefaultViewBookmarkCount)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "Attempting to save bookmark at invalid index");
            return false;
        }

        auto registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
            return false;
        }

        const LocalViewBookmarkComponent* bookmarkComponent = FindOrCreateLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Error("LocalViewBookmarkLoader", bookmarkComponent, "Couldn't find or create LocalViewBookmarkComponent.");
            return false;
        }

        const AZ::IO::PathView localBookmarksFileName(bookmarkComponent->GetLocalBookmarksFileName());
        ReadViewBookmarksSettingsRegistryFromFile(localBookmarksFileName);

        // write to the settings registry
        const bool success = registry->SetObject(LocalViewBookmarkSetRegPathAtIndex(localBookmarksFileName, index), bookmark);

        if (!success)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "couldn't remove View Bookmark at index %d", index);
            return false;
        }

        // if we managed to add the bookmark
        WriteViewBookmarksSettingsRegistryToFile(localBookmarksFileName);

        return success;
    }

    bool LocalViewBookmarkLoader::SaveLastKnownLocation(const ViewBookmark& bookmark)
    {
        const LocalViewBookmarkComponent* bookmarkComponent = FindOrCreateLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Error("LocalViewBookmarkLoader", bookmarkComponent, "Couldn't find or create LocalViewBookmarkComponent.");
            return false;
        }

        const AZ::IO::PathView localBookmarksFileName(bookmarkComponent->GetLocalBookmarksFileName());

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            // write to the settings registry
            success = registry->SetObject(LastKnownLocationSetRegPath(localBookmarksFileName), bookmark);
        }

        if (!success)
        {
            AZ_Error(
                "LocalViewBookmarkLoader", false, "View Bookmark x=%.4f, y=%.4f, z=%.4f couldn't be saved", bookmark.m_position.GetX(),
                bookmark.m_position.GetY(), bookmark.m_position.GetZ());

            return false;
        }

        // if we managed to add the bookmark
        WriteViewBookmarksSettingsRegistryToFile(localBookmarksFileName);

        return success;
    }

    bool LocalViewBookmarkLoader::ReadViewBookmarksSettingsRegistryFromFile(const AZ::IO::PathView& localBookmarksFileName)
    {
        auto registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
            return false;
        }

        // merge the current view bookmark file into the settings registry
        if (registry->MergeSettings(
                m_streamReadFn(localBookmarksFileName), AZ::SettingsRegistryInterface::Format::JsonMergePatch, ViewBookmarksRegistryPath))
        {
            ViewBookmarkVisitor viewBookmarkVisitor;
            auto VisitBookmarkKey = [&registry, &viewBookmarkVisitor](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                // The field name is the bookmarkId
                AZStd::string_view localBookmarksId = visitArgs.m_fieldName;
                if (const auto existingBookmarkEntry = viewBookmarkVisitor.m_bookmarkMap.find(localBookmarksId);
                    existingBookmarkEntry == viewBookmarkVisitor.m_bookmarkMap.end())
                {
                    viewBookmarkVisitor.m_bookmarkMap.emplace(localBookmarksId, AZStd::vector<ViewBookmark>{});
                    // Visit each localBookmarkId path
                    registry->Visit(viewBookmarkVisitor, visitArgs.m_jsonKeyPath);

                }
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };

            const bool visitedViewBookmarks = AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, VisitBookmarkKey, ViewBookmarksRegistryPath);

            if (visitedViewBookmarks)
            {
                // update local cache
                m_localBookmarks = viewBookmarkVisitor.m_bookmarkMap[localBookmarksFileName.Native()];
                m_lastKnownLocation = viewBookmarkVisitor.m_lastKnownLocation;

                // update in-memory settings registry
                registry->SetObject(LocalViewBookmarkSetRegPath(localBookmarksFileName), m_localBookmarks);
                if (m_lastKnownLocation.has_value())
                {
                    registry->SetObject(LastKnownLocationSetRegPath(localBookmarksFileName), m_lastKnownLocation.value());
                }
            }

            return visitedViewBookmarks;
        }

        // remove cached local bookmarks if a view bookmark file could not be loaded
        m_localBookmarks.clear();
        m_lastKnownLocation.reset();

        return false;
    }

    AZStd::optional<ViewBookmark> LocalViewBookmarkLoader::LoadBookmarkAtIndex(const int index)
    {
        const LocalViewBookmarkComponent* bookmarkComponent = FindOrCreateLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "Couldn't find or create LocalViewBookmarkComponent.");
            return AZStd::nullopt;
        }

        ReadViewBookmarksSettingsRegistryFromFile(AZ::IO::PathView(bookmarkComponent->GetLocalBookmarksFileName()));

        if (index >= 0 && index < m_localBookmarks.size())
        {
            return m_localBookmarks[index];
        }

        AZ_Error("LocalViewBookmarkLoader", false, "Couldn't load View Bookmark from file.");
        return AZStd::nullopt;
    }

    bool LocalViewBookmarkLoader::RemoveBookmarkAtIndex(const int index)
    {
        if (index < 0 || index >= DefaultViewBookmarkCount)
        {
            return false;
        }

        const LocalViewBookmarkComponent* bookmarkComponent = FindOrCreateLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Error("LocalViewBookmarkLoader", bookmarkComponent, "Couldn't find or create LocalViewBookmarkComponent.");
            return false;
        }

        const AZ::IO::PathView localBookmarksFileName(bookmarkComponent->GetLocalBookmarksFileName());

        bool success = false;
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            success = registry->Remove(LocalViewBookmarkSetRegPathAtIndex(localBookmarksFileName, index));
        }

        if (!success)
        {
            AZ_Error("LocalViewBookmarkLoader", false, "Couldn't remove View Bookmark at index %d", index);
            return false;
        }

        // if we managed to remove the bookmark
        WriteViewBookmarksSettingsRegistryToFile(localBookmarksFileName);

        return success;
    }

    AZStd::optional<ViewBookmark> LocalViewBookmarkLoader::LoadLastKnownLocation()
    {
        const LocalViewBookmarkComponent* bookmarkComponent = FindOrCreateLocalViewBookmarkComponent();
        if (!bookmarkComponent)
        {
            AZ_Error("LocalViewBookmarkLoader", bookmarkComponent, "Couldn't find or create LocalViewBookmarkComponent.");
            return AZStd::nullopt;
        }

        ReadViewBookmarksSettingsRegistryFromFile(AZ::IO::PathView(bookmarkComponent->GetLocalBookmarksFileName()));

        return m_lastKnownLocation;
    }

    LocalViewBookmarkComponent* LocalViewBookmarkLoader::FindOrCreateLocalViewBookmarkComponent()
    {
        auto prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        if (!prefabEditorEntityOwnershipInterface)
        {
            return nullptr;
        }

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
        {
            // record an undo step if a local view bookmark component is added and configured
            ScopedUndoBatch undoBatch("SetupLocalViewBookmarks");

            if (!bookmarkComponent)
            {
                undoBatch.MarkEntityDirty(containerEntityId);

                // if we didn't find a component then we add it and return it.
                containerEntity->Deactivate();
                bookmarkComponent = containerEntity->CreateComponent<LocalViewBookmarkComponent>();
                containerEntity->Activate();

                AZ_Assert(bookmarkComponent, "Couldn't create LocalViewBookmarkComponent.");
            }

            // if the field is empty, we don't have a file linked to the prefab, so we create one and we save it in the component
            if (bookmarkComponent->GetLocalBookmarksFileName().empty())
            {
                undoBatch.MarkEntityDirty(containerEntityId);
                bookmarkComponent->SetLocalBookmarksFileName(GenerateBookmarkFileName().Native());
            }
        }

        if (const auto localBookmarksFileName = AZ::IO::PathView(bookmarkComponent->GetLocalBookmarksFileName()); !m_fileExistsFn(localBookmarksFileName))
        {
            auto registry = AZ::SettingsRegistry::Get();
            if (!registry)
            {
                AZ_Error("LocalViewBookmarkLoader", false, "Unable to access global settings registry.");
                return nullptr;
            }

            // initialize default locations to zero, this is a temporary solution to match the 12 locations of the legacy system
            // once there is a UI for the view bookmarks these lines should be removed
            if (const auto setRegPath = LocalViewBookmarkSetRegPath(localBookmarksFileName);
                !registry->SetObject(setRegPath, AZStd::vector<ViewBookmark>(DefaultViewBookmarkCount, ViewBookmark())))
            {
                AZ_Error("LocalViewBookmarkLoader", false, "SetObject for \"%s\" failed", setRegPath.c_str());
                return nullptr;
            }

            WriteViewBookmarksSettingsRegistryToFile(localBookmarksFileName);
        }

        return bookmarkComponent;
    }

    void LocalViewBookmarkLoader::OverrideStreamWriteFn(StreamWriteFn streamWriteFn)
    {
        m_streamWriteFn = AZStd::move(streamWriteFn);
    }

    void LocalViewBookmarkLoader::OverrideStreamReadFn(StreamReadFn streamReadFn)
    {
        m_streamReadFn = AZStd::move(streamReadFn);
    }

    void LocalViewBookmarkLoader::OverrideFileExistsFn(FileExistsFn fileExistsFn)
    {
        m_fileExistsFn = AZStd::move(fileExistsFn);
    }

    static AZ::Vector3 RadiansToDegrees(const AZ::Vector3& radians)
    {
        return AZ::Vector3(AZ::RadToDeg(radians.GetX()), AZ::RadToDeg(radians.GetY()), AZ::RadToDeg(radians.GetZ()));
    }

    static ViewBookmark ViewBookmarkFromCameraState(const AzFramework::CameraState& cameraState)
    {
        return { cameraState.m_position,
                 RadiansToDegrees(AzFramework::EulerAngles(
                     AZ::Matrix3x3::CreateFromColumns(cameraState.m_side, cameraState.m_forward, cameraState.m_up))) };
    }

    AZStd::optional<ViewBookmark> StoreViewBookmarkLastKnownLocationFromActiveCamera()
    {
        bool found = false;
        AzFramework::CameraState cameraState;
        Camera::EditorCameraRequestBus::BroadcastResult(found, &Camera::EditorCameraRequestBus::Events::GetActiveCameraState, cameraState);

        if (found)
        {
            return StoreViewBookmarkLastKnownLocationFromCameraState(cameraState);
        }

        return AZStd::nullopt;
    }

    AZStd::optional<ViewBookmark> StoreViewBookmarkLastKnownLocationFromCameraState(const AzFramework::CameraState& cameraState)
    {
        ViewBookmarkInterface* viewBookmarkInterface = AZ::Interface<ViewBookmarkInterface>::Get();

        if (const ViewBookmark viewBookmark = ViewBookmarkFromCameraState(cameraState);
            viewBookmarkInterface->SaveLastKnownLocation(viewBookmark))
        {
            return viewBookmark;
        }

        return AZStd::nullopt;
    }

    AZStd::optional<ViewBookmark> StoreViewBookmarkFromActiveCameraAtIndex(const int index)
    {
        bool found = false;
        AzFramework::CameraState cameraState;
        Camera::EditorCameraRequestBus::BroadcastResult(found, &Camera::EditorCameraRequestBus::Events::GetActiveCameraState, cameraState);

        if (found)
        {
            return StoreViewBookmarkFromCameraStateAtIndex(index, cameraState);
        }

        return AZStd::nullopt;
    }

    AZStd::optional<ViewBookmark> StoreViewBookmarkFromCameraStateAtIndex(const int index, const AzFramework::CameraState& cameraState)
    {
        ViewBookmarkInterface* viewBookmarkInterface = AZ::Interface<ViewBookmarkInterface>::Get();

        if (const ViewBookmark viewBookmark = ViewBookmarkFromCameraState(cameraState);
            viewBookmarkInterface->SaveBookmarkAtIndex(ViewBookmarkFromCameraState(cameraState), index))
        {
            return viewBookmark;
        }

        return AZStd::nullopt;
    }
} // namespace AzToolsFramework
