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

// Qt
#include <QPainter>
#include <QSize>
#include <QWidget>

// Landscape Canvas
#include <Editor/Nodes/UI/GradientPreviewThumbnailItem.h>

namespace LandscapeCanvasEditor
{
    static const QSize PREVIEW_SIZE = QSize(256, 256);
    static const QSize PREVIEW_MARGIN = QSize(40, 40);

    GradientPreviewThumbnailItem::GradientPreviewThumbnailItem(const AZ::EntityId& gradientId, QGraphicsItem* parent)
        : GraphModelIntegration::ThumbnailItem(parent)
    {
        // Dependency monitor must be connected to an owner/observer as a target for notifications.
        // Generating a place holder entity
        m_observerEntityStub = AZ::Entity::MakeId();
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(m_observerEntityStub);

        SetGradientEntity(gradientId);
    }

    GradientPreviewThumbnailItem::~GradientPreviewThumbnailItem()
    {
        GradientSignal::GradientPreviewRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        m_dependencyMonitor.Reset();
    }

    void GradientPreviewThumbnailItem::SetGradientEntity(const AZ::EntityId& id)
    {
        m_sampler = {};
        m_sampler.m_gradientId = id;
        m_sampler.m_ownerEntityId = id;

        GradientSignal::GradientPreviewRequestBus::Handler::BusDisconnect();
        GradientSignal::GradientPreviewRequestBus::Handler::BusConnect(id);

        Refresh();
    }

    QSizeF GradientPreviewThumbnailItem::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
    {
        switch (which)
        {
        case Qt::MinimumSize:
        case Qt::PreferredSize:
            return PREVIEW_SIZE + PREVIEW_MARGIN;
        case Qt::MaximumSize:
            return QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        default:
            break;
        }

        return constraint;
    }

    void GradientPreviewThumbnailItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        (void)option;
        (void)widget;

        if (!m_previewImage.isNull())
        {
            // Draw the preview centered in our frame
            QRectF frame(QPointF(0, 0), geometry().size());
            QPointF topLeft = frame.center() - (QPointF(PREVIEW_SIZE.width(), PREVIEW_SIZE.height()) / 2);
            painter->drawImage(topLeft, m_previewImage);
        }
    }

    void GradientPreviewThumbnailItem::OnCompositionChanged()
    {
        Refresh();
    }

    void GradientPreviewThumbnailItem::Refresh()
    {
        if (!m_refreshInProgress)
        {
            m_refreshInProgress = true;

            m_dependencyMonitor.Reset();
            m_dependencyMonitor.ConnectOwner(m_observerEntityStub);
            m_dependencyMonitor.ConnectDependency(m_sampler.m_gradientId);

            AZ::EntityId previewEntity;
            GradientSignal::GradientPreviewContextRequestBus::BroadcastResult(previewEntity, &GradientSignal::GradientPreviewContextRequestBus::Events::GetPreviewEntity);
            m_dependencyMonitor.ConnectDependency(previewEntity);

            QueueUpdate();
            m_refreshInProgress = false;
        }
    }

    AZ::EntityId GradientPreviewThumbnailItem::CancelRefresh()
    {
        if (OnCancelRefresh())
        {
            return m_sampler.m_gradientId;
        }

        return AZ::EntityId();
    }

    void GradientPreviewThumbnailItem::OnUpdate()
    {
        update();
    }

    QSize GradientPreviewThumbnailItem::GetPreviewSize() const
    {
        return PREVIEW_SIZE;
    }

} //namespace LandscapeCanvasEditor
