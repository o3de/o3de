/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : interface for the CViewManager class.


#ifndef CRYINCLUDE_EDITOR_VIEWMANAGER_H
#define CRYINCLUDE_EDITOR_VIEWMANAGER_H

#pragma once

#include "Cry_Geo.h"
#include "Viewport.h"
#include "QtViewPaneManager.h"
// forward declaration.
class CLayoutWnd;
class CViewport;

namespace AzToolsFramework
{
    class ManipulatorManager;
}

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** Manages set of viewports.
*/
class SANDBOX_API CViewManager
    : public IEditorNotifyListener
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    static bool IsMultiViewportEnabled();

    void ReleaseView(CViewport* pViewport);

    CViewport* GetViewport(EViewportType type) const;
    CViewport* GetViewport(const QString& name) const;

    //! Find viewport at Screen point.
    CViewport* GetViewportAtPoint(const QPoint& point) const;

    //! Retrieves the position in world space corresponding to the point clicked by the user.
    //! Will take context menus and cursor position into account as appropriate.
    AZ::Vector3 GetClickPositionInViewportSpace() const;

    void SelectViewport(CViewport* pViewport);
    CViewport* GetSelectedViewport() const { return m_pSelectedView; }

    void    SetAxisConstrain(int axis);

    void    SetZoomFactor(float zoom);
    float   GetZoomFactor() const { return m_zoomFactor; }

    //! Reset all views.
    void    ResetViews();
    //! Update all views.
    void    UpdateViews(int flags = 0xFFFFFFFF);

    void SetUpdateRegion(const AABB& updateRegion) { m_updateRegion = updateRegion; };
    const AABB& GetUpdateRegion() { return m_updateRegion; };

    /** Get 2D viewports origin.
    */
    Vec3 GetOrigin2D() const { return m_origin2D; }

    /** Assign 2D viewports origin.
    */
    void SetOrigin2D(const Vec3& org) { m_origin2D = org; };

    /** Assign zoom factor for 2d viewports.
    */
    void SetZoom2D(float zoom);

    /** Get zoom factor of 2d viewports.
    */
    float GetZoom2D() const { return m_zoom2D; };

    //////////////////////////////////////////////////////////////////////////
    //! Get number of currently existing viewports.
    virtual int GetViewCount() { return static_cast<int>(m_viewports.size()); };
    //! Get viewport by index.
    //! @param index 0 <= index < GetViewportCount()
    virtual CViewport* GetView(int index) { return m_viewports[index]; }

    //////////////////////////////////////////////////////////////////////////
    //! Get current layout window.
    //! @return Pointer to the layout window, can be nullptr.
    virtual CLayoutWnd* GetLayout() const;

    //! Cycle between different 2D viewports type on same view pane.
    virtual void Cycle2DViewport();

    //////////////////////////////////////////////////////////////////////////
    // Retrieve main game viewport, where the full game is rendered in 3D.
    CViewport* GetGameViewport() const;

    //! Get number of Game Viewports
    int GetNumberOfGameViewports();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> GetManipulatorManager();

private:
    friend class CEditorImpl;
    friend class QtViewport;

    void IdleUpdate();
    void RegisterViewport(CViewport* vp);
    void UnregisterViewport(CViewport* vp);

private:
    CViewManager();
    ~CViewManager();
    //////////////////////////////////////////////////////////////////////////
    //FIELDS.
    float   m_zoomFactor;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AABB m_updateRegion;

    //! Origin of 2d viewports.
    Vec3 m_origin2D;
    //! Zoom of 2d viewports.
    float m_zoom2D;

    int m_nGameViewports;
    bool m_bGameViewportsUpdated;

    //! Array of currently existing viewports.
    std::vector<CViewport*> m_viewports;

    CViewport* m_pSelectedView;

    AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_VIEWMANAGER_H
