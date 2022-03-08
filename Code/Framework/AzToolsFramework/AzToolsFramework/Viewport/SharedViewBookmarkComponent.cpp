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
#include <AzToolsFramework/Viewport/SharedViewBookmarkComponent.h>

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

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorViewBookmarks>()
                ->Field("ViewBookmarks", &EditorViewBookmarks::m_viewBookmarks);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorViewBookmarks>("EditorViewBookmarks", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Editor View Bookmarks")
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

    void SharedViewBookmarkComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorViewBookmarks::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<EditorViewBookmarks>();

            serializeContext->Class<SharedViewBookmarkComponent, EditorComponentBase>()->Version(0)->Field(
                "ViewBookmarks", &SharedViewBookmarkComponent::m_viewBookmark);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SharedViewBookmarkComponent>(
                        "Shared View Bookmark Component", "The ViewBookmark Component allows to store bookmarks for a prefab")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                    ->Attribute(AZ::Edit::Attributes::Category, "View Bookmarks")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Comment.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Comment.svg")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SharedViewBookmarkComponent::m_viewBookmark, "ViewBookmarks", "ViewBookmarks")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false);
            }
        }
    }

    AZStd::optional<ViewBookmark> SharedViewBookmarkComponent::GetBookmarkAtIndex(int index) const
    {
        if (index >= 0 && index < m_viewBookmark.m_viewBookmarks.size())
        {
            return AZStd::optional<ViewBookmark>(m_viewBookmark.m_viewBookmarks[index]);
        }
        return AZStd::optional<ViewBookmark>();
    }

    void SharedViewBookmarkComponent::AddBookmark(ViewBookmark viewBookmark)
    {
        m_viewBookmark.m_viewBookmarks.push_back(viewBookmark);
    }

    void SharedViewBookmarkComponent::RemoveBookmarkAtIndex([[maybe_unused]] int index)
    {
    }

    void SharedViewBookmarkComponent::ModifyBookmarkAtIndex(int index, ViewBookmark newBookmark)
    {
        m_viewBookmark.m_viewBookmarks[index] = newBookmark;
    }

    void SharedViewBookmarkComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("EditorViewbookmarkingService"));
    }

    void SharedViewBookmarkComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("EditorViewbookmarkingService"));
    }

} // namespace AzToolsFramework
