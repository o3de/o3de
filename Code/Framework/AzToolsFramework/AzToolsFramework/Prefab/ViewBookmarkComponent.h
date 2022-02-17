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

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<ViewBookmark>()
                        ->Version(0)
                        ->Field("Position", &ViewBookmark::m_position)
                        ->Field("Rotation", &ViewBookmark::m_rotation);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<ViewBookmark>("ViewBookmark Data", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "ViewBookmark")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Vector3, &ViewBookmark::m_position, "Position", "")
                            ->DataElement(AZ::Edit::UIHandlers::Vector3, &ViewBookmark::m_rotation, "Rotation", "");
                    }
                }
            }

            AZ::Vector3 m_position;
            AZ::Vector3 m_rotation;
        };

        struct EditorViewBookmarks final
        {
            AZ_CLASS_ALLOCATOR(EditorViewBookmarks, AZ::SystemAllocator, 0);
            AZ_RTTI(EditorViewBookmarks, "{EA0B8FF9-F706-4115-8226-E3F54F1EE8A1}");

            static void Reflect(AZ::ReflectContext* context)
            {
                ViewBookmark::Reflect(context);

                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<EditorViewBookmarks>()
                        ->Field("Last Known Location", &EditorViewBookmarks::m_lastKnownLocation)
                        ->Field("View Bookmarks", &EditorViewBookmarks::m_viewBookmarks);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<EditorViewBookmarks>("EditorViewBookmarks", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Editor View Bookmarks")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorViewBookmarks::m_lastKnownLocation, "Last Known Location", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorViewBookmarks::m_viewBookmarks, "View Bookmarks", "")
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &EditorViewBookmarks::GetBookmarkLabel);
                    }
                }
            }

            AZStd::string GetBookmarkLabel(int index) const
            {
                return AZStd::string::format("View Bookmark %d", index);
            }
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
        protected:

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
            {
                services.push_back(AZ_CRC("EditoViewbookmarkingService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
            {
                services.push_back(AZ_CRC("EditoViewbookmarkingService"));
            }

        private:
            // A user editable view Bookmark
            EditorViewBookmarks m_viewBookmark;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
