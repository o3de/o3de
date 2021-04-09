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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"

#include <Cry_Camera.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <ITimer.h>
#include <I3DEngine.h>
#include <IPhysicsDebugRenderer.h>
#include <IEditor.h>
#include <Util/Image.h>

#include <QMouseEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QApplication>

#include "QViewport.h"
#include "QViewportEvents.h"
#include "QViewportConsumer.h"
#include "QViewportSettings.h"
#include "Serialization.h"

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/API/AtomActiveInterface.h>

#include <AzQtComponents/Utilities/QtWindowUtilities.h>

struct QViewport::SPreviousContext
{
    CCamera renderCamera;
    CCamera systemCamera;
    int width;
    int height;
    HWND window;
    bool isMainViewport;
};

static void DrawGridLine(IRenderAuxGeom& aux, ColorB col, const float alpha, const float alphaFalloff, const float slide, const float halfSlide, [[maybe_unused]] const float maxSlide, const Vec3& stepDir, const Vec3& orthoDir, const SViewportState& state, const SViewportGridSettings& gridSettings)
{
    ColorB colEnd = col;

    float weight = 1.0f - (slide / halfSlide);
    if (slide > halfSlide)
    {
        weight = (slide - halfSlide) / halfSlide;
    }

    float orthoWeight = 1.0f;

    if (gridSettings.circular)
    {
        float invWeight = 1.0f - weight;
        orthoWeight = sqrtf((invWeight * 2) - (invWeight * invWeight));
    }
    else
    {
        orthoWeight = 1.0f;
    }

    col.a = aznumeric_cast<uint8_t>((1.0f - (weight * (1.0f - alphaFalloff))) * alpha);
    colEnd.a = aznumeric_cast<uint8_t>(alphaFalloff * alpha);

    Vec3 orthoStep = state.gridOrigin.q * (orthoDir * halfSlide * orthoWeight);

    Vec3 point = state.gridOrigin * (-(stepDir * halfSlide) + (stepDir * slide));
    Vec3 points[3] = {
        point,
        point - orthoStep,
        point + orthoStep
    };

    aux.DrawLine(points[0], col, points[1], colEnd);
    aux.DrawLine(points[0], col, points[2], colEnd);
}

static void DrawGridLines(IRenderAuxGeom& aux, const uint count, const uint interStepCount, const Vec3& stepDir, const float stepSize, const Vec3& orthoDir, const float offset, const SViewportState& state, const SViewportGridSettings& gridSettings)
{
    const uint countHalf = count / 2;
    Vec3 step = stepDir * stepSize;
    Vec3 orthoStep = orthoDir * aznumeric_cast<float>(countHalf);
    Vec3 maxStep = step * aznumeric_cast<float>(countHalf);// + stepDir*fabs(offset);
    const float maxStepLen = count * stepSize;
    const float halfStepLen = countHalf * stepSize;

    float interStepSize = interStepCount > 0 ? (stepSize / interStepCount) : stepSize;
    const float alphaMulMain = (float)gridSettings.mainColor.a;
    const float alphaMulInter = (float)gridSettings.middleColor.a;
    const float alphaFalloff = 1.0f - (gridSettings.alphaFalloff / 100.0f);

    for (int i = 0; i < count + 2; i++)
    {
        float pointSlide = i * stepSize + offset;
        if (pointSlide > 0.0f && pointSlide < maxStepLen)
        {
            DrawGridLine(aux, gridSettings.mainColor, alphaMulMain, alphaFalloff, pointSlide, halfStepLen, maxStepLen, stepDir, orthoDir, state, gridSettings);
        }

        for (int d = 1; d < interStepCount; d++)
        {
            float interSlide = ((i - 1) * stepSize) + offset + (d * interStepSize);
            if (interSlide > 0.0f && interSlide < maxStepLen)
            {
                DrawGridLine(aux, gridSettings.middleColor, alphaMulInter, alphaFalloff, interSlide, halfStepLen, maxStepLen, stepDir, orthoDir, state, gridSettings);
            }
        }
    }
}

static void DrawGrid(IRenderAuxGeom& aux, const SViewportState& state, const SViewportGridSettings& gridSettings)
{
    const uint count = gridSettings.count * 2;
    const float gridSize = gridSettings.spacing * gridSettings.count * 2.0f;
    const float halfGridSize = gridSettings.spacing * gridSettings.count;

    const float stepSize = gridSize / count;
    DrawGridLines(aux, count, gridSettings.interCount, Vec3(1.0f, 0.0f, 0.0f), stepSize, Vec3(0.0f, 1.0f, 0.0f), state.gridCellOffset.x, state, gridSettings);
    DrawGridLines(aux, count, gridSettings.interCount, Vec3(0.0f, 1.0f, 0.0f), stepSize, Vec3(1.0f, 0.0f, 0.0f), state.gridCellOffset.y, state, gridSettings);
}

static void DrawOrigin(IRenderAuxGeom& aux, const ColorB& col)
{
    const float scale = 0.3f;
    const float lineWidth = 4.0f;
    aux.DrawLine(Vec3(-scale, 0, 0), col, Vec3(scale, 0, 0), col, lineWidth);
    aux.DrawLine(Vec3(0, -scale, 0), col, Vec3(0, scale, 0), col, lineWidth);
    aux.DrawLine(Vec3(0, 0, -scale), col, Vec3(0, 0, scale), col, lineWidth);
}

static void DrawOrigin(IRenderAuxGeom& aux, const int left, const int top, const float scale, const Matrix34 cameraTM)
{
    Vec3 originPos = Vec3(aznumeric_cast<float>(left), aznumeric_cast<float>(top), 0);
    Quat originRot = Quat(0.707107f, 0.707107f, 0, 0) * Quat(cameraTM).GetInverted();
    Vec3 x = originPos + originRot * Vec3(1, 0, 0) * scale;
    Vec3 y = originPos + originRot * Vec3(0, 1, 0) * scale;
    Vec3 z = originPos + originRot * Vec3(0, 0, 1) * scale;
    ColorF xCol(1, 0, 0);
    ColorF yCol(0, 1, 0);
    ColorF zCol(0, 0, 1);
    const float lineWidth = 2.0f;

    aux.DrawLine(originPos, xCol, x, xCol, lineWidth);
    aux.DrawLine(originPos, yCol, y, yCol, lineWidth);
    aux.DrawLine(originPos, zCol, z, zCol, lineWidth);
}

struct QViewport::SPrivate
{
    CDLight m_VPLight0;
    CDLight m_sun;
};

QViewport::QViewport(QWidget* parent, StartupMode startupMode)
    : QWidget(parent)
    , m_renderContextCreated(false)
    , m_updating(false)
    , m_width(0)
    , m_height(0)
    , m_fastMode(false)
    , m_slowMode(false)
    , m_lastTime(0)
    , m_lastFrameTime(0.0f)
    , m_averageFrameTime(0.0f)
    , m_sceneDimensions(1.0f, 1.0f, 1.0f)
    , m_creatingRenderContext(false)
    , m_timer(0)
    , m_cameraSmoothPosRate(0)
    , m_cameraSmoothRotRate(0)
    , m_settings(new SViewportSettings())
    , m_state(new SViewportState())
    , m_useArrowsForNavigation(true)
    , m_mouseMovementsSinceLastFrame(0)
    , m_private(new SPrivate())
    , m_cameraControlMode(CameraControlMode::NONE)
{
    if (startupMode & StartupMode_Immediate)
        Startup();
}

void QViewport::Startup()
{
    m_frameTimer = new QElapsedTimer();
    CreateRenderContext();

    m_camera.reset(new CCamera());
    ResetCamera();

    m_mousePressPos = QCursor::pos();

    UpdateBackgroundColor();

    setUpdatesEnabled(false);
    setMouseTracking(true);
    m_LightRotationRadian = 0;
    m_frameTimer->start();
}

QViewport::~QViewport()
{
    DestroyRenderContext();
}

void QViewport::UpdateBackgroundColor()
{
    QPalette pal(palette());
    pal.setColor(QPalette::Window, QColor(m_settings->background.topColor.r,
            m_settings->background.topColor.g,
            m_settings->background.topColor.b,
            m_settings->background.topColor.a));
    setPalette(pal);
    setAutoFillBackground(true);
}

bool QViewport::ScreenToWorldRay(Ray* ray, int x, int y)
{
    if (!GetIEditor()->GetEnv()->pRenderer)
    {
        return false;
    }

    SetCurrentContext();

    Vec3 pos0, pos1;
    float wx, wy, wz;
    if (!GetIEditor()->GetEnv()->pRenderer->UnProjectFromScreen(float(x), float(m_height - y), 0, &wx, &wy, &wz))
    {
        RestorePreviousContext();
        return false;
    }
    pos0(wx, wy, wz);
    if (!GetIEditor()->GetEnv()->pRenderer->UnProjectFromScreen(float(x), float(m_height - y), 1, &wx, &wy, &wz))
    {
        RestorePreviousContext();
        return false;
    }
    pos1(wx, wy, wz);

    RestorePreviousContext();

    Vec3 v = (pos1 - pos0);
    v = v.GetNormalized();

    ray->origin = pos0;
    ray->direction = v;
    return true;
}

QPoint QViewport::ProjectToScreen(const Vec3& wp)
{
    float x, y, z;

    SetCurrentContext();
    GetIEditor()->GetEnv()->pRenderer->ProjectToScreen(wp.x, wp.y, wp.z, &x, &y, &z);
    if (_finite(x) || _finite(y))
    {
        RestorePreviousContext();
        return QPoint(int((x / 100.0) * Width()), int((y / 100.0) * Height()));
    }
    RestorePreviousContext();

    return QPoint(0, 0);
}

void QViewport::LookAt(const Vec3& target, float radius, bool snap)
{
    QuatT   cameraTarget = m_state->cameraTarget;
    CreateLookAt(target, radius, cameraTarget);
    CameraMoved(cameraTarget, snap);
}

int QViewport::Width() const
{
    return rect().width();
}

int QViewport::Height() const
{
    return rect().height();
}

bool QViewport::CreateRenderContext()
{
    if (m_creatingRenderContext || !isVisible())
    {
        return false;
    }

    HWND windowHandle = reinterpret_cast<HWND>(QWidget::winId());

    if( AZ::Interface<AzFramework::AtomActiveInterface>::Get() && m_renderContextCreated && windowHandle == m_lastHwnd)
    {
        // the hwnd has not changed, no need to destroy and recreate context (and swap chain etc)
        return false;
    }

    m_creatingRenderContext = true;
    DestroyRenderContext();
    if (windowHandle && GetIEditor()->GetEnv()->pRenderer && !m_renderContextCreated)
    {
        m_renderContextCreated = true;

        if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
        {
            AzFramework::WindowRequestBus::Handler::BusConnect(windowHandle);
            AzFramework::WindowSystemNotificationBus::Broadcast(&AzFramework::WindowSystemNotificationBus::Handler::OnWindowCreated, windowHandle);

            m_lastHwnd = windowHandle;
        }

        StorePreviousContext();
        GetIEditor()->GetEnv()->pRenderer->CreateContext(windowHandle);
        RestorePreviousContext();

        m_creatingRenderContext = false;
        return true;
    }
    m_creatingRenderContext = false;
    return false;
}

void QViewport::DestroyRenderContext()
{
    if (GetIEditor()->GetEnv()->pRenderer && m_renderContextCreated)
    {
        HWND windowHandle = reinterpret_cast<HWND>(QWidget::winId());

        if (windowHandle != GetIEditor()->GetEnv()->pRenderer->GetHWND())
        {
            GetIEditor()->GetEnv()->pRenderer->DeleteContext(windowHandle);
        }
        m_renderContextCreated = false;

        AzFramework::WindowNotificationBus::Event(windowHandle, &AzFramework::WindowNotificationBus::Handler::OnWindowClosed);
        AzFramework::WindowRequestBus::Handler::BusDisconnect();
        m_lastHwnd = 0;
    }
}

void QViewport::StorePreviousContext()
{
    SPreviousContext previous;
    previous.width = GetIEditor()->GetEnv()->pRenderer->GetWidth();
    previous.height = GetIEditor()->GetEnv()->pRenderer->GetHeight();
    previous.window = reinterpret_cast<HWND>(GetIEditor()->GetEnv()->pRenderer->GetCurrentContextHWND());
    previous.renderCamera = GetIEditor()->GetEnv()->pRenderer->GetCamera();
    previous.systemCamera = GetISystem()->GetViewCamera();
    previous.isMainViewport = GetIEditor()->GetEnv()->pRenderer->IsCurrentContextMainVP();
    m_previousContexts.push_back(previous);
}

void QViewport::SetCurrentContext()
{
    StorePreviousContext();

    if (m_camera.get() == 0)
    {
        return;
    }

    HWND windowHandle = reinterpret_cast<HWND>(QWidget::winId());
    GetIEditor()->GetEnv()->pRenderer->SetCurrentContext(windowHandle);
    GetIEditor()->GetEnv()->pRenderer->ChangeViewport(0, 0, m_width, m_height);
    GetIEditor()->GetEnv()->pRenderer->SetCamera(*m_camera);
    GetIEditor()->GetEnv()->pSystem->SetViewCamera(*m_camera);
}

void QViewport::RestorePreviousContext()
{
    if (m_previousContexts.empty())
    {
        assert(0);
        return;
    }

    SPreviousContext x = m_previousContexts.back();
    m_previousContexts.pop_back();
    GetIEditor()->GetEnv()->pRenderer->SetCurrentContext(x.window);
    GetIEditor()->GetEnv()->pRenderer->ChangeViewport(0, 0, x.width, x.height, x.isMainViewport);
    GetIEditor()->GetEnv()->pRenderer->SetCamera(x.renderCamera);
    GetIEditor()->GetEnv()->pSystem->SetViewCamera(x.systemCamera);
}

void QViewport::Serialize(IArchive& ar)
{
    if (!ar.IsEdit())
    {
        ar(m_state->cameraTarget, "cameraTarget", "Camera Target");
    }
}

struct AutoBool
{
    AutoBool(bool* value)
        : m_value(value)
    {
        * m_value = true;
    }

    ~AutoBool()
    {
        * m_value = false;
    }

    bool* m_value;
};

void QViewport::Update()
{
    int64 time = m_frameTimer->elapsed();
    if (m_lastTime == 0)
    {
        m_lastTime = time;
    }
    m_lastFrameTime = (time - m_lastTime) * 0.001f;
    m_lastTime = time;
    if (m_averageFrameTime == 0.0f)
    {
        m_averageFrameTime = m_lastFrameTime;
    }
    else
    {
        m_averageFrameTime = 0.01f * m_lastFrameTime + 0.99f * m_averageFrameTime;
    }

    if (GetIEditor()->GetEnv()->pRenderer == 0 ||
        GetIEditor()->GetEnv()->p3DEngine == 0)
    {
        return;
    }

    if (!isVisible())
    {
        return;
    }

    if (!m_renderContextCreated)
    {
        return;
    }

    if (m_updating)
    {
        return;
    }

    AutoBool updating(&m_updating);

    if (m_resizeWindowEvent)
    {
        HWND windowHandle = reinterpret_cast<HWND>(QWidget::winId());
        AzFramework::WindowNotificationBus::Event(windowHandle, &AzFramework::WindowNotificationBus::Handler::OnWindowResized, m_width, m_height);
        m_resizeWindowEvent = false;
    }

    if (hasFocus())
    {
        ProcessMouse();
        ProcessKeys();
    }

    if ((m_width <= 0) || (m_height <= 0))
    {
        return;
    }

    RenderInternal();
}

void QViewport::CaptureMouse()
{
    grabMouse();
}

void QViewport::ReleaseMouse()
{
    releaseMouse();
}

void QViewport::SetForegroundUpdateMode([[maybe_unused]] bool foregroundUpdate)
{
    //m_timer->setInterval(foregroundUpdate ? 2 : 50);
}

CCamera* QViewport::Camera() const 
{ 
    return m_camera.get(); 
}

void QViewport::SetSceneDimensions(const Vec3& size) 
{ 
    m_sceneDimensions = size; 
}

const SViewportSettings& QViewport::GetSettings() const 
{
    return *m_settings; 
}

const SViewportState& QViewport::GetState() const 
{
    return *m_state; 
}

void QViewport::SetSize(const QSize& size)
{ 
    m_width = size.width(); 
    m_height = size.height(); 
}

float QViewport::GetLastFrameTime() 
{ 
    return m_lastFrameTime; 
}


void QViewport::ProcessMouse()
{
    QPoint point = mapFromGlobal(QCursor::pos());

    if (point == m_mousePressPos)
    {
        return;
    }

    if (m_cameraControlMode == CameraControlMode::ZOOM)
    {
        if (!(m_settings->camera.transformRestraint & eCameraTransformRestraint_Zoom))
        {
            float speedScale = CalculateMoveSpeed(m_fastMode, m_slowMode, true);

            // Zoom.
            QuatT qt = m_state->cameraTarget;
            Vec3 ydir = qt.GetColumn1().GetNormalized();
            Vec3 pos = qt.t;
            pos = pos - 0.2f * ydir * aznumeric_cast<float>(m_mousePressPos.y() - point.y()) * speedScale;
            qt.t = pos;
            CameraMoved(qt, false);

            // Check to see if the orbit target is behind the camera's view
            // position
            Vec3 target = m_state->orbitTarget;
            Vec3 at = target - pos;
            float isAlmostBehind = at * ydir;
            if (isAlmostBehind < 0.01f)
            {
                // Force the orbit target to be slightly in front of the view
                // position
                m_state->orbitRadius = 0.01f;
                m_state->orbitTarget = qt.t + ydir * 0.01f;
            }
            else
            {
                m_state->orbitRadius = at.GetLength();
            }

            AzQtComponents::SetCursorPos(mapToGlobal(m_mousePressPos));
        }
    }
    else if (m_cameraControlMode == CameraControlMode::ROTATE)
    {
        if (!(m_settings->camera.transformRestraint & eCameraTransformRestraint_Rotation))
        {
            Ang3 angles(aznumeric_cast<float>(-point.y() + m_mousePressPos.y()), 0, aznumeric_cast<float>(-point.x() + m_mousePressPos.x()));
            angles = angles * 0.001f * m_settings->camera.rotationSpeed;

            QuatT qt = m_state->cameraTarget;
            Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(qt.q));
            ypr.x += angles.z;
            ypr.y += angles.x;
            ypr.y = clamp_tpl(ypr.y, -1.5f, 1.5f);

            qt.q = Quat(CCamera::CreateOrientationYPR(ypr));

            // Move the orbit target with the rotate operation.
            float distanceFromTarget = (qt.t - m_state->orbitTarget).GetLength();
            Vec3 ydir = qt.GetColumn1().GetNormalized();
            m_state->orbitTarget = qt.t + ydir * distanceFromTarget;

            CameraMoved(qt, false);

            AzQtComponents::SetCursorPos(mapToGlobal(m_mousePressPos));
        }
    }
    else if (m_cameraControlMode == CameraControlMode::PAN)
    {
        if (!(m_settings->camera.transformRestraint & eCameraTransformRestraint_Panning))
        {
            float speedScale = CalculateMoveSpeed(m_fastMode, m_slowMode, true) * 3;
            speedScale = max(0.1f, speedScale);

            // Slide.
            QuatT qt = m_state->cameraTarget;
            Vec3 xdir = qt.GetColumn0().GetNormalized();
            Vec3 zdir = qt.GetColumn2().GetNormalized();

            Vec3 delta = 0.0025f * xdir * aznumeric_cast<float>(point.x() - m_mousePressPos.x()) * speedScale + 0.0025f * zdir * aznumeric_cast<float>(m_mousePressPos.y() - point.y()) * speedScale;
            qt.t += delta;

            // Move the orbit target with the pan operation.  This ensures the
            // center of the orbit moves with the camera as it pans.
            m_state->orbitTarget += delta;

            CameraMoved(qt, false);

            AzQtComponents::SetCursorPos(mapToGlobal(m_mousePressPos));
        }
    }
    else if (m_cameraControlMode == CameraControlMode::ORBIT)
    {
        // Rotate around orbit target.
        QuatT   cameraTarget = m_state->cameraTarget;
        Vec3    at = cameraTarget.t - m_state->orbitTarget;
        float   distanceFromTarget = at.GetLength();
        if (distanceFromTarget > 0.001f)
        {
            at /= distanceFromTarget;
        }
        else
        {
            at = Vec3(0.0f, m_state->orbitRadius, 0.0f);
            distanceFromTarget = m_state->orbitRadius;
        }

        Vec3                up = Vec3(0.0f, 0.0f, 1.0f);
        const Vec3  right = at.Cross(up).GetNormalized();
        up = right.Cross(at).GetNormalized();

        Ang3                angles = CCamera::CreateAnglesYPR(Matrix33::CreateFromVectors(right, at, up));
        const Ang3  delta = Ang3(aznumeric_cast<float>(-point.y() + m_mousePressPos.y()), 0.0f, aznumeric_cast<float>(-point.x() + m_mousePressPos.x())) * 0.002f * m_settings->camera.rotationSpeed;
        angles.x += delta.z;
        angles.y -= delta.x;
        angles.y = clamp_tpl(angles.y, -1.5f, 1.5f);

        cameraTarget.t = m_state->orbitTarget + CCamera::CreateOrientationYPR(angles).TransformVector(Vec3(0.0f, distanceFromTarget, 0.0f));
        m_state->orbitRadius = distanceFromTarget;

        CameraMoved(cameraTarget, true);

        AzQtComponents::SetCursorPos(mapToGlobal(m_mousePressPos));
    }
}

void QViewport::ProcessKeys()
{
    if (!m_renderContextCreated)
    {
        return;
    }

    float deltaTime = m_lastFrameTime;

    if (deltaTime > 0.1f)
    {
        deltaTime = 0.1f;
    }

    QuatT qt = m_state->cameraTarget;
    Vec3 ydir = qt.GetColumn1().GetNormalized();
    Vec3 xdir = qt.GetColumn0().GetNormalized();
    Vec3 pos = qt.t;

    float moveSpeed = CalculateMoveSpeed(m_fastMode, m_slowMode);
    bool hasPressedKey = false;

    if ((m_useArrowsForNavigation && CheckVirtualKey(Qt::Key_Up)) || CheckVirtualKey(Qt::Key_W))
    {
        hasPressedKey = true;
        Vec3 delta = deltaTime * moveSpeed * ydir;
        qt.t += delta;
        m_state->orbitTarget += delta;
        CameraMoved(qt, false);
    }

    if ((m_useArrowsForNavigation && CheckVirtualKey(Qt::Key_Down)) || CheckVirtualKey(Qt::Key_S))
    {
        hasPressedKey = true;
        Vec3 delta = deltaTime * moveSpeed * ydir;
        qt.t -= delta;
        m_state->orbitTarget -= delta;
        CameraMoved(qt, false);
    }

    if (m_cameraControlMode != CameraControlMode::ORBIT && ((m_useArrowsForNavigation && CheckVirtualKey(Qt::Key_Left)) || CheckVirtualKey(Qt::Key_A)))
    {
        hasPressedKey = true;
        Vec3 delta = deltaTime * moveSpeed * xdir;
        qt.t -= delta;
        m_state->orbitTarget -= delta;
        CameraMoved(qt, false);
    }

    if (m_cameraControlMode != CameraControlMode::ORBIT && ((m_useArrowsForNavigation && CheckVirtualKey(Qt::Key_Right)) || CheckVirtualKey(Qt::Key_D)))
    {
        hasPressedKey = true;
        Vec3 delta = deltaTime * moveSpeed * xdir;
        qt.t += delta;
        m_state->orbitTarget += delta;
        CameraMoved(qt, false);
    }

    if (CheckVirtualKey(Qt::RightButton) | CheckVirtualKey(Qt::MiddleButton))
    {
        hasPressedKey = true;
    }
}

void QViewport::CameraMoved(QuatT qt, bool snap)
{
    if (m_cameraControlMode == CameraControlMode::ORBIT)
    {
        CreateLookAt(m_state->orbitTarget, m_state->orbitRadius, qt);
    }
    m_state->cameraTarget = qt;
    if (snap)
    {
        m_state->lastCameraTarget = qt;
    }
    SignalCameraMoved(qt);
}

void QViewport::OnKeyEvent(const SKeyEvent& ev)
{
    for (size_t i = 0; i < m_consumers.size(); ++i)
    {
        m_consumers[i]->OnViewportKey(ev);
    }
    SignalKey(ev);
}

void QViewport::OnMouseEvent(const SMouseEvent& ev)
{
    if (ev.type == SMouseEvent::EType::TYPE_MOVE)
    {
        // Make sure we don't process more than one mouse event per frame, so we don't
        // end up consuming all the "idle" time
        ++m_mouseMovementsSinceLastFrame;

        if (m_mouseMovementsSinceLastFrame > 1)
        {
            // we can't discard all movement events, the last one should be delivered.
            m_pendingMouseMoveEvent = ev;
            return;
        }
    }

    for (size_t i = 0; i < m_consumers.size(); ++i)
    {
        m_consumers[i]->OnViewportMouse(ev);
    }
    SignalMouse(ev);
}

void QViewport::PreRender()
{
    SRenderContext rc;
    rc.camera = m_camera.get();
    rc.viewport = this;

    SignalPreRender(rc);


    const float fov = DEG2RAD(m_settings->camera.fov);
    const float fTime = m_lastFrameTime;
    float lastRotWeight = 0.0f;

    QuatT targetTM = m_state->cameraTarget;
    QuatT currentTM = m_state->lastCameraTarget;

    if ((targetTM.t - currentTM.t).len() > 0.0001f)
    {
        SmoothCD(currentTM.t, m_cameraSmoothPosRate, fTime, targetTM.t, m_settings->camera.smoothPos);
    }
    else
    {
        m_cameraSmoothPosRate = Vec3(0);
    }

    SmoothCD(lastRotWeight, m_cameraSmoothRotRate, fTime, 1.0f, m_settings->camera.smoothRot);

    if (lastRotWeight >= 1.0f)
    {
        m_cameraSmoothRotRate = 0.0f;
    }

    currentTM = QuatT(Quat::CreateNlerp(currentTM.q, targetTM.q, lastRotWeight), currentTM.t);

    m_state->lastCameraParentFrame = m_state->cameraParentFrame;
    m_state->lastCameraTarget = currentTM;

    m_camera->SetFrustum(m_width, m_height, fov, m_settings->camera.nearClip, GetIEditor()->GetEnv()->p3DEngine->GetMaxViewDistance());
    m_camera->SetMatrix(Matrix34(m_state->cameraParentFrame * currentTM));
}

void QViewport::Render()
{
    IRenderAuxGeom* aux = GetIEditor()->GetEnv()->pRenderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags oldFlags = aux->GetRenderFlags();

    if (m_settings->grid.showGrid)
    {
        aux->SetRenderFlags(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeNone | e_DepthWriteOff | e_DepthTestOn);
        DrawGrid(*aux, *m_state, m_settings->grid);
    }

    if (m_settings->grid.origin)
    {
        aux->SetRenderFlags(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeNone | e_DepthWriteOff | e_DepthTestOn);
        DrawOrigin(*aux, m_settings->grid.originColor);
    }

    if (m_settings->camera.showViewportOrientation)
    {
        aux->SetRenderFlags(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeNone | e_DepthWriteOn | e_DepthTestOn);
        TransformationMatrices backupSceneMatrices;
        GetIEditor()->GetEnv()->pRenderer->Set2DMode(m_width, m_height, backupSceneMatrices);
        DrawOrigin(*aux, 50, m_height - 50, 20.0f, m_camera->GetMatrix());
        GetIEditor()->GetEnv()->pRenderer->Unset2DMode(backupSceneMatrices);
    }

    // Force grid, origin and viewport orientation to render by calling Flush(). This ensures that they are always drawn behind other geometry
    aux->Flush();
    aux->SetRenderFlags(oldFlags);

    // wireframe mode
    CScopedWireFrameMode scopedWireFrame(GetIEditor()->GetEnv()->pRenderer, m_settings->rendering.wireframe ? R_WIREFRAME_MODE : R_SOLID_MODE);

    SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(*m_camera, SRenderingPassInfo::DEFAULT_FLAGS, true);
    GetIEditor()->GetEnv()->pRenderer->BeginSpawningGeneratingRendItemJobs(passInfo.ThreadID());
    GetIEditor()->GetEnv()->pRenderer->BeginSpawningShadowGeneratingRendItemJobs(passInfo.ThreadID());
    GetIEditor()->GetEnv()->pRenderer->EF_ClearSkinningDataPool();
    GetIEditor()->GetEnv()->pRenderer->EF_StartEf(passInfo);

    SRendParams rp;


    //---------------------------------------------------------------------------------------
    //---- add light    -------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    /////////////////////////////////////////////////////////////////////////////////////
    // Confetti Start
    /////////////////////////////////////////////////////////////////////////////////////
    // If time of day enabled, add sun light to preview - Confetti Vera.
    if (m_settings->rendering.sunlight)
    {
        rp.AmbientColor.r = GetIEditor()->Get3DEngine()->GetSunColor().x / 255.0f * m_settings->lighting.m_brightness;
        rp.AmbientColor.g = GetIEditor()->Get3DEngine()->GetSunColor().y / 255.0f * m_settings->lighting.m_brightness;
        rp.AmbientColor.b = GetIEditor()->Get3DEngine()->GetSunColor().z / 255.0f * m_settings->lighting.m_brightness;

        m_private->m_sun.SetPosition(passInfo.GetCamera().GetPosition() + GetIEditor()->Get3DEngine()->GetSunDir());
        // The radius value respect the sun radius settings in Engine.
        // Please refer to the function C3DEngine::UpdateSun(const SRenderingPassInfo &passInfo). -- Vera, Confetti
        m_private->m_sun.m_fRadius = 100000000; //Radius of the sun from Engine.
        m_private->m_sun.SetLightColor(GetIEditor()->Get3DEngine()->GetSunColor());
        m_private->m_sun.SetSpecularMult(GetIEditor()->Get3DEngine()->GetGlobalParameter(E3DPARAM_SUN_SPECULAR_MULTIPLIER));
        m_private->m_sun.m_Flags |= DLF_DIRECTIONAL | DLF_SUN | DLF_THIS_AREA_ONLY | DLF_LM | DLF_SPECULAROCCLUSION |
            ((GetIEditor()->Get3DEngine()->IsSunShadows() && passInfo.RenderShadows()) ? DLF_CASTSHADOW_MAPS : 0);
        m_private->m_sun.m_sName = "Sun";

        GetIEditor()->GetEnv()->pRenderer->EF_ADDDlight(&m_private->m_sun, passInfo);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    // Confetti End
    /////////////////////////////////////////////////////////////////////////////////////
    else // Add directional light
    {
        rp.AmbientColor.r = m_settings->lighting.m_ambientColor.r / 255.0f * m_settings->lighting.m_brightness;
        rp.AmbientColor.g = m_settings->lighting.m_ambientColor.g / 255.0f * m_settings->lighting.m_brightness;
        rp.AmbientColor.b = m_settings->lighting.m_ambientColor.b / 255.0f * m_settings->lighting.m_brightness;

        // Directional light
        if (m_settings->lighting.m_useLightRotation)
        {
            m_LightRotationRadian += m_averageFrameTime;
        }
        if (m_LightRotationRadian > gf_PI)
        {
            m_LightRotationRadian = -gf_PI;
        }

        Matrix33 LightRot33 =   Matrix33::CreateRotationZ(m_LightRotationRadian);

        f32 lightMultiplier = m_settings->lighting.m_lightMultiplier;
        f32 lightSpecMultiplier = m_settings->lighting.m_lightSpecMultiplier;

        f32 lightOrbit = 15.0f;
        Vec3 LPos0 = Vec3(-lightOrbit, lightOrbit, lightOrbit / 2);
        m_private->m_VPLight0.SetPosition(LightRot33 * LPos0);

        Vec3 d0;
        d0.x = f32(m_settings->lighting.m_directionalLightColor.r) / 255.0f;
        d0.y = f32(m_settings->lighting.m_directionalLightColor.g) / 255.0f;
        d0.z = f32(m_settings->lighting.m_directionalLightColor.b) / 255.0f;
        m_private->m_VPLight0.SetLightColor(ColorF(d0.x * lightMultiplier, d0.y * lightMultiplier, d0.z * lightMultiplier, 0));
        m_private->m_VPLight0.SetSpecularMult(lightSpecMultiplier);

        m_private->m_VPLight0.m_Flags = DLF_SUN | DLF_DIRECTIONAL;
        GetIEditor()->GetEnv()->pRenderer->EF_ADDDlight(&m_private->m_VPLight0, passInfo);
    }

    //---------------------------------------------------------------------------------------

    Matrix34 tm(IDENTITY);
    rp.pMatrix = &tm;
    rp.pPrevMatrix = &tm;

    rp.dwFObjFlags  = 0;

    SRenderContext rc;
    rc.camera = m_camera.get();
    rc.viewport = this;
    rc.passInfo = &passInfo;
    rc.renderParams = &rp;


    for (size_t i = 0; i < m_consumers.size(); ++i)
    {
        m_consumers[i]->OnViewportRender(rc);
    }
    SignalRender(rc);

    if ((m_settings->rendering.fps == true) && (m_averageFrameTime != 0.0f))
    {
        GetIEditor()->GetEnv()->pRenderer->Draw2dLabel(12.0f, 12.0f, 1.25f, ColorF(1, 1, 1, 1), false, "FPS: %.2f", 1.0f / m_averageFrameTime);
    }

    GetIEditor()->GetEnv()->pRenderer->EF_EndEf3D(SHDF_STREAM_SYNC, -1, -1, passInfo);

    if (m_mouseMovementsSinceLastFrame > 0)
    {
        m_mouseMovementsSinceLastFrame = 0;

        // Make sure we deliver at least last mouse movement event
        OnMouseEvent(m_pendingMouseMoveEvent);
    }
}

void QViewport::RenderInternal()
{
    {
        threadID mainThread = 0;
        threadID renderThread = 0;
        GetIEditor()->GetEnv()->pRenderer->GetThreadIDs(mainThread, renderThread);
        const threadID currentThreadId = CryGetCurrentThreadId();

        // I'm not sure if this criteria is right. It might not be restrictive enough, but it's at least strict enough to prevent
        // the crash we encountered.
        const uint32 workerThreadId = AZ::JobContext::GetGlobalContext()->GetJobManager().GetWorkerThreadId();
        const bool isValidThread = (workerThreadId != AZ::JobManager::InvalidWorkerThreadId) || mainThread == currentThreadId || renderThread == currentThreadId;

        if (!isValidThread)
        {
            AZ_Assert(false, "Attempting to render QViewport on unsupported thread %" PRI_THREADID, currentThreadId);
            return;
        }
    }


    SetCurrentContext();
    GetIEditor()->GetEnv()->pSystem->RenderBegin();

    ColorF viewportBackgroundColor(m_settings->background.topColor.r / 255.0f, m_settings->background.topColor.g / 255.0f, m_settings->background.topColor.b / 255.0f);
    GetIEditor()->GetEnv()->pRenderer->ClearTargetsImmediately(FRT_CLEAR, viewportBackgroundColor);
    GetIEditor()->GetEnv()->pRenderer->ResetToDefault();

    // Call PreRender to interpolate the new camera position
    PreRender();
    GetIEditor()->GetEnv()->pRenderer->SetCamera(*m_camera);

    IRenderAuxGeom* aux = GetIEditor()->GetEnv()->pRenderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags oldFlags = aux->GetRenderFlags();

    if (m_settings->background.useGradient)
    {
        Vec3 frustumVertices[8];
        m_camera->GetFrustumVertices(frustumVertices);
        Vec3 lt = Vec3::CreateLerp(frustumVertices[0], frustumVertices[4], 0.10f);
        Vec3 lb = Vec3::CreateLerp(frustumVertices[1], frustumVertices[5], 0.10f);
        Vec3 rb = Vec3::CreateLerp(frustumVertices[2], frustumVertices[6], 0.10f);
        Vec3 rt = Vec3::CreateLerp(frustumVertices[3], frustumVertices[7], 0.10f);
        aux->SetRenderFlags(e_Mode3D | e_AlphaNone | e_FillModeSolid | e_CullModeNone | e_DepthWriteOff | e_DepthTestOn);
        ColorB topColor = m_settings->background.topColor;
        ColorB bottomColor = m_settings->background.bottomColor;
        aux->DrawTriangle(lt, topColor,     rt, topColor,    rb, bottomColor);
        aux->DrawTriangle(lb, bottomColor,  rb, bottomColor, lt, topColor);
        aux->Flush();
    }
    aux->SetRenderFlags(oldFlags);

    Render();

    bool renderStats = false;
    GetIEditor()->GetEnv()->pSystem->RenderEnd(renderStats, false);
    RestorePreviousContext();
}

void QViewport::GetImageOffscreen(CImageEx& image, const QSize& customSize)
{
    if ((m_width == 0) || (m_height == 0))
    {
        // This can occur, for example, if the material editor window is sized to zero OR
        // if it is docked as a tab in another view pane window and is not the active tab when the editor starts.
        image.Allocate(1, 1);
        image.Clear();
        return;
    }

    IRenderer* renderer = GetIEditor()->GetRenderer();

    renderer->EnableSwapBuffers(false);
    RenderInternal();
    renderer->EnableSwapBuffers(true);

    int w;
    int h;

    if (customSize.isValid())
    {
        w = customSize.width();
        h = customSize.height();
    }
    else
    {
        w = width();
        h = height();
    }

    image.Allocate(w, h);

    // the renderer will read the frame buffer of the current render context, so we need to set ours as the current before we execute this command.
    SetCurrentContext();
    renderer->ReadFrameBufferFast(image.GetData(), w, h);
    RestorePreviousContext();
}

void QViewport::SetWindowTitle(const AZStd::string& title)
{
    // Do not support the WindowRequestBus changing the editor window title
    AZ_UNUSED(title);
}

AzFramework::WindowSize QViewport::GetClientAreaSize() const
{
    const QWidget* window = this->window();
    QSize windowSize = window->size();
    return AzFramework::WindowSize(windowSize.width(), windowSize.height());
}

void QViewport::ResizeClientArea(AzFramework::WindowSize clientAreaSize)
{
    QWidget* window = this->window();
    window->resize(aznumeric_cast<int>(clientAreaSize.m_width), aznumeric_cast<int>(clientAreaSize.m_height));
}

bool QViewport::GetFullScreenState() const
{
    // QViewport does not currently support full screen.
    return false;
}

void QViewport::SetFullScreenState([[maybe_unused]]bool fullScreenState)
{
    // QViewport does not currently support full screen.
}

bool QViewport::CanToggleFullScreenState() const
{
    // QViewport does not currently support full screen.
    return false;
}

void QViewport::ToggleFullScreenState()
{
    // QViewport does not currently support full screen.
}

void QViewport::ResetCamera()
{
    *m_state = SViewportState();
    m_camera->SetMatrix(Matrix34(m_state->cameraTarget));
}

void QViewport::SetSettings(const SViewportSettings& settings)
{
    *m_settings = settings;
}

void QViewport::SetState(const SViewportState& state)
{
    *m_state = state;
}

float QViewport::CalculateMoveSpeed(bool shiftPressed, bool ctrlPressed, bool scaleWithOrbitDistance) const
{
    // The value used to caculate speedScale respects the value used in RenderViewPort.
    // Please refer to the function: CRenderViewport::ProcessKeys().-- Vera, Confetti
    float speedScale = 20;

    speedScale *= m_settings->camera.moveSpeed;

    float moveSpeed = speedScale;

    if (shiftPressed)
    {
        moveSpeed *= m_settings->camera.fastMoveMultiplier;
    }
    if (ctrlPressed)
    {
        moveSpeed *= m_settings->camera.slowMoveMultiplier;
    }
    if (scaleWithOrbitDistance)
    {
        // Slow the movement down as we get closer to the orbit target
        QuatT qt = m_state->cameraTarget;
        float distanceFromTarget = (qt.t - m_state->orbitTarget).GetLength();
        moveSpeed *= distanceFromTarget * 0.01f;
        // Prevent the speed from going too close to 0, which would prevent movement
        moveSpeed = max(0.001f, moveSpeed);
    }

    return moveSpeed;
}

void QViewport::CreateLookAt(const Vec3& target, float radius, QuatT& cameraTarget) const
{
    Vec3    at = target - cameraTarget.t;
    float   distanceFromTarget = at.GetLength();
    if (distanceFromTarget > 0.001f)
    {
        at /= distanceFromTarget;
    }
    else
    {
        at                                  = Vec3(0.0f, radius, 0.0f);
        distanceFromTarget  = radius;
    }
    if (distanceFromTarget < radius)
    {
        distanceFromTarget  = radius;
        cameraTarget.t          = target - (at * radius);
    }
    Vec3                up = Vec3(0.0f, 0.0f, 1.0f);
    const Vec3  right   = at.Cross(up).GetNormalized();
    up                          = right.Cross(at).GetNormalized();
    cameraTarget.q  = Quat(Matrix33::CreateFromVectors(right, at, up));
}

void QViewport::UpdateCameraControlMode(QMouseEvent* ev)
{
    Qt::MouseButton mouseButton = ev->button();
    Qt::KeyboardModifiers modifiers = ev->modifiers();
    if (mouseButton & Qt::RightButton && mouseButton & Qt::MiddleButton)
    {
        m_cameraControlMode = CameraControlMode::ZOOM;
    }
    else if (mouseButton == Qt::MiddleButton)
    {
        if (modifiers & Qt::ALT)
        {
            m_cameraControlMode = CameraControlMode::ORBIT;
        }
        else
        {
            if (m_cameraControlMode == CameraControlMode::ROTATE)
            {
                m_cameraControlMode = CameraControlMode::ZOOM;
            }
            else
            {
                m_cameraControlMode = CameraControlMode::PAN;
            }
        }
    }
    else if (mouseButton == Qt::RightButton)
    {
        if (m_cameraControlMode == CameraControlMode::PAN || (modifiers & Qt::ALT))
        {
            m_cameraControlMode = CameraControlMode::ZOOM;
        }
        else
        {
            m_cameraControlMode = CameraControlMode::ROTATE;
        }
    }
    else
    {
        m_cameraControlMode = CameraControlMode::NONE;
    }
}

void QViewport::mousePressEvent(QMouseEvent* ev)
{
    SMouseEvent me;
    me.type = SMouseEvent::TYPE_PRESS;
    me.button = SMouseEvent::EButton(ev->button());
    me.x = ev->x();
    me.y = ev->y();
    me.viewport = this;
    me.shift = (ev->modifiers() & Qt::SHIFT) != 0;
    me.control = (ev->modifiers() & Qt::CTRL) != 0;
    OnMouseEvent(me);

    QWidget::mousePressEvent(ev);
    setFocus();

    m_mousePressPos = ev->pos();

    UpdateCameraControlMode(ev);
    if (m_cameraControlMode != CameraControlMode::NONE)
    {
        QApplication::setOverrideCursor(Qt::BlankCursor);
    }
}

void QViewport::mouseReleaseEvent(QMouseEvent* ev)
{
    SMouseEvent me;
    me.type = SMouseEvent::TYPE_RELEASE;
    me.button = SMouseEvent::EButton(ev->button());
    me.x = ev->x();
    me.y = ev->y();
    me.viewport = this;
    OnMouseEvent(me);

    m_cameraControlMode = CameraControlMode::NONE;
    QWidget::mouseReleaseEvent(ev);
    QApplication::restoreOverrideCursor();
}

void QViewport::wheelEvent(QWheelEvent* ev)
{
    QuatT qt = m_state->cameraTarget;
    Vec3 ydir = qt.GetColumn1().GetNormalized();
    Vec3 pos = qt.t;
    const float wheelSpeed = m_settings->camera.zoomSpeed * (m_fastMode ? m_settings->camera.fastMoveMultiplier : 1.0f) * (m_slowMode ? m_settings->camera.slowMoveMultiplier : 1.0f);
    pos += 0.01f * ydir * aznumeric_cast<float>(ev->angleDelta().y()) * wheelSpeed;
    qt.t = pos;
    CameraMoved(qt, false);
}

void QViewport::mouseMoveEvent(QMouseEvent* ev)
{
    SMouseEvent me;
    me.type = SMouseEvent::TYPE_MOVE;
    me.button = SMouseEvent::EButton(ev->button());
    me.x = ev->x();
    me.y = ev->y();
    me.viewport = this;
    m_fastMode = (ev->modifiers() & Qt::SHIFT) != 0;
    m_slowMode = (ev->modifiers() & Qt::CTRL) != 0;
    OnMouseEvent(me);

    QWidget::mouseMoveEvent(ev);
}

void QViewport::keyPressEvent(QKeyEvent* ev)
{
    SKeyEvent event;
    event.type = SKeyEvent::TYPE_PRESS;
    event.key = ev->key() | ev->modifiers();
    m_fastMode = (ev->modifiers() & Qt::SHIFT) != 0;
    m_slowMode = (ev->modifiers() & Qt::CTRL) != 0;
    OnKeyEvent(event);

    QWidget::keyPressEvent(ev);
}

void QViewport::keyReleaseEvent(QKeyEvent* ev)
{
    SKeyEvent event;
    event.type = SKeyEvent::TYPE_RELEASE;
    event.key = ev->key() | ev->modifiers();
    m_fastMode = (ev->modifiers() & Qt::SHIFT) != 0;
    m_slowMode = (ev->modifiers() & Qt::CTRL) != 0;
    OnKeyEvent(event);
    QWidget::keyReleaseEvent(ev);
}

void QViewport::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);

#if defined(AZ_PLATFORM_WINDOWS)
    // Needed for high DPI mode on windows
    const qreal ratio = devicePixelRatioF();
#else
    const qreal ratio = 1.0f;
#endif
    int cx = aznumeric_cast<int>(ev->size().width() * ratio);
    int cy = aznumeric_cast<int>(ev->size().height() * ratio);
    if (cx == 0 || cy == 0)
    {
        return;
    }

    m_width = cx;
    m_height = cy;

    // We queue the window resize event in case the windows is hidden.
    // If the QWidget is hidden, the native windows does not resize and the
    // swapchain may have the incorrect size. We need to wait
    // until it's visible to trigger the resize event.
    m_resizeWindowEvent = true;

    GetIEditor()->GetEnv()->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, cx, cy);
    SignalUpdate();
    Update();
}

void QViewport::showEvent(QShowEvent* ev)
{
    // force a context create once we're shown
    // This must be queued, as the showEvent is sent before the widget is actually shown, not after
    QMetaObject::invokeMethod(this, "ForceRebuildRenderContext", Qt::QueuedConnection);

    QWidget::showEvent(ev);
}

void QViewport::ForceRebuildRenderContext()
{
    CreateRenderContext();
}

void QViewport::moveEvent(QMoveEvent* ev)
{
    QWidget::moveEvent(ev);

    GetIEditor()->GetEnv()->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, ev->pos().x(), ev->pos().y());
}

bool QViewport::event(QEvent* ev)
{
    bool result = QWidget::event(ev);

    if (ev->type() == QEvent::WinIdChange)
    {
        CreateRenderContext();
    }

    if (ev->type() == QEvent::ShortcutOverride)
    {
        // When a shortcut is matched, Qt's event processing sends out a shortcut override event
        // to allow other systems to override it. If it's not overridden, then the key events
        // get processed as a shortcut, even if the widget that's the target has a keyPress event
        // handler. So, we need to communicate that we've processed the shortcut override
        // which will tell Qt not to process it as a shortcut and instead pass along the
        // keyPressEvent.

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
        QKeySequence key(keyEvent->key() | keyEvent->modifiers());

        for (size_t i = 0; i < m_consumers.size(); ++i)
        {
            if (m_consumers[i]->ProcessesViewportKey(key))
            {
                ev->accept();
                return true;
            }
        }
    }

    return result;
}

void QViewport::paintEvent(QPaintEvent* ev)
{
    QWidget::paintEvent(ev);
}

void QViewport::AddConsumer(QViewportConsumer* consumer)
{
    RemoveConsumer(consumer);
    m_consumers.push_back(consumer);
}

void QViewport::RemoveConsumer(QViewportConsumer* consumer)
{
    m_consumers.erase(std::remove(m_consumers.begin(), m_consumers.end(), consumer), m_consumers.end());
}

void QViewport::SetUseArrowsForNavigation(bool useArrowsForNavigation)
{
    m_useArrowsForNavigation = useArrowsForNavigation;
}

