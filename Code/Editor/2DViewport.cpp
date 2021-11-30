/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// Qt
#include <QTimer>

// Editor
#include "2DViewport.h"
#include "CryEditDoc.h"
#include "DisplaySettings.h"
#include "GameEngine.h"
#include "Settings.h"
#include "ViewManager.h"
#include "Include/IObjectManager.h"
#include "Include/HitContext.h"
#include "Objects/SelectionGroup.h"


#define MARKER_SIZE 6.0f
#define MARKER_DIR_SIZE 10.0f
#define SELECTION_RADIUS 30.0f

#define GL_RGBA 0x1908
#define GL_BGRA 0x80E1

#define BACKGROUND_COLOR Vec3(1.0f, 1.0f, 1.0f)
#define SELECTION_RECT_COLOR Vec3(0.8f, 0.8f, 0.8f)
#define MINOR_GRID_COLOR    Vec3(0.55f, 0.55f, 0.55f)
#define MAJOR_GRID_COLOR    Vec3(0.6f, 0.6f, 0.6f)
#define AXIS_GRID_COLOR     Vec3(0, 0, 0)
#define GRID_TEXT_COLOR     Vec3(0, 0, 1.0f)

#define MAX_WORLD_SIZE 10000

enum Viewport2dTitleMenuCommands
{
    ID_SHOW_LABELS,
    ID_SHOW_GRID
};


//////////////////////////////////////////////////////////////////////////
static void OnMenuLabels()
{
    GetIEditor()->GetDisplaySettings()->DisplayLabels(!GetIEditor()->GetDisplaySettings()->IsDisplayLabels());
}

//////////////////////////////////////////////////////////////////////////
static void OnMenuGrid()
{
    CViewport* pViewport = GetIEditor()->GetViewManager()->GetSelectedViewport();
    if (pViewport)
    {
        if (Q2DViewport* p2DViewport = viewport_cast<Q2DViewport*>(pViewport))
        {
            p2DViewport->SetShowGrid(!p2DViewport->GetShowGrid());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
inline Vec3 SnapToSize(Vec3 v, double size)
{
    Vec3 snapped;
    snapped.x = static_cast<f32>(floor((v.x / size) + 0.5) * size);
    snapped.y = static_cast<f32>(floor((v.y / size) + 0.5) * size);
    snapped.z = static_cast<f32>(floor((v.z / size) + 0.5) * size);
    return snapped;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
Q2DViewport::Q2DViewport(QWidget* parent)
    : QtViewport(parent)
{
    // Scroll offset equals origin
    m_rcSelect.setRect(0, 0, 0, 0);

    m_viewType = ET_ViewportXY;
    m_axis = VPA_XY;

    m_bShowTerrain = true;
    m_bShowGrid = true;
    m_bShowObjectsInfo = true;
    m_bShowMinorGridLines = true;
    m_bShowMajorGridLines = true;
    m_bShowNumbers = true;
    m_bAutoAdjustGrids = true;
    m_gridAlpha = 1;

    m_origin2D.Set(0, 0, 0);

    m_colorGridText = QColor(220, 220, 220);
    m_colorAxisText = QColor(220, 220, 220);
    m_colorBackground = QColor(128, 128, 128);

    m_screenTM.SetIdentity();
    m_screenTM_Inverted.SetIdentity();

    m_fZoomFactor = 1;
    m_bContentValid = false;

    m_bShowViewMarker = true;

    m_displayContext.pIconManager = GetIEditor()->GetIconManager();

    /*
       In MFC OnCreate was delayed behind the scenes, being called at some time
       after the constructor but just before the window is shown for the first
       time. Using QTimer is simple way to accomplish the same thing. 
       
       QTimer relies on the event loop, so by calling QTimer::singleShot with 0
       timeout, the constructor will finish immediately, followed by a call to
       OnCreate shortly after, once Qt's event loop is up and running. 
    */
    QTimer::singleShot(0, [&]() { OnCreate(); });
}

//////////////////////////////////////////////////////////////////////////
Q2DViewport::~Q2DViewport()
{
    OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::SetType(EViewportType type)
{
    m_viewType = type;

    switch (m_viewType)
    {
    case ET_ViewportXY:
        m_axis = VPA_XY;
        break;
    case ET_ViewportXZ:
        m_axis = VPA_XZ;
        break;
    case ET_ViewportYZ:
        m_axis = VPA_YZ;
        break;
    }
    ;

    SetAxis(m_axis);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::SetAxis(EViewportAxis axis)
{
    m_axis = axis;
    switch (m_axis)
    {
    case VPA_XY:
        m_cullAxis = 2;
        break;
    case VPA_YX:
        m_cullAxis = 2;
        break;
    case VPA_XZ:
        m_cullAxis = 1;
        break;
    case VPA_YZ:
        m_cullAxis = 0;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::CalculateViewTM()
{
    Matrix34 viewTM;
    Matrix34 tm;
    tm.SetIdentity();
    viewTM.SetIdentity();
    float fScale = GetZoomFactor();
    Vec3 origin = GetOrigin2D();

    float height = m_rcClient.height() / fScale;

    Vec3 v1;
    switch (m_axis)
    {
    case VPA_XY:
        tm = Matrix33::CreateScale(Vec3(fScale, -fScale, fScale)) * tm; // No fScale for Z
        tm.SetTranslation(Vec3(-origin.x, height + origin.y, 0) * fScale);
        break;
    case VPA_YX:
        tm.SetFromVectors(Vec3(0, 1, 0) * fScale, Vec3(1, 0, 0) * fScale, Vec3(0, 0, 1) * fScale, Vec3(0, 0, 0));
        tm.SetTranslation(Vec3(-origin.x, height + origin.y, 0) * fScale);
        break;
    case VPA_XZ:
        viewTM.SetFromVectors(Vec3(1, 0, 0), Vec3(0, 0, 1), Vec3(0, 1, 0), Vec3(0, 0, 0));
        tm.SetFromVectors(Vec3(1, 0, 0) * fScale, Vec3(0, 0, 1) * fScale, Vec3(0, -1, 0) * fScale, Vec3(0, 0, 0));
        tm.SetTranslation(Vec3(-origin.x, height + origin.z, 0) * fScale);
        break;

    case VPA_YZ:
        viewTM.SetFromVectors(Vec3(0, 1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0), Vec3(0, 0, 0));
        tm.SetFromVectors(Vec3(0, 0, 1) * fScale, Vec3(1, 0, 0) * fScale, Vec3(0, -1, 0) * fScale, Vec3(0, 0, 0)); // No fScale for Z
        tm.SetTranslation(Vec3(-origin.y, height + origin.z, 0) * fScale);
        break;
    }
    SetViewTM(viewTM);
    m_screenTM = tm;
    m_screenTM_Inverted = m_screenTM.GetInverted();

    SetConstructionMatrix(COORDS_VIEW, GetViewTM());
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::ResetContent()
{
    QtViewport::ResetContent();
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::UpdateContent(int flags)
{
    QtViewport::UpdateContent(flags);
    if (flags & eUpdateObjects)
    {
        m_bContentValid = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnRButtonDown([[maybe_unused]] Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (GetIEditor()->IsInGameMode())
    {
        return;
    }

    if (!hasFocus())
    {
        setFocus();
    }

    SetCurrentCursor(STD_CURSOR_MOVE, QString());

    // Save the mouse down position
    m_RMouseDownPos = point;

    m_prevZoomFactor = GetZoomFactor();
    //m_prevScrollOffset = m_cScrollOffset;

    CaptureMouse();
    SetViewMode(ScrollZoomMode);

    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnRButtonUp([[maybe_unused]] Qt::KeyboardModifiers modifiers, [[maybe_unused]] const QPoint& point)
{
    ReleaseMouse();
    SetViewMode(NothingMode);

    GetViewManager()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnMButtonDown([[maybe_unused]] Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    // Save the mouse down position
    m_RMouseDownPos = point;

    SetCurrentCursor(STD_CURSOR_MOVE, QString());

    // Save the mouse down position
    m_RMouseDownPos = point;

    m_prevZoomFactor = GetZoomFactor();

    SetViewMode(ScrollZoomMode);

    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnMButtonUp([[maybe_unused]] Qt::KeyboardModifiers modifiers, [[maybe_unused]] const QPoint& point)
{
    SetViewMode(NothingMode);

    ReleaseMouse();
    m_bContentValid = false;
    GetViewManager()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    ////////////////////////////////////////////////////////////////////////
    // User pressed the left mouse button
    ////////////////////////////////////////////////////////////////////////
    if (GetViewMode() != NothingMode)
    {
        return;
    }

    m_cMouseDownPos = point;
    m_prevZoomFactor = GetZoomFactor();
    //m_prevScrollOffset = m_cScrollOffset;

    QtViewport::OnLButtonDown(modifiers, point);
    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    ////////////////////////////////////////////////////////////////////////
    // Process the various events depending on the selection and the view
    // mode
    ////////////////////////////////////////////////////////////////////////
    QtViewport::OnLButtonUp(modifiers, point);

    GetViewManager()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnMouseWheel(Qt::KeyboardModifiers, short zDelta, [[maybe_unused]] const QPoint& pt)
{
    float z = GetZoomFactor();
    float scale = 1.2f * fabs(zDelta / 120.0f);
    if (zDelta >= 0)
    {
        z = z * scale;
    }
    else
    {
        z = z / scale;
    }
    SetZoom(z, m_cMousePos);

    GetViewManager()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnMouseMove(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, const QPoint& point)
{
    m_cMousePos = point;

    if (GetViewMode() == ScrollZoomMode)
    {
        // Capture the mouse after the user has moved a bit, otherwise the
        // right click context menu won't popup
        if (!m_mouseCaptured && ((point - m_RMouseDownPos).manhattanLength() > QApplication::startDragDistance()))
        {
            CaptureMouse();
        }

        // You can only scroll while the middle mouse button is down
        if (buttons & Qt::RightButton || buttons & Qt::MiddleButton)
        {
            if (modifiers & Qt::ShiftModifier)
            {
                // Zoom to mouse position.
                float z = m_prevZoomFactor + (point.y() - m_RMouseDownPos.y()) * 0.02f;
                SetZoom(z, m_RMouseDownPos);
            }
            else
            {
                // Set the new scrolled coordinates
                float fScale = GetZoomFactor();
                float ofsx, ofsy;
                GetScrollOffset(ofsx, ofsy);
                ofsx -= (point.x() - m_RMouseDownPos.x()) / fScale;
                ofsy += (point.y() - m_RMouseDownPos.y()) / fScale;
                SetScrollOffset(ofsx, ofsy);
                m_RMouseDownPos = point;
            }
        }
        return;
    }

    QtViewport::OnMouseMove(modifiers, buttons, point);

    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::SetScrollOffset(float x, float y, [[maybe_unused]] bool bLimits)
{
    Vec3 org = GetOrigin2D();
    switch (m_axis)
    {
    case VPA_XY:
    case VPA_YX:
        org.x = x;
        org.y = y;
        break;
    case VPA_XZ:
        org.x = x;
        org.z = y;
        break;
    case VPA_YZ:
        org.y = x;
        org.z = y;
        break;
    }

    SetOrigin2D(org);

    CalculateViewTM();
    //GetViewManager()->UpdateViews(eUpdateObjects);
    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::GetScrollOffset(float& x, float& y)
{
    Vec3 origin = GetOrigin2D();
    switch (m_axis)
    {
    case VPA_XY:
    case VPA_YX:
        x = origin.x;
        y = origin.y;
        break;
    case VPA_XZ:
        x = origin.x;
        y = origin.z;
        break;
    case VPA_YZ:
        x = origin.y;
        y = origin.z;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::SetZoom(float fZoomFactor, const QPoint& center)
{
    if (fZoomFactor < m_minZoom)
    {
        fZoomFactor = m_minZoom;
    }
    else if (fZoomFactor > m_maxZoom)
    {
        fZoomFactor = m_maxZoom;
    }

    // Zoom to mouse position.
    float ofsx, ofsy;
    GetScrollOffset(ofsx, ofsy);

    float s1 = GetZoomFactor();
    float s2 = fZoomFactor;

    SetZoomFactor(fZoomFactor);

    // Calculate new offset to center zoom on mouse.
    float x2 = static_cast<float>(center.x());
    float y2 = static_cast<float>(m_rcClient.height() - center.y());
    ofsx = -(x2 / s2 - x2 / s1 - ofsx);
    ofsy = -(y2 / s2 - y2 / s1 - ofsy);
    SetScrollOffset(ofsx, ofsy, true);

    CalculateViewTM();

    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::resizeEvent(QResizeEvent* event)
{
    ////////////////////////////////////////////////////////////////////////
    // Re-evaluate the zoom / scroll offset values
    // TODO: Restore the zoom rectangle instead of resetting it
    ////////////////////////////////////////////////////////////////////////

    QtViewport::resizeEvent(event);

    m_rcClient = rect();
    CalculateViewTM();
    m_bContentValid = false;
}

void Q2DViewport::showEvent(QShowEvent* event)
{
    QtViewport::showEvent(event);
    m_bContentValid = false;
    QMetaObject::invokeMethod(this, "Update", Qt::QueuedConnection);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    // nothing to draw here, Render() draws on the overlay widget which has no updates enabled to avoid flicker
    CGameEngine* ge = GetIEditor()->GetGameEngine();
    setRenderOverlayVisible(ge ? ge->IsLevelLoaded() : false);
}

//////////////////////////////////////////////////////////////////////////
int Q2DViewport::OnCreate()
{
    // Calculate the View transformation matrix.
    CalculateViewTM();

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::Update()
{
    if (!m_bContentValid)
    {
        Render();
    }
    m_bContentValid = true;
    QtViewport::Update();
}

//////////////////////////////////////////////////////////////////////////
QPoint Q2DViewport::WorldToView(const Vec3& wp) const
{
    Vec3 sp = m_screenTM.TransformPoint(wp);
    QPoint p = QPoint(static_cast<int>(sp.x), static_cast<int>(sp.y));
    return p;
}

//////////////////////////////////////////////////////////////////////////
Vec3    Q2DViewport::ViewToWorld(const QPoint& vp, [[maybe_unused]] bool* collideWithTerrain, [[maybe_unused]] bool onlyTerrain, [[maybe_unused]] bool bSkipVegetation, [[maybe_unused]] bool bTestRenderMesh, [[maybe_unused]] bool* collideWithObject) const
{
    Vec3 wp = m_screenTM_Inverted.TransformPoint(Vec3(static_cast<f32>(vp.x()), static_cast<f32>(vp.y()), 0.0f));
    switch (m_axis)
    {
    case VPA_XY:
    case VPA_YX:
        wp.z = 0;
        break;
    case VPA_XZ:
        wp.y = 0;
        break;
    case VPA_YZ:
        wp.x = 0;
        break;
    }
    return wp;
}

//////////////////////////////////////////////////////////////////////////
void    Q2DViewport::ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const
{
    raySrc = ViewToWorld(vp);
    switch (m_axis)
    {
    case VPA_XY:
    case VPA_YX:
        raySrc.z = MAX_WORLD_SIZE;
        rayDir(0, 0, -1);
        break;
    case VPA_XZ:
        raySrc.y = MAX_WORLD_SIZE;
        rayDir(0, -1, 0);
        break;
    case VPA_YZ:
        raySrc.x = MAX_WORLD_SIZE;
        rayDir(-1, 0, 0);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
float Q2DViewport::GetScreenScaleFactor([[maybe_unused]] const Vec3& worldPoint) const
{
    return 400.0f / GetZoomFactor();
    //return 100.0f / ;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnTitleMenu(QMenu* menu)
{
    bool bShowGrid = true;
    if (Q2DViewport* p2DViewport = viewport_cast<Q2DViewport*>(GetIEditor()->GetViewManager()->GetSelectedViewport()))
    {
        bShowGrid = p2DViewport->GetShowGrid();
    }

    QAction* action = menu->addAction(tr("Labels"));
    action->setCheckable(true);
    action->setChecked(GetIEditor()->GetDisplaySettings()->IsDisplayLabels());
    connect(action, &QAction::triggered, this, OnMenuLabels);

    action = menu->addAction(tr("Grid"));
    action->setCheckable(true);
    action->setChecked(bShowGrid);
    connect(action, &QAction::triggered, this, OnMenuGrid);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnDestroy()
{
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::Render()
{
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::Draw(DisplayContext& dc)
{
    dc.DepthTestOff();
    dc.DepthWriteOff();
    if (m_bShowGrid)
    {
        DrawGrid(dc);
    }

    DrawObjects(dc);
    DrawSelection(dc);
    if (m_bShowViewMarker)
    {
        DrawViewerMarker(dc);
    }
    DrawAxis(dc);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::DrawGrid(DisplayContext& dc, bool bNoXNumbers)
{
    float gridSize = 1.0f;
    if (gridSize < 0.00001f)
    {
        return;
    }

    float fZ = 0;

    //////////////////////////////////////////////////////////////////////////
    float fScale = GetZoomFactor();

    int width = m_rcClient.width();
    int height = m_rcClient.height();

    float origin[2];
    GetScrollOffset(origin[0], origin[1]);

    //////////////////////////////////////////////////////////////////////////
    // Draw Minor grid lines.
    //////////////////////////////////////////////////////////////////////////

    float pixelsPerGrid = gridSize * fScale;

    m_fGridZoom = 1.0f;

    if (m_bAutoAdjustGrids)
    {
        int griditers = 0;
        pixelsPerGrid = gridSize * fScale;
        while (pixelsPerGrid <= 5 && griditers++ < 20)
        {
            pixelsPerGrid = gridSize * fScale;
        }
    }

    Matrix34 viewTM = GetViewTM().GetInverted() * m_screenTM_Inverted;
    Matrix34 viewTM_Inv = m_screenTM * GetViewTM();

    Vec3 viewP0 = viewTM.TransformPoint(Vec3(0.0f, 0.0f, 0.0f));
    Vec3 viewP1 = viewTM.TransformPoint(Vec3(static_cast<f32>(m_rcClient.width()), static_cast<f32>(m_rcClient.height()), 0.0f));

    Vec3 viewP_Text = viewTM.TransformPoint(Vec3(0.0f, static_cast<f32>(m_rcClient.height()), 0.0f));

    if (m_bShowMinorGridLines && (!m_bAutoAdjustGrids || pixelsPerGrid > 5))
    {
        //////////////////////////////////////////////////////////////////////////
        // Draw Minor grid lines.
        //////////////////////////////////////////////////////////////////////////

        Vec3 vec0 = SnapToSize(viewP0, gridSize);
        Vec3 vec1 = SnapToSize(viewP1, gridSize);
        if (vec0.x > vec1.x)
        {
            std::swap(vec0.x, vec1.x);
        }
        if (vec0.y > vec1.y)
        {
            std::swap(vec0.y, vec1.y);
        }

        dc.SetColor(MINOR_GRID_COLOR, m_gridAlpha);
        for (float dy = vec0.y; dy < vec1.y; dy += gridSize)
        {
            Vec3 p0 = viewTM_Inv.TransformPoint(Vec3(viewP0.x, dy, 0));
            Vec3 p1 = viewTM_Inv.TransformPoint(Vec3(viewP1.x, dy, 0));
            dc.DrawLine(Vec3(p0.x, p0.y, fZ), Vec3(p1.x, p1.y, fZ));
        }
        for (float dx = vec0.x; dx < vec1.x; dx += gridSize)
        {
            Vec3 p0 = viewTM_Inv.TransformPoint(Vec3(dx, viewP0.y, 0));
            Vec3 p1 = viewTM_Inv.TransformPoint(Vec3(dx, viewP1.y, 0));
            dc.DrawLine(Vec3(p0.x, p0.y, fZ), Vec3(p1.x, p1.y, fZ));
        }
    }
    if (m_bShowMajorGridLines)
    {
        //////////////////////////////////////////////////////////////////////////
        // Draw Major grid lines.
        //////////////////////////////////////////////////////////////////////////
        gridSize = gridSize * 1.0f;

        if (m_bAutoAdjustGrids)
        {
            int iters = 0;
            pixelsPerGrid = gridSize * fScale;
            while (pixelsPerGrid < 20 && iters < 20)
            {
                gridSize = gridSize * 2;
                pixelsPerGrid = gridSize * fScale;
                iters++;
            }
        }
        if (!m_bAutoAdjustGrids || pixelsPerGrid > 20)
        {
            Vec3 vec0 = SnapToSize(viewP0, gridSize);
            Vec3 vec1 = SnapToSize(viewP1, gridSize);
            if (vec0.x > vec1.x)
            {
                std::swap(vec0.x, vec1.x);
            }
            if (vec0.y > vec1.y)
            {
                std::swap(vec0.y, vec1.y);
            }

            dc.SetColor(MAJOR_GRID_COLOR, m_gridAlpha);

            for (float dy = vec0.y; dy < vec1.y; dy += gridSize)
            {
                Vec3 p0 = viewTM_Inv.TransformPoint(Vec3(viewP0.x, dy, 0));
                Vec3 p1 = viewTM_Inv.TransformPoint(Vec3(viewP1.x, dy, 0));
                dc.DrawLine(Vec3(p0.x, p0.y, fZ), Vec3(p1.x, p1.y, fZ));
            }
            for (float dx = vec0.x; dx < vec1.x; dx += gridSize)
            {
                Vec3 p0 = viewTM_Inv.TransformPoint(Vec3(dx, viewP0.y, 0));
                Vec3 p1 = viewTM_Inv.TransformPoint(Vec3(dx, viewP1.y, 0));
                dc.DrawLine(Vec3(p0.x, p0.y, fZ), Vec3(p1.x, p1.y, fZ));
            }

            // Draw numbers.
            if (m_bShowNumbers)
            {
                char text[64];
                dc.SetColor(m_colorGridText);

                if (!bNoXNumbers)
                {
                    // Draw horizontal grid text.
                    for (float dx = vec0.x; dx <= vec1.x; dx += gridSize)
                    {
                        Vec3 p = viewTM_Inv.TransformPoint(Vec3(dx, viewP_Text.y, 0));
                        sprintf_s(text, "%i", (int)(dx));
                        dc.Draw2dTextLabel(p.x - 8, p.y - 13, 1, text);
                    }
                }
                for (float dy = vec0.y; dy <= vec1.y; dy += gridSize)
                {
                    Vec3 p = viewTM_Inv.TransformPoint(Vec3(viewP_Text.x, dy, 0));
                    sprintf_s(text, "%i", (int)(dy));
                    dc.Draw2dTextLabel(p.x + 3, p.y - 12, 1, text);
                }
            }
        }
    }

    // Draw Main Axis.
    {
        Vec3 org = m_screenTM.TransformPoint(Vec3(0, 0, 0));
        dc.SetColor(AXIS_GRID_COLOR);
        dc.DrawLine(Vec3(org.x, 0.0f, fZ), Vec3(org.x, static_cast<f32>(height), fZ));
        dc.DrawLine(Vec3(0.0f, org.y, fZ), Vec3(static_cast<f32>(width), org.y, fZ));
    }
    //////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::DrawAxis(DisplayContext& dc)
{
    float cl = 0.85f;
    char xstr[2], ystr[2], zstr[2];
    Vec3 colx, coly, colz;
    Vec3 colorX(cl, 0, 0);
    Vec3 colorY(0, cl, 0);
    Vec3 colorZ(0, 0, cl);
    switch (m_axis)
    {
    case VPA_XY:
        azstrcpy(xstr, AZ_ARRAY_SIZE(xstr), "x");
        azstrcpy(ystr, AZ_ARRAY_SIZE(ystr),  "y");
        azstrcpy(zstr, AZ_ARRAY_SIZE(zstr),  "z");
        colx = colorX;
        coly = colorY;
        colz = colorZ;
        break;
    case VPA_YX:
        azstrcpy(xstr, AZ_ARRAY_SIZE(xstr), "x");
        azstrcpy(ystr, AZ_ARRAY_SIZE(ystr), "y");
        azstrcpy(zstr, AZ_ARRAY_SIZE(zstr), "z");
        colx = colorY;
        coly = colorX;
        colz = colorZ;
        break;
    case VPA_XZ:
        azstrcpy(xstr, AZ_ARRAY_SIZE(xstr), "x");
        azstrcpy(ystr, AZ_ARRAY_SIZE(ystr), "z");
        azstrcpy(zstr, AZ_ARRAY_SIZE(zstr), "y");
        colx = colorX;
        coly = colorZ;
        colz = colorY;
        break;
    case VPA_YZ:
        azstrcpy(xstr, AZ_ARRAY_SIZE(xstr), "y");
        azstrcpy(ystr, AZ_ARRAY_SIZE(ystr), "z");
        azstrcpy(zstr, AZ_ARRAY_SIZE(zstr), "x");
        colx = colorY;
        coly = colorZ;
        colz = colorX;
        break;
    }

    int height = m_rcClient.height();

    int size = 25;
    Vec3 pos(30.0f, static_cast<f32>(height - 15), 1.0f);

    dc.SetColor(colx.x, colx.y, colx.z, 1);
    dc.DrawLine(pos, pos + Vec3(static_cast<f32>(size), 0.0f, 0.0f));

    dc.SetColor(coly.x, coly.y, coly.z, 1.0f);
    dc.DrawLine(pos, pos - Vec3(0.0f, static_cast<f32>(size), 0.0f));

    dc.SetColor(m_colorAxisText);
    pos.x -= 3.0f;
    pos.y -= 4.0f;
    pos.z = 2.0f;
    dc.Draw2dTextLabel(pos.x + size + 4, pos.y - 2, 1, xstr);
    dc.Draw2dTextLabel(pos.x + 3, pos.y - size, 1, ystr);
    dc.Draw2dTextLabel(pos.x - 5, pos.y + 5, 1, zstr);
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::DrawSelection(DisplayContext& dc)
{
    AABB box;
    GetIEditor()->GetSelectedRegion(box);

    if (!IsEquivalent(box.min, box.max, 0))
    {
        switch (m_axis)
        {
        case VPA_XY:
        case VPA_YX:
            box.min.z = box.max.z = 0;
            break;
        case VPA_XZ:
            box.min.y = box.max.y = 0;
            break;
        case VPA_YZ:
            box.min.x = box.max.x = 0;
            break;
        }

        dc.PushMatrix(GetScreenTM());
        dc.SetColor(SELECTION_RECT_COLOR.x, SELECTION_RECT_COLOR.y, SELECTION_RECT_COLOR.z, 1);
        dc.DrawWireBox(box.min, box.max);
        dc.PopMatrix();
    }

    if (!m_selectedRect.isEmpty())
    {
        dc.SetColor(SELECTION_RECT_COLOR.x, SELECTION_RECT_COLOR.y, SELECTION_RECT_COLOR.z, 1);
        QPoint p1(m_selectedRect.left(), m_selectedRect.top());
        QPoint p2(m_selectedRect.right() + 1, m_selectedRect.bottom() +1);
        dc.DrawLine(
            Vec3(static_cast<f32>(p1.x()), static_cast<f32>(p1.y()), 0.0f), Vec3(static_cast<f32>(p2.x()), static_cast<f32>(p1.y()), 0.0f));
        dc.DrawLine(
            Vec3(static_cast<f32>(p1.x()), static_cast<f32>(p2.y()), 0.0f), Vec3(static_cast<f32>(p2.x()), static_cast<f32>(p2.y()), 0.0f));
        dc.DrawLine(
            Vec3(static_cast<f32>(p1.x()), static_cast<f32>(p1.y()), 0.0f), Vec3(static_cast<f32>(p1.x()), static_cast<f32>(p2.y()), 0.0f));
        dc.DrawLine(
            Vec3(static_cast<f32>(p2.x()), static_cast<f32>(p1.y()), 0.0f), Vec3(static_cast<f32>(p2.x()), static_cast<f32>(p2.y()), 0.0f));
    }
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::DrawViewerMarker(DisplayContext& dc)
{
    float noScale = 1.0f / GetZoomFactor();

    CViewport* pVP = GetViewManager()->GetGameViewport();
    if (!pVP)
    {
        return;
    }

    Ang3 viewAngles = Ang3::GetAnglesXYZ(Matrix33(pVP->GetViewTM()));

    Matrix34 tm;
    switch (m_axis)
    {
    case VPA_XY:
    case VPA_YX:
        tm = Matrix34::CreateRotationXYZ(Ang3(0, 0, viewAngles.z));
        break;
    case VPA_XZ:
        tm = Matrix34::CreateRotationXYZ(Ang3(viewAngles.x, viewAngles.y, viewAngles.z));
        break;
    case VPA_YZ:
        tm = Matrix34::CreateRotationXYZ(Ang3(viewAngles.x, 0, viewAngles.z));
        break;
    }
    tm.SetTranslation(pVP->GetViewTM().GetTranslation());

    dc.PushMatrix(GetScreenTM() * tm);

    Vec3 dim(MARKER_SIZE, MARKER_SIZE / 2, MARKER_SIZE);
    dc.SetColor(QColor(0, 0, 255)); // blue
    dc.DrawWireBox(-dim * noScale, dim * noScale);

    constexpr float DefaultFov = 60.f;
    float fov = DefaultFov;

    Vec3 q[4];
    float dist = 30;
    float ta = (float)tan(0.5f * fov);
    float h = dist * ta;
    float w = h * gSettings.viewports.fDefaultAspectRatio; //  ASPECT ??
    //float h = w / GetAspect();
    q[0] = Vec3(w, dist, h) * noScale;
    q[1] = Vec3(-w, dist, h) * noScale;
    q[2] = Vec3(-w, dist, -h) * noScale;
    q[3] = Vec3(w, dist, -h) * noScale;

    // Draw frustum.
    dc.DrawLine(Vec3(0, 0, 0), q[0]);
    dc.DrawLine(Vec3(0, 0, 0), q[1]);
    dc.DrawLine(Vec3(0, 0, 0), q[2]);
    dc.DrawLine(Vec3(0, 0, 0), q[3]);

    // Draw quad.
    dc.DrawPolyLine(q, 4);

    dc.PopMatrix();
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::DrawObjects(DisplayContext& dc)
{
    dc.PushMatrix(GetScreenTM());
    if (m_bShowObjectsInfo)
    {
        GetIEditor()->GetObjectManager()->Display(dc);
    }

    dc.PopMatrix();
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::MakeConstructionPlane([[maybe_unused]] int axis)
{
    m_constructionMatrix[COORDS_VIEW] = GetViewTM();

    Vec3 p(0, 0, 0);

    switch (m_axis)
    {
    case VPA_XY:
        AssignConstructionPlane(p, p + Vec3(0, 1, 0), p + Vec3(1, 0, 0));
        break;
    case VPA_YX:
        m_constructionPlane.SetPlane(p, p + Vec3(1, 0, 0), p + Vec3(0, 1, 0));
        break;
    case VPA_XZ:
        AssignConstructionPlane(p, p + Vec3(0, 0, 1), p + Vec3(1, 0, 0));
        break;
    case VPA_YZ:
        AssignConstructionPlane(p, p + Vec3(0, 0, 1), p + Vec3(0, 1, 0));
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& Q2DViewport::GetConstructionMatrix(RefCoordSys coordSys)
{
    if (coordSys == COORDS_VIEW)
    {
        return GetViewTM();
    }
    return m_constructionMatrix[coordSys];
}

//////////////////////////////////////////////////////////////////////////
AABB Q2DViewport::GetWorldBounds(const QPoint& pnt1, const QPoint& pnt2)
{
    Vec3 org;
    AABB box;
    box.Reset();
    box.Add(ViewToWorld(pnt1));
    box.Add(ViewToWorld(pnt2));

    int maxSize = MAX_WORLD_SIZE;
    switch (m_axis)
    {
    case VPA_XY:
    case VPA_YX:
        box.min.z = static_cast<f32>(-maxSize);
        box.max.z = static_cast<f32>(maxSize);
        break;
    case VPA_XZ:
        box.min.y = static_cast<f32>(-maxSize);
        box.max.y = static_cast<f32>(maxSize);
        break;
    case VPA_YZ:
        box.min.x = static_cast<f32>(-maxSize);
        box.max.x = static_cast<f32>(maxSize);
        break;
    }
    return box;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::OnDragSelectRectangle(const QRect &rect, [[maybe_unused]] bool bNormalizeRect)
{
    Vec3 org;
    AABB box;
    box.Reset();

    Vec3 p1 = ViewToWorld(rect.topLeft());
    Vec3 p2 = ViewToWorld(rect.bottomRight() + QPoint(1,1));
    org = p1;

    // Calculate selection volume.
    box.Add(p1);
    box.Add(p2);

    int maxSize = MAX_WORLD_SIZE;

    char szNewStatusText[512];
    float w, h;

    switch (m_axis)
    {
    case VPA_XY:
        box.min.z = static_cast<f32>(-maxSize);
        box.max.z = static_cast<f32>(maxSize);

        w = box.max.x - box.min.x;
        h = box.max.y - box.min.y;
        sprintf_s(szNewStatusText, "X:%g Y:%g W:%g H:%g", org.x, org.y, w, h);
        break;
    case VPA_YX:
        box.min.z = static_cast<f32>(-maxSize);
        box.max.z = static_cast<f32>(maxSize);

        w = box.max.y - box.min.y;
        h = box.max.x - box.min.x;
        sprintf_s(szNewStatusText, "X:%g Y:%g W:%g H:%g", org.x, org.y, w, h);
        break;
    case VPA_XZ:
        box.min.y = static_cast<f32>(-maxSize);
        box.max.y = static_cast<f32>(maxSize);

        w = box.max.x - box.min.x;
        h = box.max.z - box.min.z;
        sprintf_s(szNewStatusText, "X:%g Z:%g  W:%g H:%g", org.x, org.z, w, h);
        break;
    case VPA_YZ:
        box.min.x = static_cast<f32>(-maxSize);
        box.max.x = static_cast<f32>(maxSize);

        w = box.max.y - box.min.y;
        h = box.max.z - box.min.z;
        sprintf_s(szNewStatusText, "Y:%g Z:%g  W:%g H:%g", org.y, org.z, w, h);
        break;
    }

    GetIEditor()->SetSelectedRegion(box);

    // Show marker position in the status bar
    GetIEditor()->SetStatusText(szNewStatusText);
}

//////////////////////////////////////////////////////////////////////////
bool Q2DViewport::HitTest(const QPoint& point, HitContext& hitInfo)
{
    hitInfo.bounds = &m_displayBounds;
    return QtViewport::HitTest(point, hitInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool Q2DViewport::IsBoundsVisible(const AABB& box) const
{
    // If at least part of bbox is visible then its visible.
    if (m_displayBounds.IsIntersectBox(box))
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::CenterOnSelection()
{
    CSelectionGroup* sel = GetIEditor()->GetSelection();
    if (sel->IsEmpty())
    {
        return;
    }

    AABB bounds = sel->GetBounds();
    CenterOnAABB(bounds);
}

void Q2DViewport::CenterOnAABB(const AABB& aabb)
{
    Vec3 selPos = aabb.GetCenter();

    Vec3 v1 = ViewToWorld(m_rcClient.bottomLeft());
    Vec3 v2 = ViewToWorld(m_rcClient.topRight());
    Vec3 vofs = (v2 - v1) * 0.5f;
    selPos -= vofs;
    SetOrigin2D(selPos);

    m_bContentValid = false;
}


//////////////////////////////////////////////////////////////////////////
Vec3 Q2DViewport::GetOrigin2D() const
{
    if (gSettings.viewports.bSync2DViews)
    {
        return GetViewManager()->GetOrigin2D();
    }
    else
    {
        return m_origin2D;
    }
}

//////////////////////////////////////////////////////////////////////////
void Q2DViewport::SetOrigin2D(const Vec3& org)
{
    m_origin2D = org;
    if (gSettings.viewports.bSync2DViews)
    {
        GetViewManager()->SetOrigin2D(m_origin2D);
    }
}

#include <moc_2DViewport.cpp>
