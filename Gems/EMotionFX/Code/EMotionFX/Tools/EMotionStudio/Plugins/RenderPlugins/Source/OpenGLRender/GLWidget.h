/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_GLWIDGET_H
#define __EMSTUDIO_GLWIDGET_H

#if !defined(Q_MOC_RUN)
#include "../RenderPluginsConfig.h"

#include <EMotionFX/Rendering/OpenGL2/Source/glactor.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GBuffer.h>
#include <EMotionFX/Rendering/Common/Camera.h>
#include <EMotionFX/Rendering/Common/TransformationManipulator.h>
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderPlugin.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderWidget.h"
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventManager.h>

#include <AzCore/Debug/Timer.h>
//#include <QtGui>
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QFont>
#include <QFontMetrics>
#endif


namespace EMStudio
{
    // forward declaration
    class OpenGLRenderPlugin;
    class RenderViewWidget;

    class GLWidget
        : public QOpenGLWidget
        , public RenderWidget
        , protected QOpenGLExtraFunctions
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(GLWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERPLUGIN);

    public:
        GLWidget(RenderViewWidget* renderViewWidget, OpenGLRenderPlugin* parentPlugin);
        virtual ~GLWidget();

        void initializeGL() override;
        void paintGL() override;
        void resizeGL(int width, int height) override;

    private slots:
        void CloneSelectedActorInstances()                      { CommandSystem::CloneSelectedActorInstances(); }
        void RemoveSelectedActorInstances()                     { CommandSystem::RemoveSelectedActorInstances(); }
        void MakeSelectedActorInstancesInvisible()              { CommandSystem::MakeSelectedActorInstancesInvisible(); }
        void MakeSelectedActorInstancesVisible()                { CommandSystem::MakeSelectedActorInstancesVisible(); }
        void UnselectSelectedActorInstances()                   { CommandSystem::UnselectSelectedActorInstances(); }
        void ResetToBindPose()                                  { CommandSystem::ResetToBindPose(); }

    protected:
        void mouseMoveEvent(QMouseEvent* event) override        { RenderWidget::OnMouseMoveEvent(this, event); }
        void mousePressEvent(QMouseEvent* event) override       { RenderWidget::OnMousePressEvent(this, event); }
        void mouseReleaseEvent(QMouseEvent* event) override     { RenderWidget::OnMouseReleaseEvent(this, event); }
        void wheelEvent(QWheelEvent* event) override            { RenderWidget::OnWheelEvent(this, event); }

        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

        void Render() override;
        void Update() override                                  { update(); }
        void RenderBorder(const MCore::RGBAColor& color);

        RenderGL::GBuffer                       m_gBuffer;
        OpenGLRenderPlugin*                     m_parentRenderPlugin;
        QFont                                   m_font;
        QFontMetrics*                           m_fontMetrics;
        AZ::Debug::Timer                        m_renderTimer;
        AZ::Debug::Timer                        m_perfTimer;
    };
} // namespace EMStudio


#endif
