/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation file


#include "EditorDefs.h"

#include "CustomAspectRatioDlg.h"
#include "CustomResolutionDlg.h"
#include "ViewPane.h"

// Qt
#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QScrollArea>
#include <QToolBar>

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>

#include <AzQtComponents/Components/Style.h>

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

// Editor
#include "ViewManager.h"
#include "Settings.h"
#include "LayoutWnd.h"
#include "Viewport.h"
#include "LayoutConfigDialog.h"
#include "MainWindow.h"
#include "QtViewPaneManager.h"
#include "EditorViewportWidget.h"
#include <Editor/EditorViewportSettings.h>

static const std::pair<int, int> ViewportRatios[] = { { 16, 9 }, { 16, 10 }, { 4, 3 }, { 5, 4 } };
static const size_t ViewportRatiosCount = sizeof(ViewportRatios) / sizeof(ViewportRatios[0]);
static const std::pair<int, int> ViewportResolutions[] =
    { { 1280, 720 }, { 1920, 1080 }, { 2560, 1440 }, { 2048, 858 }, { 1998, 1080 }, { 3480, 2160 } };
static const size_t ViewportResolutionsCount = sizeof(ViewportResolutions) / sizeof(ViewportResolutions[0]);
static constexpr int SortKeySpacing = 100;

/////////////////////////////////////////////////////////////////////////////
// CLayoutViewPane
//////////////////////////////////////////////////////////////////////////
CLayoutViewPane::CLayoutViewPane(QWidget* parent)
    : AzQtComponents::ToolBarArea(parent)
{
    m_viewport = nullptr;
    m_active = false;
    m_nBorder = VIEW_BORDER;

    m_bFullscreen = false;

    // Set up an optional scrollable area for our viewport.  We'll use this for times that we want a fixed-size
    // viewport independent of main window size.
    m_viewportScrollArea = new QScrollArea(this);
    m_viewportScrollArea->setContentsMargins(QMargins());
    m_viewportScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
    m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
    m_toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();
    if (m_actionManagerInterface && m_menuManagerInterface && m_toolBarManagerInterface)
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    // If this is being instantiated after the Action Manager was alreadi initialized, add the toolbar.
    // Else it will be added in OnToolBarRegistrationHook.
    if (QToolBar* toolBar = m_toolBarManagerInterface->GenerateToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier))
    {
        addToolBar(Qt::TopToolBarArea, toolBar);
    }

    m_id = -1;
}

CLayoutViewPane::~CLayoutViewPane() 
{
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

    if (m_viewportScrollArea)
    {
        // We only ever add m_viewport into our scroll area, which we manage separately,
        // so make sure to take it back before deleting m_scrollArea. Otherwise it will
        // try and get deleted as a part of deleting m_scrollArea.
        m_viewportScrollArea->takeWidget();
        delete m_viewportScrollArea;
    }

    OnDestroy(); 
}

void CLayoutViewPane::OnMenuRegistrationHook()
{
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Viewport Camera Settings";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportCameraMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Viewport Debug Info";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportDebugInfoMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Viewport Helpers";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportHelpersMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Viewport Size";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportSizeMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Ratio";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportSizeRatioMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Resolution";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportSizeResolutionMenuIdentifier, menuProperties);
        }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Viewport Options";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, menuProperties);
    }
}

void CLayoutViewPane::OnToolBarRegistrationHook()
{
    // Register top viewport toolbar.
    AzToolsFramework::ToolBarProperties toolBarProperties;
    toolBarProperties.m_name = "Viewport ToolBar";
    m_toolBarManagerInterface->RegisterToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier, toolBarProperties);

    // Add toolbar to top of viewport.
    QToolBar* toolBar = m_toolBarManagerInterface->GenerateToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier);
    addToolBar(Qt::TopToolBarArea, toolBar);
}

void CLayoutViewPane::OnActionRegistrationHook()
{
    // Dummy Action with Resize Icon
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.resizeIcon";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Viewport Size";
        actionProperties.m_iconPath = ":/Menu/resolution.svg";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
            }
        );
    }

    // Dummy Action with Menu Icon
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.menuIcon";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Options";
        actionProperties.m_iconPath = ":/Menu/menu.svg";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
            }
        );
    }

    // Viewport Debug Information
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.info.toggle";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Toggle Between States";
        actionProperties.m_category = "Viewport Debug Information";
        actionProperties.m_iconPath = ":/Menu/debug.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [viewportTitleDlg = m_viewportTitleDlg]
            {
                viewportTitleDlg->OnToggleDisplayInfo();
            },
            []() -> bool
            {
                AZ::AtomBridge::ViewportInfoDisplayState currentState = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
                AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
                    currentState, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);

                return currentState != AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
            }
        );

        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier, actionIdentifier);
    }
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.info.normal";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Normal";
        actionProperties.m_category = "Viewport Debug Information";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [viewportTitleDlg = m_viewportTitleDlg]
            {
                viewportTitleDlg->SetNormalViewportInfo();
            },
            []() -> bool
            {
                AZ::AtomBridge::ViewportInfoDisplayState currentState = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
                AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
                    currentState, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);

                return currentState == AZ::AtomBridge::ViewportInfoDisplayState::NormalInfo;
            }
        );

        m_actionManagerInterface->AddActionToUpdater(
            EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier, actionIdentifier);
    }
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.info.full";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Full";
        actionProperties.m_category = "Viewport Debug Information";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [viewportTitleDlg = m_viewportTitleDlg]
            {
                viewportTitleDlg->SetFullViewportInfo();
            },
            []() -> bool
            {
                AZ::AtomBridge::ViewportInfoDisplayState currentState = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
                AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
                    currentState, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);

                return currentState == AZ::AtomBridge::ViewportInfoDisplayState::FullInfo;
            }
        );

        m_actionManagerInterface->AddActionToUpdater(
            EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier, actionIdentifier);
    }
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.info.compact";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Compact";
        actionProperties.m_category = "Viewport Debug Information";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [viewportTitleDlg = m_viewportTitleDlg]
            {
                viewportTitleDlg->SetCompactViewportInfo();
            },
            []() -> bool
            {
                AZ::AtomBridge::ViewportInfoDisplayState currentState = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
                AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
                    currentState, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);

                return currentState == AZ::AtomBridge::ViewportInfoDisplayState::CompactInfo;
            }
        );

        m_actionManagerInterface->AddActionToUpdater(
            EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier, actionIdentifier);
    }
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.info.none";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "None";
        actionProperties.m_category = "Viewport Debug Information";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [viewportTitleDlg = m_viewportTitleDlg]
            {
                viewportTitleDlg->SetNoViewportInfo();
            },
            []() -> bool
            {
                AZ::AtomBridge::ViewportInfoDisplayState currentState = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
                AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
                    currentState, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);

                return currentState == AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
            });

        m_actionManagerInterface->AddActionToUpdater(
            EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier, actionIdentifier);
    }

    // Viewport Size

    for (size_t i = 0; i < ViewportRatiosCount; ++i)
    {
        int width = ViewportRatios[i].first;
        int height = ViewportRatios[i].second;

        AZStd::string actionIdentifier = AZStd::string::format("o3de.action.viewport.size.ratio[%i:%i]", width, height);
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = AZStd::string::format("%i:%i", width, height);
        actionProperties.m_category = "Viewport Size Ratio";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this, w = width, h = height]
            {
                SetAspectRatio(w, h);
            }
        );
    }
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.size.ratio.custom";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Custom...";
        actionProperties.m_category = "Viewport Size Ratio";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [&]
            {
                const QRect viewportRect = GetViewport()->rect();
                const unsigned int width = viewportRect.width();
                const unsigned int height = viewportRect.height();

                int whGCD = AZ::GetGCD(width, height);
                CCustomAspectRatioDlg aspectRatioInputDialog(width / whGCD, height / whGCD, this);

                if (aspectRatioInputDialog.exec() == QDialog::Accepted)
                {
                    const unsigned int aspectX = aspectRatioInputDialog.GetX();
                    const unsigned int aspectY = aspectRatioInputDialog.GetY();

                    SetAspectRatio(aspectX, aspectY);
                }
            }
        );
    }
    for (size_t i = 0; i < ViewportResolutionsCount; ++i)
    {
        int width = ViewportResolutions[i].first;
        int height = ViewportResolutions[i].second;

        AZStd::string actionIdentifier = AZStd::string::format("o3de.action.viewport.size.resolution[%i:%i]", width, height);
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = AZStd::string::format("%i:%i", width, height);
        actionProperties.m_category = "Viewport Size Resolution";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [&, w = width, h = height]
            {
                ResizeViewport(w, h);
            }
        );
    }
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.viewport.size.resolution.custom";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Custom...";
        actionProperties.m_category = "Viewport Size Resolution";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                const QRect rectViewport = GetViewport()->rect();
                CCustomResolutionDlg resDlg(rectViewport.width(), rectViewport.height(), parentWidget());
                if (resDlg.exec() == QDialog::Accepted)
                {
                    ResizeViewport(resDlg.GetWidth(), resDlg.GetHeight());
                }
            }
        );
    }
}

void CLayoutViewPane::OnMenuBindingHook()
{
    // Camera
    {
        m_menuManagerInterface->AddWidgetToMenu(EditorIdentifiers::ViewportCameraMenuIdentifier, "o3de.widgetAction.viewport.fieldOfView", 100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportCameraMenuIdentifier, "o3de.action.view.goToPosition", 200);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportCameraMenuIdentifier, 300);
        m_menuManagerInterface->AddWidgetToMenu(
            EditorIdentifiers::ViewportCameraMenuIdentifier, "o3de.widgetAction.viewport.cameraSpeedScale", 400);
    }

    // Debug Info
    {
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportDebugInfoMenuIdentifier, "o3de.action.viewport.info.normal", 100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportDebugInfoMenuIdentifier, "o3de.action.viewport.info.full", 200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportDebugInfoMenuIdentifier, "o3de.action.viewport.info.compact", 300);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportDebugInfoMenuIdentifier, "o3de.action.viewport.info.none", 400);
    }

    // Helpers
    {
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportHelpersMenuIdentifier, "o3de.action.view.toggleIcons", 100);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportHelpersMenuIdentifier, 200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportHelpersMenuIdentifier, "o3de.action.view.showHelpers", 300);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportHelpersMenuIdentifier, "o3de.action.view.showSelectedEntityHelpers", 400);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportHelpersMenuIdentifier, "o3de.action.view.hideHelpers", 500);
    }

    // Size
    {
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewportSizeMenuIdentifier, EditorIdentifiers::ViewportSizeRatioMenuIdentifier, 100);
        {
            for (size_t i = 0; i < ViewportRatiosCount; ++i)
            {
                int width = ViewportRatios[i].first;
                int height = ViewportRatios[i].second;
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.viewport.size.ratio[%i:%i]", width, height);

                m_menuManagerInterface->AddActionToMenu(
                    EditorIdentifiers::ViewportSizeRatioMenuIdentifier, actionIdentifier, SortKeySpacing * (aznumeric_cast<int>(i) + 1));
            }
            
            m_menuManagerInterface->AddSeparatorToMenu(
                EditorIdentifiers::ViewportSizeRatioMenuIdentifier, SortKeySpacing * (aznumeric_cast<int>(ViewportRatiosCount) + 1));
            m_menuManagerInterface->AddActionToMenu(
                EditorIdentifiers::ViewportSizeRatioMenuIdentifier,
                "o3de.action.viewport.size.ratio.custom",
                SortKeySpacing * (aznumeric_cast<int>(ViewportRatiosCount) + 2));
        }
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewportSizeMenuIdentifier, EditorIdentifiers::ViewportSizeResolutionMenuIdentifier, 200);
        {
            for (size_t i = 0; i < ViewportResolutionsCount; ++i)
            {
                int width = ViewportResolutions[i].first;
                int height = ViewportResolutions[i].second;
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.viewport.size.resolution[%i:%i]", width, height);

                m_menuManagerInterface->AddActionToMenu(
                    EditorIdentifiers::ViewportSizeResolutionMenuIdentifier, actionIdentifier, SortKeySpacing * (aznumeric_cast<int>(i) + 1));
            }

            m_menuManagerInterface->AddSeparatorToMenu(
                EditorIdentifiers::ViewportSizeResolutionMenuIdentifier, SortKeySpacing * (aznumeric_cast<int>(ViewportResolutionsCount) + 1));
            m_menuManagerInterface->AddActionToMenu(
                EditorIdentifiers::ViewportSizeResolutionMenuIdentifier,
                "o3de.action.viewport.size.resolution.custom",
                SortKeySpacing * (aznumeric_cast<int>(ViewportResolutionsCount) + 2));
        }
    }

    // Options
    {
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, "o3de.action.edit.snap.toggleGridSnapping", 300);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, "o3de.action.edit.snap.toggleShowingGrid", 400);
        m_menuManagerInterface->AddWidgetToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, "o3de.widgetAction.viewport.gridSnappingSize", 500);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, 600);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, "o3de.action.edit.snap.toggleAngleSnapping", 700);
        m_menuManagerInterface->AddWidgetToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, "o3de.widgetAction.viewport.angleSnappingSize", 800);
    }
}

void CLayoutViewPane::OnToolBarBindingHook()
{
    m_toolBarManagerInterface->AddWidgetToToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.widgetAction.expander", 300);
    m_toolBarManagerInterface->AddWidgetToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.widgetAction.prefab.editVisualMode", 400);
    m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.view.goToPosition", EditorIdentifiers::ViewportCameraMenuIdentifier, 500);
    m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.viewport.info.toggle", EditorIdentifiers::ViewportDebugInfoMenuIdentifier, 600);
    m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.view.showHelpers", EditorIdentifiers::ViewportHelpersMenuIdentifier, 700);
    m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.viewport.resizeIcon", EditorIdentifiers::ViewportSizeMenuIdentifier, 800);
    m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.viewport.menuIcon", EditorIdentifiers::ViewportOptionsMenuIdentifier, 900);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetViewClass(const QString& sClass)
{
    if (m_viewport && m_viewPaneClass == sClass)
    {
        return;
    }
    m_viewPaneClass = sClass;

    ReleaseViewport();

    QWidget* newPane = QtViewPaneManager::instance()->CreateWidget(sClass);
    if (newPane)
    {
        newPane->setProperty("IsViewportWidget", true);
        AttachViewport(newPane);
    }
}

//////////////////////////////////////////////////////////////////////////
QString CLayoutViewPane::GetViewClass() const
{
    return m_viewPaneClass;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnDestroy()
{
    ReleaseViewport();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SwapViewports(CLayoutViewPane* pView)
{
    QWidget* pViewport = pView->GetViewport();
    QWidget* pViewportOld = m_viewport;

    std::swap(m_viewPaneClass, pView->m_viewPaneClass);

    AttachViewport(pViewport);
    pView->AttachViewport(pViewportOld);
}

void CLayoutViewPane::SetViewportExpansionPolicy(ViewportExpansionPolicy policy)
{
    m_viewportPolicy = policy;

    switch (policy)
    {
        // If the requested policy is "FixedSize", wrap our viewport area in a scrollable
        // region so that we can always make the viewport a guaranteed fixed size regardless of the
        // main window size.  The scrollable area will auto-resize with the main window, but the
        // viewport won't.
        case ViewportExpansionPolicy::FixedSize:
            {
                QWidget* scrollViewport = m_viewportScrollArea->viewport();
                m_viewportScrollArea->setWidget(m_viewport);
                m_viewport->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                // For some reason, the QScrollArea is adding a margin all around our viewable area,
                // so we'll shrink our viewport by an appropriate offset (twice the margin thickness) 
                // so that it continues to fit without requiring scroll bars after switching size policies.
                m_viewport->resize(m_viewport->width() - (scrollViewport->x() * 2), m_viewport->height() - (scrollViewport->y() * 2));
                SetMainWidget(m_viewportScrollArea);
                update();

            }
            break;
        // If the requested policy is "AutoExpand", just put the viewport area directly in the ViewPane.
        // It will auto-resize with the main window, but requests to change the viewport size might not 
        // result in the exact size being requested, depending on main window size and layout.
        case ViewportExpansionPolicy::AutoExpand:
            {
                m_viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                SetMainWidget(m_viewport);
                update();
            }
            break;
    }
}


//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::AttachViewport(QWidget* pViewport)
{
    if (pViewport == m_viewport)
    {
        return;
    }

    DisconnectRenderViewportInteractionRequestBus();
    m_viewport = pViewport;
    if (pViewport)
    {
        SetViewportExpansionPolicy(ViewportExpansionPolicy::AutoExpand);

        if (QtViewport* vp = qobject_cast<QtViewport*>(pViewport))
        {
            vp->SetViewportId(GetId());
            vp->SetViewPane(this);
            if (EditorViewportWidget* renderViewport = viewport_cast<EditorViewportWidget*>(vp))
            {
                renderViewport->ConnectViewportInteractionRequestBus();
            }
        }

        m_viewport->setVisible(true);

        setWindowTitle(m_viewPaneClass);

        if (QtViewport* vp = qobject_cast<QtViewport*>(pViewport))
        {
            OnFOVChanged(vp->GetFOV());
        }
        else
        {
            OnFOVChanged(SandboxEditor::CameraDefaultFovRadians());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::DetachViewport()
{
    DisconnectRenderViewportInteractionRequestBus();
    OnFOVChanged(SandboxEditor::CameraDefaultFovRadians());
    m_viewport = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ReleaseViewport()
{
    if (m_viewport)
    {
        DisconnectRenderViewportInteractionRequestBus();
        m_viewport->deleteLater();
        m_viewport = nullptr;
    }
}

void CLayoutViewPane::DisconnectRenderViewportInteractionRequestBus()
{
    if (QtViewport* vp = qobject_cast<QtViewport*>(m_viewport))
    {
        if (EditorViewportWidget* renderViewport = viewport_cast<EditorViewportWidget*>(vp))
        {
            renderViewport->DisconnectViewportInteractionRequestBus();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ResizeViewport(int width, int height)
{
    if (!m_viewport)
    {
        return;
    }

    // Get our MainWidget, which will either be the viewport or a scrollable area around the
    // viewport, depending on which expansion policy we've chosen.
    QWidget* mainWidget = GetMainWidget();
    if (!mainWidget)
    {
        mainWidget = m_viewport;
    }

    // If our main widget is a scroll area, specifically get the size of the viewable area within the scroll area.
    // This way, even if we currently have scroll bars visible, we'll try to resize our main window and scroll area
    // to make the entire viewport visible.
    QSize mainWidgetSize = mainWidget->size();
    if (QScrollArea* scrollArea = qobject_cast<QScrollArea*>(mainWidget))
    {
        mainWidgetSize = scrollArea->viewport()->size();
    }

    // Make sure our requestedSize stays within "legal" bounds.
    const QSize requestedSize = QSize(AZ::GetClamp(width, MIN_VIEWPORT_RES, MAX_VIEWPORT_RES), 
                                      AZ::GetClamp(height, MIN_VIEWPORT_RES, MAX_VIEWPORT_RES));

    // The delta between our current and requested size is going to be used to try and resize the main window
    // (either growing or shrinking) by the exact same amount so that the new viewport size is still completely
    // visible without needing to adjust any other widget sizes.  
    // Note that we're specifically taking the delta from the main widget, not the viewport.
    // In the "AutoResize" case, this will still directly take the delta from our viewport, but in the
    // "FixedSize" case we need to take the delta from the scroll area's viewable area size, since that's actually 
    // the one we need to grow/shrink proportional to.
    const QSize deltaSize = requestedSize - mainWidgetSize;

    // Do nothing if the new size is the same as the old size.
    if (deltaSize == QSize(0, 0))
    {
        return;
    }

    MainWindow* mainWindow = MainWindow::instance();

    // We need to adjust the main window size to make it larger/smaller as appropriate
    // to fit the newly-resized viewport, so start by making sure it isn't maximized.
    if (mainWindow->isMaximized())
    {
        mainWindow->showNormal();
    }

    // Resize our main window by the amount that we want our viewport to change.
    // This is intended to grow our viewport by the same amount.  However, this logic is a 
    // little flawed and should get revisited at some point:
    // 1) It's possible that the mainWindow won't actually change to the requested size, if the requested
    // size is larger than the current display resolution (Qt::SetGeometry will fire a second resize event
    // that shrinks the mainWindow back to the display resolution and will emit a warning in debug builds),
    // or smaller than the minimum size allowed by the various widgets in the window.
    // 2) If LayoutWnd contains multiple viewports, the delta change of the main window will affect the delta
    // size of LayoutWnd, which is then divided proportionately among the multiple viewports, so the 1:1 size
    // assumption in the logic below isn't correct in the multi-viewport case.
    // 3) Sometimes Qt will just change this by 1 pixel (either smaller or bigger) with a second subsequent
    // resize event for no apparent reason.
    // 4) The layout of the docked windows *around* the viewport widget can affect how it grows and shrinks,
    // so even if you try to change the size of the layout widget, it might auto-resize afterwards to fill any
    // gaps between it and other widgets (console window, entity inspector, etc).
    mainWindow->move(0, 0);
    mainWindow->resize(mainWindow->size() + deltaSize);

    // We can avoid the problems above by using the "FixedSize" policy.  If we've chosen this policy, we'll make
    // the viewport a scrollable region that's exactly the resolution requested here.  This is useful for screenshots
    // in automation testing among other things, since this way we can guarantee the resolution of the screenshot matches
    // the resolution of any golden images we're comparing against.
    if (m_viewportPolicy == ViewportExpansionPolicy::FixedSize)
    {
        m_viewport->resize(requestedSize);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetAspectRatio(unsigned int x, unsigned int y)
{
    if (x == 0 || y == 0)
    {
        return;
    }

    const QRect viewportRect = m_viewport->rect();

    // Round height to nearest multiple of y aspect, then scale width according to ratio
    const unsigned int height = ((viewportRect.height() + y - 1) / y) * y;
    const unsigned int width = height / y * x;

    ResizeViewport(width, height);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetViewportFOV(const float fovDegrees)
{
    if (EditorViewportWidget* pRenderViewport = qobject_cast<EditorViewportWidget*>(m_viewport))
    {
        const auto fovRadians = AZ::DegToRad(fovDegrees);
        pRenderViewport->SetFOV(fovRadians);

        // if viewport camera is active, make selected fov new default
        SandboxEditor::SetCameraDefaultFovRadians(fovRadians);

        OnFOVChanged(fovRadians);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ToggleMaximize()
{
    ////////////////////////////////////////////////////////////////////////
    // Switch in and out of fullscreen mode for a edit view
    ////////////////////////////////////////////////////////////////////////
    CLayoutWnd* wnd = GetIEditor()->GetViewManager()->GetLayout();
    if (wnd)
    {
        wnd->MaximizeViewport(GetId());
    }
    setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnMenuLayoutConfig()
{
    if (GetIEditor()->IsInGameMode())
    {
        // you may not change your viewports while game mode is running.
        CryLog("You may not change viewport configuration while in game mode.");
        return;
    }

    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (layout)
    {
        CLayoutConfigDialog dlg;
        dlg.SetLayout(layout->GetLayout());
        if (dlg.exec() == QDialog::Accepted)
        {
            // Will kill this Pane. so must be last line in this function.
            layout->CreateLayout(dlg.GetLayout());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnMenuViewSelected(const QString& paneName)
{
    if (GetIEditor()->IsInGameMode())
    {
        CryLog("You may not change viewport configuration while in game mode.");
        // you may not change your viewports while game mode is running.
        return;
    }

    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (layout)
    {
        layout->BindViewport(this, paneName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnMenuMaximized()
{
    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (m_viewport && layout)
    {
        layout->MaximizeViewport(GetId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::ShowTitleMenu()
{
    ////////////////////////////////////////////////////////////////////////
    // Process clicks on the view buttons and the menu button
    ////////////////////////////////////////////////////////////////////////
    // Only continue when we have a viewport.

    // Create pop up menu.
    QMenu root(this);
    if (QtViewport* vp = qobject_cast<QtViewport*>(m_viewport))
    {
        vp->OnTitleMenu(&root);
    }

    if (!root.isEmpty())
    {
        root.addSeparator();
    }

    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    QAction* action = root.addAction(tr("Maximized"));
    if (layout)
    {
        connect(action, &QAction::triggered, layout, &CLayoutWnd::MaximizeViewport);
    }
    action->setChecked(IsFullscreen());

    action = root.addAction(tr("Configure Layout..."));
    if (!CViewManager::IsMultiViewportEnabled())
    {
        action->setDisabled(true);
    }

    // NOTE: this must be a QueuedConnection, so that it executes after the menu is deleted.
    // Changing the layout can cause the current "this" pointer to be deleted
    // and since we've made the "this" pointer the menu's parent,
    // it gets deleted when the "this" pointer is deleted. Since it's not a heap object,
    // that causes a crash. Using a QueuedConnection forces the layout config to happen
    // after the QMenu is cleaned up on the stack.
    connect(action, &QAction::triggered, this, &CLayoutViewPane::OnMenuLayoutConfig, Qt::QueuedConnection);

    root.exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        ToggleMaximize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::focusInEvent([[maybe_unused]] QFocusEvent* event)
{
    // Forward SetFocus call to child viewport.
    if (m_viewport)
    {
        m_viewport->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetFullscren(bool f)
{
    m_bFullscreen = f;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::SetFocusToViewport()
{
    if (m_viewport)
    {
        m_viewport->window()->activateWindow();
        m_viewport->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutViewPane::OnFOVChanged(const float fovRadians)
{
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Broadcast(
        &AzToolsFramework::ViewportInteraction::ViewportSettingNotifications::OnCameraFovChanged, fovRadians);
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    AZ::Vector2 PyGetViewPortSize()
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane && viewPane->GetViewport())
        {
            const QRect rcViewport = viewPane->GetViewport()->rect();
            return AZ::Vector2(static_cast<float>(rcViewport.width()), static_cast<float>(rcViewport.height()));
        }
        else
        {
            return AZ::Vector2();
        }
    }

    void PySetViewPortSize(int width, int height)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            viewPane->ResizeViewport(width, height);
        }
    }

    static void PyUpdateViewPort()
    {
        GetIEditor()->UpdateViews(eRedrawViewports);
    }

    void PyResizeViewport(int width, int height)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();

        if (viewPane)
        {
            viewPane->ResizeViewport(width, height);
        }
    }

    void PyBindViewport(const char* viewportName)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            GetIEditor()->GetViewManager()->GetLayout()->BindViewport(viewPane, viewportName);
        }
    }

    void PySetViewportExpansionPolicy(const char* policy)
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            if (AzFramework::StringFunc::Equal(policy, "AutoExpand"))
            {
                viewPane->SetViewportExpansionPolicy(CLayoutViewPane::ViewportExpansionPolicy::AutoExpand);
            }
            else if (AzFramework::StringFunc::Equal(policy, "FixedSize"))
            {
                viewPane->SetViewportExpansionPolicy(CLayoutViewPane::ViewportExpansionPolicy::FixedSize);
            }
        }
    }

    const char* PyGetViewportExpansionPolicy()
    {
        CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();
        if (viewPane)
        {
            switch (viewPane->GetViewportExpansionPolicy())
            {
                case CLayoutViewPane::ViewportExpansionPolicy::AutoExpand: return "AutoExpand";
                case CLayoutViewPane::ViewportExpansionPolicy::FixedSize: return "FixedSize";
            }
        }

        return "";
    }

    unsigned int PyGetViewportCount()
    {
        CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
        return layout ? layout->GetViewPaneCount() : 0;
    }

    unsigned int PyGetActiveViewport()
    {
        CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
        if (layout)
        {
            CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView();

            for (unsigned int index = 0; index < layout->GetViewPaneCount(); index++)
            {
                if (viewPane == layout->GetViewPaneByIndex(index))
                {
                    return index;
                }
            }
        }

        AZ_Error("Main", false, "No active viewport found.");
        return 0;
    }

    void PySetActiveViewport(unsigned int viewportIndex)
    {
        [[maybe_unused]] bool success = false;
        CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
        if (layout)
        {
            CLayoutViewPane* viewPane = layout->GetViewPaneByIndex(viewportIndex);
            if (viewPane)
            {
                viewPane->SetFocusToViewport();
                MainWindow::instance()->SetActiveView(viewPane);
                success = true;
            }
        }
        AZ_Error("Main", success, "Active viewport was not set successfully.");
    }

    unsigned int PyGetViewPaneLayout()
    {
        CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
        return layout ? layout->GetLayout() : ET_Layout0;
    }

    void PySetViewPaneLayout(unsigned int layoutId)
    {
        AZ_PUSH_DISABLE_WARNING(4296, "-Wunknown-warning-option")
        if ((layoutId >= ET_Layout0) && (layoutId <= ET_Layout8))
        AZ_POP_DISABLE_WARNING
        {
            CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
            if (layout)
            {
                layout->CreateLayout(static_cast<EViewLayout>(layoutId));
            }
        }
        else
        {
            AZ_Error("Main", false, "Invalid layout (%u), only values from %u to %u are valid.", layoutId, ET_Layout0, ET_Layout8);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

namespace AzToolsFramework
{
    void ViewPanePythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("get_viewport_size", PyGetViewPortSize, nullptr, "Get the width and height of the active viewport."));
            addLegacyGeneral(behaviorContext->Method("set_viewport_size", PySetViewPortSize, nullptr, "Set the width and height of the active viewport."));
            addLegacyGeneral(behaviorContext->Method("update_viewport", PyUpdateViewPort, nullptr, "Update all visible SDK viewports."));
            addLegacyGeneral(behaviorContext->Method("resize_viewport", PyResizeViewport, nullptr, "Resizes the viewport resolution to a given width & height."));
            addLegacyGeneral(behaviorContext->Method("bind_viewport", PyBindViewport, nullptr, "Binds the viewport to a specific view like 'Top', 'Front', 'Perspective'."));
            addLegacyGeneral(behaviorContext->Method("get_viewport_expansion_policy", PyGetViewportExpansionPolicy, nullptr, "Returns whether viewports are auto-resized with the main window ('AutoExpand') or if they remain a fixed size ('FixedSize')."));
            addLegacyGeneral(behaviorContext->Method("set_viewport_expansion_policy", PySetViewportExpansionPolicy, nullptr, "Sets whether viewports are auto-resized with the main window ('AutoExpand') or if they remain a fixed size ('FixedSize')."));
            addLegacyGeneral(behaviorContext->Method("get_viewport_count", PyGetViewportCount, nullptr, "Get the total number of viewports."));
            addLegacyGeneral(behaviorContext->Method("get_active_viewport", PyGetActiveViewport, nullptr, "Get the active viewport index."));
            addLegacyGeneral(behaviorContext->Method("set_active_viewport", PySetActiveViewport, nullptr, "Set the active viewport by index."));
            addLegacyGeneral(behaviorContext->Method("get_view_pane_layout", PyGetViewPaneLayout, nullptr, "Get the active view pane layout."));
            addLegacyGeneral(behaviorContext->Method("set_view_pane_layout", PySetViewPaneLayout, nullptr, "Set the active view pane layout."));
        }
    }
}

#include <moc_ViewPane.cpp>
