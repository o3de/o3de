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
#pragma once

#include <qrect.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>

#include <AzCore/Math/Vector2.h>

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/set.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>

namespace GraphCanvas
{
    class BookmarkManagerComponent
        : public AZ::Component
        , public BookmarkManagerRequestBus::Handler
        , public BookmarkNotificationBus::MultiHandler
    {
    public:
        AZ_COMPONENT(BookmarkManagerComponent, "{A8F08DEA-0F42-4236-9E1E-B93C964B113F}");
        static void Reflect(AZ::ReflectContext* context);
        
        BookmarkManagerComponent();
        ~BookmarkManagerComponent();

        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // BookmarkNotifications
        ////
        
        // BookmarkRequestBus
        bool CreateBookmarkAnchor(const AZ::Vector2& position, int bookmarkIndex) override;

        void RegisterBookmark(const AZ::EntityId& bookmarkId) override;
        void UnregisterBookmark(const AZ::EntityId& bookmarkId) override;

        bool IsBookmarkRegistered(const AZ::EntityId& bookmarkId) const override;
        AZ::EntityId FindBookmarkForShortcut(int shortcut) const override;

        bool RequestShortcut(const AZ::EntityId& bookmarkId, int quickIndex) override;        

        void ActivateShortcut(int index) override;
        void JumpToBookmark(const AZ::EntityId& bookmarkId) override;
        ////
        
    private:

        void UnregisterShortcut(const AZ::EntityId& bookmarkId);

        AZStd::fixed_vector<AZ::EntityId, 10>              m_shortcuts;
        AZStd::set<AZ::EntityId>                           m_bookmarks;
    };
}