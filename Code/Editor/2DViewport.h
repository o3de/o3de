/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_2DVIEWPORT_H
#define CRYINCLUDE_EDITOR_2DVIEWPORT_H

#pragma once

#if !defined(Q_MOC_RUN)
#include "Viewport.h"
#include "Objects/DisplayContext.h"
#endif

class QMenu;

/** 2D Viewport used mostly for indoor editing, Front/Top/Left viewports.
*/
class Q2DViewport
    : public QtViewport
{
    Q_OBJECT
public:
    enum ViewMode
    {
        NothingMode = 0,
        ScrollZoomMode,
    };

    Q2DViewport(QWidget* parent = nullptr);
    virtual ~Q2DViewport();

    void SetType(EViewportType type) override;
    EViewportType GetType() const override { return m_viewType; }
    float GetAspectRatio() const override { return 1.0f; };

    void ResetContent() override;
    void UpdateContent(int flags) override;

public slots:
    // Called every frame to update viewport.
    void Update() override;

public:
    //! Map world space position to viewport position.
    QPoint     WorldToView(const Vec3& wp) const override;

    QPoint WorldToViewParticleEditor(const Vec3& wp, int width, int height) const override; //Eric@conffx

    //! Map viewport position to world space position.
    Vec3        ViewToWorld(const QPoint& vp, bool* collideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const override;
    //! Map viewport position to world space ray from camera.
    void        ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const override;

    void OnTitleMenu(QMenu* menu) override;

    bool HitTest(const QPoint& point, HitContext& hitInfo) override;
    bool IsBoundsVisible(const AABB& box) const override;

    // ovverided from CViewport.
    float GetScreenScaleFactor(const Vec3& worldPoint) const override;
    float GetScreenScaleFactor([[maybe_unused]] const CCamera& camera, [[maybe_unused]] const Vec3& object_position) override { return 1; } //Eric@conffx

    // Overrided from CViewport.
    void OnDragSelectRectangle(const QRect &rect, bool bNormalizeRect = false) override;
    void CenterOnSelection() override;
    void CenterOnAABB(const AABB& aabb) override;

    /** Get 2D viewports origin.
    */
    Vec3 GetOrigin2D() const;

    /** Assign 2D viewports origin.
    */
    void SetOrigin2D(const Vec3& org);


    void SetShowViewMarker(bool bEnable) { m_bShowViewMarker = bEnable; }

    void SetShowGrid(bool bShowGrid) { m_bShowGrid = bShowGrid; }
    bool GetShowGrid() const { return m_bShowGrid; };

    void SetShowObjectsInfo(bool bShowObjectsInfo) { m_bShowObjectsInfo = bShowObjectsInfo; }
    bool GetShowObjectsInfo() const { return m_bShowObjectsInfo; }

    void SetGridLines(bool bShowMinor, bool bShowMajor) { m_bShowMinorGridLines = bShowMinor; m_bShowMajorGridLines = bShowMajor; }
    void SetGridLineNumbers(bool bShowNumbers) { m_bShowNumbers = bShowNumbers; }
    void SetAutoAdjust(bool bAuto) { m_bAutoAdjustGrids = bAuto; }


protected:
    enum EViewportAxis
    {
        VPA_XY,
        VPA_XZ,
        VPA_YZ,
        VPA_YX,
    };

    void SetAxis(EViewportAxis axis);

    // Scrolling / zooming related
    virtual void SetScrollOffset(float x, float y, bool bLimits = true);
    virtual void GetScrollOffset(float& x, float& y); // Only x and y components used.

    virtual void SetZoom(float fZoomFactor, const QPoint& center);

    // overrides from CViewport.
    void MakeConstructionPlane(int axis) override;
    const Matrix34& GetConstructionMatrix(RefCoordSys coordSys) override;

    //! Calculate view transformation matrix.
    virtual void CalculateViewTM();

    virtual void SetViewMode(ViewMode eViewMode)    { m_eViewMode = eViewMode; };
    ViewMode GetViewMode() { return m_eViewMode; };

protected slots:
    // Render
    void Render();

protected:
    // Draw everything.
    virtual void Draw(DisplayContext& dc);

    // Draw elements of viewport.
    void DrawGrid(DisplayContext& dc, bool bNoXNumbers = false);
    void DrawAxis(DisplayContext& dc);
    void DrawSelection(DisplayContext& dc);
    void DrawObjects(DisplayContext& dc);
    void DrawViewerMarker(DisplayContext& dc);

    AABB GetWorldBounds(const QPoint& p1, const QPoint& p2);

    void OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void OnMouseMove(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, const QPoint& point) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    int OnCreate();
    void OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void OnRButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void OnMouseWheel(Qt::KeyboardModifiers modifiers, short zDelta, const QPoint& pt) override;
    void OnDestroy();

protected:
    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////

    //! XY/XZ/YZ mode of this 2D viewport.
    EViewportType m_viewType;
    EViewportAxis m_axis;

    //! Axis to cull normals with in this Viewport.
    int m_cullAxis;

    // Viewport origin point.
    Vec3 m_origin2D;

    // Scrolling / zooming related
    QPoint m_cMousePos;
    QPoint m_RMouseDownPos;

    float m_prevZoomFactor;
    QSize m_prevScrollOffset;

    QRect m_rcSelect;
    QRect m_rcClient;

    AABB m_displayBounds;

    bool m_bShowTerrain;
    bool m_bShowViewMarker;
    bool m_bShowGrid;
    bool m_bShowObjectsInfo;
    bool m_bShowMinorGridLines;
    bool m_bShowMajorGridLines;
    bool m_bShowNumbers;
    bool m_bAutoAdjustGrids;

    Matrix34 m_screenTM_Inverted;

    float m_gridAlpha;
    QColor m_colorGridText;
    QColor m_colorAxisText;
    QColor m_colorBackground;
    bool m_bContentValid;

    DisplayContext m_displayContext;
    ViewMode m_eViewMode = NothingMode;

    //May be changed by parent classes of Q2DViewport
    //CTopRendererWnd for example will change m_maxZoom to 250.0f
    float m_minZoom = 0.01f;
    float m_maxZoom = 500.0f;
};

//////////////////////////////////////////////////////////////////////////
class C2DViewport_XY
    : public Q2DViewport
{
    Q_OBJECT
public:
    static const GUID& GetClassID()
    {
        return QtViewport::GetClassID<C2DViewport_XY>();
    }

    C2DViewport_XY(QWidget* parent = nullptr)
        : Q2DViewport(parent) { SetType(ET_ViewportXY); }
};

//////////////////////////////////////////////////////////////////////////
class C2DViewport_XZ
    : public Q2DViewport
{
    Q_OBJECT
public:
    static const GUID& GetClassID()
    {
        return QtViewport::GetClassID<C2DViewport_XZ>();
    }

    C2DViewport_XZ(QWidget* parent = nullptr)
        : Q2DViewport(parent) { SetType(ET_ViewportXZ); }
};

//////////////////////////////////////////////////////////////////////////
class C2DViewport_YZ
    : public Q2DViewport
{
    Q_OBJECT
public:
    static const GUID& GetClassID()
    {
        return QtViewport::GetClassID<C2DViewport_YZ>();
    }

    C2DViewport_YZ(QWidget* parent = nullptr)
        : Q2DViewport(parent) { SetType(ET_ViewportYZ); }
};

#endif // CRYINCLUDE_EDITOR_2DVIEWPORT_H
