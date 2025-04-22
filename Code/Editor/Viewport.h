/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : interface for the CViewport class.

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <Cry_Color.h>
#include "IPostRenderer.h"
#include "Include/IDisplayViewport.h"
#include "Include/SandboxAPI.h"
#include <QPointer>
#include <QMenu>

#if defined(Q_OS_WIN)
#include <QtWinExtras/qwinfunctions.h>
#endif

#include <AzCore/Math/Uuid.h>
#include <IEditor.h>
#endif

namespace AzQtComponents
{
    class ViewportDragContext;
}

// forward declarations.
class CCryEditDoc;
class CLayoutViewPane;
class CViewManager;
struct HitContext;
class CImageEx;
class QMenu;

/** Type of viewport.
*/
enum EViewportType
{
    ET_ViewportUnknown = 0,
    ET_ViewportXY,
    ET_ViewportXZ,
    ET_ViewportYZ,
    ET_ViewportCamera,
    ET_ViewportMap,
    ET_ViewportModel,
    ET_ViewportZ, //!< Z Only viewport.
    ET_ViewportUI,

    ET_ViewportLast,
};

//////////////////////////////////////////////////////////////////////////
// Standart cursors viewport can display.
//////////////////////////////////////////////////////////////////////////
enum EStdCursor
{
    STD_CURSOR_DEFAULT,
    STD_CURSOR_HIT,
    STD_CURSOR_MOVE,
    STD_CURSOR_ROTATE,
    STD_CURSOR_SCALE,
    STD_CURSOR_SEL_PLUS,
    STD_CURSOR_SEL_MINUS,
    STD_CURSOR_SUBOBJ_SEL,
    STD_CURSOR_SUBOBJ_SEL_PLUS,
    STD_CURSOR_SUBOBJ_SEL_MINUS,
    STD_CURSOR_HAND,

    STD_CURSOR_GAME,

    STD_CURSOR_LAST,
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class SANDBOX_API CViewport
    : public IDisplayViewport
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    typedef void(* DropCallback)(CViewport* viewport, int ptx, int pty, void* custom);

    virtual ~CViewport() {}

    virtual void SetActiveWindow() = 0;

    // Changed my view manager.
    void SetViewManager(CViewManager* viewMgr) { m_viewManager = viewMgr; };

    //! Access to view manager.
    CViewManager* GetViewManager() const { return m_viewManager; };

    virtual void AddPostRenderer(IPostRenderer* pPostRenderer) = 0;
    virtual bool RemovePostRenderer(IPostRenderer* pPostRenderer) = 0;

    virtual bool DestroyWindow() { return false; }

    /** Get type of this viewport.
    */
    virtual EViewportType GetType() const { return ET_ViewportUnknown; }
    /** Must be overriden in derived classes.
    */
    virtual void SetType(EViewportType type) = 0;

    /** Get name of viewport
    */
    virtual QString GetName() const = 0;

    virtual void SetSelected([[maybe_unused]] bool const bSelect){}

    //! Resets current selection region.
    virtual void ResetSelectionRegion() = 0;
    //! Set 2D selection rectangle.
    virtual void SetSelectionRectangle(const QRect& rect) = 0;
    inline void SetSelectionRectangle(const QPoint& startMousePosition, const QPoint& currentMousePosition)
    {
        // QRect's bottom/right are width - 1, height - 1, so when specifying the right position
        // directly in a QRect, we need to offset it by -1.
        SetSelectionRectangle(QRect(startMousePosition, currentMousePosition - QPoint(1, 1)));
    }
    //! Return 2D selection rectangle.
    virtual QRect GetSelectionRectangle() const = 0;
    //! Called when dragging selection rectangle.
    virtual void OnDragSelectRectangle(const QRect& rect, bool bNormalizeRect = false) = 0;
    void OnDragSelectRectangle(const QPoint& startMousePosition, const QPoint& currentMousePosition, bool bNormalizeRect = false)
    {
        // QRect's bottom/right are width - 1, height - 1, so when specifying the right position
        // directly in a QRect, we need to offset it by -1.
        OnDragSelectRectangle(QRect(startMousePosition, currentMousePosition - QPoint(1, 1)), bNormalizeRect);
    }

    virtual void ResetContent() = 0;
    virtual void UpdateContent(int flags) = 0;

    virtual void SetAxisConstrain(int axis) = 0;
    int GetAxisConstrain() const { return GetIEditor()->GetAxisConstrains(); };

    virtual Vec3 SnapToGrid(const Vec3& vec) = 0;

    //! Get selection precision tolerance.
    virtual float GetSelectionTolerance() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // View matrix.
    //////////////////////////////////////////////////////////////////////////
    //! Set current view matrix,
    //! This is a matrix that transforms from world to view space.
    virtual void SetViewTM([[maybe_unused]] const Matrix34& tm)
    {
        AZ_Error("CryLegacy", false, "QtViewport::SetViewTM not implemented");
    }

    //! Get current view matrix.
    //! This is a matrix that transforms from world space to view space.
    const Matrix34& GetViewTM() const override
    {
        AZ_Error("CryLegacy", false, "QtViewport::GetViewTM not implemented");
        static const Matrix34 m;
        return m;
    };

    //////////////////////////////////////////////////////////////////////////
    //! Get current screen matrix.
    //! Screen matrix transform from World space to Screen space.
    const Matrix34& GetScreenTM() const override
    {
        return m_screenTM;
    }

    //! Map viewport position to world space position.
    Vec3        ViewToWorld(const QPoint& vp, bool* pCollideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const override = 0;
    //! Convert point on screen to world ray.
    void        ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const override = 0;
    //! Get normal for viewport position
    virtual Vec3        ViewToWorldNormal(const QPoint& vp, bool onlyTerrain, bool bTestRenderMesh = false) = 0;

    //! Performs hit testing of 2d point in view to find which object hit.
    virtual bool HitTest(const QPoint& point, HitContext& hitInfo) = 0;

    // Access to the member m_bAdvancedSelectMode so interested modules can know its value.
    virtual bool GetAdvancedSelectModeFlag() = 0;

    virtual void ToggleCameraObject() {}
    virtual bool IsSequenceCamera() const { return false;  }

    //! Center viewport on selection.
    virtual void CenterOnSelection() = 0;
    virtual void CenterOnAABB(const AABB& aabb) = 0;

    /** Set ID of this viewport
    */
    virtual void SetViewportId(int id) { m_nCurViewportID = id; };

    /** Get ID of this viewport
    */
    virtual int GetViewportId() const { return m_nCurViewportID; };

    // Store final Game Matrix ready for editor
    void SetGameTM(const Matrix34& tm) { m_gameTM = tm; };

    //////////////////////////////////////////////////////////////////////////
    // Drag and drop support on viewports.
    // To be overrided in derived classes.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetGlobalDropCallback(DropCallback dropCallback, void* dropCallbackCustom)
    {
        m_dropCallback = dropCallback;
        m_dropCallbackCustom = dropCallbackCustom;
    }

    virtual void BeginUndo() = 0;
    virtual void AcceptUndo(const QString& undoDescription) = 0;
    virtual void CancelUndo() = 0;
    virtual void RestoreUndo() = 0;
    virtual bool IsUndoRecording() const = 0;

    virtual void CaptureMouse() {};
    virtual void SetCapture() { CaptureMouse(); }
    virtual void ReleaseMouse() {};

    virtual void ResetCursor() = 0;
    virtual void SetCursor(const QCursor& cursor) = 0;

    virtual void SetCurrentCursor(EStdCursor stdCursor) = 0;
    virtual void SetCurrentCursor(EStdCursor stdCursor, const QString& str) = 0;
    virtual void SetSupplementaryCursorStr(const QString& str) = 0;
    virtual void SetCursorString(const QString& str) = 0;

    virtual void SetFocus() = 0;
    virtual void Invalidate(bool bErase = true) = 0;

    // Is overridden by RenderViewport
    virtual void SetFOV([[maybe_unused]] float fov) {}
    virtual float GetFOV() const;

    virtual QObject* qobject() { return nullptr; }
    virtual QWidget* widget() { return nullptr; }

    virtual void OnTitleMenu([[maybe_unused]] QMenu* menu) {}

    void SetViewPane(CLayoutViewPane* viewPane) { m_viewPane = viewPane; }

    CViewport *asCViewport() override { return this; }

protected:
    CLayoutViewPane* m_viewPane = nullptr;
    CViewManager* m_viewManager;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    // Screen Matrix
    Matrix34 m_screenTM;
    int m_nCurViewportID;
    // Final game view matrix before dropping back to editor
    Matrix34 m_gameTM;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    // Custom drop callback (Leroy@Conffx)
    DropCallback m_dropCallback;
    void* m_dropCallbackCustom;
};

template<typename T>
typename std::enable_if<std::is_base_of<QObject, typename std::remove_cv<typename std::remove_pointer<T>::type>::type>::value, T>::type
viewport_cast(CViewport* viewport)
{
    if (viewport == nullptr)
    {
        return nullptr;
    }
    if (QObject* obj = viewport->qobject())
    {
        return qobject_cast<T>(obj);
    }
    else
    {
        return nullptr;
    }
}

/** Base class for all Editor Viewports.
*/
class SANDBOX_API QtViewport
    : public QWidget
    , public CViewport
{
    Q_OBJECT
public:
    QObject* qobject() override { return this; }
    QWidget* widget() override { return this; }

    template<typename T>
    static const GUID& GetClassID()
    {
        static GUID guid = [] {
            return AZ::Uuid::CreateRandom();
        } ();
        return guid;
    }

    QtViewport(QWidget* parent = nullptr);
    virtual ~QtViewport();

    void SetActiveWindow() override { activateWindow(); }

    //! Called while window is idle.
    void Update() override;

    /** Set name of this viewport.
    */
    void SetName(const QString& name);

    /** Get name of viewport
    */
    QString GetName() const override;

    void SetFocus() override { setFocus(); }
    void Invalidate([[maybe_unused]] bool bErase = 1) override { update(); }

    // Is overridden by RenderViewport
    void SetFOV([[maybe_unused]] float fov) override {}
    float GetFOV() const override;

    // Must be overridden in derived classes.
    // Returns:
    //   e.g. 4.0/3.0
    float GetAspectRatio() const override = 0;
    void GetDimensions(int* pWidth, int* pHeight) const override;
    void ScreenToClient(QPoint& pPoint) const override;

    void ResetContent() override;
    void UpdateContent(int flags) override;

    //! Set current zoom factor for this viewport.
    virtual void SetZoomFactor(float fZoomFactor);

    //! Get current zoom factor for this viewport.
    virtual float GetZoomFactor() const;

    virtual void OnActivate();
    virtual void OnDeactivate();

    //! Map world space position to viewport position.
    QPoint WorldToView(const Vec3& wp) const override;

    //! Map world space position to 3D viewport position.
    Vec3 WorldToView3D(const Vec3& wp, int nFlags = 0) const override;

    //! Map viewport position to world space position.
    virtual Vec3 ViewToWorld(const QPoint& vp, bool* pCollideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const override;
    //! Convert point on screen to world ray.
    virtual void        ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const override;
    //! Get normal for viewport position
    virtual Vec3        ViewToWorldNormal(const QPoint& vp, bool onlyTerrain, bool bTestRenderMesh = false) override;


    //! Snap any given 3D world position to grid lines if snap is enabled.
    Vec3 SnapToGrid(const Vec3& vec) override;

    //! Returns the screen scale factor for a point given in world coordinates.
    //! This factor gives the width in world-space units at the point's distance of the viewport.
    float GetScreenScaleFactor([[maybe_unused]] const Vec3& worldPoint) const  override { return 1; };

    void SetAxisConstrain(int axis) override;

    /// Take raw input and create a final mouse interaction.
    /// @attention Do not map **point** from widget to viewport explicitly,
    /// this is handled internally by BuildMouseInteraction - just pass directly.
    virtual AzToolsFramework::ViewportInteraction::MouseInteraction BuildMouseInteraction(
        Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, const QPoint& point);

    //////////////////////////////////////////////////////////////////////////
    // Selection.
    //////////////////////////////////////////////////////////////////////////
    //! Resets current selection region.
    void ResetSelectionRegion() override;
    //! Set 2D selection rectangle.
    void SetSelectionRectangle(const QRect& rect) override;

    //! Return 2D selection rectangle.
    QRect GetSelectionRectangle() const override { return m_selectedRect; };
    //! Called when dragging selection rectangle.
    void OnDragSelectRectangle(const QRect& rect, bool bNormalizeRect = false) override;
    //! Get selection precision tolerance.
    float GetSelectionTolerance() const override { return m_selectionTolerance; }
    //! Center viewport on selection.
    void CenterOnSelection() override {}
    void CenterOnAABB([[maybe_unused]] const AABB& aabb) override {}

    //! Performs hit testing of 2d point in view to find which object hit.
    bool HitTest(const QPoint& point, HitContext& hitInfo) override;

    float GetDistanceToLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& point) const override;

    // Access to the member m_bAdvancedSelectMode so interested modules can know its value.
    bool GetAdvancedSelectModeFlag() override;

    void GetPerpendicularAxis(EAxis* pAxis, bool* pIs2D) const override;

    void DegradateQuality(bool bEnable);

    //////////////////////////////////////////////////////////////////////////
    // Undo for viewpot operations.
    void BeginUndo() override;
    void AcceptUndo(const QString& undoDescription) override;
    void CancelUndo() override;
    void RestoreUndo() override;
    bool IsUndoRecording() const override;
    //////////////////////////////////////////////////////////////////////////

    //! Get prefered original size for this viewport.
    //! if 0, then no preference.
    virtual QSize GetIdealSize() const;

    //! Check if world space bounding box is visible in this view.
    bool IsBoundsVisible(const AABB& box) const override;

    //////////////////////////////////////////////////////////////////////////

    void SetCursor(const QCursor& cursor) override
    {
        setCursor(cursor);
    }

    // Set`s current cursor string.
    void SetCurrentCursor(const QCursor& hCursor, const QString& cursorString);
    void SetCurrentCursor(EStdCursor stdCursor, const QString& cursorString) override;
    void SetCurrentCursor(EStdCursor stdCursor) override;
    void SetCursorString(const QString& cursorString) override;
    void ResetCursor() override;
    void SetSupplementaryCursorStr(const QString& str) override;

    void AddPostRenderer(IPostRenderer* pPostRenderer) override;
    bool RemovePostRenderer(IPostRenderer* pPostRenderer) override;

    void CaptureMouse() override { m_mouseCaptured = true;  QWidget::grabMouse(); }
    void ReleaseMouse() override { m_mouseCaptured = false;  QWidget::releaseMouse(); }

    void setRay(QPoint& vp, Vec3& raySrc, Vec3& rayDir) override;

    QPoint m_vp;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Vec3 m_raySrc;
    Vec3 m_rayDir;

    // Greater than 0 while running MouseCallback() function. It needs to be a counter
    // because of recursive calls to MouseCallback(). It's used to make an exception
    // during the SScopedCurrentContext count check of m_cameraSetForWidgetRenderingCount.
    int m_processingMouseCallbacksCounter = 0;

    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
protected:
    friend class CViewManager;
    bool IsVectorInValidRange(const Vec3& v) const { return fabs(v.x) < 1e+8 && fabs(v.y) < 1e+8 && fabs(v.z) < 1e+8; }
    void AssignConstructionPlane(const Vec3& p1, const Vec3& p2, const Vec3& p3);
    HWND renderOverlayHWND() const;
    void setRenderOverlayVisible(bool);
    bool isRenderOverlayVisible() const;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    virtual void OnMouseMove(Qt::KeyboardModifiers, Qt::MouseButtons, const QPoint&) {}
    virtual void OnMouseWheel(Qt::KeyboardModifiers, short zDelta, const QPoint&);
    virtual void OnLButtonDown(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnLButtonUp(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnRButtonDown(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnRButtonUp(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnMButtonDblClk(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnMButtonDown(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnMButtonUp(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnLButtonDblClk(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnRButtonDblClk(Qt::KeyboardModifiers, const QPoint&) {}
    virtual void OnKeyDown([[maybe_unused]] UINT nChar, [[maybe_unused]] UINT nRepCnt, [[maybe_unused]] UINT nFlags) {}
    virtual void OnKeyUp([[maybe_unused]] UINT nChar, [[maybe_unused]] UINT nRepCnt, [[maybe_unused]] UINT nFlags) {}
#if defined(AZ_PLATFORM_WINDOWS)
    void OnRawInput(UINT wParam, HRAWINPUT lParam);
#endif
    void OnSetCursor();

    virtual void BuildDragDropContext(
        AzQtComponents::ViewportDragContext& context, AzFramework::ViewportId viewportId, const QPoint& point);

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AzToolsFramework::ViewportUi::ViewportUiManager m_viewportUi;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    float m_selectionTolerance;
    QMenu m_cViewMenu;

    mutable float m_fZoomFactor;

    QPoint m_cMouseDownPos;

    //! Current selected rectangle.
    QRect m_selectedRect;

    int m_activeAxis;

    // When true selection helpers will be shown and hit tested against.
    bool m_bAdvancedSelectMode;

    //////////////////////////////////////////////////////////////////////////
    // Standard cursors.
    //////////////////////////////////////////////////////////////////////////
    QCursor m_hCursor[STD_CURSOR_LAST];
    QCursor m_hCurrCursor;

    //! Mouse is over this object.
    QString m_cursorStr;
    QString m_cursorSupplementaryStr;

    static bool m_bDegradateQuality;

    // Grid size modifier due to zoom.
    float m_fGridZoom;

    int m_nLastUpdateFrame;
    int m_nLastMouseMoveFrame;

    QRect m_rcClient;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING

    typedef std::vector<_smart_ptr<IPostRenderer> > PostRenderers;
    PostRenderers   m_postRenderers;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

protected:
    bool m_mouseCaptured = false;

private:
    QWidget m_renderOverlay;
};
