/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
// qgraphicsitem.h(450): warning C4251: 'QGraphicsItem::d_ptr': class 'QScopedPointer<QGraphicsItemPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QGraphicsItem'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QGraphicsItem>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/GraphicsItems/GraphicsEffect.h>
#include <GraphCanvas/GraphicsItems/PulseBus.h>

namespace GraphCanvas
{
    class AnimatedPulseControlPoint
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimatedPulseControlPoint, AZ::SystemAllocator);
        
        AnimatedPulseControlPoint(const QPointF& startPoint, const QPointF& endPoint);
        ~AnimatedPulseControlPoint() = default;
        
        QPointF GetPoint(float percent) const;        
        
    private:
    
        QPointF m_startPoint;
        QPointF m_delta;
    };
    
    class AnimatedPulseConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimatedPulseControlPoint, AZ::SystemAllocator);
        
        AnimatedPulseConfiguration();
        ~AnimatedPulseConfiguration() = default;

        bool                                        m_enableGradient;
        QColor                                      m_drawColor;
        float                                       m_durationSec;        
        qreal                                       m_zValue;
        AZStd::vector< AnimatedPulseControlPoint >  m_controlPoints;
    };
    
    class AnimatedPulse
        : public GraphicsEffect<QGraphicsItem>
        , public AZ::TickBus::Handler
        , public AZ::SystemTickBus::Handler
        , public PulseRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimatedPulse, AZ::SystemAllocator);

        AnimatedPulse(const AnimatedPulseConfiguration& pulseConfiguration);
        ~AnimatedPulse() override;

        // SystemTickBus
        void OnSystemTick() override;
        ////
        
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;
        ////
        
        // GraphicsItem
        QRectF boundingRect() const override;
        
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        ////
        
        // GraphicsEffect
        void OnGraphicsEffectCancelled() override;
        ////
        
    private:
    
        QRectF m_boundingRect;
    
        float m_elapsedDuration;        
        AnimatedPulseConfiguration m_configuration;
    };
}
