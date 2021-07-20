/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation of the CViewManager class.


#include "EditorDefs.h"

#include "ViewManager.h"

// AzCore
#include <AzCore/std/smart_ptr/make_shared.h>

// AzToolsFramework
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

// Editor
#include "Settings.h"
#include "MainWindow.h"
#include "LayoutWnd.h"
#include "2DViewport.h"
#include "TopRendererWnd.h"
#include "RenderViewport.h"
#include "EditorViewportWidget.h"
#include "CryEditDoc.h"

#include <AzCore/Console/IConsole.h>

AZ_CVAR(bool, ed_useAtomNativeViewport, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Use the new Atom-native Editor viewport (experimental, not yet stable");

bool CViewManager::IsMultiViewportEnabled()
{
    // Enable multi-viewport for legacy renderer, or if we're using the new fully Atom-native viewport
    return ed_useAtomNativeViewport;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CViewManager::CViewManager()
{
    m_zoomFactor = 1;

    m_origin2D(0, 0, 0);
    m_zoom2D = 1.0f;

    m_cameraObjectId = GUID_NULL;

    m_updateRegion.min = Vec3(-100000, -100000, -100000);
    m_updateRegion.max = Vec3(100000, 100000, 100000);

    m_pSelectedView = NULL;

    m_nGameViewports = 0;
    m_bGameViewportsUpdated = false;

    QtViewOptions viewportOptions;
    viewportOptions.paneRect = QRect(0, 0, 400, 400);
    viewportOptions.canHaveMultipleInstances = true;

    viewportOptions.viewportType = ET_ViewportXY;
    RegisterQtViewPane<C2DViewport_XY>(GetIEditor(), "Top", LyViewPane::CategoryViewport, viewportOptions);

    viewportOptions.viewportType = ET_ViewportXZ;
    RegisterQtViewPane<C2DViewport_XZ>(GetIEditor(), "Front", LyViewPane::CategoryViewport, viewportOptions);

    viewportOptions.viewportType = ET_ViewportYZ;
    RegisterQtViewPane<C2DViewport_YZ>(GetIEditor(), "Left", LyViewPane::CategoryViewport, viewportOptions);

    viewportOptions.viewportType = ET_ViewportCamera;
    if (ed_useAtomNativeViewport)
    {
        RegisterQtViewPaneWithName<EditorViewportWidget>(GetIEditor(), "Perspective", LyViewPane::CategoryViewport, viewportOptions);
    }
    else
    {
        RegisterQtViewPaneWithName<CRenderViewport>(GetIEditor(), "Perspective", LyViewPane::CategoryViewport, viewportOptions);
    }

    viewportOptions.viewportType = ET_ViewportMap;
    RegisterQtViewPane<QTopRendererWnd>(GetIEditor(), "Map", LyViewPane::CategoryViewport, viewportOptions);

    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CViewManager::~CViewManager()
{
    GetIEditor()->UnregisterNotifyListener(this);

    m_viewports.clear();
}

void CViewManager::ReleaseView(CViewport* pViewport)
{
    pViewport->DestroyWindow();
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::RegisterViewport(CViewport* pViewport)
{
    pViewport->SetViewManager(this);
    m_viewports.push_back(pViewport);

    // the type of added viewport can be changed later
    m_bGameViewportsUpdated = false;
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::UnregisterViewport(CViewport* pViewport)
{
    if (m_pSelectedView == pViewport)
    {
        m_pSelectedView = NULL;
    }
    stl::find_and_erase(m_viewports, pViewport);
    m_bGameViewportsUpdated = false;
}

//////////////////////////////////////////////////////////////////////////
CViewport* CViewManager::GetViewport(EViewportType type) const
{
    ////////////////////////////////////////////////////////////////////////
    // Returns the first view which has a render window of a specific
    // type attached
    ////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < m_viewports.size(); i++)
    {
        if (m_viewports[i]->GetType() == type)
        {
            return m_viewports[i];
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CViewport* CViewManager::GetViewport(const QString& name) const
{
    for (int i = 0; i < m_viewports.size(); i++)
    {
        if (QString::compare(m_viewports[i]->GetName(), name, Qt::CaseInsensitive) == 0)
        {
            return m_viewports[i];
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::SetZoomFactor(float zoom)
{
    m_zoomFactor = zoom;
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::UpdateViews(int flags)
{
    // Update each attached view,
    for (int i = 0; i < m_viewports.size(); i++)
    {
        m_viewports[i]->UpdateContent(flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::ResetViews()
{
    // Reset each attached view,
    for (int i = 0; i < m_viewports.size(); i++)
    {
m_viewports[i]->ResetContent();
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::IdleUpdate()
{
    // Update each attached view,
    for (int i = 0; i < m_viewports.size(); i++)
    {
        if (m_viewports[i]->GetType() != ET_ViewportCamera || (GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady()))
        {
            m_viewports[i]->Update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void    CViewManager::SetAxisConstrain(int axis)
{
    for (int i = 0; i < m_viewports.size(); i++)
    {
        m_viewports[i]->SetAxisConstrain(axis);
    }
}

//////////////////////////////////////////////////////////////////////////
CLayoutWnd* CViewManager::GetLayout() const
{
    return MainWindow::instance()->GetLayout();
}

void CViewManager::SetZoom2D(float zoom)
{
    m_zoom2D = zoom;
    if (m_zoom2D > 460.0f)
    {
        m_zoom2D = 460.0f;
    }
};

//////////////////////////////////////////////////////////////////////////
void CViewManager::Cycle2DViewport()
{
    GetLayout()->Cycle2DViewport();
}

//////////////////////////////////////////////////////////////////////////
CViewport* CViewManager::GetViewportAtPoint(const QPoint& point) const
{
    QWidget* widget = QApplication::widgetAt(point);
    return qobject_cast<QtViewport*>(widget);
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::SelectViewport(CViewport* pViewport)
{
    // Audio: Handle viewport change for listeners

    if (m_pSelectedView != NULL && m_pSelectedView != pViewport)
    {
        m_pSelectedView->SetSelected(false);

    }

    m_pSelectedView = pViewport;

    if (m_pSelectedView != NULL)
    {
        m_pSelectedView->SetSelected(true);
    }
}

//////////////////////////////////////////////////////////////////////////
CViewport* CViewManager::GetGameViewport() const
{
    if (CRenderViewport::GetPrimaryViewport())
    {
        return CRenderViewport::GetPrimaryViewport();
    }

    return GetViewport(ET_ViewportCamera);;
}

//////////////////////////////////////////////////////////////////////////
int CViewManager::GetNumberOfGameViewports()
{
    if (m_bGameViewportsUpdated)
    {
        return m_nGameViewports;
    }

    m_nGameViewports = 0;
    for (int i = 0; i < m_viewports.size(); ++i)
    {
        if (m_viewports[i]->GetType() == ET_ViewportCamera)
        {
            ++m_nGameViewports;
        }
    }
    m_bGameViewportsUpdated = true;

    return m_nGameViewports;
}


//////////////////////////////////////////////////////////////////////////
void CViewManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
        IdleUpdate();
        break;
    case eNotify_OnUpdateViewports:
        UpdateViews();
        break;
    }
}

AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> CViewManager::GetManipulatorManager()
{
    return m_manipulatorManager;
}
