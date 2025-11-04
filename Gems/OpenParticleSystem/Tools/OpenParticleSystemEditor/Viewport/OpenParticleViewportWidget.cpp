/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>

#include <OpenParticleViewportRenderer.h>
#include <OpenParticleViewportWidget.h>
#include <Viewport/InputController/OpenParticleEditorViewportInputControllerBus.h>
#include <AzFramework/Viewport/ViewportControllerList.h>

#include <QHBoxLayout>
#include <QVBoxLayout>


namespace OpenParticleSystemEditor
{
    OpenParticleViewportWidget::OpenParticleViewportWidget(QWidget* parent)
        : AtomToolsFramework::RenderViewportWidget(parent)
    {
        if (this->objectName().isEmpty())
        {
            this->setObjectName(QString::fromUtf8("OpenParticleViewportWidget"));
        }
        this->resize(869, 574);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
        this->setSizePolicy(sizePolicy);
        this->setAutoFillBackground(false);
        this->setStyleSheet(QString::fromUtf8(""));
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        QWidget* widget = new QWidget(this);
        widget->setObjectName(QString::fromUtf8("widget"));
        QHBoxLayout* horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        widget->setMouseTracking(true);
        verticalLayout->addWidget(widget);

        m_renderer = AZStd::make_unique<OpenParticleViewportRenderer>(GetViewportContext()->GetWindowContext());
        GetControllerList()->Add(m_renderer->GetController());
        GetViewportContext()->SetRenderScene(m_renderer->GetScene());
        OpenParticleViewportWidgetRequestsBus::Handler::BusConnect();
    }

    OpenParticleViewportWidget::~OpenParticleViewportWidget()
    {
        OpenParticleViewportWidgetRequestsBus::Handler::BusDisconnect();
    }

    AzFramework::CameraState OpenParticleViewportWidget::CameraState()
    {
        return GetCameraState();
    }

    AzFramework::ViewportId OpenParticleViewportWidget::ViewportId()
    {
        return GetId();
    }

    void OpenParticleViewportWidget::showEvent([[maybe_unused]] QShowEvent* event)
    {
        m_renderer->ActiveView();
    }

    void OpenParticleViewportWidget::hideEvent([[maybe_unused]] QHideEvent* event)
    {
        m_renderer->DeactiveView();
    }

    void OpenParticleViewportWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Z)
        {
            EBUS_EVENT(OpenParticleEditorViewportInputControllerRequestBus, Reset);
        }
    }
}

