/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkComponent.h>
#include <Viewport/ViewBookmarkLoaderInterface.h>

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

    void LocalViewBookmarkComponent::Reflect(AZ::ReflectContext* context)
    {
        ViewBookmark::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<AZStd::vector<ViewBookmark>>();

            serializeContext->Class<LocalViewBookmarkComponent, EditorComponentBase>()->Version(1)->Field(
                "LocalBookmarkFileName", &LocalViewBookmarkComponent::m_localBookmarksFileName);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<LocalViewBookmarkComponent>(
                        "Local View Bookmark Component",
                        "The Local View Bookmark Component allows the user to store bookmarks in a custom setreg file.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Local View Bookmarks")
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                    ->Attribute(AZ::Edit::Attributes::RemoveableByUser, false)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                    ->Attribute(AZ::Edit::Attributes::Category, "View Bookmarks")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &LocalViewBookmarkComponent::m_localBookmarksFileName, "Local Bookmarks File Name",
                        "");
            }
        }
    }

    const AZStd::string& LocalViewBookmarkComponent::GetLocalBookmarksFileName() const
    {
        return m_localBookmarksFileName;
    }

    void LocalViewBookmarkComponent::SetLocalBookmarksFileName(AZStd::string localBookmarksFileName)
    {
        m_localBookmarksFileName = AZStd::move(localBookmarksFileName);
    }
} // namespace AzToolsFramework
