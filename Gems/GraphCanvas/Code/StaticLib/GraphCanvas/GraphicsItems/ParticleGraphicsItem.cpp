/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsScene>
#include <QPainter>
#include <QTimer>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/GraphicsItems/ParticleGraphicsItem.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Utils/QtDrawingUtils.h>
#include <GraphCanvas/Utils/QtVectorMath.h>

namespace GraphCanvas
{
    /////////////////////////
    // ParticleGraphicsItem
    /////////////////////////
    
    ParticleGraphicsItem::ParticleGraphicsItem(const ParticleConfiguration& particleConfiguration)
        : GraphicsEffect<QGraphicsItem>()
        , m_configuration(particleConfiguration)
        , m_elapsedDuration(0)
    {
        AZ::TickBus::Handler::BusConnect();

        if (m_configuration.m_alphaFade && m_configuration.m_fadeTime.count() == 0)
        {
            m_configuration.m_alphaStart = m_configuration.m_alphaEnd;
            m_configuration.m_alphaFade = false;
        }        
    
        m_boundingRect = m_configuration.m_boundingArea;
        setPos(m_boundingRect.topLeft());

        m_boundingRect.moveTopLeft(QPointF(0, 0));

        switch (m_configuration.m_particleShape)
        {
        case ParticleConfiguration::Shape::Circle:
        {            
            m_clipPath.addEllipse(m_boundingRect);
            break;
        }
        case ParticleConfiguration::Shape::Square:
            m_clipPath.addRect(m_boundingRect);
            break;
        }

        m_impulse = m_configuration.m_initialImpulse;        

        setZValue(m_configuration.m_zValue);
    }

    ParticleGraphicsItem::~ParticleGraphicsItem()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }
    
    void ParticleGraphicsItem::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (m_configuration.m_rotate)
        {
            float delta = m_configuration.m_rotationSpeed * deltaTime;

            QTransform itemTransform = transform();
            itemTransform.rotate(delta);

            setTransform(itemTransform);
        }

        if (m_configuration.m_hasGravity)
        {
            static const QPointF gravity(0, 640.0f);
            QPointF delta = gravity * deltaTime;
            m_impulse += delta;
        }

        // Don't do anything if we are close to zero
        if (m_impulse.manhattanLength() >= 0.1f)
        {
            QPointF positionDelta = m_impulse * deltaTime;
            setPos(pos() + positionDelta);
        }

        m_elapsedDuration += deltaTime;

        if (m_elapsedDuration >= (m_configuration.m_lifespan.count() * 0.001f))
        {
            m_elapsedDuration = m_configuration.m_lifespan.count() * 0.001f;

            AZ::TickBus::Handler::BusDisconnect();
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void ParticleGraphicsItem::OnSystemTick()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        QGraphicsScene* graphicsScene = this->scene();
        graphicsScene->removeItem(this);
        delete this;
    }
    
    QRectF ParticleGraphicsItem::boundingRect() const
    {
        return m_boundingRect;
    }
    
    void ParticleGraphicsItem::paint([[maybe_unused]] QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option, [[maybe_unused]] QWidget* widget)
    {
        painter->save();

        float alpha = m_configuration.m_alphaStart;

        if (m_configuration.m_alphaFade)
        {
            float percent = m_elapsedDuration / (m_configuration.m_fadeTime.count() * 0.001f);
            alpha = m_configuration.m_alphaStart + (m_configuration.m_alphaEnd - m_configuration.m_alphaStart) * percent;
        }

        setOpacity(alpha);

        if (m_configuration.m_styleHelper)
        {
            painter->setClipPath(m_clipPath);
            QtDrawingUtils::FillArea((*painter), m_boundingRect, (*m_configuration.m_styleHelper));
        }
        else
        {
            painter->setPen(Qt::PenStyle::NoPen);

            painter->setBrush(m_configuration.m_color);

            switch (m_configuration.m_particleShape)
            {
            case ParticleConfiguration::Shape::Circle:
                painter->drawEllipse(m_boundingRect);
                break;
            case ParticleConfiguration::Shape::Square:
                painter->drawRect(m_boundingRect);
                break;
            default:
                break;
            }
        }

        painter->restore();
    }
}
