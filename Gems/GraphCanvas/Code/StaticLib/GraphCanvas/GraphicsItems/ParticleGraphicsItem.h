/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

// qbrush.h(118): warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QBrush>
#include <QGraphicsItem>
#include <QPen>
#include <QTransform>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/chrono/chrono.h>

#include <GraphCanvas/GraphicsItems/GraphicsEffect.h>

namespace GraphCanvas
{
    namespace Styling
    {
        class StyleHelper;
    }

    class ParticleConfiguration
    {
    public:
        enum class Shape
        {
            Circle,
            Square
        };

        Shape m_particleShape = Shape::Square;

        const Styling::StyleHelper* m_styleHelper = nullptr;
        QColor m_color;

        AZStd::chrono::milliseconds m_lifespan = AZStd::chrono::milliseconds(250);

        bool m_rotate = false;
        float m_rotationSpeed = 0.0f;

        bool m_alphaFade = false;
        AZStd::chrono::milliseconds m_fadeTime = AZStd::chrono::milliseconds(250);
        float m_alphaStart = 1.0f;
        float m_alphaEnd = 0.0f;

        bool m_hasGravity = false;

        QPointF m_initialImpulse = QPoint(0,0);

        QRectF m_boundingArea;
        float m_initialRotation = 0.0f;

        int m_zValue = 0;
    };

    class ParticleGraphicsItem
        : public GraphicsEffect<QGraphicsItem>
        , public AZ::TickBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ParticleGraphicsItem, AZ::SystemAllocator);

        ParticleGraphicsItem(const ParticleConfiguration& particleConfiguration);
        ~ParticleGraphicsItem();

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;
        ////

        // SystemTickBus
        void OnSystemTick() override;
        ////

        // GraphicsItem
        QRectF boundingRect() const override;

        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        ////

    private:

        ParticleConfiguration m_configuration;

        float m_elapsedDuration;
        QPointF m_impulse;

        QRectF m_boundingRect;
        QPainterPath m_clipPath;
    };
}
