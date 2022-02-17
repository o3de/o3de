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
#include <AzToolsFramework/Prefab/ViewBookmarkComponent.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void ViewBookmarkComponent::Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ViewBookmarkComponent, EditorComponentBase>()
                    ->Version(1)->
                    Field("Configuration", &ViewBookmarkComponent::m_comment);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext
                        ->Class<ViewBookmarkComponent>(
                            "View BookmarkComponent", "The ViewBookmark Component allows to store bookmarks for a prefab")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                        ->Attribute(AZ::Edit::Attributes::Category, "View Bookmarks")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Comment.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Comment.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components")
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &ViewBookmarkComponent::m_comment, "", "Comment")
                        ->Attribute(AZ_CRC("PlaceholderText", 0xa23ec278), "Add comment text here");
                }
            }
        }


    } // namespace Prefab
} // namespace AzToolsFramework
