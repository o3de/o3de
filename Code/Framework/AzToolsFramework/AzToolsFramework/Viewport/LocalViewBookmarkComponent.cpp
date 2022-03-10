/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkComponent.h>

namespace AzToolsFramework
{
    void LocalViewBookmarkComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LocalViewBookmarkComponent, EditorComponentBase>()->Field(
                "LocalBookmarkFileName", &LocalViewBookmarkComponent::m_localBookmarksFileName);
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<LocalViewBookmarkComponent>(
                        "Local View Bookmark Component",
                        "The Local View Bookmark Component allows the user to store bookmarks in a custom setreg file.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Local View Bookmarks")
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                    ->Attribute(AZ::Edit::Attributes::Category, "View Bookmarks")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Comment.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Comment.svg")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &LocalViewBookmarkComponent::m_localBookmarksFileName, "Local Bookmarks File Name", "");
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
