/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Prefab/ViewBookmarkComponent.h>

namespace AzToolsFramework
{
    void ViewBookmark::Reflect(AZ::ReflectContext* context)
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

    void EditorViewBookmarks::Reflect(AZ::ReflectContext* context)
    {
        ViewBookmark::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorViewBookmarks>()
                ->Field("LocalBookmarkFileName", &EditorViewBookmarks::m_localBookmarksFileName)
                ->Field("ViewBookmarks", &EditorViewBookmarks::m_viewBookmarks);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorViewBookmarks>("EditorViewBookmarks", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Editor View Bookmarks")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorViewBookmarks::m_localBookmarksFileName, "Local Bookmarks FileName", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorViewBookmarks::m_viewBookmarks, "View Bookmarks", "")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &EditorViewBookmarks::GetBookmarkLabel);
            }
        }
    }

    AZStd::string EditorViewBookmarks::GetBookmarkLabel(int index) const
    {
        return AZStd::string::format("View Bookmark %d", index);
    }

    void ViewBookmarkComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorViewBookmarks::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->RegisterGenericType<EditorViewBookmarks>();

            serializeContext->Class<ViewBookmarkComponent, EditorComponentBase>()->Version(0)->Field(
                "ViewBookmarks", &ViewBookmarkComponent::m_viewBookmark);

            serializeContext->RegisterGenericType<AZStd::vector<AZ::Uuid>>();

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext
                    ->Class<ViewBookmarkComponent>(
                        "View Bookmark Component", "The ViewBookmark Component allows to store bookmarks for a prefab")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                    ->Attribute(AZ::Edit::Attributes::Category, "View Bookmarks")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Comment.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Comment.svg")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ViewBookmarkComponent::m_viewBookmark, "ViewBookmarks", "ViewBookmarks")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false);
            }
        }
    }

    ViewBookmark ViewBookmarkComponent::GetBookmarkAtIndex(int index) const
    {
        auto& bookmarkVector = m_viewBookmark.m_viewBookmarks;
        if (index >= 0 && index < bookmarkVector.size())
        {
            return bookmarkVector[index];
        }
        return ViewBookmark();
    }

    void ViewBookmarkComponent::AddBookmark(ViewBookmark viewBookmark)
    {
        auto& bookmarkVector = m_viewBookmark.m_viewBookmarks;
        bookmarkVector.push_back(viewBookmark);
    }

    void ViewBookmarkComponent::SaveLastKnownLocation([[maybe_unused]]ViewBookmark newLastKnownLocation)
    {
        //m_viewBookmark.m_lastKnownLocation = newLastKnownLocation;
    }

    ViewBookmark ViewBookmarkComponent::GetLastKnownLocation() const
    {
        return ViewBookmark(); // m_viewBookmark.m_lastKnownLocation;
    }

    void ViewBookmarkComponent::ModifyBookmarkAtIndex(int index, ViewBookmark newBookmark)
    {
        auto& bookmarkVector = m_viewBookmark.m_viewBookmarks;
        bookmarkVector[index] = newBookmark;
    }

    AZStd::string ViewBookmarkComponent::GetLocalBookmarksFileName() const
    {
        return m_viewBookmark.m_localBookmarksFileName;
    }

    void ViewBookmarkComponent::SetLocalBookmarksFileName(const AZStd::string& localBookmarksFileName)
    {
        m_viewBookmark.m_localBookmarksFileName = localBookmarksFileName;
    }

    void ViewBookmarkComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("EditoViewbookmarkingService"));
    }

    void ViewBookmarkComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("EditoViewbookmarkingService"));
    }

} // namespace AzToolsFramework
