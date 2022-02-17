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
            EditorViewBookmarks::Reflect(context);

            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->RegisterGenericType<EditorViewBookmarks>();

                serializeContext->Class<ViewBookmarkComponent, EditorComponentBase>()
                    ->Version(0)
                    ->Field("ViewBookmarks", &ViewBookmarkComponent::m_viewBookmark);

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


    } // namespace Prefab
} // namespace AzToolsFramework
