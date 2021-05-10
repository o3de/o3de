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

#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// Class to implement the WindowRequestBus::Handler instead of the QViewport class.
// This is to bypass a link warning that occurs in unity builds if EditorCommon dll
// is linked along with a gem that also implements WindowRequestBus::Handler. The
// issue is that QViewport has a dllexport so it causes duplicate code to be linked
// in and a warning that there will be two of the same symbol in memory
class QViewportRequests
    : public AzFramework::WindowRequestBus::Handler
{
public:
    QViewportRequests(QViewport& viewport)
        : m_viewport(viewport)
    {
    }

    ~QViewportRequests() override
    {
        AzFramework::WindowRequestBus::Handler::BusDisconnect();
    }

    // WindowRequestBus::Handler...
    void SetWindowTitle(const AZStd::string& title) override
    {
        m_viewport.SetWindowTitle(title);
    }

    AzFramework::WindowSize GetClientAreaSize() const override
    {
        return m_viewport.GetClientAreaSize();
    }

    void ResizeClientArea(AzFramework::WindowSize clientAreaSize) override
    {
        m_viewport.ResizeClientArea(clientAreaSize);
    }

    bool GetFullScreenState() const override
    {
        return m_viewport.GetFullScreenState();
    }

    void SetFullScreenState(bool fullScreenState) override
    {
        m_viewport.SetFullScreenState(fullScreenState);
    }

    bool CanToggleFullScreenState() const override
    {
        return m_viewport.CanToggleFullScreenState();
    }

    void ToggleFullScreenState() override
    {
        m_viewport.ToggleFullScreenState();
    }

private:
    QViewport& m_viewport;
};

struct QViewport::SPreviousContext
{
    CCamera renderCamera;
    CCamera systemCamera;
    int width;
    int height;
    HWND window;
    bool isMainViewport;
};

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
    m_viewportRequests = AZStd::make_unique<QViewportRequests>(*this);

    if (startupMode & StartupMode_Immediate)
        Startup();
}

void QViewport::Startup()
{
    m_frameTimer = new QElapsedTimer();

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
    m_viewportRequests.reset();
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
    AZ_UNUSED(ray);
    AZ_UNUSED(x);
    AZ_UNUSED(y);
    return false;
}

QPoint QViewport::ProjectToScreen(const Vec3&)
{
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

    m_camera->SetFrustum(m_width, m_height, fov, m_settings->camera.nearClip);
    m_camera->SetMatrix(Matrix34(m_state->cameraParentFrame * currentTM));
}

void QViewport::Render()
{
}

void QViewport::RenderInternal()
{
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

    GetIEditor()->GetEnv()->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, cx, cy);
    SignalUpdate();
    Update();
}

void QViewport::showEvent(QShowEvent* ev)
{
    QWidget::showEvent(ev);
}

void QViewport::moveEvent(QMoveEvent* ev)
{
    QWidget::moveEvent(ev);

    GetIEditor()->GetEnv()->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, ev->pos().x(), ev->pos().y());
}

bool QViewport::event(QEvent* ev)
{
    bool result = QWidget::event(ev);

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

