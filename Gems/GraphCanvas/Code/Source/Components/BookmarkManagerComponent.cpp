/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "precompiled.h"

#include <qgraphicsitem.h>

#include <AzCore/Component/Entity.h>

#include <Components/BookmarkManagerComponent.h>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

namespace GraphCanvas
{
    /////////////////////////////
    // BookmarkManagerComponent
    /////////////////////////////

    void BookmarkManagerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<BookmarkManagerComponent, AZ::Component>()
                ->Version(1)
                ->Field("QuickBookmarks", &BookmarkManagerComponent::m_shortcuts)
                ->Field("Bookmarks", &BookmarkManagerComponent::m_bookmarks)
            ;
        }
    }
    
    BookmarkManagerComponent::BookmarkManagerComponent()
    {        
    }
    
    BookmarkManagerComponent::~BookmarkManagerComponent()
    {
    }

    void BookmarkManagerComponent::Init()
    {
        m_shortcuts.resize(m_shortcuts.max_size());
    }

    void BookmarkManagerComponent::Activate()
    {
        BookmarkManagerRequestBus::Handler::BusConnect(GetEntityId());
    }

    void BookmarkManagerComponent::Deactivate()
    {
        BookmarkManagerRequestBus::Handler::BusDisconnect();
    }

    bool BookmarkManagerComponent::CreateBookmarkAnchor(const AZ::Vector2& position, int bookmarkShortcut)
    {
        AZ::Entity* bookmarkAnchorEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(bookmarkAnchorEntity, &GraphCanvasRequests::CreateBookmarkAnchorAndActivate);

        if (bookmarkAnchorEntity)
        {
            AZ::EntityId bookmarkId = bookmarkAnchorEntity->GetId();
            BookmarkRequestBus::Event(bookmarkId, &BookmarkRequests::SetShortcut, bookmarkShortcut);

            SceneRequestBus::Event(GetEntityId(), &SceneRequests::AddBookmarkAnchor, bookmarkId, position);
        }

        return bookmarkAnchorEntity != nullptr;
    }

    void BookmarkManagerComponent::RegisterBookmark(const AZ::EntityId& bookmarkId)
    {
        auto resultPair = m_bookmarks.insert(bookmarkId);

        if (resultPair.second)
        {
            int shortcut = k_unusedShortcut;

            BookmarkRequestBus::EventResult(shortcut, bookmarkId, &BookmarkRequests::GetShortcut);

            if (shortcut == k_findShortcut)
            {
                for (unsigned int i = 1; i < m_shortcuts.size(); ++i)
                {
                    if (!m_shortcuts[i].IsValid())
                    {
                        shortcut = i;
                        break;
                    }
                }
            }

            if (shortcut > 0)
            {
                RequestShortcut(bookmarkId, shortcut);
            }
            else
            {
                BookmarkRequestBus::Event(bookmarkId, &BookmarkRequests::SetShortcut, k_unusedShortcut);
            }

            BookmarkNotificationBus::MultiHandler::BusConnect(bookmarkId);
            BookmarkManagerNotificationBus::Event(GetEntityId(), &BookmarkManagerNotifications::OnBookmarkAdded, bookmarkId);
        }
    }

    void BookmarkManagerComponent::UnregisterBookmark(const AZ::EntityId& bookmarkId)
    {
        auto bookmarkIter = m_bookmarks.find(bookmarkId);

        AZ_Warning("Graph Canvas", bookmarkIter != m_bookmarks.end(), "Trying to unregister a bookmark from a manager it is not registered to.");
        if (bookmarkIter != m_bookmarks.end())
        {
            UnregisterShortcut(bookmarkId);
            m_bookmarks.erase(bookmarkIter);

            BookmarkNotificationBus::MultiHandler::BusDisconnect(bookmarkId);
            BookmarkManagerNotificationBus::Event(GetEntityId(), &BookmarkManagerNotifications::OnBookmarkRemoved, bookmarkId);
        }
    }

    bool BookmarkManagerComponent::IsBookmarkRegistered(const AZ::EntityId& bookmarkId) const
    {
        return m_bookmarks.count(bookmarkId) > 0;
    }

    AZ::EntityId BookmarkManagerComponent::FindBookmarkForShortcut(int shortcut) const
    {
        if (shortcut <= 0 || shortcut >= m_shortcuts.size())
        {
            return AZ::EntityId();
        }

        return m_shortcuts[shortcut];
    }

    bool BookmarkManagerComponent::RequestShortcut(const AZ::EntityId& bookmark, int shortcut)
    {
        if ((shortcut <= 0 && shortcut != k_unusedShortcut) || shortcut >= m_shortcuts.size())
        {
            return false;
        }

        AZ::EntityId previousBookmark;

        if (shortcut != k_unusedShortcut)
        {
            // If something else is already using these values. Remove the quick index from the old one
            // And assign it to the new one.
            if (m_shortcuts[shortcut].IsValid())
            {
                previousBookmark = m_shortcuts[shortcut];
                BookmarkRequestBus::Event(m_shortcuts[shortcut], &BookmarkRequests::SetShortcut, k_unusedShortcut);
            }
        }

        // Order here matters. Since when creating a new anchor, the index get's preset. We want to track the previous values
        // so we can signal out updates correctly, so we want to unset the previous element first, then try to unset the bookmark's
        // previous. This handles both cases, with minimal extra steps.
        int previousIndex = k_unusedShortcut;
        BookmarkRequestBus::EventResult(previousIndex, bookmark, &BookmarkRequests::GetShortcut);

        UnregisterShortcut(bookmark);

        if (shortcut != k_unusedShortcut)
        {
            m_shortcuts[shortcut] = bookmark;
        }

        BookmarkRequestBus::Event(bookmark, &BookmarkRequests::SetShortcut, shortcut);
        BookmarkManagerNotificationBus::Event(GetEntityId(), &BookmarkManagerNotifications::OnShortcutChanged, shortcut, previousBookmark, bookmark);
        return true;
    }

    void BookmarkManagerComponent::ActivateShortcut(int shortcut)
    {
        JumpToBookmark(FindBookmarkForShortcut(shortcut));
    }
    
    void BookmarkManagerComponent::JumpToBookmark(const AZ::EntityId& bookmark)
    {
        if (!bookmark.IsValid())
        {
            return;
        }

        QRectF bookmarkTarget;
        BookmarkRequestBus::EventResult(bookmarkTarget, bookmark, &BookmarkRequests::GetBookmarkTarget);

        ViewId viewId;
        SceneRequestBus::EventResult(viewId, GetEntityId(), &SceneRequests::GetViewId);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, GetEntityId(), &SceneRequests::GetEditorId);

        bool enableViewportControl = false;
        AssetEditorSettingsRequestBus::EventResult(enableViewportControl, editorId, &AssetEditorSettingsRequests::IsBookmarkViewportControlEnabled);

        if (enableViewportControl)
        {
            ViewRequestBus::Event(viewId, &ViewRequests::DisplayArea, bookmarkTarget);
        }
        else
        {
            ViewRequestBus::Event(viewId, &ViewRequests::CenterOnArea, bookmarkTarget);
        }

        BookmarkNotificationBus::Event(bookmark, &BookmarkNotifications::OnBookmarkTriggered);
    }

    void BookmarkManagerComponent::UnregisterShortcut(const AZ::EntityId& bookmark)
    {
        int previousIndex = k_unusedShortcut;
        BookmarkRequestBus::EventResult(previousIndex, bookmark, &BookmarkRequests::GetShortcut);

        if (previousIndex >= 0 && previousIndex < m_shortcuts.size())
        {
            m_shortcuts[previousIndex].SetInvalid();
        }
    }
}