/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {

        struct ViewBookmark
        {
            AZ_CLASS_ALLOCATOR(ViewBookmark, AZ::SystemAllocator, 0);
            AZ_RTTI(ViewBookmark, "{522A38D9-6FFF-4B96-BECF-B4D0F7ABCD25}");

            ViewBookmark()
                : m_position(AZ::Vector3::CreateZero())
                , m_rotation(AZ::Vector3::CreateZero())
            {
            }

            static void Reflect(AZ::ReflectContext* context);

            AZ::Vector3 m_position;
            AZ::Vector3 m_rotation;
        };

        struct EditorViewBookmarks final
        {
            AZ_CLASS_ALLOCATOR(EditorViewBookmarks, AZ::SystemAllocator, 0);
            AZ_RTTI(EditorViewBookmarks, "{EA0B8FF9-F706-4115-8226-E3F54F1EE8A1}");

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string GetBookmarkLabel(int index) const;

            ViewBookmark m_lastKnownLocation;
            AZStd::vector<ViewBookmark> m_viewBookmarks;

        };

        class ViewBookmarkComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            static constexpr const char* const ViewBookmarkComponentTypeId = "{6959832F-9382-4C7D-83AC-380DA9F138DE}";

            AZ_COMPONENT(ViewBookmarkComponent, ViewBookmarkComponentTypeId, EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            
            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override{};
            void Deactivate() override{};
            //////////////////////////////////////////////////////////////////////////

            ViewBookmark GetBookmarkAtIndex(int index) const;
            void AddBookmark(ViewBookmark viewBookmark);
            void SaveLastKnownLocation(ViewBookmark newLastKnownLocation);
            ViewBookmark GetLastKnownLocation() const;
            void ModifyBookmarkAtIndex(int index, ViewBookmark newBookmark);

        protected:

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        private:
            // A user editable view Bookmark
            EditorViewBookmarks m_viewBookmark;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
