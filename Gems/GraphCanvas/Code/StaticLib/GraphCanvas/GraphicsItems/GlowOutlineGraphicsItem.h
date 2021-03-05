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

#include <AzCore/PlatformDef.h>
// qbrush.h(118): warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QSequentialAnimationGroup>
#include <QGraphicsItem>
#include <QPen>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/types.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/GraphicsItems/GraphicsEffect.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace GraphCanvas
{
    class GlowOutlineConfiguration
    {
    public:
        qreal m_blurRadius;
        QPen m_pen;

        AZStd::chrono::milliseconds m_pulseRate = AZStd::chrono::milliseconds(0);

        qreal m_zValue = 0;

        qreal m_maxAlpha = 1.0f;
        qreal m_minAlpha = 0.75f;
    };

    class FixedGlowOutlineConfiguration
        : public GlowOutlineConfiguration
    {
    public:
        QPainterPath m_painterPath;
    };

    class SceneMemberGlowOutlineConfiguration
        : public GlowOutlineConfiguration
    {
    public:
        AZ::EntityId m_sceneMember;
    };
    
    class GlowOutlineGraphicsItem
        : public GraphicsEffect<QGraphicsPathItem>
        , public ConnectionVisualNotificationBus::Handler
        , public GeometryNotificationBus::Handler
        , public AZ::TickBus::Handler
        , public AZ::SystemTickBus::Handler
        , public GraphCanvas::ViewNotificationBus::Handler
        , public AssetEditorSettingsNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GlowOutlineGraphicsItem, AZ::SystemAllocator, 0);

        GlowOutlineGraphicsItem(const FixedGlowOutlineConfiguration& configuration);
        GlowOutlineGraphicsItem(const SceneMemberGlowOutlineConfiguration& configuration);

        ~GlowOutlineGraphicsItem() override;

        // ConnectionVisualNotificationBus
        void OnConnectionPathUpdated() override;
        ////

        // SystemTick
        void OnSystemTick();
        ////

        // TickBus
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
        ////

        // GeometryNotificationBus::Handler
        void OnPositionChanged(const AZ::EntityId& /*targetEntity*/, const AZ::Vector2& /*position*/) override;
        void OnBoundsChanged();
        ////

        // ViewNotificationBus
        void OnZoomChanged(qreal zoomLevel) override;
        ////

        // AssetEditorSettingsNotificationBus
        void OnSettingsChanged() override;
        ////

    protected:

        // GraphicsEffectInterface
        void OnEditorIdSet() override;
        ////

    private:

        void ConfigureGlowOutline(const GlowOutlineConfiguration& outlineConfiguration);
        void UpdateOutlinePath();

        QPen m_pen;

        qreal m_defaultPenWidth;

        qreal m_pulseTime;
        qreal m_currentTime;
        qreal m_opacityStart;
        qreal m_opacityEnd;

        QPainterPath m_painterPath;        
        AZ::EntityId m_trackingSceneMember;
    };
}
