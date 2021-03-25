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
        void resizeGL(int width, int height);

    private slots:
        void CloneSelectedActorInstances()                      { CommandSystem::CloneSelectedActorInstances(); }
        void RemoveSelectedActorInstances()                     { CommandSystem::RemoveSelectedActorInstances(); }
        void MakeSelectedActorInstancesInvisible()              { CommandSystem::MakeSelectedActorInstancesInvisible(); }
        void MakeSelectedActorInstancesVisible()                { CommandSystem::MakeSelectedActorInstancesVisible(); }
        void UnselectSelectedActorInstances()                   { CommandSystem::UnselectSelectedActorInstances(); }
        void ResetToBindPose()                                  { CommandSystem::ResetToBindPose(); }

    protected:
        void mouseMoveEvent(QMouseEvent* event)                 { RenderWidget::OnMouseMoveEvent(this, event); }
        void mousePressEvent(QMouseEvent* event)                { RenderWidget::OnMousePressEvent(this, event); }
        void mouseReleaseEvent(QMouseEvent* event)              { RenderWidget::OnMouseReleaseEvent(this, event); }
        void wheelEvent(QWheelEvent* event)                     { RenderWidget::OnWheelEvent(this, event); }
        void keyPressEvent(QKeyEvent* event)                    { RenderWidget::OnKeyPressEvent(this, event); }
        void keyReleaseEvent(QKeyEvent* event)                  { RenderWidget::OnKeyReleaseEvent(this, event); }

        void focusInEvent(QFocusEvent* event);
        void focusOutEvent(QFocusEvent* event);

        void Render();
        void Update()                                           { update(); }
        void RenderBorder(const MCore::RGBAColor& color);

        RenderGL::GBuffer                       mGBuffer;
        OpenGLRenderPlugin*                     mParentRenderPlugin;
        QFont                                   mFont;
        QFontMetrics*                           mFontMetrics;
        AZ::Debug::Timer                        mRenderTimer;
        AZ::Debug::Timer                        mPerfTimer;
    };
} // namespace EMStudio


#endif
