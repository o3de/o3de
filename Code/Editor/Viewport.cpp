/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "Viewport.h"

// Qt
#include <QPainter>

// AzQtComponents
#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>

#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

// Editor
#include "ViewManager.h"
#include "Include/ITransformManipulator.h"
#include "Include/HitContext.h"
#include "Objects/ObjectManager.h"
#include "Util/3DConnexionDriver.h"
#include "PluginManager.h"
#include "Include/IRenderListener.h"
#include "GameEngine.h"
#include "Settings.h"


#ifdef LoadCursor
#undef LoadCursor
#endif

//////////////////////////////////////////////////////////////////////
// Viewport drag and drop support
//////////////////////////////////////////////////////////////////////

void QtViewport::BuildDragDropContext(AzQtComponents::ViewportDragContext& context, const QPoint& pt)
{
    context.m_hitLocation = AZ::Vector3::CreateZero();
    context.m_hitLocation = GetHitLocation(pt);
}


void QtViewport::dragEnterEvent(QDragEnterEvent* event)
{
    if (!GetIEditor()->GetGameEngine()->IsLevelLoaded())
    {
        return;
    }

    // first use the legacy pathway, which assumes its always okay as long as any callback is installed
    if (m_dropCallback)
    {
        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);
    }
    else
    {
        // new bus-based way of doing it (install a listener!)
        using namespace AzQtComponents;
        ViewportDragContext context;
        BuildDragDropContext(context, event->pos());
        DragAndDropEventsBus::Event(DragAndDropContexts::EditorViewport, &DragAndDropEvents::DragEnter, event, context);
    }
}

void QtViewport::dragMoveEvent(QDragMoveEvent* event)
{
    if (!GetIEditor()->GetGameEngine()->IsLevelLoaded())
    {
        return;
    }

    // first use the legacy pathway, which assumes its always okay as long as any callback is installed
    if (m_dropCallback)
    {
        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);
    }
    else
    {
        // new bus-based way of doing it (install a listener!)
        using namespace AzQtComponents;
        ViewportDragContext context;
        BuildDragDropContext(context, event->pos());
        DragAndDropEventsBus::Event(DragAndDropContexts::EditorViewport, &DragAndDropEvents::DragMove, event, context);
    }
}

void QtViewport::dropEvent(QDropEvent* event)
{
    using namespace AzQtComponents;
    if (!GetIEditor()->GetGameEngine()->IsLevelLoaded())
    {
        return;
    }

    // first use the legacy pathway, which assumes its always okay as long as any callback is installed
    if (m_dropCallback)
    {
        m_dropCallback(this, event->pos().x(), event->pos().y(), m_dropCallbackCustom);
        event->setAccepted(true);
    }
    else
    {
        // new bus-based way of doing it (install a listener!)
        ViewportDragContext context;
        BuildDragDropContext(context, event->pos());
        DragAndDropEventsBus::Event(DragAndDropContexts::EditorViewport, &DragAndDropEvents::Drop, event, context);
    }
}

void QtViewport::dragLeaveEvent(QDragLeaveEvent* event)
{
    using namespace AzQtComponents;
    DragAndDropEventsBus::Event(DragAndDropContexts::EditorViewport, &DragAndDropEvents::DragLeave, event);
}


//////////////////////////////////////////////////////////////////////////
float CViewport::GetFOV() const
{
    return gSettings.viewports.fDefaultFov;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool QtViewport::m_bDegradateQuality = false;

QtViewport::QtViewport(QWidget* parent)
    : QWidget(parent)
    , m_renderOverlay(this)
{
    m_cViewMenu.addMenu(tr("&View Options"))->addAction(tr("&Fullscreen"));
    //connect(action, &QAction::triggered, );
    m_selectionTolerance = 0;
    m_fGridZoom = 1.0f;

    m_nLastUpdateFrame = 0;
    m_nLastMouseMoveFrame = 0;

    //////////////////////////////////////////////////////////////////////////
    // Init standard cursors.
    //////////////////////////////////////////////////////////////////////////
    m_hCurrCursor = QCursor();
    m_hCursor[STD_CURSOR_DEFAULT] = QCursor(Qt::ArrowCursor);
    m_hCursor[STD_CURSOR_HIT] = CMFCUtils::LoadCursor(IDC_POINTER_OBJHIT);
    m_hCursor[STD_CURSOR_MOVE] = CMFCUtils::LoadCursor(IDC_POINTER_OBJECT_MOVE);
    m_hCursor[STD_CURSOR_ROTATE] = CMFCUtils::LoadCursor(IDC_POINTER_OBJECT_ROTATE);
    m_hCursor[STD_CURSOR_SCALE] = CMFCUtils::LoadCursor(IDC_POINTER_OBJECT_SCALE);
    m_hCursor[STD_CURSOR_SEL_PLUS] = CMFCUtils::LoadCursor(IDC_POINTER_PLUS);
    m_hCursor[STD_CURSOR_SEL_MINUS] = CMFCUtils::LoadCursor(IDC_POINTER_MINUS);
    m_hCursor[STD_CURSOR_SUBOBJ_SEL] = CMFCUtils::LoadCursor(IDC_POINTER_SO_SELECT);
    m_hCursor[STD_CURSOR_SUBOBJ_SEL_PLUS] = CMFCUtils::LoadCursor(IDC_POINTER_SO_SELECT_PLUS);
    m_hCursor[STD_CURSOR_SUBOBJ_SEL_MINUS] = CMFCUtils::LoadCursor(IDC_POINTER_SO_SELECT_MINUS);

    m_activeAxis = AXIS_TERRAIN;

    for (int i = 0; i < LAST_COORD_SYSTEM; i++)
    {
        m_constructionMatrix[i].SetIdentity();
    }
    m_screenTM.SetIdentity();

    m_pMouseOverObject = nullptr;

    m_bAdvancedSelectMode = false;

    m_constructionPlane.SetPlane(Vec3_OneZ, Vec3_Zero);
    m_constructionPlaneAxisX = Vec3_Zero;
    m_constructionPlaneAxisY = Vec3_Zero;

    GetIEditor()->GetViewManager()->RegisterViewport(this);

    m_nCurViewportID = MAX_NUM_VIEWPORTS - 1;
    m_dropCallback = nullptr; // Leroy@Conffx

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Create drop target to handle Qt drop events.
    setAcceptDrops(true);

    m_renderOverlay.setVisible(true);
    m_renderOverlay.setUpdatesEnabled(false);
    m_renderOverlay.setMouseTracking(true);
    m_renderOverlay.setObjectName("renderOverlay");
    m_renderOverlay.winId(); // Force the render overlay to create a backing native window

    m_viewportUi.InitializeViewportUi(this, &m_renderOverlay);

    setAcceptDrops(true);
}

//////////////////////////////////////////////////////////////////////////
QtViewport::~QtViewport()
{
    GetIEditor()->GetViewManager()->UnregisterViewport(this);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::ScreenToClient(QPoint& pPoint) const
{
    pPoint = mapFromGlobal(pPoint);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::GetDimensions(int* pWidth, int* pHeight) const
{
    if (pWidth)
    {
        *pWidth = width();
    }
    if (pHeight)
    {
        *pHeight = height();
    }
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::RegisterRenderListener(IRenderListener*    piListener)
{
#ifdef _DEBUG
    size_t nCount(0);
    size_t nTotal(0);

    nTotal = m_cRenderListeners.size();
    for (nCount = 0; nCount < nTotal; ++nCount)
    {
        if (m_cRenderListeners[nCount] == piListener)
        {
            assert(!"Registered the same RenderListener multiple times.");
            break;
        }
    }
#endif //_DEBUG
    m_cRenderListeners.push_back(piListener);
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::UnregisterRenderListener(IRenderListener*  piListener)
{
    size_t nCount(0);
    size_t nTotal(0);

    nTotal = m_cRenderListeners.size();
    for (nCount = 0; nCount < nTotal; ++nCount)
    {
        if (m_cRenderListeners[nCount] == piListener)
        {
            m_cRenderListeners.erase(m_cRenderListeners.begin() + nCount);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::IsRenderListenerRegistered(IRenderListener*    piListener)
{
    size_t nCount(0);
    size_t nTotal(0);

    nTotal = m_cRenderListeners.size();
    for (nCount = 0; nCount < nTotal; ++nCount)
    {
        if (m_cRenderListeners[nCount] == piListener)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::AddPostRenderer(IPostRenderer* pPostRenderer)
{
    stl::push_back_unique(m_postRenderers, pPostRenderer);
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::RemovePostRenderer(IPostRenderer* pPostRenderer)
{
    PostRenderers::iterator itr = m_postRenderers.begin();
    PostRenderers::iterator end = m_postRenderers.end();
    for (; itr != end; ++itr)
    {
        if (*itr == pPostRenderer)
        {
            break;
        }
    }

    if (itr != end)
    {
        m_postRenderers.erase(itr);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnMouseWheel([[maybe_unused]] Qt::KeyboardModifiers modifiers, short zDelta, [[maybe_unused]] const QPoint& pt)
{
    if (zDelta != 0)
    {
        float z = GetZoomFactor() + (zDelta / 120.0f) * 0.5f;

        SetZoomFactor(z);
        GetIEditor()->GetViewManager()->SetZoomFactor(z);
    }
}

//////////////////////////////////////////////////////////////////////////
QString QtViewport::GetName() const
{
    return windowTitle();
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetName(const QString& name)
{
    setWindowTitle(name);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_renderOverlay.setGeometry(rect());
    Update();
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    MouseCallback(eMouseLeave, QPoint(), Qt::KeyboardModifiers(), Qt::MouseButtons());
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QPainter painter(this);

    // Fill the entire client area
    painter.fillRect(rect(), QColor(0xf0, 0xf0, 0xf0));
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnActivate()
{
    ////////////////////////////////////////////////////////////////////////
    // Make this edit window the current one
    ////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnDeactivate()
{
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::ResetContent()
{
    m_pMouseOverObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::UpdateContent(int flags)
{
    if (flags & eRedrawViewports)
    {
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::Update()
{
    m_viewportUi.Update();

    m_bAdvancedSelectMode = false;

    if (CheckVirtualKey(Qt::Key_Space) && !CheckVirtualKey(Qt::Key_Shift) && hasFocus())
    {
        m_bAdvancedSelectMode = true;
    }

    m_nLastUpdateFrame++;
}

//////////////////////////////////////////////////////////////////////////
QPoint QtViewport::WorldToView(const Vec3& wp) const
{
    return QPoint(static_cast<int>(wp.x), static_cast<int>(wp.y));
}

//////////////////////////////////////////////////////////////////////////
Vec3 QtViewport::WorldToView3D(const Vec3& wp, [[maybe_unused]] int nFlags) const
{
    QPoint p = WorldToView(wp);
    Vec3 out;
    out.x = static_cast<f32>(p.x());
    out.y = static_cast<f32>(p.y());
    out.z = wp.z;
    return out;
}

//////////////////////////////////////////////////////////////////////////
Vec3    QtViewport::ViewToWorld(const QPoint& vp, bool* pCollideWithTerrain, [[maybe_unused]] bool onlyTerrain, [[maybe_unused]] bool bSkipVegetation, [[maybe_unused]] bool bTestRenderMesh, [[maybe_unused]] bool* collideWithObject) const
{
    Vec3 wp;
    wp.x = static_cast<f32>(vp.x());
    wp.y = static_cast<f32>(vp.y());
    wp.z = 0;
    if (pCollideWithTerrain)
    {
        *pCollideWithTerrain = true;
    }
    return wp;
}

//////////////////////////////////////////////////////////////////////////
Vec3    QtViewport::ViewToWorldNormal([[maybe_unused]] const QPoint& vp, [[maybe_unused]] bool onlyTerrain, [[maybe_unused]] bool bTestRenderMesh)
{
    return Vec3(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
void    QtViewport::ViewToWorldRay([[maybe_unused]] const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const
{
    raySrc(0, 0, 0);
    rayDir(0, 0, -1);
}

void QtViewport::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->modifiers(), event->pos());
        break;
    }
}

void QtViewport::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->modifiers(), event->pos());
        break;
    }

    // For MFC compatibility, send a spurious move event after a button release.
    // CryDesigner depends on this behaviour
    mouseMoveEvent(event);
}

void QtViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDblClk(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonDblClk(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonDblClk(event->modifiers(), event->pos());
        break;
    }
}

void QtViewport::mouseMoveEvent(QMouseEvent* event)
{
    OnMouseMove(event->modifiers(), event->buttons(), event->pos());
    OnSetCursor();
}

void QtViewport::wheelEvent(QWheelEvent* event)
{
    OnMouseWheel(event->modifiers(), static_cast<short>(event->angleDelta().y()), event->position().toPoint());
    event->accept();
}

void QtViewport::keyPressEvent(QKeyEvent* event)
{
    int nativeKey = event->nativeVirtualKey();
#if AZ_TRAIT_OS_PLATFORM_APPLE
    // nativeVirtualKey is always zero on macOS, therefore we
    // need to manually set the nativeKey based on the Qt key
    switch (event->key())
    {
        case Qt::Key_Control:
            nativeKey = VK_CONTROL;
            break;
        case Qt::Key_Alt:
            nativeKey = VK_MENU;
            break;
        case Qt::Key_QuoteLeft:
            nativeKey = VK_OEM_3;
            break;
        case Qt::Key_BracketLeft:
            nativeKey = VK_OEM_4;
            break;
        case Qt::Key_BracketRight:
            nativeKey = VK_OEM_6;
            break;
        case Qt::Key_Comma:
            nativeKey = VK_OEM_COMMA;
            break;
        case Qt::Key_Period:
            nativeKey = VK_OEM_PERIOD;
            break;
        case Qt::Key_Escape:
            nativeKey = VK_ESCAPE;
            break;
    }
#endif
    OnKeyDown(nativeKey, 1, event->nativeModifiers());
}

void QtViewport::keyReleaseEvent(QKeyEvent* event)
{
    int nativeKey = event->nativeVirtualKey();
#if AZ_TRAIT_OS_PLATFORM_APPLE
    // nativeVirtualKey is always zero on macOS, therefore we
    // need to manually set the nativeKey based on the Qt key
    switch (event->key())
    {
        case Qt::Key_Control:
            nativeKey = VK_CONTROL;
            break;
        case Qt::Key_Alt:
            nativeKey = VK_MENU;
            break;
        case Qt::Key_QuoteLeft:
            nativeKey = VK_OEM_3;
            break;
        case Qt::Key_BracketLeft:
            nativeKey = VK_OEM_4;
            break;
        case Qt::Key_BracketRight:
            nativeKey = VK_OEM_6;
            break;
        case Qt::Key_Comma:
            nativeKey = VK_OEM_COMMA;
            break;
        case Qt::Key_Period:
            nativeKey = VK_OEM_PERIOD;
            break;
        case Qt::Key_Escape:
            nativeKey = VK_ESCAPE;
            break;
    }
#endif
    OnKeyUp(nativeKey, 1, event->nativeModifiers());
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    // Save the mouse down position
    m_cMouseDownPos = point;

    if (MouseCallback(eMouseLDown, point, modifiers))
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    // Check Edit Tool.
    MouseCallback(eMouseLUp, point, modifiers);
}
//////////////////////////////////////////////////////////////////////////
void QtViewport::OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    MouseCallback(eMouseRDown, point, modifiers);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnRButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    MouseCallback(eMouseRUp, point, modifiers);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    // Check Edit Tool.
    MouseCallback(eMouseMDown, point, modifiers);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    // Move the viewer to the mouse location.
    // Check Edit Tool.
    MouseCallback(eMouseMUp, point, modifiers);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnMButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    MouseCallback(eMouseMDblClick, point, modifiers);
}


//////////////////////////////////////////////////////////////////////////
void QtViewport::OnMouseMove(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, const QPoint& point)
{
    MouseCallback(eMouseMove, point, modifiers, buttons);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnSetCursor()
{
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::ResetSelectionRegion()
{
    AABB box(Vec3(0, 0, 0), Vec3(0, 0, 0));
    GetIEditor()->SetSelectedRegion(box);
    m_selectedRect = QRect();
}

void QtViewport::SetSelectionRectangle(const QRect& rect)
{
    m_selectedRect = rect.normalized();
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnDragSelectRectangle(const QRect& rect, bool bNormalizeRect)
{
    Vec3 org;
    AABB box;
    box.Reset();

    //adjust QRect bottom and right corner once before extracting bottom/right coordinates
    const QRect correctedRect = rect.adjusted(0, 0, 1, 1);
    Vec3 p1 = ViewToWorld(correctedRect.topLeft());
    Vec3 p2 = ViewToWorld(correctedRect.bottomRight());
    org = p1;
    // Calculate selection volume.
    if (!bNormalizeRect)
    {
        box.Add(p1);
        box.Add(p2);
    }
    else
    {
        const QRect rc = correctedRect.normalized();
        box.Add(ViewToWorld(rc.topLeft()));
        box.Add(ViewToWorld(rc.topRight()));
        box.Add(ViewToWorld(rc.bottomLeft()));
        box.Add(ViewToWorld(rc.bottomRight()));
    }

    box.min.z = -10000;
    box.max.z = 10000;
    GetIEditor()->SetSelectedRegion(box);

    // Show marker position in the status bar
    float w = box.max.x - box.min.x;
    float h = box.max.y - box.min.y;
    char szNewStatusText[512];
    sprintf_s(szNewStatusText, "X:%g Y:%g Z:%g  W:%g H:%g", org.x, org.y, org.z, w, h);
    GetIEditor()->SetStatusText(szNewStatusText);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (GetIEditor()->IsInGameMode())
    {
        // Ignore double clicks while in game.
        return;
    }

    MouseCallback(eMouseLDblClick, point, modifiers);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnRButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    MouseCallback(eMouseRDblClick, point, modifiers);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnKeyDown([[maybe_unused]] UINT nChar, [[maybe_unused]] UINT nRepCnt, [[maybe_unused]] UINT nFlags)
{
    if (GetIEditor()->IsInGameMode())
    {
        // Ignore key downs while in game.
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::OnKeyUp([[maybe_unused]] UINT nChar, [[maybe_unused]] UINT nRepCnt, [[maybe_unused]] UINT nFlags)
{
    if (GetIEditor()->IsInGameMode())
    {
        // Ignore key downs while in game.
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetCurrentCursor(const QCursor& hCursor, const QString& cursorString)
{
    setCursor(hCursor);
    m_cursorStr = cursorString;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetCurrentCursor(EStdCursor stdCursor, const QString& cursorString)
{
    QCursor hCursor = m_hCursor[stdCursor];
    setCursor(hCursor);
    m_cursorStr = cursorString;
}

void QtViewport::SetSupplementaryCursorStr(const QString& str)
{
    m_cursorSupplementaryStr = str;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetCurrentCursor(EStdCursor stdCursor)
{
    QCursor hCursor = m_hCursor[stdCursor];
    setCursor(hCursor);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetCursorString(const QString& cursorString)
{
    m_cursorStr = cursorString;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::ResetCursor()
{
    SetCurrentCursor(STD_CURSOR_DEFAULT, QString());
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetConstructionOrigin(const Vec3& worldPos)
{
    Matrix34 tm;
    tm.SetIdentity();
    tm.SetTranslation(worldPos);
    SetConstructionMatrix(COORDS_LOCAL, tm);
    SetConstructionMatrix(COORDS_PARENT, tm);
    SetConstructionMatrix(COORDS_USERDEFINED, tm);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetConstructionMatrix(RefCoordSys coordSys, const Matrix34& xform)
{
    m_constructionMatrix[coordSys] = xform;
    m_constructionMatrix[coordSys].OrthonormalizeFast(); // Remove scale component from matrix.
    if (coordSys == COORDS_LOCAL)
    {
        m_constructionMatrix[COORDS_VIEW].SetTranslation(xform.GetTranslation());
        m_constructionMatrix[COORDS_WORLD].SetTranslation(xform.GetTranslation());
        m_constructionMatrix[COORDS_USERDEFINED].SetIdentity();
        m_constructionMatrix[COORDS_USERDEFINED].SetTranslation(xform.GetTranslation());
        m_constructionMatrix[COORDS_PARENT] = xform;
    }
    MakeConstructionPlane(GetAxisConstrain());
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& QtViewport::GetConstructionMatrix(RefCoordSys coordSys)
{
    if (coordSys == COORDS_VIEW)
    {
        return m_constructionMatrix[COORDS_WORLD];
    }
    return m_constructionMatrix[coordSys];
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::AssignConstructionPlane(const Vec3& p1, const Vec3& p2, const Vec3& p3)
{
    m_constructionPlane.SetPlane(p1, p2, p3);
    m_constructionPlaneAxisX = p3 - p1;
    m_constructionPlaneAxisY = p2 - p1;
}

//////////////////////////////////////////////////////////////////////////
HWND QtViewport::renderOverlayHWND() const
{
    return reinterpret_cast<HWND>(m_renderOverlay.winId());
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::setRenderOverlayVisible(bool visible)
{
    m_renderOverlay.setVisible(visible);
}

bool QtViewport::isRenderOverlayVisible() const
{
    return m_renderOverlay.isVisible();
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::MakeConstructionPlane(int axis)
{
    QPoint cursorPos;
    if (m_mouseCaptured)
    {
        cursorPos = m_cMouseDownPos;
    }
    else
    {
        cursorPos = QCursor::pos();
        ScreenToClient(cursorPos);
    }

    Vec3 raySrc(Vec3_Zero), rayDir(Vec3_OneX);
    ViewToWorldRay(cursorPos, raySrc, rayDir);

    int coordSys = GetIEditor()->GetReferenceCoordSys();

    Vec3 xAxis(Vec3_OneX);
    Vec3 yAxis(Vec3_OneY);
    Vec3 zAxis(Vec3_OneZ);

    xAxis = m_constructionMatrix[coordSys].TransformVector(xAxis);
    yAxis = m_constructionMatrix[coordSys].TransformVector(yAxis);
    zAxis = m_constructionMatrix[coordSys].TransformVector(zAxis);

    Vec3 pos = m_constructionMatrix[coordSys].GetTranslation();

    if (axis == AXIS_X)
    {
        // X direction.
        Vec3 n;
        float d1 = fabs(rayDir.Dot(yAxis));
        float d2 = fabs(rayDir.Dot(zAxis));
        if (d1 > d2)
        {
            n = yAxis;
        }
        else
        {
            n = zAxis;
        }
        if (rayDir.Dot(n) < 0)
        {
            n = -n;                    // face construction plane to the ray.
        }
        //Vec3 n = Vec3(0,0,1);
        Vec3 v1 = n.Cross(xAxis);
        Vec3 v2 = n.Cross(v1);
        AssignConstructionPlane(pos, pos + v2, pos + v1);
    }
    else if (axis == AXIS_Y)
    {
        // Y direction.
        Vec3 n;
        float d1 = fabs(rayDir.Dot(xAxis));
        float d2 = fabs(rayDir.Dot(zAxis));
        if (d1 > d2)
        {
            n = xAxis;
        }
        else
        {
            n = zAxis;
        }
        if (rayDir.Dot(n) < 0)
        {
            n = -n;                    // face construction plane to the ray.
        }
        Vec3 v1 = n.Cross(yAxis);
        Vec3 v2 = n.Cross(v1);
        AssignConstructionPlane(pos, pos + v2, pos + v1);
    }
    else if (axis == AXIS_Z)
    {
        // Z direction.
        Vec3 n;
        float d1 = fabs(rayDir.Dot(xAxis));
        float d2 = fabs(rayDir.Dot(yAxis));
        if (d1 > d2)
        {
            n = xAxis;
        }
        else
        {
            n = yAxis;
        }
        if (rayDir.Dot(n) < 0)
        {
            n = -n;                    // face construction plane to the ray.
        }
        Vec3 v1 = n.Cross(zAxis);
        Vec3 v2 = n.Cross(v1);
        AssignConstructionPlane(pos, pos + v2, pos + v1);
    }
    else if (axis == AXIS_XY)
    {
        AssignConstructionPlane(pos, pos + yAxis, pos + xAxis);
    }
    else if (axis == AXIS_XZ)
    {
        AssignConstructionPlane(pos, pos + zAxis, pos + xAxis);
    }
    else if (axis == AXIS_YZ)
    {
        AssignConstructionPlane(pos, pos + zAxis, pos + yAxis);
    }
    else
    {
        AssignConstructionPlane(pos, pos + yAxis, pos + xAxis);
    }
}

//////////////////////////////////////////////////////////////////////////
Vec3 QtViewport::MapViewToCP(const QPoint& point, int axis)
{
    AZ_PROFILE_FUNCTION(Editor);

    if (axis == AXIS_TERRAIN)
    {
        return SnapToGrid(ViewToWorld(point));
    }

    MakeConstructionPlane(axis);

    Vec3 raySrc(Vec3_Zero), rayDir(Vec3_OneX);
    ViewToWorldRay(point, raySrc, rayDir);
    
    Vec3 v;

    Ray ray(raySrc, rayDir);
    if (!Intersect::Ray_Plane(ray, m_constructionPlane, v))
    {
        Plane inversePlane = m_constructionPlane;
        inversePlane.n = -inversePlane.n;
        inversePlane.d = -inversePlane.d;
        if (!Intersect::Ray_Plane(ray, inversePlane, v))
        {
            v = Vec3_Zero;
        }
    }

    // Snap value to grid.
    v = SnapToGrid(v);

    return v;
}

//////////////////////////////////////////////////////////////////////////
Vec3 QtViewport::GetCPVector(const Vec3& p1, const Vec3& p2, int axis)
{
    Vec3 v = p2 - p1;

    int coordSys = GetIEditor()->GetReferenceCoordSys();

    Vec3 xAxis(Vec3_OneX);
    Vec3 yAxis(Vec3_OneY);
    Vec3 zAxis(Vec3_OneZ);

    // In local coordinate system transform axises by construction matrix.
    xAxis = m_constructionMatrix[coordSys].TransformVector(xAxis);
    yAxis = m_constructionMatrix[coordSys].TransformVector(yAxis);
    zAxis = m_constructionMatrix[coordSys].TransformVector(zAxis);

    if (axis == AXIS_X || axis == AXIS_Y || axis == AXIS_Z)
    {
        // Project vector v on transformed axis x,y or z.
        Vec3 axisVector;
        if (axis == AXIS_X)
        {
            axisVector = xAxis;
        }
        if (axis == AXIS_Y)
        {
            axisVector = yAxis;
        }
        if (axis == AXIS_Z)
        {
            axisVector = zAxis;
        }

        // Project vector on construction plane into the one of axises.
        v = v.Dot(axisVector) * axisVector;
    }
    else if (axis == AXIS_XY)
    {
        // Project vector v on transformed plane x/y.
        Vec3 planeNormal = xAxis.Cross(yAxis);
        Vec3 projV = v.Dot(planeNormal) * planeNormal;
        v = v - projV;
    }
    else if (axis == AXIS_XZ)
    {
        // Project vector v on transformed plane x/y.
        Vec3 planeNormal = xAxis.Cross(zAxis);
        Vec3 projV = v.Dot(planeNormal) * planeNormal;
        v = v - projV;
    }
    else if (axis == AXIS_YZ)
    {
        // Project vector v on transformed plane x/y.
        Vec3 planeNormal = yAxis.Cross(zAxis);
        Vec3 projV = v.Dot(planeNormal) * planeNormal;
        v = v - projV;
    }
    else if (axis == AXIS_TERRAIN)
    {
        v.z = 0;
    }
    return v;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetAxisConstrain(int axis)
{
    m_activeAxis = axis;
};

AzToolsFramework::ViewportInteraction::MouseInteraction QtViewport::BuildMouseInteraction(
    [[maybe_unused]] Qt::MouseButtons buttons, [[maybe_unused]] Qt::KeyboardModifiers modifiers, [[maybe_unused]] const QPoint& point)
{
    // Implemented by sub-class
    return AzToolsFramework::ViewportInteraction::MouseInteraction();
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::HitTest(const QPoint& point, HitContext& hitInfo)
{
    Vec3 raySrc(0, 0, 0), rayDir(1, 0, 0);
    ViewToWorldRay(point, hitInfo.raySrc, hitInfo.rayDir);
    hitInfo.view = this;
    hitInfo.point2d = point;
    if (m_bAdvancedSelectMode)
    {
        hitInfo.bUseSelectionHelpers = true;
    }

    const int viewportId = GetViewportId();

    AzToolsFramework::EntityIdList visibleEntityIds;
    AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Event(
        viewportId, &AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Events::FindVisibleEntities,
        visibleEntityIds);

    // Look through all visible entities to find the closest one to the specified mouse point
    using namespace AzToolsFramework::ViewportInteraction;
    AZ::EntityId entityIdUnderCursor;
    float closestDistance = std::numeric_limits<float>::max();
    MouseInteraction mouseInteraction = BuildMouseInteraction(QGuiApplication::mouseButtons(),
        QGuiApplication::queryKeyboardModifiers(),
        point);
    for (auto entityId : visibleEntityIds)
    {
        using AzFramework::ViewportInfo;
        // Check if components provide an aabb
        if (const AZ::Aabb aabb = AzToolsFramework::CalculateEditorEntitySelectionBounds(entityId, ViewportInfo{ viewportId });
            aabb.IsValid())
        {
            // Coarse grain check
            if (AzToolsFramework::AabbIntersectMouseRay(mouseInteraction, aabb))
            {
                // If success, pick against specific component
                if (AzToolsFramework::PickEntity(
                    entityId, mouseInteraction,
                    closestDistance, viewportId))
                {
                    entityIdUnderCursor = entityId;
                }
            }
        }
    }

    // If we hit a valid Entity, then store the distance in the HitContext
    // so that the caller can use this for calculations
    if (entityIdUnderCursor.IsValid())
    {
        hitInfo.dist = closestDistance;
        return true;
    }

    return false;
}

AZ::Vector3 QtViewport::GetHitLocation(const QPoint& point)
{
    Vec3 pos = Vec3(ZERO);
    HitContext hit;
    if (HitTest(point, hit))
    {
        pos = hit.raySrc + hit.rayDir * hit.dist;
        pos = SnapToGrid(pos);
    }
    else
    {
        bool hitTerrain;
        pos = ViewToWorld(point, &hitTerrain);
        if (hitTerrain)
        {
            pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
        }
        pos = SnapToGrid(pos);
    }

    return AZ::Vector3(pos.x, pos.y, pos.z);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::SetZoomFactor(float fZoomFactor)
{
    m_fZoomFactor = fZoomFactor;
    if (gSettings.viewports.bSync2DViews && GetType() != ET_ViewportCamera && GetType() != ET_ViewportModel)
    {
        GetViewManager()->SetZoom2D(fZoomFactor);
    }
};

//////////////////////////////////////////////////////////////////////////
float QtViewport::GetZoomFactor() const
{
    if (gSettings.viewports.bSync2DViews && GetType() != ET_ViewportCamera && GetType() != ET_ViewportModel)
    {
        m_fZoomFactor = GetViewManager()->GetZoom2D();
    }
    return m_fZoomFactor;
};

//////////////////////////////////////////////////////////////////////////
Vec3 QtViewport::SnapToGrid(const Vec3& vec)
{
    return vec;
}

float QtViewport::GetGridStep() const
{
    return 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::BeginUndo()
{
    DegradateQuality(true);
    GetIEditor()->BeginUndo();
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::AcceptUndo(const QString& undoDescription)
{
    DegradateQuality(false);
    GetIEditor()->AcceptUndo(undoDescription);
    GetIEditor()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::CancelUndo()
{
    DegradateQuality(false);
    GetIEditor()->CancelUndo();
    GetIEditor()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::RestoreUndo()
{
    GetIEditor()->RestoreUndo();
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::IsUndoRecording() const
{
    return GetIEditor()->IsUndoRecording();
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::DegradateQuality(bool bEnable)
{
    m_bDegradateQuality = bEnable;
}

//////////////////////////////////////////////////////////////////////////
QSize QtViewport::GetIdealSize() const
{
    return QSize(0, 0);
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::IsBoundsVisible([[maybe_unused]] const AABB& box) const
{
    // Always visible in standard implementation.
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::HitTestLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& hitpoint, int pixelRadius, float* pToCameraDistance) const
{
    float dist = GetDistanceToLine(lineP1, lineP2, hitpoint);
    if (dist <= pixelRadius)
    {
        if (pToCameraDistance)
        {
            Vec3 raySrc, rayDir;
            ViewToWorldRay(hitpoint, raySrc, rayDir);
            Vec3 rayTrg = raySrc + rayDir * 10000.0f;

            Vec3 pa, pb;
            float mua, mub;
            LineLineIntersect(lineP1, lineP2, raySrc, rayTrg, pa, pb, mua, mub);
            *pToCameraDistance = mub;
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
float QtViewport::GetDistanceToLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& point) const
{
    QPoint p1 = WorldToView(lineP1);
    QPoint p2 = WorldToView(lineP2);

    return PointToLineDistance2D(
        Vec3(static_cast<f32>(p1.x()), static_cast<f32>(p1.y()), 0.0f),
        Vec3(static_cast<f32>(p2.x()), static_cast<f32>(p2.y()), 0.0f),
        Vec3(static_cast<f32>(point.x()), static_cast<f32>(point.y()), 0.0f));
}

//////////////////////////////////////////////////////////////////////////
void QtViewport::GetPerpendicularAxis(EAxis* pAxis, bool* pIs2D) const
{
    EViewportType vpType = GetType();
    switch (vpType)
    {
    case ET_ViewportXY:
        if (pIs2D)
        {
            *pIs2D = true;
        }
        if (pAxis)
        {
            *pAxis = AXIS_Z;
        }
        break;
    case ET_ViewportXZ:
        if (pIs2D)
        {
            *pIs2D = true;
        }
        if (pAxis)
        {
            *pAxis = AXIS_Y;
        }
        break;
    case ET_ViewportYZ:
        if (pIs2D)
        {
            *pIs2D = true;
        }
        if (pAxis)
        {
            *pAxis = AXIS_X;
        }
        break;
    case ET_ViewportMap:
    case ET_ViewportZ:
        if (pIs2D)
        {
            *pIs2D = true;
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::GetAdvancedSelectModeFlag()
{
    return m_bAdvancedSelectMode;
}

//////////////////////////////////////////////////////////////////////////
bool QtViewport::MouseCallback(EMouseEvent event, const QPoint& point, Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons)
{
    AZ_PROFILE_FUNCTION(Editor);

    // Ignore any mouse events in game mode.
    if (GetIEditor()->IsInGameMode())
    {
        return true;
    }

    // We must ignore mouse events when we are in the middle of an assert.
    // Reason: If we have an assert called from an engine module under the editor, if we call this function,
    // it may call the engine again and cause a deadlock.
    // Concrete example: CryPhysics called from Trackview causing an assert, and moving the cursor over the viewport
    // would cause the editor to freeze as it calls CryPhysics again for a raycast while it didn't release the lock.
    if (gEnv->pSystem->IsAssertDialogVisible())
    {
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // Hit test gizmo objects.
    //////////////////////////////////////////////////////////////////////////
    bool bAltClick = (modifiers & Qt::AltModifier);
    bool bCtrlClick = (modifiers & Qt::ControlModifier);
    bool bShiftClick = (modifiers & Qt::ShiftModifier);

    int flags = (bCtrlClick ? MK_CONTROL : 0) |
        (bShiftClick ? MK_SHIFT : 0) |
        ((buttons& Qt::LeftButton) ? MK_LBUTTON : 0) |
        ((buttons& Qt::MiddleButton) ? MK_MBUTTON : 0) |
        ((buttons& Qt::RightButton) ? MK_RBUTTON : 0);

    switch (event)
    {
    case eMouseMove:

        if (m_nLastUpdateFrame == m_nLastMouseMoveFrame)
        {
            // If mouse move event generated in the same frame, ignore it.
            return false;
        }
        m_nLastMouseMoveFrame = m_nLastUpdateFrame;

        // Skip the marker position update if anything is selected, since it is only used
        // by the info bar which doesn't show the marker when there is an active selection.
        // This helps a performance issue when calling ViewToWorld (which calls RayWorldIntersection)
        // on every mouse movement becomes very expensive in scenes with large amounts of entities.
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        if (!(buttons & Qt::RightButton) /* && m_nLastUpdateFrame != m_nLastMouseMoveFrame*/ && (selection && selection->IsEmpty()))
        {
            //m_nLastMouseMoveFrame = m_nLastUpdateFrame;
            Vec3 pos = ViewToWorld(point);
            GetIEditor()->SetMarkerPosition(pos);
        }
        break;
    }

    QPoint tempPoint(point.x(), point.y());

    //////////////////////////////////////////////////////////////////////////
    // Handle viewport manipulators.
    //////////////////////////////////////////////////////////////////////////
    if (!bAltClick)
    {
        ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();
        if (pManipulator)
        {
            if (pManipulator->MouseCallback(this, event, tempPoint, flags))
            {
                return true;
            }
        }
    }

    return false;
}
//////////////////////////////////////////////////////////////////////////
void QtViewport::ProcessRenderLisneters(DisplayContext& rstDisplayContext)
{
    size_t nCount(0);
    size_t nTotal(0);

    nTotal = m_cRenderListeners.size();
    for (nCount = 0; nCount < nTotal; ++nCount)
    {
        m_cRenderListeners[nCount]->Render(rstDisplayContext);
    }
}
//////////////////////////////////////////////////////////////////////////
#if defined(AZ_PLATFORM_WINDOWS)
void QtViewport::OnRawInput([[maybe_unused]] UINT wParam, HRAWINPUT lParam)
{
    static C3DConnexionDriver* p3DConnexionDriver = 0;

    if (GetType() == ET_ViewportCamera)
    {
        if (!p3DConnexionDriver)
        {
            p3DConnexionDriver = (C3DConnexionDriver*)GetIEditor()->GetPluginManager()->GetPluginByGUID("{AD109901-9128-4ffd-8E67-137CB2B1C41B}");
        }
        if (p3DConnexionDriver)
        {
            S3DConnexionMessage msg;
            if (p3DConnexionDriver->GetInputMessageData((LPARAM)lParam, msg))
            {
                if (msg.bGotTranslation || msg.bGotRotation)
                {
                    static int all6DOFs[6] = { 0 };
                    if (msg.bGotTranslation)
                    {
                        all6DOFs[0] = msg.raw_translation[0];
                        all6DOFs[1] = msg.raw_translation[1];
                        all6DOFs[2] = msg.raw_translation[2];
                    }
                    if (msg.bGotRotation)
                    {
                        all6DOFs[3] = msg.raw_rotation[0];
                        all6DOFs[4] = msg.raw_rotation[1];
                        all6DOFs[5] = msg.raw_rotation[2];
                    }

                    Matrix34 viewTM = GetViewTM();

                    // Scale axis according to CVars
                    ICVar* sys_scale3DMouseTranslation = gEnv->pConsole->GetCVar("sys_scale3DMouseTranslation");
                    ICVar* sys_Scale3DMouseYPR = gEnv->pConsole->GetCVar("sys_Scale3DMouseYPR");
                    float fScaleYPR = sys_Scale3DMouseYPR->GetFVal();

                    float s = 0.01f * gSettings.cameraMoveSpeed;
                    Vec3 t = Vec3(s * all6DOFs[0], -s * all6DOFs[1], -s * all6DOFs[2] * 0.5f);
                    t *= sys_scale3DMouseTranslation->GetFVal();

                    float as = 0.001f * gSettings.cameraMoveSpeed;
                    Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(viewTM));
                    ypr.x += -all6DOFs[5] * as * fScaleYPR;
                    ypr.y = AZStd::clamp(ypr.y + all6DOFs[3] * as * fScaleYPR, -1.5f, 1.5f); // to keep rotation in reasonable range
                    ypr.z = 0;                                                  // to have camera always upward

                    viewTM = Matrix34(CCamera::CreateOrientationYPR(ypr), viewTM.GetTranslation());
                    viewTM = viewTM * Matrix34::CreateTranslationMat(t);

                    SetViewTM(viewTM);
                }
            }
        }
    }
}
#endif
//////////////////////////////////////////////////////////////////////////
float QtViewport::GetFOV() const
{
    return gSettings.viewports.fDefaultFov;
}
//////////////////////////////////////////////////////////////////////////
void QtViewport::setRay(QPoint& vp, Vec3& raySrc, Vec3& rayDir)
{
    m_vp = vp;
    m_raySrc = raySrc;
    m_rayDir = rayDir;
}
////////////////////////////////////////////////////////////////////////
void QtViewport::setHitcontext(QPoint& vp, Vec3& raySrc, Vec3& rayDir)
{
    vp = m_vp;
    raySrc = m_raySrc;
    rayDir = m_rayDir;
}

#include <moc_Viewport.cpp>
