/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation file


#include "EditorDefs.h"
#include "LayoutWnd.h"

#include "CustomAspectRatioDlg.h"
#include "CustomResolutionDlg.h"
#include "MainWindow.h"
#include "QtViewPaneManager.h"
#include "ViewPane.h"

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <QApplication>
#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <QAction>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <QSettings>
#include <algorithm>

static const std::pair<int, int> ViewportRatios[] = { { 16, 9 }, { 16, 10 }, { 4, 3 }, { 5, 4 } };
static const size_t ViewportRatiosCount = sizeof(ViewportRatios) / sizeof(ViewportRatios[0]);
static const std::pair<int, int> ViewportResolutions[] =
    { { 1280, 720 }, { 1920, 1080 }, { 2560, 1440 }, { 2048, 858 }, { 1998, 1080 }, { 3480, 2160 } };
static const size_t ViewportResolutionsCount = sizeof(ViewportResolutions) / sizeof(ViewportResolutions[0]);
static constexpr int SortKeySpacing = 100;

CLayoutWnd::CLayoutWnd(QWidget* parent)
    : AzQtComponents::DockMainWindow(parent, Qt::Widget)
    , m_actionManagerInterface(AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
    , m_menuManagerInterface(AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get())
    , m_toolBarManagerInterface(AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get())
{
    setDockOptions(GroupedDragging | AllowNestedDocks | AllowTabbedDocks);
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* widget) { SelectPaneForWidget(widget); });
    qApp->installEventFilter(this);
}

CLayoutWnd::~CLayoutWnd()
{
    qApp->removeEventFilter(this);
    disconnect(qApp, nullptr, this, nullptr);
    while (!m_docks.isEmpty())
    {
        delete m_docks.takeLast();
    }
}

bool CLayoutWnd::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        SelectPaneForWidget(qobject_cast<QWidget*>(object));
    }
    return QMainWindow::eventFilter(object, event);
}

void CLayoutWnd::SelectPaneForWidget(QWidget* widget)
{
    while (widget && MainWindow::instance())
    {
        if (auto* pane = qobject_cast<CLayoutViewPane*>(widget))
        {
            MainWindow::instance()->SetActiveView(pane);
            return;
        }
        widget = widget->parentWidget();
    }
}

CLayoutViewPane* CLayoutWnd::Pane(int slot) const
{
    return qobject_cast<CLayoutViewPane*>(m_docks.at(slot)->widget());
}

CLayoutViewPane* CLayoutWnd::GetViewPaneByIndex(unsigned int index) const
{
    for (int slot = 0; slot < m_docks.size(); ++slot)
    {
        if (!m_closedDocks.contains(m_docks[slot]) && (m_maximizedSlot < 0 || m_maximizedSlot == slot) && index-- == 0)
        {
            return Pane(slot);
        }
    }
    return nullptr;
}

unsigned int CLayoutWnd::GetViewPaneCount() const
{
    return m_maximizedSlot >= 0 ? 1 : aznumeric_cast<unsigned int>(m_docks.size() - m_closedDocks.size());
}

void CLayoutWnd::UpdateDockFeatures()
{
    const bool canClose = GetViewPaneCount() > 1;
    for (auto* dock : m_docks)
    {
        auto features = dock->features();
        features.setFlag(QDockWidget::DockWidgetClosable, canClose);
        dock->setFeatures(features);
    }
}

void CLayoutWnd::MaximizeViewport(int slot)
{
    if (slot < 0 || slot >= m_docks.size() || m_docks.size() == 1)
    {
        return;
    }
    auto* dock = m_docks[slot];
    if (dock->isFloating())
    {
        if (dock->isMaximized())
        {
            dock->showNormal();
        }
        else
        {
            dock->showMaximized();
        }
        return;
    }
    if (m_maximizedSlot >= 0)
    {
        m_maximizedSlot = -1;
        for (auto* hidden : m_hiddenByMaximize)
        {
            hidden->show();
        }
        m_hiddenByMaximize.clear();
    }
    else
    {
        m_maximizedSlot = slot;
        for (auto* other : m_docks)
        {
            if (other != dock && other->isVisible())
            {
                m_hiddenByMaximize.push_back(other);
                other->hide();
            }
        }
    }
    Pane(slot)->SetFullscreen(m_maximizedSlot >= 0);
    UpdateDockFeatures();
    Pane(slot)->SetFocusToViewport();
}

QString CLayoutWnd::ViewportTypeToClassName(EViewportType type)
{
    auto* pane = QtViewPaneManager::instance()->GetViewportPane(type);
    return pane ? pane->m_name : QString();
}

QDockWidget* CLayoutWnd::CreateViewportDock()
{
    const int slot = aznumeric_cast<int>(m_docks.size());
    auto* add = MainWindow::instance()->IsPreview() ? nullptr
        : new QAction(QIcon(QStringLiteral(":/MainWindow/toolbars/add-perspective.svg")), tr("Add perspective viewport"));
    auto* dock = new AzQtComponents::StyledDockWidget(this, add);
    if (add)
    {
        add->setObjectName(QStringLiteral("addPerspectiveViewport"));
        connect(add, &QAction::triggered, this, [this, dock] { AddPerspectiveTab(dock); });
    }
    auto* pane = new CLayoutViewPane(slot, dock);
    dock->setObjectName(QStringLiteral("PerspectiveViewport%1").arg(slot));
    dock->setWidget(pane);
    connect(pane, &QWidget::windowTitleChanged, dock, &QWidget::setWindowTitle);
    m_docks.append(dock);
    connect(dock, &AzQtComponents::StyledDockWidget::aboutToClose, this, [this, dock]
    {
        m_closedDocks.insert(dock);
        UpdateDockFeatures();
    });
    connect(dock, &QDockWidget::visibilityChanged, this, [this, pane, dock](bool visible)
    {
        if (!visible && dock->isHidden() && MainWindow::instance() && MainWindow::instance()->GetActiveView() == pane)
        {
            for (auto* remaining : m_docks)
            {
                if (remaining != dock && remaining->isVisible())
                {
                    MainWindow::instance()->SetActiveView(qobject_cast<CLayoutViewPane*>(remaining->widget()));
                    break;
                }
            }
        }
    });
    return dock;
}

void CLayoutWnd::AddPerspectiveTab(QDockWidget* source)
{
    if (GetIEditor()->IsInGameMode())
    {
        return;
    }
    if (m_maximizedSlot >= 0)
    {
        MaximizeViewport(m_maximizedSlot);
    }
    auto* tabGroup = AzQtComponents::DockTabWidget::ParentTabWidget(source);
    QWidget* anchor = tabGroup ? tabGroup->parentWidget() : source;
    auto* host = qobject_cast<QMainWindow*>(anchor->parentWidget());
    if (!host)
    {
        return;
    }

    auto* dock = m_closedDocks.isEmpty() ? CreateViewportDock() : *m_closedDocks.begin();
    m_closedDocks.remove(dock);
    auto* pane = qobject_cast<CLayoutViewPane*>(dock->widget());
    pane->SetViewClass(ViewportTypeToClassName(ET_ViewportCamera));
    QtViewPaneManager::instance()->GetDockManager()->tabifyDockWidget(source, dock, host);
    qobject_cast<CLayoutViewPane*>(source->widget())->SetFullscreen(false);
    dock->show();
    UpdateDockFeatures();
    MainWindow::instance()->SetActiveView(pane);
    pane->SetFocusToViewport();
}

void CLayoutWnd::CreateLayout(EViewLayout layout, bool bindViewports, EViewportType defaultView)
{
    constexpr int paneCounts[] = { 1, 2, 2, 3, 3, 3, 3, 4, 4 };
    if (layout < ET_Layout0 || layout > ET_Layout8)
    {
        return;
    }
    m_layout = layout;
    m_hiddenByMaximize.clear();
    m_closedDocks.clear();
    m_maximizedSlot = -1;
    const int count = paneCounts[layout];
    while (m_docks.size() > count)
    {
        delete m_docks.takeLast();
    }
    for (int slot = 0; slot < count; ++slot)
    {
        if (slot == m_docks.size())
        {
            CreateViewportDock();
        }
        auto* dock = m_docks[slot];
        if (auto* tabs = AzQtComponents::DockTabWidget::ParentTabWidget(dock))
        {
            tabs->removeTab(dock);
        }
        if (auto* host = qobject_cast<QMainWindow*>(dock->parentWidget()))
        {
            host->removeDockWidget(dock);
        }
        dock->setParent(this);
        dock->setFloating(false);
        Pane(slot)->SetFullscreen(count == 1);
        if (bindViewports)
        {
            Pane(slot)->SetViewClass(ViewportTypeToClassName(slot == 0 ? defaultView : ET_ViewportCamera));
        }
    }
    // Each preset starts with the left or upper pane. Split that dock again for nested rows and columns.
    auto split = [this](int first, int second, Qt::Orientation orientation)
    {
        splitDockWidget(m_docks[first], m_docks[second], orientation);
        m_docks[second]->show();
        const int extent = orientation == Qt::Horizontal ? width() : height();
        const int half = std::max(1, extent / 2);
        resizeDocks({ m_docks[first], m_docks[second] }, { half, half }, orientation);
    };
    const bool reversed = layout == ET_Layout1 || layout == ET_Layout2 || layout == ET_Layout3 ||
        layout == ET_Layout5 || layout == ET_Layout7 || layout == ET_Layout8;
    const int first = reversed ? 1 : 0;
    addDockWidget(Qt::TopDockWidgetArea, m_docks[first]);
    m_docks[first]->show();
    if (count >= 2)
    {
        const bool vertical = layout == ET_Layout2 || layout == ET_Layout5 || layout == ET_Layout6 || layout == ET_Layout8;
        split(first, layout == ET_Layout7 ? 2 : 1 - first, vertical ? Qt::Vertical : Qt::Horizontal);
        if (count >= 3)
        {
            if (layout == ET_Layout7)
            {
                split(1, 3, Qt::Vertical);
                split(2, 0, Qt::Vertical);
            }
            else
            {
                split(1, 2, vertical ? Qt::Horizontal : Qt::Vertical);
                if (layout == ET_Layout8)
                {
                    split(2, 3, Qt::Horizontal);
                    const int third = std::max(1, width() / 3);
                    resizeDocks({ m_docks[1], m_docks[2], m_docks[3] }, { third, third, third }, Qt::Horizontal);
                }
            }
        }
    }
    UpdateDockFeatures();
    MainWindow::instance()->SetActiveView(Pane(0));
}

const char* CLayoutWnd::GetConfigGroupName()
{
    return "ViewportLayout";
}

void CLayoutWnd::SaveConfig()
{
    QSettings settings;
    settings.beginGroup(GetConfigGroupName());
    settings.setValue("Layout", static_cast<int>(m_layout));
    settings.setValue("Maximized", m_maximizedSlot + 1);
    QStringList classes;
    for (int slot = 0; slot < m_docks.size(); ++slot)
    {
        classes.append(m_closedDocks.contains(m_docks[slot]) ? QString() : Pane(slot)->GetViewClass());
    }
    while (!classes.isEmpty() && classes.last().isEmpty())
    {
        classes.removeLast();
    }
    settings.setValue("Viewports", classes);
}

bool CLayoutWnd::LoadConfig()
{
    QSettings settings;
    settings.beginGroup(GetConfigGroupName());
    const int layout = settings.value("Layout", -1).toInt();
    if (layout < ET_Layout0 || layout > ET_Layout8)
    {
        return false;
    }
    CreateLayout(static_cast<EViewLayout>(layout), false);
    const QVariant savedPanes = settings.value("Viewports");
    // Older configurations stored a comma-separated list of classes for a fixed preset.
    const bool hasPaneSlots = savedPanes.metaType().id() == QMetaType::QStringList;
    const QStringList classes = hasPaneSlots ? savedPanes.toStringList() : savedPanes.toString().split(',');
    for (int slot = 0; slot < m_docks.size(); ++slot)
    {
        auto name = classes.value(slot);
        Pane(slot)->SetViewClass(name.isEmpty() ? ViewportTypeToClassName(ET_ViewportCamera) : name);
    }
    if (hasPaneSlots)
    {
        for (auto slot = m_docks.size(); slot < classes.size(); ++slot)
        {
            AddPerspectiveTab(m_docks.front());
        }
        for (int slot = 0; slot < m_docks.size(); ++slot)
        {
            if (classes.value(slot).isEmpty() && GetViewPaneCount() > 1)
            {
                m_docks[slot]->close();
            }
        }
    }
    MaximizeViewport(settings.value("Maximized", 0).toInt() - 1);
    MainWindow::instance()->SetActiveView(GetViewPaneByIndex(0));
    return true;
}

void CLayoutWnd::ResetLayout()
{
    QSettings settings;
    settings.remove(GetConfigGroupName());
    CreateLayout(ET_Layout0);
}

CLayoutViewPane* CLayoutWnd::FindViewByClass(const QString& name)
{
    for (int slot = 0; slot < m_docks.size(); ++slot)
    {
        if (Pane(slot)->GetViewClass() == name)
        {
            return Pane(slot);
        }
    }
    return nullptr;
}

bool CLayoutWnd::CycleViewport(EViewportType from, EViewportType to)
{
    const QString viewClassName = ViewportTypeToClassName(from);
    CLayoutViewPane* vp = FindViewByClass(viewClassName);
    if (vp)
    {
        vp->SetViewClass(ViewportTypeToClassName(to));
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::Cycle2DViewport()
{
    // Cycle between 3 2D viewports.
    switch (m_layout)
    {
    case ET_Layout0:
        if (CycleViewport(ET_ViewportCamera, ET_ViewportXY))
        {
            return;
        }
        if (CycleViewport(ET_ViewportXY, ET_ViewportXZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportXZ, ET_ViewportYZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportYZ, ET_ViewportCamera))
        {
            return;
        }
        break;

    default:
        if (CycleViewport(ET_ViewportXY, ET_ViewportXZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportXZ, ET_ViewportYZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportYZ, ET_ViewportXY))
        {
            return;
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::OnMenuRegistrationHook()
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

void CLayoutWnd::OnToolBarRegistrationHook()
{
    // Register top viewport toolbar.
    AzToolsFramework::ToolBarProperties toolBarProperties;
    toolBarProperties.m_name = "Viewport ToolBar";
    m_toolBarManagerInterface->RegisterToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier, toolBarProperties);
}

void CLayoutWnd::OnActionRegistrationHook()
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

    using InfoState = AZ::AtomBridge::ViewportInfoDisplayState;
    using InfoBus = AZ::AtomBridge::AtomViewportInfoDisplayRequestBus;
    const auto displayState = []
    {
        InfoState state = InfoState::NoInfo;
        InfoBus::BroadcastResult(state, &InfoBus::Events::GetDisplayState);
        return state;
    };
    const auto setDisplayState = [](InfoState state)
    {
        InfoBus::Broadcast(&InfoBus::Events::SetDisplayState, state);
    };
    const auto registerInfoAction = [this](const char* id, const char* name, const auto& trigger, const auto& checked, const char* icon = "")
    {
        AzToolsFramework::ActionProperties properties;
        properties.m_name = name;
        properties.m_iconPath = icon;
        properties.m_category = "Viewport Debug Information";
        m_actionManagerInterface->RegisterCheckableAction(EditorIdentifiers::MainWindowActionContextIdentifier, id, properties, trigger, checked);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier, id);
    };
    registerInfoAction("o3de.action.viewport.info.toggle", "Toggle Between States",
        [=] { setDisplayState(static_cast<InfoState>((static_cast<int>(displayState()) + 1) % static_cast<int>(InfoState::Invalid))); },
        [=] { return displayState() != InfoState::NoInfo; }, ":/Menu/debug.svg");

    struct InfoAction
    {
        const char* id;
        const char* name;
        InfoState state;
    };
    constexpr InfoAction infoActions[] = {
        { "o3de.action.viewport.info.normal", "Normal", InfoState::NormalInfo },
        { "o3de.action.viewport.info.full", "Full", InfoState::FullInfo },
        { "o3de.action.viewport.info.compact", "Compact", InfoState::CompactInfo },
        { "o3de.action.viewport.info.none", "None", InfoState::NoInfo }
    };
    for (const auto& action : infoActions)
    {
        registerInfoAction(action.id, action.name, [=] { setDisplayState(action.state); }, [=] { return displayState() == action.state; });
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
            [width, height]
            {
                if (auto* pane = MainWindow::instance()->GetActiveView())
                {
                    pane->SetAspectRatio(width, height);
                }
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
            []
            {
                auto* pane = MainWindow::instance()->GetActiveView();
                if (!pane || !pane->GetViewport())
                {
                    return;
                }
                const QRect viewportRect = pane->GetViewport()->rect();
                const unsigned int width = viewportRect.width();
                const unsigned int height = viewportRect.height();

                const int whGCD = AZ::GetGCD(width, height);
                if (whGCD == 0)
                {
                    return;
                }
                CCustomAspectRatioDlg aspectRatioInputDialog(width / whGCD, height / whGCD, pane);

                if (aspectRatioInputDialog.exec() == QDialog::Accepted)
                {
                    const unsigned int aspectX = aspectRatioInputDialog.GetX();
                    const unsigned int aspectY = aspectRatioInputDialog.GetY();

                    pane->SetAspectRatio(aspectX, aspectY);
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
            [width, height]
            {
                if (auto* pane = MainWindow::instance()->GetActiveView())
                {
                    pane->ResizeViewport(width, height);
                }
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
            []
            {
                auto* pane = MainWindow::instance()->GetActiveView();
                if (!pane || !pane->GetViewport())
                {
                    return;
                }
                const QRect rectViewport = pane->GetViewport()->rect();
                CCustomResolutionDlg resDlg(rectViewport.width(), rectViewport.height(), pane);
                if (resDlg.exec() == QDialog::Accepted)
                {
                    pane->ResizeViewport(resDlg.GetWidth(), resDlg.GetHeight());
                }
            }
        );
    }
}

void CLayoutWnd::OnMenuBindingHook()
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

        // Geometry snapping, after the grid and angle entries it belongs alongside. A separator
        // first because the entries above quantise a value, while this snaps to what is in the
        // level - related enough to share the menu, different enough not to read as one group.
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportOptionsMenuIdentifier, 850);
        m_menuManagerInterface->AddActionToMenu(
            EditorIdentifiers::ViewportOptionsMenuIdentifier, "o3de.action.edit.snap.geometrySnapping", 875);
    }
}

void CLayoutWnd::OnToolBarBindingHook()
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
        EditorIdentifiers::ViewportTopToolBarIdentifier,
        "o3de.action.edit.snap.geometrySnapping",
        EditorIdentifiers::EditModifySnapToMenuIdentifier,
        850);
    m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
        EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.viewport.menuIcon", EditorIdentifiers::ViewportOptionsMenuIdentifier, 900);
}

//////////////////////////////////////////////////////////////////////////
