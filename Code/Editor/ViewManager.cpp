/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation of the CViewManager class.


#include "EditorDefs.h"

#include "ViewManager.h"

// Qt
#include <QTimer>

// AzCore
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// AzToolsFramework
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

// Atom
#include <Atom/RPI.Public/ViewportContextBus.h>

// Editor
#include "Settings.h"
#include "MainWindow.h"
#include "LayoutWnd.h"
#include "EditorViewportWidget.h"
#include "CryEditDoc.h"
#include "QtViewPaneManager.h"
#include "ViewPane.h"

#include <AzCore/Console/IConsole.h>

static constexpr AZStd::string_view MultiViewportToggleKey = "/O3DE/Viewport/MultiViewportEnabled";

bool CViewManager::IsMultiViewportEnabled()
{
    bool isMultiViewportEnabled = false;

    // Retrieve new action manager setting
    if (auto* registry = AZ::SettingsRegistry::Get())
    {
        registry->Get(isMultiViewportEnabled, MultiViewportToggleKey);
    }

    return isMultiViewportEnabled;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CViewManager::CViewManager()
{
    m_zoomFactor = 1;

    m_origin2D(0, 0, 0);
    m_zoom2D = 1.0f;

    m_updateRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-100000, -100000, -100000), AZ::Vector3(100000, 100000, 100000));

    m_pSelectedView = nullptr;

    m_nGameViewports = 0;
    m_bGameViewportsUpdated = false;

    QtViewOptions viewportOptions;
    viewportOptions.paneRect = QRect(0, 0, 400, 400);
    viewportOptions.canHaveMultipleInstances = true;
    // The raw widget registration only serves CLayoutViewPane::SetViewClass (preview mode); a bare
    // EditorViewportWidget never receives a viewport id, so opening it as a pane would be inert.
    viewportOptions.showInMenu = false;
    // Viewports stay interactive in component/ImGui modes: disabling one freezes its camera and
    // drops input releases, latching stale modifier state in ViewportManipulatorController.
    viewportOptions.isDisabledInComponentMode = false;
    viewportOptions.isDisabledInImGuiMode = false;

    viewportOptions.viewportType = ET_ViewportCamera;
    RegisterQtViewPaneWithName<EditorViewportWidget>(GetIEditor(), "Perspective", LyViewPane::CategoryViewport, viewportOptions);

    QtViewOptions dockableViewportOptions = viewportOptions;
    dockableViewportOptions.paneRect = QRect(0, 0, 800, 450);
    dockableViewportOptions.showInMenu = true;
    QtViewPaneManager::instance()->RegisterPane(
        LyViewPane::EditorViewport,
        LyViewPane::CategoryViewport,
        [](QWidget* parent = nullptr) -> QWidget*
        {
            auto* pane = new CLayoutViewPane(parent);

            // The first pane adopts the viewport the boot layout window created. Moving it within the
            // same toplevel and reattaching it under its existing id keeps its native window, and with
            // it the swapchain, viewport context and camera.
            MainWindow* mainWindow = MainWindow::instance();
            if (QWidget* adopted = mainWindow ? mainWindow->TakeCentralViewportForDocking() : nullptr)
            {
                pane->SetId(qobject_cast<QtViewport*>(adopted)->GetViewportId());
                pane->AttachViewport(adopted);
                return pane;
            }

            // Fresh viewports wait one event-loop tick, until the pane is wrapped in its dock: the
            // swapchain binds to the native window present at attach time, and reparenting afterwards
            // recreates that window and orphans the swapchain.
            QTimer::singleShot(
                0, pane,
                [pane]
                {
                    auto* viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
                    int viewportId = 0;
                    while (viewportContextManager && viewportContextManager->GetViewportContextById(viewportId))
                    {
                        ++viewportId;
                    }

                    pane->SetId(viewportId);
                    auto* viewport = new EditorViewportWidget("Perspective", pane);
                    viewport->setProperty("IsViewportWidget", true);
                    pane->AttachViewport(viewport);
                });
            return pane;
        },
        dockableViewportOptions);

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
    const bool viewportUiAnchorClosing = m_pSelectedView == pViewport;
    m_pSelectedView = viewportUiAnchorClosing ? nullptr : m_pSelectedView;

    stl::find_and_erase(m_viewports, pViewport);
    m_bGameViewportsUpdated = false;

    if (viewportUiAnchorClosing)
    {
        // Park the viewport UI overlay back on its owner now that its anchor viewport is going away.
        AnchorViewportUiTo(GetViewportUiOwner());
    }
}

//////////////////////////////////////////////////////////////////////////
QtViewport* CViewManager::GetViewportUiOwner() const
{
    for (CViewport* viewport : m_viewports)
    {
        if (viewport->GetViewportId() == AzToolsFramework::ViewportUi::DefaultViewportId)
        {
            return static_cast<QtViewport*>(viewport);
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::AnchorViewportUiTo(CViewport* pViewport)
{
    // The editor keeps one viewport UI widget set and it rides the viewport it is anchored to.
    QtViewport* owner = GetViewportUiOwner();
    if (owner && pViewport)
    {
        owner->AnchorViewportUiTo(static_cast<QtViewport*>(pViewport));
    }
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
    return nullptr;
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
    return nullptr;
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
    const auto viewportIter = AZStd::find_if(
        m_viewports.begin(),
        m_viewports.end(),
        [&point](CViewport* viewport) -> bool
        {
            auto* widget = viewport->widget();
            return widget && widget->rect().contains(widget->mapFromGlobal(point));
        }
    );

    return (viewportIter == m_viewports.end()) ? nullptr : *viewportIter;
}

//////////////////////////////////////////////////////////////////////////
AZ::Vector3 CViewManager::GetClickPositionInViewportSpace() const
{
    // Retrieve click position.
    QPoint clickPos = QCursor::pos();

    // If a context menu is active, get its position as the click position.
    if (auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get())
    {
        auto outcome = menuManagerInterface->GetLastContextMenuPosition();
        if (outcome.IsSuccess())
        {
            clickPos = outcome.GetValue();
        }
    }

    // If the click position was on a viewport, retrieve the position in world coordinates.
    AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();
    if (CViewport* view = GetViewportAtPoint(clickPos))
    {
        QPoint relativeCursor = view->widget()->mapFromGlobal(clickPos);
        worldPosition = AzToolsFramework::FindClosestPickIntersection(
            view->GetViewportId(),
            AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(relativeCursor),
            AzToolsFramework::EditorPickRayLength,
            AzToolsFramework::GetDefaultEntityPlacementDistance());
    }

    return worldPosition;
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::SelectViewport(CViewport* pViewport)
{
    // Audio: Handle viewport change for listeners

    if (m_pSelectedView != nullptr && m_pSelectedView != pViewport)
    {
        m_pSelectedView->SetSelected(false);

    }

    m_pSelectedView = pViewport;

    if (MainWindow* mainWindow = MainWindow::instance())
    {
        mainWindow->SetActiveView(m_pSelectedView ? m_pSelectedView->GetViewPane() : nullptr);
    }

    if (m_pSelectedView != nullptr)
    {
        m_pSelectedView->SetSelected(true);

        // The focused viewport is what the default viewport context alias resolves to, and its
        // world is the active world for selection, undo and focus.
        if (auto* viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get())
        {
            if (viewportContextManager->GetViewportContextById(m_pSelectedView->GetViewportId()))
            {
                viewportContextManager->SetDefaultViewportContext(m_pSelectedView->GetViewportId());
            }
        }

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequests::SetFocusedViewport, m_pSelectedView->GetViewportId());

        AnchorViewportUiTo(m_pSelectedView);
    }
}

//////////////////////////////////////////////////////////////////////////
CViewport* CViewManager::GetGameViewport() const
{
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
