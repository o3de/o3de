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
#include <QDockWidget>
#include <QTimer>

// AzCore
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// AzQtComponents
#include <AzQtComponents/Components/DockTabWidget.h>

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
#include "PrefabEditorPane.h"
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

static bool IsPrefabEditorViewport(const CViewport* viewport)
{
    const CLayoutViewPane* viewPane = viewport ? viewport->GetViewPane() : nullptr;
    return viewPane && qobject_cast<PrefabEditorPane*>(viewPane->parentWidget()) != nullptr;
}

//! Brings the pane hosting a viewport to the front, the same way opening a pane does.
static void ShowViewportPane(const CViewport* viewport)
{
    const CLayoutViewPane* viewPane = viewport ? viewport->GetViewPane() : nullptr;
    for (QWidget* widget = viewPane ? viewPane->parentWidget() : nullptr; widget; widget = widget->parentWidget())
    {
        auto* dockWidget = qobject_cast<QDockWidget*>(widget);
        if (!dockWidget)
        {
            continue;
        }

        if (AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(dockWidget))
        {
            tabWidget->setCurrentIndex(tabWidget->indexOf(dockWidget));
        }
        else
        {
            dockWidget->show();
            dockWidget->raise();
        }
        return;
    }
}

//! Gives a pane its viewport one event-loop tick late, once it is wrapped in its dock: the swapchain binds to
//! the native window present at attach time, and reparenting afterwards recreates that window and orphans it.
static void AttachDeferredViewport(CLayoutViewPane* viewPane, QWidget* pendingLevelHost)
{
    QTimer::singleShot(
        0, viewPane,
        [viewPane, pendingLevelHost]
        {
            auto* viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            int viewportId = 0;
            while (viewportContextManager && viewportContextManager->GetViewportContextById(viewportId))
            {
                ++viewportId;
            }

            viewPane->SetId(viewportId);

            // Layout persistence reads the id off the dock's own widget, which is the host when the two differ.
            pendingLevelHost->setProperty("ViewportId", viewportId);

            viewPane->AttachViewport(new EditorViewportWidget("Perspective", viewPane));

            // A pane restored from a layout, or opened onto a prefab, reopens on the level it was showing.
            const QByteArray levelPath = pendingLevelHost->property("PendingLevelPath").toString().toUtf8();
            if (levelPath.isEmpty())
            {
                return;
            }

            AzFramework::EntityContextId worldId = AzFramework::EntityContextId::CreateNull();
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                worldId, &AzToolsFramework::EditorEntityContextRequests::LoadWorld,
                AZ::IO::PathView(levelPath.constData()));
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequests::BindViewportToWorld, viewportId, worldId);
        });
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
    m_pLastEditorView = nullptr;

    m_nGameViewports = 0;
    m_bGameViewportsUpdated = false;

    QtViewOptions viewportOptions;
    viewportOptions.paneRect = QRect(0, 0, 400, 400);
    viewportOptions.canHaveMultipleInstances = true;
    // Only CLayoutViewPane::SetViewClass (preview mode) uses this; a bare widget never gets a viewport id.
    viewportOptions.showInMenu = false;
    // Disabling a viewport freezes its camera and drops input releases, latching stale modifier state.
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

            AttachDeferredViewport(pane, pane);
            return pane;
        },
        dockableViewportOptions);

    // A prefab opened for edit is a world of its own, with its own lighting rather than the level's.
    QtViewOptions prefabEditorOptions = dockableViewportOptions;
    prefabEditorOptions.viewportType = -1;
    QtViewPaneManager::instance()->RegisterPane(
        LyViewPane::PrefabEditor,
        LyViewPane::CategoryTools,
        [](QWidget* parent = nullptr) -> QWidget*
        {
            auto* pane = new PrefabEditorPane(parent);
            AttachDeferredViewport(pane->GetViewPane(), pane);
            return pane;
        },
        prefabEditorOptions);

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
    const bool anchorClosing = m_pSelectedView == pViewport;
    // The viewport hosting the UI widget set can close while other viewports remain, in which case the
    // overlay has to move to a surviving host or it is lost for the rest of the session.
    const bool uiHostClosing = GetViewportUiOwner() == pViewport;

    if (anchorClosing)
    {
        m_pSelectedView = nullptr;
    }

    if (m_pLastEditorView == pViewport)
    {
        m_pLastEditorView = nullptr;
    }

    stl::find_and_erase(m_viewports, pViewport);
    m_bGameViewportsUpdated = false;

    if (anchorClosing || uiHostClosing)
    {
        AnchorViewportUiTo(m_pSelectedView ? m_pSelectedView : GetViewportUiOwner());
    }
}

//////////////////////////////////////////////////////////////////////////
QtViewport* CViewManager::GetViewportUiOwner() const
{
    // Prefer the default viewport, but fall back to any live one. The default viewport can be closed
    // while others remain, and returning nullptr there would leave the editor with no viewport UI at
    // all - no component mode clusters, no sub-mode switchers - until it is restarted.
    QtViewport* fallback = nullptr;
    for (CViewport* viewport : m_viewports)
    {
        QtViewport* qtViewport = viewport_cast<QtViewport*>(viewport);
        if (!qtViewport)
        {
            continue;
        }

        if (viewport->GetViewportId() == AzToolsFramework::ViewportUi::DefaultViewportId)
        {
            return qtViewport;
        }

        fallback = fallback ? fallback : qtViewport;
    }

    return fallback;
}

//////////////////////////////////////////////////////////////////////////
void CViewManager::AnchorViewportUiTo(CViewport* pViewport)
{
    // The editor keeps one viewport UI widget set and it rides the viewport it is anchored to.
    QtViewport* owner = GetViewportUiOwner();
    QtViewport* target = viewport_cast<QtViewport*>(pViewport);
    if (owner && target)
    {
        owner->AnchorViewportUiTo(target);
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

    if (!IsPrefabEditorViewport(m_pSelectedView))
    {
        m_pLastEditorView = m_pSelectedView;
    }

    if (MainWindow* mainWindow = MainWindow::instance())
    {
        mainWindow->SetActiveView(m_pSelectedView ? m_pSelectedView->GetViewPane() : nullptr);
    }

    if (m_pSelectedView != nullptr)
    {
        m_pSelectedView->SetSelected(true);

        // The default viewport context alias resolves to the focused viewport, whose world is the active one.
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
    case eNotify_OnBeginGameMode:
        // A prefab world has no game in it. This runs before StartPlayInEditor, so re-selecting redirects it.
        if (IsPrefabEditorViewport(m_pSelectedView) && m_pLastEditorView)
        {
            ShowViewportPane(m_pLastEditorView);
            SelectViewport(m_pLastEditorView);
        }
        break;
    }
}

AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> CViewManager::GetManipulatorManager()
{
    return m_manipulatorManager;
}
