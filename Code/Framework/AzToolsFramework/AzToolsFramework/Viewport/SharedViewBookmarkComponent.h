/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Viewport/ViewBookmarkLoaderInterface.h>

namespace AzToolsFramework
{
    //! @class EditorViewBookmarks.
    //! @brief struct that stores a vector of View bookmarks.
    struct EditorViewBookmarks final
    {
        AZ_CLASS_ALLOCATOR(EditorViewBookmarks, AZ::SystemAllocator);
        AZ_TYPE_INFO(EditorViewBookmarks, "{EA0B8FF9-F706-4115-8226-E3F54F1EE8A1}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string GetBookmarkLabel(int index) const;

        AZStd::vector<ViewBookmark> m_viewBookmarks;
    };

    //! @class SharedViewBookmarkComponent.
    //! @brief component that stores a vector of View bookmarks stored in the prefab.
    //! so they can be shared in version control easily.
    class SharedViewBookmarkComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        static inline constexpr AZ::TypeId ViewBookmarkComponentTypeId{ "{6959832F-9382-4C7D-83AC-380DA9F138DE}" };

        AZ_EDITOR_COMPONENT(SharedViewBookmarkComponent, ViewBookmarkComponentTypeId, EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::optional<ViewBookmark> GetBookmarkAtIndex(int index) const;
        void AddBookmark(ViewBookmark viewBookmark);
        bool RemoveBookmarkAtIndex(int index);
        bool ModifyBookmarkAtIndex(int index, const ViewBookmark& newBookmark);

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

    private:
        //! A user editable list of View Bookmarks
        EditorViewBookmarks m_viewBookmark;
    };

} // namespace AzToolsFramework
