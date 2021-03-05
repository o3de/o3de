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

#include <QPainter>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/ColorUtils.h>

namespace GraphCanvas
{    
    ////////////////////////////
    // BookmarkAnchorComponent
    ////////////////////////////

    void BookmarkAnchorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<BookmarkAnchorComponentSaveData>()
                ->Version(3)
                    ->Field("QuickIndex", &BookmarkAnchorComponentSaveData::m_shortcut)
                    ->Field("Name", &BookmarkAnchorComponentSaveData::m_bookmarkName)
                    ->Field("Color", &BookmarkAnchorComponentSaveData::m_color)
                    ->Field("Position", &BookmarkAnchorComponentSaveData::m_position)
                    ->Field("Dimension", &BookmarkAnchorComponentSaveData::m_dimension)
                ;

            serializeContext->Class<BookmarkAnchorComponent, GraphCanvasPropertyComponent>()
                ->Version(1)
                    ->Field("SaveData", &BookmarkAnchorComponent::m_saveData)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<BookmarkAnchorComponentSaveData>("BookmarkAnchorComponent", "The Save data utilized by the BookmarkAnchorComponent")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BookmarkAnchorComponentSaveData::m_bookmarkName, "Bookmark Name", "The name associated with the given Bookmark Anchor")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BookmarkAnchorComponentSaveData::OnBookmarkNameChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BookmarkAnchorComponentSaveData::m_color, "Color", "The color associated with the given Bookmark Anchor")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BookmarkAnchorComponentSaveData::OnBookmarkColorChanged)
                    ;

                editContext->Class<BookmarkAnchorComponent>("BookmarkAnchorComponent", "The Save data utilized by the BookmarkAnchorComponent")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BookmarkAnchorComponent::m_saveData, "Save Data", "Save Data")
                    ;
            }
        }
    }

    BookmarkAnchorComponent::BookmarkAnchorComponent()
        : m_saveData(this)
    {
    }

    void BookmarkAnchorComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();
    }

    void BookmarkAnchorComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();

        BookmarkRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void BookmarkAnchorComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        EntitySaveDataRequestBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        BookmarkRequestBus::Handler::BusDisconnect();
        SceneBookmarkRequestBus::Handler::BusDisconnect();
    }

    void BookmarkAnchorComponent::OnSceneSet(const AZ::EntityId& sceneId)
    {
        BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RegisterBookmark, GetEntityId());
        SceneBookmarkRequestBus::Handler::BusConnect(sceneId);

        if (m_saveData.m_bookmarkName.empty())
        {
            EditorId editorId;
            SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);

            AZ::u32 bookmarkId = 0;
            SceneBookmarkActionBus::EventResult(bookmarkId, sceneId, &SceneBookmarkActions::GetNewBookmarkCounter);

            m_saveData.m_bookmarkName = AZStd::string::format("Bookmark #%u", bookmarkId);

            AZ::EntityId viewId;
            SceneRequestBus::EventResult(viewId, sceneId, &SceneRequests::GetViewId);

            QRectF viewport;
            ViewRequestBus::EventResult(viewport, viewId, &ViewRequests::GetViewableAreaInSceneCoordinates);

            m_saveData.SetVisibleArea(viewport);
        }
    }

    void BookmarkAnchorComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
    {
        SceneBookmarkRequestBus::Handler::BusDisconnect(sceneId);
        BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::UnregisterBookmark, GetEntityId());
    }

    void BookmarkAnchorComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphSerialization&)
    {
        AZ::EntityId conflictId;
        BookmarkManagerRequestBus::EventResult(conflictId, graphId, &BookmarkManagerRequests::FindBookmarkForShortcut, m_saveData.m_shortcut);
        if (m_saveData.m_shortcut < 0 || conflictId.IsValid())
        {
            // If we have a conflict. We're going to copy the 'spirit' of the bookmark
            // Rather then the actual bookmark.
            //
            // This means we will re-randomize the color, assign a new shortcut, and a default name
            // If we do not have a shortcut, we'll do this anyway since we can't be sure what state we are currently in.
            m_saveData.m_shortcut = k_findShortcut;
            m_saveData.m_color = ColorUtils::GetRandomColor();

            AZ::u32 bookmarkId = 0;
            SceneBookmarkActionBus::EventResult(bookmarkId, graphId, &SceneBookmarkActions::GetNewBookmarkCounter);

            m_saveData.m_bookmarkName = AZStd::string::format("Bookmark #%u", bookmarkId);
        }
    }

    AZ::EntityId BookmarkAnchorComponent::GetBookmarkId() const
    {
        return GetEntityId();
    }

    void BookmarkAnchorComponent::RemoveBookmark()
    {
        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        if (graphId.IsValid())
        {
            AZStd::unordered_set<AZ::EntityId> deleteIds = { GetEntityId() };
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deleteIds);
        }
    }

    int BookmarkAnchorComponent::GetShortcut() const
    {
        return m_saveData.m_shortcut;
    }

    void BookmarkAnchorComponent::SetShortcut(int shortcut)
    {
        m_saveData.m_shortcut = shortcut;
    }

    AZStd::string BookmarkAnchorComponent::GetBookmarkName() const
    {
        return m_saveData.m_bookmarkName;
    }

    void BookmarkAnchorComponent::SetBookmarkName(const AZStd::string& bookmarkName)
    {
        m_saveData.m_bookmarkName = bookmarkName;
        OnBookmarkNameChanged();
    }

    QRectF BookmarkAnchorComponent::GetBookmarkTarget() const
    {
        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, graphId, &SceneRequests::GetEditorId);

        bool trackVisibleArea = false;
        AssetEditorSettingsRequestBus::EventResult(trackVisibleArea, editorId, &AssetEditorSettingsRequests::IsBookmarkViewportControlEnabled);

        QGraphicsItem* graphicsItem;
        SceneMemberUIRequestBus::EventResult(graphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (trackVisibleArea && m_saveData.HasVisibleArea())
        {
            return m_saveData.GetVisibleArea(graphicsItem->pos());
        }
        else
        {
            return graphicsItem->sceneBoundingRect();
        }
    }

    QColor BookmarkAnchorComponent::GetBookmarkColor() const
    {
        return ConversionUtils::AZToQColor(m_saveData.m_color);
    }

    void BookmarkAnchorComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {        
        BookmarkAnchorComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<BookmarkAnchorComponentSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void BookmarkAnchorComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        const BookmarkAnchorComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<BookmarkAnchorComponentSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    void BookmarkAnchorComponent::OnBookmarkNameChanged()
    {
        BookmarkNotificationBus::Event(GetEntityId(), &BookmarkNotifications::OnBookmarkNameChanged);        
    }

    void BookmarkAnchorComponent::OnBookmarkColorChanged()
    {
        BookmarkNotificationBus::Event(GetEntityId(), &BookmarkNotifications::OnBookmarkColorChanged);
    }
}
