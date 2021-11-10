/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "GLWidget.h"
#include "OpenGLRenderPlugin.h"
#include <EMotionFX/Rendering/Common/OrbitCamera.h>
#include <EMotionFX/Rendering/Common/OrthographicCamera.h>
#include <EMotionFX/Rendering/Common/FirstPersonCamera.h>
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Recorder.h>
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderViewWidget.h"


namespace EMStudio
{
    // constructor
    GLWidget::GLWidget(RenderViewWidget* parentWidget, OpenGLRenderPlugin* parentPlugin)
        : QOpenGLWidget(parentWidget)
        , RenderWidget(parentPlugin, parentWidget)
    {
        m_parentRenderPlugin = parentPlugin;

        // construct the font metrics used for overlay text rendering
        m_font.setPointSize(10);
        m_fontMetrics = new QFontMetrics(m_font);

        // create our default camera
        SwitchCamera(CAMMODE_ORBIT);

        // setup to get focus when we click or use the mouse wheel
        setFocusPolicy((Qt::FocusPolicy)(Qt::ClickFocus | Qt::WheelFocus));
        setMouseTracking(true);

        setAutoFillBackground(false);
    }


    // destructor
    GLWidget::~GLWidget()
    {
        // destruct the font metrics used for overlay text rendering
        delete m_fontMetrics;
    }


    // initialize the Qt OpenGL widget (overloaded from the widget base class)
    void GLWidget::initializeGL()
    {
        // initializeOpenGLFunctions() and m_parentRenderPlugin->InitializeGraphicsManager must be called first to ensure
        // all OpenGL functions have been resolved before doing anything that could make GL calls (e.g. resizing)
        initializeOpenGLFunctions();
        m_parentRenderPlugin->InitializeGraphicsManager();
        if (m_parentRenderPlugin->GetGraphicsManager())
        {
            m_parentRenderPlugin->GetGraphicsManager()->SetGBuffer(&m_gBuffer);
        }

        // set minimum render view dimensions
        setMinimumHeight(100);
        setMinimumWidth(100);

        m_perfTimer.StampAndGetDeltaTimeInSeconds();
    }


    // resize the Qt OpenGL widget (overloaded from the widget base class)
    void GLWidget::resizeGL(int width, int height)
    {
        // don't resize in case the render widget is hidden
        if (isHidden())
        {
            return;
        }

        m_parentRenderPlugin->GetRenderUtil()->Validate();

        m_width  = width;
        m_height = height;
        m_gBuffer.Resize(width, height);

        RenderGL::GraphicsManager* graphicsManager = m_parentRenderPlugin->GetGraphicsManager();
        if (graphicsManager == nullptr || m_camera == nullptr)
        {
            return;
        }
    }


    // trigger a render
    void GLWidget::Render()
    {
        update();
    }


    // render frame
    void GLWidget::paintGL()
    {
        //SetVSync(false);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // don't render in case the render widget is hidden
        if (isHidden())
        {
            return;
        }

        m_renderTimer.Stamp();

        // render the scene
        RenderGL::GraphicsManager* graphicsManager = m_parentRenderPlugin->GetGraphicsManager();
        if (graphicsManager == nullptr || m_camera == nullptr)
        {
            return;
        }

        painter.beginNativePainting();

        graphicsManager->SetGBuffer(&m_gBuffer);

        RenderOptions* renderOptions = m_parentRenderPlugin->GetRenderOptions();

        // get a pointer to the render utility
        RenderGL::GLRenderUtil* renderUtil = m_parentRenderPlugin->GetGraphicsManager()->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // set this as the active widget
        // note that this is done in paint() instead of by the plugin because of delay when glwidget::update is called
        MCORE_ASSERT(m_parentRenderPlugin->GetActiveViewWidget() == nullptr);
        m_parentRenderPlugin->SetActiveViewWidget(m_viewWidget);

        // set the background colors
        graphicsManager->SetClearColor(renderOptions->GetBackgroundColor());
        graphicsManager->SetGradientSourceColor(renderOptions->GetGradientSourceColor());
        graphicsManager->SetGradientTargetColor(renderOptions->GetGradientTargetColor());
        graphicsManager->SetUseGradientBackground(m_viewWidget->GetRenderFlag(RenderViewWidget::RENDER_USE_GRADIENTBACKGROUND));

        // needed to make multiple viewports working
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);

        // tell the system about the current viewport
        glViewport(0, 0, aznumeric_cast<GLsizei>(m_width * devicePixelRatioF()), aznumeric_cast<GLsizei>(m_height * devicePixelRatioF()));
        renderUtil->SetDevicePixelRatio(aznumeric_cast<float>(devicePixelRatioF()));

        graphicsManager->SetRimAngle        (renderOptions->GetRimAngle());
        graphicsManager->SetRimIntensity    (renderOptions->GetRimIntensity());
        graphicsManager->SetRimWidth        (renderOptions->GetRimWidth());
        graphicsManager->SetRimColor        (renderOptions->GetRimColor());
        graphicsManager->SetMainLightAngleA (renderOptions->GetMainLightAngleA());
        graphicsManager->SetMainLightAngleB (renderOptions->GetMainLightAngleB());
        graphicsManager->SetMainLightIntensity(renderOptions->GetMainLightIntensity());
        graphicsManager->SetSpecularIntensity(renderOptions->GetSpecularIntensity());

        // update the camera
        UpdateCamera();

        graphicsManager->SetCamera(m_camera);

        graphicsManager->BeginRender();

        // render grid
        RenderGrid();

        // render characters
        RenderActorInstances();

        // iterate through all plugins and render their helper data
        RenderCustomPluginData();

        // disable backface culling after rendering the actors
        glDisable(GL_CULL_FACE);

        // render the gizmos
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        RenderWidget::RenderManipulators();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        graphicsManager->EndRender();

        // render overlay
        // do this in the very end as we're clearing the depth buffer here

        // render axis on the bottom left which shows the current orientation of the camera relative to the global axis
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        MCommon::Camera* camera = m_camera;
        if (m_camera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            camera = m_axisFakeCamera;
        }
        graphicsManager->SetCamera(camera);

        RenderWidget::RenderAxis();

        graphicsManager->SetCamera(m_camera);

        glPopAttrib();

        // render node filter string
        RenderWidget::RenderNodeFilterString();

        // render the border around the render view
        if (EMotionFX::GetRecorder().GetIsRecording() == false && EMotionFX::GetRecorder().GetIsInPlayMode() == false)
        {
            if (m_parentRenderPlugin->GetFocusViewWidget() == m_viewWidget)
            {
                RenderBorder(MCore::RGBAColor(1.0f, 0.647f, 0.0f));
            }
            else
            {
                RenderBorder(MCore::RGBAColor(0.0f, 0.0f, 0.0f));
            }
        }
        else
        {
            if (EMotionFX::GetRecorder().GetIsRecording())
            {
                renderUtil->RenderText(5, 5, "RECORDING MODE", MCore::RGBAColor(0.8f, 0.0f, 0.0f), 9.0f);
                RenderBorder(MCore::RGBAColor(0.8f, 0.0f, 0.0f));
            }
            else
            if (EMotionFX::GetRecorder().GetIsInPlayMode())
            {
                renderUtil->RenderText(5, 5, "PLAYBACK MODE", MCore::RGBAColor(0.0f, 0.8f, 0.0f), 9.0f);
                RenderBorder(MCore::RGBAColor(0.0f, 0.8f, 0.0f));
            }
        }

        // makes no GL context the current context, needed in multithreaded environments
        //doneCurrent(); // Ben: results in a white screen
        m_parentRenderPlugin->SetActiveViewWidget(nullptr);

        painter.endNativePainting();

        if (renderOptions->GetShowFPS())
        {
            const float renderTime = m_renderTimer.GetDeltaTimeInSeconds() * 1000.0f;

            // get the time delta between the current time and the last frame
            const float perfTimeDelta = m_perfTimer.StampAndGetDeltaTimeInSeconds();

            static float fpsTimeElapsed = 0.0f;
            static uint32 fpsNumFrames = 0;
            static uint32 lastFPS = 0;
            fpsTimeElapsed += perfTimeDelta;
            fpsNumFrames++;
            if (fpsTimeElapsed > 1.0f)
            {
                lastFPS         = fpsNumFrames;
                fpsTimeElapsed  = 0.0f;
                fpsNumFrames    = 0;
            }

            const AZStd::string perfTempString = AZStd::string::format("%d FPS (%.1f ms)", lastFPS, renderTime);

            // initialize the painter and get the font metrics
            EMStudioManager::RenderText(painter, perfTempString.c_str(), QColor(150, 150, 150), m_font, *m_fontMetrics, Qt::AlignRight, QRect(width() - 55, height() - 20, 50, 20));
        }
    }


    void GLWidget::RenderBorder(const MCore::RGBAColor& color)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0f, m_width, m_height, 0.0f, 0.0f, 1.0f);
        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity();
        //glTranslatef(0.375f, 0.375f, 0.0f);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        glLineWidth(3.0f);

        glColor3f(color.m_r, color.m_g, color.m_b);
        glBegin(GL_LINES);
        // left
        glVertex2f(0.0f, 0.0f);
        glVertex2f(0.0f, aznumeric_cast<GLfloat>(m_height));
        // bottom
        glVertex2f(0.0f, aznumeric_cast<GLfloat>(m_height));
        glVertex2f(aznumeric_cast<GLfloat>(m_width), aznumeric_cast<GLfloat>(m_height));
        // top
        glVertex2f(0.0f, 0.0f);
        glVertex2f(aznumeric_cast<GLfloat>(m_width), 0);
        // right
        glVertex2f(aznumeric_cast<GLfloat>(m_width), 0.0f);
        glVertex2f(aznumeric_cast<GLfloat>(m_width), aznumeric_cast<float>(m_height));
        glEnd();

        glLineWidth(1.0f);
    }


    void GLWidget::focusInEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        m_parentRenderPlugin->SetFocusViewWidget(m_viewWidget);
        grabKeyboard();
    }


    void GLWidget::focusOutEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        m_parentRenderPlugin->SetFocusViewWidget(nullptr);
        releaseKeyboard();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/RenderPlugins/Source/OpenGLRender/moc_GLWidget.cpp>
