/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// LmbrCentral
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

// Gradient Signal
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <GradientSignal/Editor/EditorGradientPreviewRenderer.h>

// Graph Model
#include <GraphModel/Integration/ThumbnailItem.h>

namespace LandscapeCanvasEditor
{
    class GradientPreviewThumbnailItem
        : public GraphModelIntegration::ThumbnailItem
        , public GradientSignal::EditorGradientPreviewRenderer
        , public LmbrCentral::DependencyNotificationBus::Handler
        , public GradientSignal::GradientPreviewRequestBus::Handler
    {
    public:
        AZ_RTTI(GradientPreviewThumbnailItem, "{D2FA7FB4-9E47-41AD-95A2-818910B09A67}", GraphModelIntegration::ThumbnailItem);

        using SampleFilterFunc = AZStd::function<float(float)>;

        GradientPreviewThumbnailItem(const AZ::EntityId& gradientId, QGraphicsItem* parent = nullptr);
        ~GradientPreviewThumbnailItem() override;

        void SetGradientEntity(const AZ::EntityId& id);

        //////////////////////////////////////////////////////////////////////////
        // QGraphicsLayoutItem
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = QSizeF()) const override;

        //////////////////////////////////////////////////////////////////////////
        // QGraphicsItem
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus::Handler
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // GradientPreviewRequestBus::Handler
        void Refresh() override;
        AZ::EntityId CancelRefresh() override;

    protected:
        void OnUpdate() override;
        QSize GetPreviewSize() const override;

        AZ::EntityId m_observerEntityStub;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        bool m_refreshInProgress = false;
    };

} //namespace LandscapeCanvasEditor
