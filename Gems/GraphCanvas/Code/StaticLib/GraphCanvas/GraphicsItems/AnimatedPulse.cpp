/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsScene>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/GraphicsItems/AnimatedPulse.h>
#include <GraphCanvas/Utils/QtVectorMath.h>

namespace GraphCanvas
{
    //////////////////////////////
    // AnimatedPulseControlPoint
    //////////////////////////////
    
    AnimatedPulseControlPoint::AnimatedPulseControlPoint(const QPointF& startPoint, const QPointF& endPoint)
        : m_startPoint(startPoint)
    {        
        m_delta = endPoint - startPoint;
    }
    
    QPointF AnimatedPulseControlPoint::GetPoint(float percent) const
    {
        return m_startPoint + m_delta * percent;
    }
    
    ///////////////////////////////
    // AnimatedPulseConfiguration
    ///////////////////////////////
    
    AnimatedPulseConfiguration::AnimatedPulseConfiguration()
        : m_enableGradient(false)
        , m_drawColor(0,0,0)
        , m_durationSec(1.0f)
    {        
    }
    
    //////////////////
    // AnimatedPulse
    //////////////////
    
    AnimatedPulse::AnimatedPulse(const AnimatedPulseConfiguration& pulseConfiguration)
        : GraphicsEffect<QGraphicsItem>()
        , m_elapsedDuration(0)
        , m_configuration(pulseConfiguration)
    {
        PulseRequestBus::Handler::BusConnect(GetEffectId());
        AZ::TickBus::Handler::BusConnect();

        setAcceptHoverEvents(false);
        setAcceptDrops(false);
        setAcceptTouchEvents(false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setFlag(QGraphicsItem::ItemIsFocusable, false);

        if (!m_configuration.m_controlPoints.empty())
        {
            QPointF testPoint = m_configuration.m_controlPoints.front().GetPoint(1.0f);
            m_boundingRect.setTopLeft(testPoint);
            m_boundingRect.setBottomRight(testPoint);
        }
      
        for (const AnimatedPulseControlPoint& controlPoint : m_configuration.m_controlPoints)
        {
            for (float controlValue : {0.0f, 1.0f})
            {
                QPointF testPoint = controlPoint.GetPoint(controlValue);

                if (m_boundingRect.left() > testPoint.x())
                {
                    m_boundingRect.setLeft(testPoint.x());
                }
                else if (m_boundingRect.right() < testPoint.x())
                {
                    m_boundingRect.setRight(testPoint.x());
                }

                if (m_boundingRect.top() > testPoint.y())
                {
                    m_boundingRect.setTop(testPoint.y());
                }
                else if (m_boundingRect.bottom() < testPoint.y())
                {
                    m_boundingRect.setBottom(testPoint.y());
                }
            }
        }
        
        setPos(m_boundingRect.center());
        setZValue(m_configuration.m_zValue);

        // Points are given to use in absolute coordinates, and we need them to be in relative coordinates.
        // we are positioned at the center of the to allow for the drawing to make sense. So we need to offset out bounding box
        // accordingly.
        m_boundingRect.moveTo(-m_boundingRect.width() * 0.5f, -m_boundingRect.height() * 0.5f);
    }

    AnimatedPulse::~AnimatedPulse()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void AnimatedPulse::OnSystemTick()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        PulseNotificationBus::Event(GetEffectId(), &PulseNotifications::OnPulseComplete);
        QGraphicsScene* graphicsScene = this->scene();
        graphicsScene->removeItem(this);
        delete this;
    }

    void AnimatedPulse::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        m_elapsedDuration += deltaTime;

        if (m_elapsedDuration >= m_configuration.m_durationSec)
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ::SystemTickBus::Handler::BusConnect();
        }

        update();
    }

    QRectF AnimatedPulse::boundingRect() const
    {
        return m_boundingRect;
    }

    void AnimatedPulse::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
    {
        if (m_configuration.m_controlPoints.empty())
        {
            return;
        }

        static const float k_pulseWidth = 60.0f;
        painter->save();
        float percent = m_elapsedDuration / m_configuration.m_durationSec;

        if (percent >= 1.0f)
        {
            percent = 1.0f;
        }

        painter->setPen(Qt::PenStyle::NoPen);

        if (m_configuration.m_enableGradient)
        {
            QColor fullColor = m_configuration.m_drawColor;
            fullColor.setAlpha(aznumeric_cast<int>(255 - (255 * percent)));

            QColor transparentColor(0,0,0,0);

            // This is pretty bad perf wise(causes a lot of unnecessary redrawing if the pulse is large enough).
            // This case could be cleaned up, but since we never use it, it's not really worth it.
            // This is also still not perfect, as there is a slight visible seam between the edges on hard corners.
            //
            // But to make this more performant in general will probably require a different approach here.
            // - A Radial Gradient looks really bad on oblong shapes, and bad on pretty much anything non-circular(which we don't really support here)
            for (int i = 0; i < m_configuration.m_controlPoints.size(); ++i)
            {
                QPointF sceneStartPoint = m_configuration.m_controlPoints[i].GetPoint(percent);
                QPointF sceneStartPointOffset = m_configuration.m_controlPoints[i].GetPoint(0.0f);

                int elementIndex = (i + 1) % m_configuration.m_controlPoints.size();

                QPointF sceneEndPoint = m_configuration.m_controlPoints[elementIndex].GetPoint(percent);
                QPointF sceneEndPointOffset = m_configuration.m_controlPoints[elementIndex].GetPoint(0.0f);

                QPointF endCenter = (sceneStartPoint + sceneEndPoint) * 0.5f;
                QPointF offsetCenter = (sceneStartPointOffset + sceneEndPointOffset) * 0.5f;

                float distance = QtVectorMath::GetLength(offsetCenter - endCenter);

                QLinearGradient linearGradient(offsetCenter - pos(), endCenter - pos());

                linearGradient.setColorAt(0.0, transparentColor);

                if (distance > k_pulseWidth)
                {
                    linearGradient.setColorAt((distance - k_pulseWidth) / distance, transparentColor);
                }

                linearGradient.setColorAt(1.0, fullColor);

                QPainterPath painterPath;

                painterPath.moveTo(sceneStartPoint - pos());
                painterPath.lineTo(sceneEndPoint - pos());
                painterPath.lineTo(sceneEndPointOffset - pos());
                painterPath.lineTo(sceneStartPointOffset - pos());
                painterPath.closeSubpath();

                painter->setBrush(linearGradient);
                painter->drawPath(painterPath);
            }
        }
        else
        {
            QPainterPath painterPath;

            painterPath.moveTo(m_configuration.m_controlPoints.front().GetPoint(percent) - pos());

            for (int i = 1; i < m_configuration.m_controlPoints.size(); ++i)
            {
                painterPath.lineTo(m_configuration.m_controlPoints[i].GetPoint(percent) - pos());
            }

            painterPath.closeSubpath();

            QColor drawColor = m_configuration.m_drawColor;
            drawColor.setAlpha(aznumeric_cast<int>(255 - 192 * percent));
            painter->setBrush(drawColor);

            painter->drawPath(painterPath);
        }

        painter->restore();
    }

    void AnimatedPulse::OnGraphicsEffectCancelled()
    {
        PulseNotificationBus::Event(GetEffectId(), &PulseNotifications::OnPulseCanceled);
    }    
}
