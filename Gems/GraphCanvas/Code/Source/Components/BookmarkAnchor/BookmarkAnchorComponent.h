
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qgraphicswidget.h>
#include <qcolor.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    //! Manages all of the start required by the bookmarks
    class BookmarkAnchorComponent
        : public GraphCanvasPropertyComponent
        , public BookmarkRequestBus::Handler
        , public SceneBookmarkRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
        , public BookmarkAnchorComponentSaveDataCallback
    {
    public:
        AZ_COMPONENT(BookmarkAnchorComponent, "{33C63E10-81EE-458D-A716-F63478E57517}");

        static void Reflect(AZ::ReflectContext* reflectContext);

        friend class BookmarkVisualComponentSaveData;

        BookmarkAnchorComponent();
        ~BookmarkAnchorComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        void OnRemovedFromScene(const AZ::EntityId& sceneId) override;

        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphSerialization& serializationTarget) override;
        ////

        // SceneBookmarkRequests
        AZ::EntityId GetBookmarkId() const override;
        ////

        // BookmarkRequestBus
        void RemoveBookmark() override;

        int GetShortcut() const override;
        void SetShortcut(int quickIndex) override;

        AZStd::string GetBookmarkName() const override;
        void SetBookmarkName(const AZStd::string& bookmarkName) override;

        QRectF GetBookmarkTarget() const override;
        QColor GetBookmarkColor() const override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

    protected:
        
        // SaveDataCallback
        void OnBookmarkNameChanged() override;
        void OnBookmarkColorChanged() override;
        ////

    private:

        BookmarkAnchorComponentSaveData m_saveData;

        AZ::EntityId m_sceneId;
    };
}
