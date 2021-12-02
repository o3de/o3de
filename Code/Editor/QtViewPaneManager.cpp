/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "QtViewPaneManager.h"
#include "Controls/ConsoleSCB.h"
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/Widgets/Card.h>

#include <QDockWidget>
#include <QMainWindow>
#include <QDataStream>
#include <QDebug>
#include <QCloseEvent>
#include <QLayout>
#include <QApplication>
#include <QRect>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QRubberBand>
#include <QCursor>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include "MainWindow.h"

#include <algorithm>
#include <QScopedValueRollback>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzAssetBrowser/AzAssetBrowserWindow.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzQtComponents/Utilities/AutoSettingsGroup.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/UI/Docking/DockWidgetUtils.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzQtComponents/Buses/ShortcutDispatch.h>
#include <AzQtComponents/Utilities/QtViewPaneEffects.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include "ShortcutDispatcher.h"

// Helper for EditorComponentModeNotifications to be used
// as a member instead of inheriting from EBus directly.
class ViewportEditorModeNotificationsBusImpl
    : public AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
{
 public:
    // Set the function to be called when entering ComponentMode.
    void SetEnteredComponentModeFunc(
        const AZStd::function<void(const AzToolsFramework::ViewportEditorModesInterface&)>& enteredComponentModeFunc)
    {
        m_enteredComponentModeFunc = enteredComponentModeFunc;
    }

    // Set the function to be called when leaving ComponentMode.
    void SetLeftComponentModeFunc(
        const AZStd::function<void(const AzToolsFramework::ViewportEditorModesInterface&)>& leftComponentModeFunc)
    {
        m_leftComponentModeFunc = leftComponentModeFunc;
    }

 private:
    // ViewportEditorModeNotificationsBus overrides ...
    void OnEditorModeActivated(
         const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            m_enteredComponentModeFunc(editorModeState);
        }
    }

    void OnEditorModeDeactivated(
        const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            m_leftComponentModeFunc(editorModeState);
        }
    }

    AZStd::function<void(const AzToolsFramework::ViewportEditorModesInterface&)> m_enteredComponentModeFunc; ///< Function to call when entering ComponentMode.
    AZStd::function<void(const AzToolsFramework::ViewportEditorModesInterface&)> m_leftComponentModeFunc; ///< Function to call when leaving ComponentMode.
};

struct ViewLayoutState
{
    QVector<QString> viewPanes;
    QByteArray mainWindowState;
    QMap<QString, QRect> fakeDockWidgetGeometries;
};
Q_DECLARE_METATYPE(ViewLayoutState)

static QDataStream &operator<<(QDataStream & out, const ViewLayoutState&myObj)
{
    int placeHolderVersion = 1;
    out << myObj.viewPanes << myObj.mainWindowState << placeHolderVersion << myObj.fakeDockWidgetGeometries;
    return out;
}

static QDataStream& operator>>(QDataStream& in, ViewLayoutState& myObj)
{
    in >> myObj.viewPanes;
    in >> myObj.mainWindowState;

    int version = 0;
    if (!in.atEnd())
    {
        in >> version;
        in >> myObj.fakeDockWidgetGeometries;
    }

    return in;
}

// All settings keys for stored layouts are in the form "layouts/<name>"
// When starting up, "layouts/last" is loaded
static QLatin1String s_lastLayoutName = QLatin1String("last");

static QString GetFancyViewPaneStateGroupName()
{
    return QString("%1/%2").arg("Editor").arg("fancyWindowLayouts");
}

#if AZ_TRAIT_OS_PLATFORM_APPLE
// this event filter class eats mouse events
// it is used in the non dockable fake dock widget
// to make sure its inner title bar cannot be dragged
class MouseEatingEventFilter : public QObject
{
public:
    MouseEatingEventFilter(QObject* parent)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseMove:
                return true;
            default:
                return false;
        }
    }
};
#endif

Q_GLOBAL_STATIC(QtViewPaneManager, s_viewPaneManagerInstance)


QWidget* QtViewPane::CreateWidget()
{
    QWidget* w = nullptr;

    if (m_factoryFunc)
    {
        // Although all the factory lambdas do have a default nullptr argument, this information
        // doesn't get retained when they are converted to std::function<QWidget*(QWidget*)>,
        // thus we need to set the parent explicitly.
        // At the same time, adding a default argument to the lambdas will allow for them to be
        // called exactly as before in all other places where they are not converted, so we get
        // to explicitly pass an argument only if strictly necessary.
        w = m_factoryFunc(nullptr);
    }
    else
    {
        // If a view pane was registered using RegisterCustomViewPane, then instead of a factory function, we rely
        // on ViewPaneCallbackBus::CreateViewPaneWidget to create the widget for us and then pass back the Qt windowId
        // so that we can retrieve it.
        AZ::u64 createdWidgetWinId;
        AzToolsFramework::ViewPaneCallbackBus::EventResult(createdWidgetWinId, m_name.toUtf8().constData(), &AzToolsFramework::ViewPaneCallbacks::CreateViewPaneWidget);
        w = QWidget::find(createdWidgetWinId);
    }

    return w;
}

bool QtViewPane::Close(QtViewPane::CloseModes closeModes)
{
    if (!IsConstructed())
    {
        return true;
    }

    return CloseInstance(m_dockWidget, closeModes);
}

bool QtViewPane::CloseInstance(QDockWidget* dockWidget, CloseModes closeModes)
{
    if (!dockWidget)
    {
        return false;
    }

    bool canClose = true;
    bool destroy = closeModes & CloseMode::Destroy;

    // Console is not deletable, so always hide it instead of destroying
    if (!m_options.isDeletable)
    {
        destroy = false;
    }

    if (!(closeModes & CloseMode::Force))
    {
        QCloseEvent closeEvent;

        // Prevent closing view pane if it has modal dialog open, as modal dialogs
        // are often constructed on stack and will not finish properly when the view
        // pane is destroyed.
        QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        const int numTopLevel = topLevelWidgets.size();
        for (size_t i = 0; i < numTopLevel; ++i)
        {
            QWidget* widget = topLevelWidgets[static_cast<int>(i)];
            if (widget->isModal() && widget->isVisible())
            {
                widget->activateWindow();
                return false;
            }
        }

        // Check if embedded QWidget allows view pane to be closed.
        QCoreApplication::sendEvent(dockWidget->widget(), &closeEvent);
        // If widget accepted the close event, we delete the dockwidget, which will also delete the child widget in case it doesn't have Qt::WA_DeleteOnClose
        if (!closeEvent.isAccepted())
        {
            // Widget doesn't want to close
            canClose = false;
        }
    }

    if (canClose)
    {
        if (destroy)
        {
            //important to set parent to null otherwise docking code will still find it while restoring since that happens before the delete.
            dockWidget->setParent(nullptr);
            dockWidget->deleteLater();

            if (dockWidget == m_dockWidget)
            {
                //clear dockwidget pointer otherwise if we open this pane before the delete happens we'll think it's already there, then it gets deleted on us.
                m_dockWidget.clear();
            }
        }
        else
        {
            // If the dock widget is tabbed, then just remove it from the tab widget
            AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(dockWidget);
            if (tabWidget)
            {
                tabWidget->removeTab(dockWidget);
            }
            // Otherwise just hide the widget
            else
            {
                dockWidget->hide();
            }
        }
    }

    return canClose;
}

static bool SkipTitleBarOverdraw(QtViewPane* pane)
{
    return !pane->m_options.isDockable;
}

DockWidget::DockWidget(QWidget* widget, QtViewPane* pane, [[maybe_unused]] QSettings* settings, QMainWindow* parent, AzQtComponents::FancyDocking* advancedDockManager)
    : AzQtComponents::StyledDockWidget(pane->m_name, SkipTitleBarOverdraw(pane),
#if AZ_TRAIT_OS_PLATFORM_APPLE
          pane->m_options.detachedWindow ? nullptr : parent)
#else
          parent)
#endif
    , m_mainWindow(parent)
    , m_pane(pane)
    , m_advancedDockManager(advancedDockManager)
{
    // keyboard shortcuts from any other context shouldn't trigger actions under this dock widget
    AzQtComponents::MarkAsShortcutSearchBreak(this);

    if (pane->m_options.isDeletable)
    {
        setAttribute(Qt::WA_DeleteOnClose);
    }

    QString objectNameForSave = pane->m_options.saveKeyName.length() > 0 ? pane->m_options.saveKeyName : pane->m_name;
    setObjectName(objectNameForSave);

    setWidget(widget);
    setFocusPolicy(Qt::StrongFocus);

    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
}

bool DockWidget::event(QEvent* qtEvent)
{
    // this accounts for a difference in behavior where we want all floating windows to be always parented to the main window instead of to each other, so that
    // they don't overlap in odd ways - for example, if you tear off a floating window from another floating window, under Qt's system its technically still a child of that window
    // so that window can't ever be placed on top of it.  This is not what we want.  We want you to be able to then take that window and drag it into this new one.
    // (Qt's original behavior is like that so if you double click on a floating widget it docks back into the parent which it came from - we don't use this functionality)
    if (qtEvent->type() == QEvent::WindowActivate
#if AZ_TRAIT_OS_PLATFORM_APPLE
        && !m_pane->m_options.detachedWindow
#endif
    )
    {
        reparentToMainWindowFix();
    }

    return AzQtComponents::StyledDockWidget::event(qtEvent);
}

void DockWidget::reparentToMainWindowFix()
{
    if (!isFloating() || !AzToolsFramework::DockWidgetUtils::isDockWidgetWindowGroup(parentWidget()))
    {
        return;
    }

    if (qApp->mouseButtons() & Qt::LeftButton)
    {
        // We're still dragging, lets try later
        QTimer::singleShot(200, this, &DockWidget::reparentToMainWindowFix);
        return;
    }

    // bump it up and to the left by the size of its frame, to account for the reparenting operation;
    QPoint framePos = pos();
    QPoint contentPos = mapToGlobal(QPoint(0, 0));
    move(framePos.x() - (contentPos.x() - framePos.x()), framePos.y() - (contentPos.y() - framePos.y()));

    // we have to dock this to the mainwindow, even if we're floating, so that the mainwindow knows about it.
    // if the preferred area is valid, use that. Otherwise, arbitrarily toss it in the left.
    // This is relevant because it will determine where the widget goes if the title bar is double clicked
    // after it's been detached from a QDockWidgetGroupWindow
    auto dockArea = (m_pane->m_options.preferedDockingArea != Qt::DockWidgetArea::NoDockWidgetArea) ? m_pane->m_options.preferedDockingArea : Qt::LeftDockWidgetArea;

    setParent(m_mainWindow);
    m_mainWindow->addDockWidget(dockArea, this);
    setFloating(true);
}

QString DockWidget::PaneName() const
{
    return m_pane->m_name;
}

void DockWidget::RestoreState(bool forceDefault)
{
#if AZ_TRAIT_OS_PLATFORM_APPLE
    if (m_pane->m_options.detachedWindow)
    {
        if (forceDefault)
        {
            window()->setGeometry(m_pane->m_options.paneRect);
        }
        else
        {
            QRect geometry = QtViewPaneManager::instance()->GetLayout().fakeDockWidgetGeometries[objectName()];
            if (!geometry.isValid())
            {
                geometry = m_pane->m_options.paneRect;
            }
            window()->setGeometry(geometry);
        }
        return;
    }
#endif

    // check if we can get the main window to do all the work for us first
    // (which is also the proper way to do this)
    if (!forceDefault)
    {
        // If the advanced docking is enabled, let it try to restore the dock widget
        bool restored = false;
        if (m_advancedDockManager)
        {
            restored = m_advancedDockManager->restoreDockWidget(this);
        }
        // Otherwise, let our main window do it directly
        else
        {
            restored = m_mainWindow->restoreDockWidget(this);
        }

        if (restored)
        {
            AzToolsFramework::DockWidgetUtils::correctVisibility(this);
            return;
        }
    }

    // can't rely on the main window; fall back to our preferences
    auto dockingArea = m_pane->m_options.preferedDockingArea;
    auto paneRect = m_pane->m_options.paneRect;

    // If we are floating and have multiple instances, try to calculate an appropriate rect from the most recently created, non-docked instance
    // make sure the new location would be reasonable on screen - otherwise use default paneRect for new widget positioning
    if (dockingArea == Qt::NoDockWidgetArea && m_pane->m_dockWidgetInstances.size() > 1)
    {
        static const int horizontalCascadeAmount = 20;
        static const int verticalCascadeAmount = 20;
        static const int lowerScreenEdgeBuffer = 50;

        QRect screenRect = QApplication::primaryScreen()->geometry();
        int screenHeight = screenRect.height();
        int screenWidth = screenRect.width();

        for (QList<DockWidget*>::reverse_iterator it = m_pane->m_dockWidgetInstances.rbegin(); it != m_pane->m_dockWidgetInstances.rend(); ++it)
        {
            DockWidget* dock = *it;

            if (dock != this)
            {
                QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dock->parentWidget());
                if (mainWindow && mainWindow->parentWidget())
                {
                    QPoint windowLocation = mainWindow->parentWidget()->mapToGlobal(QPoint(0, 0));

                    // Only nudge it to the right if we have room to do so
                    if (windowLocation.x() + horizontalCascadeAmount < screenWidth - paneRect.width())
                    {
                        paneRect.moveLeft(windowLocation.x() + horizontalCascadeAmount);

                        // Keep it from getting too low on the screen
                        if (windowLocation.y() + verticalCascadeAmount < screenHeight - lowerScreenEdgeBuffer)
                        {
                            paneRect.moveTop(windowLocation.y() + verticalCascadeAmount);
                        }
                    }

                    // We found an undocked window, just go ahead and break, if we couldn't adjust because of positioning,
                    // it will be placed at the default location
                    break;
                }
            }
        }
    }

    // make sure we're sized properly before we dock
    if (paneRect.isValid())
    {
        resize(paneRect.size());
    }

    // check if we should force floating
    bool floatWidget = (dockingArea == Qt::NoDockWidgetArea);

    // if we're floating, we need to move and resize again, because the act of docking may have moved us
    if (floatWidget)
    {
        // in order for saving and restoring state to work properly in Qt,
        // along with docking widgets within other floating widgets, the widget
        // must be added at least once to the main window, with a VALID area,
        // before we set it to floating.
        auto arbitraryDockingArea = Qt::LeftDockWidgetArea;
        m_mainWindow->addDockWidget(arbitraryDockingArea, this);

        // If we are using the fancy docking, let it handle making the dock
        // widget floating, or else the titlebar will be missing, since
        // floating widgets are actually contained in a floating main
        // window container
        if (m_advancedDockManager)
        {
            m_advancedDockManager->makeDockWidgetFloating(this, paneRect);
        }
        // Otherwise, we can make the dock widget floating directly and move it
        else
        {
            setFloating(true);

            // Not using setGeometry() since it excludes the frame when positioning
            if (paneRect.isValid())
            {
                resize(paneRect.size());
                move(paneRect.topLeft());
            }
        }
    }
    else
    {
        m_mainWindow->addDockWidget(dockingArea, this);
    }
}

QRect DockWidget::ProperGeometry() const
{
    QRect myGeom(pos(), size());

    // we need this state in global coordinates, but if we're parented to one of those group dock windows, there is a problem, it will be local coords.
    if (!isFloating())
    {
        if (parentWidget() && (strcmp(parentWidget()->metaObject()->className(), "QDockWidgetGroupWindow") == 0))
        {
            myGeom = QRect(parentWidget()->pos(), parentWidget()->size());
        }
    }

    return myGeom;
}

QString DockWidget::settingsKey() const
{
    return settingsKey(m_pane->m_name);
}

QString DockWidget::settingsKey(const QString& paneName)
{
    return QStringLiteral("ViewPane-") + paneName;
}

// run generic function on all widgets considered for greying out/disabling
template<typename Fn>
void SetDefaultActionsEnabled(
    const bool enabled, QtViewPanes& registeredPanes, const Fn& fn)
{
    for (QtViewPane& p : registeredPanes)
    {
        if (!p.m_dockWidgetInstances.empty())
        {
            for (auto& dockWidget : p.m_dockWidgetInstances)
            {
                const auto& paneName = dockWidget->PaneName();
                // disable/fade all widgets other than those in the EntityInspector, EntityOutliner and Console
                // note: The Console is not greyed out and the EntityInspector and EntityOutliner handle their
                // own fading when entering/leaving ComponentMode
                if (paneName != LyViewPane::EntityInspector &&
                    paneName != LyViewPane::EntityInspectorPinned &&
                    paneName != LyViewPane::Console &&
                    paneName != LyViewPane::EntityOutliner)
                {
                    fn(dockWidget->widget(), enabled);
                }
            }
        }
    }
}

QtViewPaneManager::QtViewPaneManager(QObject* parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_settings(nullptr)
    , m_restoreInProgress(false)
    , m_advancedDockManager(nullptr)
    , m_componentModeNotifications(AZStd::make_unique<ViewportEditorModeNotificationsBusImpl>())
{
    qRegisterMetaTypeStreamOperators<ViewLayoutState>("ViewLayoutState");
    qRegisterMetaTypeStreamOperators<QVector<QString> >("QVector<QString>");

    // view pane manager is interested when we enter/exit ComponentMode
    m_componentModeNotifications->BusConnect(AzToolsFramework::GetEntityContextId());
    m_windowRequest.BusConnect();

    m_componentModeNotifications->SetEnteredComponentModeFunc(
        [this](const AzToolsFramework::ViewportEditorModesInterface&)
    {
        // gray out panels when entering ComponentMode
        SetDefaultActionsEnabled(false, m_registeredPanes, [](QWidget* widget, bool on)
        {
            AzQtComponents::SetWidgetInteractEnabled(widget, on);
        });
    });

    m_componentModeNotifications->SetLeftComponentModeFunc(
        [this](const AzToolsFramework::ViewportEditorModesInterface&)
    {
        // enable panels again when leaving ComponentMode
        SetDefaultActionsEnabled(true, m_registeredPanes, [](QWidget* widget, bool on)
        {
            AzQtComponents::SetWidgetInteractEnabled(widget, on);
        });
    });

    m_windowRequest.SetEnableEditorUiFunc(
        [this](bool enable)
        {
            // gray out panels when entering ImGui mode
            SetDefaultActionsEnabled(
                enable, m_registeredPanes,
                [](QWidget* widget, bool on)
                {
                    AzQtComponents::SetWidgetInteractEnabled(widget, on);
                });
        });
}

QtViewPaneManager::~QtViewPaneManager()
{
    m_windowRequest.BusDisconnect();
    m_componentModeNotifications->BusDisconnect();
}

static bool lessThan(const QtViewPane& v1, const QtViewPane& v2)
{
    if (v1.IsViewportPane() && v2.IsViewportPane())
    {
        // Registration order (Top, Front, Left ...)
        return v1.m_id < v2.m_id;
    }
    else if (!v1.IsViewportPane() && !v2.IsViewportPane())
    {
        // Sort by name
        return v1.m_name.compare(v2.m_name, Qt::CaseInsensitive) < 0;
    }
    else
    {
        // viewports on top of non-viewports
        return v1.IsViewportPane();
    }
}

void QtViewPaneManager::RegisterPane(const QString& name, const QString& category, ViewPaneFactory factory, const AzToolsFramework::ViewPaneOptions& options)
{
    if (IsPaneRegistered(name))
    {
        return;
    }

    QtViewPane view = { NextAvailableId(), name, category, factory, nullptr, options };

    // Sorted insert
    auto it = std::upper_bound(m_registeredPanes.begin(), m_registeredPanes.end(), view, lessThan);
    m_registeredPanes.insert(it, view);

    emit registeredPanesChanged();
}

void QtViewPaneManager::UnregisterPane(const QString& name)
{
    auto it = std::find_if(m_registeredPanes.begin(), m_registeredPanes.end(),
            [name](const QtViewPane& pane) { return name == pane.m_name; });

    if (it != m_registeredPanes.end())
    {
        QtViewPane& pane = *it;

        ClosePane(&pane);

        m_knownIdsSet.removeOne(pane.m_id);
        m_registeredPanes.erase(it);
        emit registeredPanesChanged();
    }
}

QtViewPaneManager* QtViewPaneManager::instance()
{
    return s_viewPaneManagerInstance();
}

bool QtViewPaneManager::exists()
{
    return s_viewPaneManagerInstance.exists();
}

void QtViewPaneManager::SetMainWindow(AzQtComponents::DockMainWindow* mainWindow, QSettings* settings, const QByteArray& lastMainWindowState)
{
    Q_ASSERT(mainWindow && !m_mainWindow && settings && !m_settings);
    m_mainWindow = mainWindow;
    m_settings = settings;
    m_advancedDockManager = new AzQtComponents::FancyDocking(mainWindow);

    m_defaultMainWindowState = mainWindow->saveState();
    m_loadedMainWindowState = lastMainWindowState;
}

const QtViewPane* QtViewPaneManager::OpenPane(const QString& name, QtViewPane::OpenModes modes)
{
    QtViewPane* pane = GetPane(name);
    if (!pane || !pane->IsValid())
    {
        qWarning() << Q_FUNC_INFO << "Could not find pane with name" << name;
        return nullptr;
    }

    // this multi-pane code is a bit of an hack to support more than one view of the same class
    // All views are single pane, except for one in Maglev Control plugin
    // Save/Restore support of the duplicates will only be implemented if required.

    const bool isMultiPane = modes & QtViewPane::OpenMode::MultiplePanes;

    DockWidget* newDockWidget = pane->m_dockWidget;

    if (!pane->IsVisible() || isMultiPane)
    {
        if (!pane->IsConstructed() || isMultiPane)
        {
            QWidget* w = pane->CreateWidget();
            if (!w)
            {
                qWarning() << Q_FUNC_INFO << "Unable to create widget for pane with name" << name;
                return nullptr;
            }

            w->setProperty("restored", (modes & QtViewPane::OpenMode::RestoreLayout) != 0);
            newDockWidget = new DockWidget(w, pane, m_settings, m_mainWindow, m_advancedDockManager);
            AzQtComponents::StyleManager::repolishStyleSheet(newDockWidget);

            // track every new dock widget instance that we created
            pane->m_dockWidgetInstances.push_back(newDockWidget);
            connect(newDockWidget, &QObject::destroyed, this, [this, name, newDockWidget]() {
                QtViewPane* pane = GetPane(name);
                if (pane && pane->IsValid())
                {
                    pane->m_dockWidgetInstances.removeAll(newDockWidget);
                }
            });

            // only set the single instance of the dock widget on the pane if this
            // isn't a special multi-pane instance
            if (!isMultiPane)
            {
                pane->m_dockWidget = newDockWidget;
            }
            else
            {
                m_advancedDockManager->disableAutoSaveLayout(newDockWidget);
            }

            newDockWidget->setVisible(true);

            // If this pane isn't dockable, set the allowed areas to none on the
            // dock widget so the fancy docking knows to prevent it from docking
            if (!pane->m_options.isDockable)
            {
                newDockWidget->setAllowedAreas(Qt::NoDockWidgetArea);
            }

            // only emit this signal if we're not creating a non-saving instance of the view pane
            if (!isMultiPane)
            {
                emit viewPaneCreated(pane);
            }

#if AZ_TRAIT_OS_PLATFORM_APPLE
            // handle showing fake dock widgets
            if (pane->m_options.detachedWindow)
            {
                ShowFakeNonDockableDockWidget(newDockWidget, pane);
            }
#endif
        }
        else if (!AzQtComponents::DockTabWidget::IsTabbed(newDockWidget))
        {
            newDockWidget->setVisible(true);
#if AZ_TRAIT_OS_PLATFORM_APPLE
            if (pane->m_options.detachedWindow)
            {
                newDockWidget->window()->show();
            }
#endif
        }

        if ((modes & QtViewPane::OpenMode::UseDefaultState) || isMultiPane)
        {
            const bool forceToDefault = true;
            newDockWidget->RestoreState(forceToDefault);
        }
        else if (!AzQtComponents::DockTabWidget::IsTabbed(newDockWidget) && !(modes & QtViewPane::OpenMode::OnlyOpen))
        {
            newDockWidget->RestoreState();
        }
    }

    // If the dock widget is off screen (e.g. second monitor was disconnected),
    // restore its default state
    if (QApplication::desktop()->screenNumber(newDockWidget) == -1)
    {
        const bool forceToDefault = true;
        newDockWidget->RestoreState(forceToDefault);
    }

    // If the widget's window is minimized, show it.
    QWidget* window = newDockWidget->window();
    if (window->isMinimized())
    {
        window->setWindowState(window->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    }

    if (pane->IsVisible())
    {
        if (!modes.testFlag(QtViewPane::OpenMode::RestoreLayout))
        {
            newDockWidget->setFocus();
        }
    }
    else
    {
        // If the dock widget is tabbed, then set it as the active tab
        AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(newDockWidget);
        if (tabWidget)
        {
            int index = tabWidget->indexOf(newDockWidget);
            tabWidget->setCurrentIndex(index);
        }
        // Otherwise just show the widget
        else
        {
            newDockWidget->show();
        }
    }

    // When a user opens a pane, if it is docked in a floating window, make sure
    // it isn't hidden behind other floating windows or the Editor main window
    if (modes.testFlag(QtViewPane::OpenMode::None))
    {
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(newDockWidget->parentWidget());
        if (!mainWindow)
        {
            // If the parent of our dock widgets isn't a QMainWindow, then it
            // might be tabbed, so try to find the tab container dock widget
            // and then get the QMainWindow from that.
            AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(newDockWidget);
            if (tabWidget)
            {
                QDockWidget* tabDockContainer = qobject_cast<QDockWidget*>(tabWidget->parentWidget());
                if (tabDockContainer)
                {
                    mainWindow = qobject_cast<QMainWindow*>(tabDockContainer->parentWidget());
                }
            }
        }

        if (mainWindow)
        {
            // If our pane is part of a floating window, then the parent of its
            // QMainWindow will be another dock widget container that is floating.
            // If this is the case, then raise it to the front so it won't be
            // hidden behind other floating windows (or the Editor main window)
            QDockWidget* parentDockWidget = qobject_cast<QDockWidget*>(mainWindow->parentWidget());
            if (parentDockWidget && parentDockWidget->isFloating())
            {
                parentDockWidget->raise();
            }
        }
    }

    return pane;
}

QDockWidget* QtViewPaneManager::InstancePane(const QString& name)
{
    const QtViewPane* pane = OpenPane(name, QtViewPane::OpenMode::UseDefaultState | QtViewPane::OpenMode::MultiplePanes);
    QDockWidget* newPaneWidget = nullptr;
    if (pane != nullptr)
    {
        newPaneWidget = pane->m_dockWidgetInstances.last();
    }

    return newPaneWidget;
}

bool QtViewPaneManager::ClosePane(const QString& name, QtViewPane::CloseModes closeModes)
{
    if (QtViewPane* p = GetPane(name))
    {
        return ClosePane(p, closeModes);
    }

    return false;
}

bool QtViewPaneManager::ClosePaneInstance(const QString& name, QDockWidget* dockWidget, QtViewPane::CloseModes closeModes)
{
    if (QtViewPane* p = GetPane(name))
    {
        return p->CloseInstance(dockWidget, closeModes);
    }

    return false;
}

bool QtViewPaneManager::ClosePane(QtViewPane* pane, QtViewPane::CloseModes closeModes)
{
    if (pane)
    {
        // Don't allow a dock widget to be closed if it is being dragged for docking
        if (m_advancedDockManager && m_advancedDockManager->IsDockWidgetBeingDragged(pane->m_dockWidget))
        {
            return false;
        }

        return pane->Close(closeModes | QtViewPane::CloseMode::Force);
    }

    return false;
}

bool QtViewPaneManager::CloseAllPanes()
{
    for (QtViewPane& p : m_registeredPanes)
    {
        if (!p.Close())
        {
            return false; // Abort closing
        }
    }
    return true;
}

void QtViewPaneManager::CloseAllNonStandardPanes()
{
    for (QtViewPane& p : m_registeredPanes)
    {
        if (!p.m_options.isStandard)
        {
            p.Close(QtViewPane::CloseMode::Force);
        }
    }
}

void QtViewPaneManager::TogglePane(const QString& name)
{
    QtViewPane* pane = GetPane(name);
    if (!pane)
    {
        Q_ASSERT(false);
        return;
    }

    if (pane->IsVisible())
    {
        ClosePane(name);
    }
    else
    {
        OpenPane(name);
    }
}

QWidget* QtViewPaneManager::CreateWidget(const QString& paneName)
{
    QtViewPane* pane = GetPane(paneName);
    if (!pane)
    {
        qWarning() << Q_FUNC_INFO << "Couldn't find pane" << paneName << "; paneCount=" << m_registeredPanes.size();
        return nullptr;
    }

    QWidget* w = pane->CreateWidget();
    if (w)
    {
        w->setWindowTitle(paneName);
        return w;
    }

    return nullptr;
}

void QtViewPaneManager::SaveLayout()
{
    SaveLayout(s_lastLayoutName);
}

void QtViewPaneManager::RestoreLayout(bool restoreDefaults)
{
    if (!restoreDefaults)
    {
        restoreDefaults = !RestoreLayout(s_lastLayoutName);
    }

    if (restoreDefaults)
    {
        // Nothing is saved in settings, restore default layout
        RestoreDefaultLayout();
    }
}

bool QtViewPaneManager::ClosePanesWithRollback(const QVector<QString>& panesToKeepOpen)
{
    QVector<QString> closedPanes;

    // try to close all panes that aren't remaining open after relayout
    bool rollback = false;
    for (QtViewPane& p : m_registeredPanes)
    {
        // Only close the panes that aren't remaining open and are currently
        // visible (which has to include a check if the pane is tabbed since
        // it could be hidden if its not the active tab)
        if (panesToKeepOpen.contains(p.m_name) || (!p.IsVisible() && !AzQtComponents::DockTabWidget::IsTabbed(p.m_dockWidget)))
        {
            continue;
        }

        // attempt to close this pane; if Close returns false, then the close event
        // was intercepted and the pane doesn't want to close, so we should cancel the whole thing
        // and rollback
        if (!p.Close())
        {
            rollback = true;
            break;
        }

        // keep track of the panes that we closed, so we can rollback later and reopen them
        closedPanes.push_back(p.m_name);
    }

    // check if we cancelled and need to roll everything back
    if (rollback)
    {
        for (const QString& paneName : closedPanes)
        {
            // append this to the end of the event loop with a zero length timer, so that
            // all of the close/hide events above are entirely processed
            QTimer::singleShot(0, this, [paneName, this]()
                {
                    OpenPane(paneName, QtViewPane::OpenMode::RestoreLayout);
                });
        }

        return false;
    }

    return true;
}

/**
 * Restore the default layout (also known as component entity layout)
 */
void QtViewPaneManager::RestoreDefaultLayout(bool resetSettings)
{
    // Get whether the prefab system is enabled
    bool isPrefabSystemEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (resetSettings)
    {
        // We're going to do something destructive (removing all of the viewpane settings). Better confirm with the user
        auto buttonPressed = QMessageBox::warning(m_mainWindow, tr("Restore Default Layout"), tr("Are you sure you'd like to restore to the default layout? This will reset all of your view related settings."), QMessageBox::Cancel | QMessageBox::RestoreDefaults, QMessageBox::RestoreDefaults);
        if (buttonPressed != QMessageBox::RestoreDefaults)
        {
            return;
        }
    }

    // First, close all the open panes
    if (!ClosePanesWithRollback(QVector<QString>()))
    {
        return;
    }

    // Disable updates while we restore the layout to avoid temporary glitches
    // as the panes are moved around
    m_mainWindow->setUpdatesEnabled(false);

    AzToolsFramework::EntityIdList selectedEntityIds;

    // Reset all of the settings, or windows opened outside of RestoreDefaultLayout won't be reset at all.
    // Also ensure that this is done after CloseAllPanes, because settings will be saved in CloseAllPanes
    if (resetSettings)
    {
        // Store off currently selected entities
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        // Clear any selection
        AzToolsFramework::EntityIdList noEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, noEntities);

        ViewLayoutState state;

        state.viewPanes.push_back(LyViewPane::EntityOutliner);
        state.viewPanes.push_back(LyViewPane::EntityInspector);
        state.viewPanes.push_back(LyViewPane::AssetBrowser);
        state.viewPanes.push_back(LyViewPane::Console);

        if (!isPrefabSystemEnabled)
        {
            state.viewPanes.push_back(LyViewPane::LevelInspector);
        }

        state.mainWindowState = m_defaultMainWindowState;

        {
            AzQtComponents::AutoSettingsGroup settingsGroupGuard(m_settings, GetFancyViewPaneStateGroupName());
            m_settings->setValue(s_lastLayoutName, QVariant::fromValue<ViewLayoutState>(state));
        }

        m_settings->sync();

        // Let anything listening know to reset as well (*cough*CLayoutWnd*cough*)
        emit layoutReset();

        // Ensure that the main window knows it's new state
        // otherwise when we load view panes that haven't been loaded,
        // the main window will attempt to position them where they were last, not in their default spot
        m_mainWindow->restoreState(m_defaultMainWindowState);
    }

    // Reset the default view panes to be opened. Used for restoring default layout and component entity layout.
    const QtViewPane* entityOutlinerViewPane = OpenPane(LyViewPane::EntityOutliner, QtViewPane::OpenMode::UseDefaultState);
    const QtViewPane* assetBrowserViewPane = OpenPane(LyViewPane::AssetBrowser, QtViewPane::OpenMode::UseDefaultState);
    const QtViewPane* entityInspectorViewPane = OpenPane(LyViewPane::EntityInspector, QtViewPane::OpenMode::UseDefaultState);
    const QtViewPane* consoleViewPane = OpenPane(LyViewPane::Console, QtViewPane::OpenMode::UseDefaultState);

    const QtViewPane* levelInspectorPane = nullptr;
    if (!isPrefabSystemEnabled)
    {
        levelInspectorPane = OpenPane(LyViewPane::LevelInspector, QtViewPane::OpenMode::UseDefaultState);
    }

    // This class does all kinds of behind the scenes magic to make docking / restore work, especially with groups
    // so instead of doing our special default layout attach / docking right now, we want to make it happen
    // after all of the other events have been processed.
    QTimer::singleShot(0, [=]
    {
        // If we are using the new docking, set the right dock area to be absolute
        // so that the inspector will be to the right of the viewport and console
        m_advancedDockManager->setAbsoluteCornersForDockArea(m_mainWindow, Qt::RightDockWidgetArea);

        // Retrieve the width of the screen that our main window is on so we can
        // use it later for resizing our panes. The main window ends up being maximized
        // when we restore the default layout, but even if we maximize the main window
        // before doing anything else, its width won't update until after this has all
        // been processed, so we need to resize the panes based on what the main window
        // width WILL be after maximized
        int screenWidth = QApplication::desktop()->screenGeometry(m_mainWindow).width();

        // Add the console view pane first
        m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, consoleViewPane->m_dockWidget);
        consoleViewPane->m_dockWidget->setFloating(false);

        if (entityInspectorViewPane)
        {
            m_mainWindow->addDockWidget(Qt::RightDockWidgetArea, entityInspectorViewPane->m_dockWidget);
            entityInspectorViewPane->m_dockWidget->setFloating(false);

            static const float tabWidgetWidthPercentage = 0.2f;
            int newWidth = static_cast<int>((float)screenWidth * tabWidgetWidthPercentage);

            if (levelInspectorPane)
            {
                // Tab the entity inspector with the level Inspector so that when they are
                // tabbed they will be given the default width, and move the entity inspector
                // to be the first tab on the left and active
                AzQtComponents::DockTabWidget* tabWidget = m_advancedDockManager->tabifyDockWidget(levelInspectorPane->m_dockWidget, entityInspectorViewPane->m_dockWidget, m_mainWindow);
                if (tabWidget)
                {
                    tabWidget->moveTab(1, 0);
                    tabWidget->setCurrentWidget(entityInspectorViewPane->m_dockWidget);

                    QDockWidget* tabWidgetParent = qobject_cast<QDockWidget*>(tabWidget->parentWidget());
                    m_mainWindow->resizeDocks({ tabWidgetParent }, { newWidth }, Qt::Horizontal);
                }
            }
            else
            {
                m_mainWindow->resizeDocks({ entityInspectorViewPane->m_dockWidget }, { newWidth }, Qt::Horizontal);
            }
        }

        if (assetBrowserViewPane && entityOutlinerViewPane)
        {
            m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, entityOutlinerViewPane->m_dockWidget);
            entityOutlinerViewPane->m_dockWidget->setFloating(false);

            m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, assetBrowserViewPane->m_dockWidget);
            assetBrowserViewPane->m_dockWidget->setFloating(false);

            m_advancedDockManager->splitDockWidget(m_mainWindow, entityOutlinerViewPane->m_dockWidget, assetBrowserViewPane->m_dockWidget, Qt::Vertical);

            // Resize our entity outliner (and by proxy the asset browser split with it)
            // so that they get an appropriate default width since the minimum sizes have
            // been removed from these widgets
            static const float entityOutlinerWidthPercentage = 0.15f;
            int newWidth = static_cast<int>((float)screenWidth * entityOutlinerWidthPercentage);
            m_mainWindow->resizeDocks({ entityOutlinerViewPane->m_dockWidget }, { newWidth }, Qt::Horizontal);
        }

        // Re-enable updates now that we've finished restoring the layout
        m_mainWindow->setUpdatesEnabled(true);

        // Default layout should always be maximized
        // (use window() because the MainWindow may be wrapped in another window
        // like a WindowDecoratorWrapper or another QMainWindow for various layout reasons)
        m_mainWindow->window()->showMaximized();

        if (resetSettings)
        {
            // Restore selection
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, selectedEntityIds);
        }
    });
}

void QtViewPaneManager::SaveLayout(QString layoutName)
{
    if (!m_mainWindow || m_restoreInProgress)
    {
        return;
    }

    layoutName = layoutName.trimmed();

    ViewLayoutState state;
    foreach(const QtViewPane &pane, m_registeredPanes)
    {
        // Include all visible and tabbed panes in our layout, since tabbed panes
        // won't be visible if they aren't the active tab, but still need to be
        // retained in the layout
        if (pane.IsVisible() || AzQtComponents::DockTabWidget::IsTabbed(pane.m_dockWidget))
        {
            state.viewPanes.push_back(pane.m_dockWidget->PaneName());
        }
    }

    state.mainWindowState = m_advancedDockManager->saveState();

    state.fakeDockWidgetGeometries = m_fakeDockWidgetGeometries;

    SaveStateToLayout(state, layoutName);

    AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);

}

void QtViewPaneManager::SaveStateToLayout(const ViewLayoutState& state, const QString& layoutName)
{
    const bool isNew = !HasLayout(layoutName);

    {
        AzQtComponents::AutoSettingsGroup settingsGroupGuard(m_settings, GetFancyViewPaneStateGroupName());
        m_settings->setValue(layoutName, QVariant::fromValue<ViewLayoutState>(state));
    }

    m_settings->sync();

    if (isNew)
    {
        emit savedLayoutsChanged();
    }
}

#if AZ_TRAIT_OS_PLATFORM_APPLE
/*
 * This methods creates a fake wrapper dock widget around the passed dock widget. The returned dock widget has
 * no parent and can therefore be used for a contained QOpenGLWidget on macOS, since this doesn't work as expected
 * when the QOpenGLWidget has the application's main window as (grand)parent.
 * The return dock widget looks like a normal dock widget. It cannot be docked and no other dock widget can be
 * docked into it.
 */
QDockWidget* QtViewPaneManager::ShowFakeNonDockableDockWidget(AzQtComponents::StyledDockWidget* dockWidget, QtViewPane* pane)
{
    dockWidget->customTitleBar()->setButtons({});
    dockWidget->customTitleBar()->setContextMenuPolicy(Qt::NoContextMenu);
    dockWidget->customTitleBar()->installEventFilter(new MouseEatingEventFilter(dockWidget));
    auto fakeDockWidget = new AzQtComponents::StyledDockWidget(QString(), false, nullptr);
    connect(dockWidget, &QObject::destroyed, fakeDockWidget, &QObject::deleteLater);
    fakeDockWidget->setAllowedAreas(Qt::NoDockWidgetArea);
    auto titleBar = fakeDockWidget->customTitleBar();
    titleBar->setDragEnabled(true);
    titleBar->setDrawSimple(true);
    titleBar->setButtons({AzQtComponents::DockBarButton::MaximizeButton, AzQtComponents::DockBarButton::CloseButton});
    fakeDockWidget->setWidget(dockWidget);
    fakeDockWidget->show();
    fakeDockWidget->setObjectName(dockWidget->objectName());

    connect(fakeDockWidget, &AzQtComponents::StyledDockWidget::aboutToClose, this, [this, fakeDockWidget]() {
        m_fakeDockWidgetGeometries[fakeDockWidget->objectName()] = fakeDockWidget->geometry();
    });
    if (pane->m_options.isDeletable)
    {
        fakeDockWidget->setAttribute(Qt::WA_DeleteOnClose);
    }

    return fakeDockWidget;
}
#endif

void QtViewPaneManager::SerializeLayout(XmlNodeRef& parentNode) const
{
    ViewLayoutState state = GetLayout();

    XmlNodeRef paneListNode = XmlHelpers::CreateXmlNode("ViewPanes");
    parentNode->addChild(paneListNode);

    for (const QString& paneName : state.viewPanes)
    {
        XmlNodeRef paneNode = XmlHelpers::CreateXmlNode("ViewPane");
        paneNode->setContent(paneName.toUtf8().data());

        paneListNode->addChild(paneNode);
    }

    XmlNodeRef windowStateNode = XmlHelpers::CreateXmlNode("WindowState");
    windowStateNode->setContent(state.mainWindowState.toHex().data());

    parentNode->addChild(windowStateNode);
}

bool QtViewPaneManager::DeserializeLayout(const XmlNodeRef& parentNode)
{
    ViewLayoutState state;

    XmlNodeRef paneListNode = parentNode->findChild("ViewPanes");
    if (!paneListNode)
        return false;

    for (int i = 0; i < paneListNode->getChildCount(); ++i)
    {
        XmlNodeRef paneNode = paneListNode->getChild(i);
        state.viewPanes.push_back(QString(paneNode->getContent()));
    }

    XmlNodeRef windowStateNode = parentNode->findChild("WindowState");
    if (!windowStateNode)
        return false;

    state.mainWindowState = QByteArray::fromHex(windowStateNode->getContent());

    return RestoreLayout(state);
}

ViewLayoutState QtViewPaneManager::GetLayout() const
{
    ViewLayoutState state;

    foreach(const QtViewPane &pane, m_registeredPanes)
    {
        // Include all visible and tabbed panes in our layout, since tabbed panes
        // won't be visible if they aren't the active tab, but still need to be
        // retained in the layout
        if (pane.IsVisible() || AzQtComponents::DockTabWidget::IsTabbed(pane.m_dockWidget))
        {
            state.viewPanes.push_back(pane.m_dockWidget->PaneName());
        }
    }

    state.mainWindowState = m_advancedDockManager->saveState();

    state.fakeDockWidgetGeometries = m_fakeDockWidgetGeometries;

    return state;
}


bool QtViewPaneManager::RestoreLayout(QString layoutName)
{
    if (m_restoreInProgress) // Against re-entrancy
    {
        return true;
    }

    QScopedValueRollback<bool> recursionGuard(m_restoreInProgress);
    m_restoreInProgress = true;

    layoutName = layoutName.trimmed();
    if (layoutName.isEmpty())
    {
        return false;
    }

    ViewLayoutState state;
    {
        AzQtComponents::AutoSettingsGroup settingsGroupGuard(m_settings, GetFancyViewPaneStateGroupName());

        if (!m_settings->contains(layoutName))
        {
            return false;
        }

        state = m_settings->value(layoutName).value<ViewLayoutState>();
    }

    // If we have the legacy UI disabled and are restoring the last user layout,
    // if it doesn't contain the Entity Inspector and Outliner then we need to
    // save their previous layout for them and switch them to the new default
    // layout because they won't be able to do much without them
    static const QString userLegacyLayout = "User Legacy Layout";
    if (layoutName == s_lastLayoutName && !HasLayout(userLegacyLayout))
    {
        bool layoutHasEntityInspector = false;
        bool layoutHasEntityOutliner = false;
        for (const QString& paneName : state.viewPanes)
        {
            if (paneName == LyViewPane::EntityInspector)
            {
                layoutHasEntityInspector = true;
            }
            else if (paneName == LyViewPane::EntityOutliner)
            {
                layoutHasEntityOutliner = true;
            }
        }

        if (!layoutHasEntityInspector || !layoutHasEntityOutliner)
        {
            SaveStateToLayout(state, userLegacyLayout);

            QMessageBox box(AzToolsFramework::GetActiveWindow());
            box.addButton(QMessageBox::Ok);
            box.setWindowTitle(tr("Layout Saved"));
            box.setText(tr("Your layout has been automatically updated for the new Component-Entity workflows. Your old layout has been saved as \"%1\" and can be restored from the View -> Layouts menu.").arg(userLegacyLayout));
            box.exec();

            return false;
        }
    }

    if (!ClosePanesWithRollback(state.viewPanes))
    {
        return false;
    }

    // Store off currently selected entities
    AzToolsFramework::EntityIdList selectedEntityIds;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    // Clear any selection
    AzToolsFramework::EntityIdList noEntities;
    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, noEntities);

    m_fakeDockWidgetGeometries = state.fakeDockWidgetGeometries;

    for (const QString& paneName : state.viewPanes)
    {
        OpenPane(paneName, QtViewPane::OpenMode::OnlyOpen);
    }

    // must do this after opening all of the panes!
    m_advancedDockManager->restoreState(state.mainWindowState);

    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, selectedEntityIds);

    // In case of a crash it might happen that the QMainWindow state gets out of sync with the
    // QtViewPaneManager state, which would result in we opening dock widgets that QMainWindow
    // didn't know how to restore.
    // Check if that happened and return false indicating the restore failed and giving caller
    // a chance to restore the default layout.
    if (AzToolsFramework::DockWidgetUtils::hasInvalidDockWidgets(m_mainWindow))
    {
        return false;
    }

    return true;
}

bool QtViewPaneManager::RestoreLayout(const ViewLayoutState& state)
{
    m_fakeDockWidgetGeometries = state.fakeDockWidgetGeometries;

    if (!ClosePanesWithRollback(state.viewPanes))
    {
        return false;
    }

    for (const QString& paneName : state.viewPanes)
    {
        OpenPane(paneName, QtViewPane::OpenMode::OnlyOpen);
    }

    // must do this after opening all of the panes!
    m_advancedDockManager->restoreState(state.mainWindowState);

    return true;
}


void QtViewPaneManager::RenameLayout(QString name, QString newName)
{
    name = name.trimmed();
    newName = newName.trimmed();
    if (name == newName || newName.isEmpty() || name.isEmpty())
    {
        return;
    }

    {
        AzQtComponents::AutoSettingsGroup settingsGroupGuard(m_settings, GetFancyViewPaneStateGroupName());

        m_settings->setValue(newName, m_settings->value(name));
        m_settings->remove(name);
    }

    m_settings->sync();
    emit savedLayoutsChanged();
}

void QtViewPaneManager::RemoveLayout(QString layoutName)
{
    layoutName = layoutName.trimmed();
    if (layoutName.isEmpty())
    {
        return;
    }

    {
        AzQtComponents::AutoSettingsGroup settingsGroupGuard(m_settings, GetFancyViewPaneStateGroupName());
        m_settings->remove(layoutName.trimmed());
    }

    m_settings->sync();
    emit savedLayoutsChanged();
}

bool QtViewPaneManager::HasLayout(const QString& name) const
{
    return LayoutNames().contains(name.trimmed(), Qt::CaseInsensitive);
}

QStringList QtViewPaneManager::LayoutNames(bool userLayoutsOnly) const
{
    QStringList layouts;

    AzQtComponents::AutoSettingsGroup settingsGroupGuard(m_settings, GetFancyViewPaneStateGroupName());
    layouts = m_settings->childKeys();

    if (userLayoutsOnly)
    {
        layouts.removeOne(s_lastLayoutName); // "last" is internal
    }
    return layouts;
}

QtViewPanes QtViewPaneManager::GetRegisteredPanes(bool viewPaneMenuOnly) const
{
    if (!viewPaneMenuOnly)
    {
        return m_registeredPanes;
    }

    QtViewPanes panes;
    panes.reserve(30); // approximate
    std::copy_if(m_registeredPanes.cbegin(), m_registeredPanes.cend(), std::back_inserter(panes), [](QtViewPane pane)
        {
            return pane.m_options.showInMenu;
        });

    return panes;
}

QtViewPanes QtViewPaneManager::GetRegisteredMultiInstancePanes(bool viewPaneMenuOnly) const
{
    QtViewPanes panes;
    panes.reserve(30); // approximate

    if (viewPaneMenuOnly)
    {
        std::copy_if(m_registeredPanes.cbegin(), m_registeredPanes.cend(), std::back_inserter(panes), [](QtViewPane pane)
            {
                return pane.m_options.showInMenu && pane.m_options.canHaveMultipleInstances;
            });
    }
    else
    {
        std::copy_if(m_registeredPanes.cbegin(), m_registeredPanes.cend(), std::back_inserter(panes), [](QtViewPane pane)
            {
                return pane.m_options.canHaveMultipleInstances;
            });
    }

    return panes;
}

QtViewPanes QtViewPaneManager::GetRegisteredViewportPanes() const
{
    QtViewPanes viewportPanes;
    viewportPanes.reserve(5); // approximate
    std::copy_if(m_registeredPanes.cbegin(), m_registeredPanes.cend(), std::back_inserter(viewportPanes), [](QtViewPane pane)
        {
            return pane.IsViewportPane();
        });

    return viewportPanes;
}

int QtViewPaneManager::NextAvailableId()
{
    for (int candidate = ID_VIEW_OPENPANE_FIRST; candidate <= ID_VIEW_OPENPANE_LAST; ++candidate)
    {
        if (!m_knownIdsSet.contains(candidate))
        {
            m_knownIdsSet.push_back(candidate);
            return candidate;
        }
    }

    return -1;
}

QtViewPane* QtViewPaneManager::GetPane(int id)
{
    auto it = std::find_if(m_registeredPanes.begin(), m_registeredPanes.end(),
            [id](const QtViewPane& pane) { return id == pane.m_id; });

    return it == m_registeredPanes.end() ? nullptr : it;
}

QtViewPane* QtViewPaneManager::GetPane(const QString& name)
{
    auto it = std::find_if(m_registeredPanes.begin(), m_registeredPanes.end(),
            [name](const QtViewPane& pane) { return name == pane.m_name; });

    QtViewPane* foundPane = ((it == m_registeredPanes.end()) ? nullptr : it);

    if (foundPane == nullptr)
    {
        // if we couldn't find the pane based on the name (which will be the title), look it up by saveKeyName next
        it = std::find_if(m_registeredPanes.begin(), m_registeredPanes.end(),
            [name](const QtViewPane& pane) { return name == pane.m_options.saveKeyName; });

        foundPane = ((it == m_registeredPanes.end()) ? nullptr : it);
    }

    return foundPane;
}

QtViewPane* QtViewPaneManager::GetViewportPane(int viewportType)
{
    auto it = std::find_if(m_registeredPanes.begin(), m_registeredPanes.end(),
            [viewportType](const QtViewPane& pane) { return viewportType == pane.m_options.viewportType; });

    return it == m_registeredPanes.end() ? nullptr : it;
}

QDockWidget* QtViewPaneManager::GetView(const QString& name)
{
    QtViewPane* pane = GetPane(name);
    return pane ? pane->m_dockWidget : nullptr;
}

bool QtViewPaneManager::IsVisible(const QString& name)
{
    QtViewPane* view = GetPane(name);
    return view && view->IsVisible();
}

bool QtViewPaneManager::IsPaneRegistered(const QString& name) const
{
    auto it = std::find_if(m_registeredPanes.begin(), m_registeredPanes.end(),
        [name](const QtViewPane& pane) { return name == pane.m_name; });

    return it != m_registeredPanes.end();
}

#include <moc_QtViewPaneManager.cpp>
