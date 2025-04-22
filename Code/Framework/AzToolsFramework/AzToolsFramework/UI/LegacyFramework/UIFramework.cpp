/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UIFramework.hxx"

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Settings/CommandLine.h>

#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>
#include "MainWindowSavedState.h"

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // '...' needs to have dll-interface to be used by clients of class '...'
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QThread>
#include <QAction>
#include <QMenu>
#include <QFontDatabase>
#include <QResource>
#include <QProxyStyle>
AZ_POP_DISABLE_WARNING


#ifndef AZ_PLATFORM_WINDOWS
int __argc = 0;
char** __argv = nullptr;
#endif

#if AZ_TRAIT_OS_PLATFORM_APPLE
#include <mach-o/dyld.h>
#endif

//#include "PanelData.h"

// the UI Framework is the entry point into the UI side of things
// the framework can run eventually without a UI, so we have to split it up in that manner.
namespace AzToolsFramework
{
    // this ticker exists because qt supresses all timer events during modal dialogs, and we want our tickbus to tick anyway.
    QTickBusTicker::QTickBusTicker()
    {
        m_cancelled = false;
        m_processing = false;
    }
    QTickBusTicker* QTickBusTicker::SpinUp()
    {
        QTickBusTicker* worker = azcreate(QTickBusTicker, ());
        worker->m_ptrThread = azcreate(QThread, ());
        worker->moveToThread(worker->m_ptrThread);
        QApplication::connect(worker->m_ptrThread, SIGNAL(started()), worker, SLOT(process()));
        worker->m_ptrThread->start();

        return worker;
    }

    void QTickBusTicker::process()
    {
        m_processing = true;
        while (!m_cancelled)
        {
            QThread::currentThread()->msleep(10);
            emit doTick();
        }
        m_processing = false;
    }

    void QTickBusTicker::cancel()
    {
        m_cancelled = true;
        m_ptrThread->quit();
        while ((m_processing) || (m_ptrThread->isRunning()))
        {
            QThread::currentThread()->msleep(1);
        }

        azdestroy(m_ptrThread);
    }

    // the Qt Application extends QApplication and handles DDE messages and atomic registration:

    void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
    {
        (void)context;

        QByteArray localMsg = msg.toLocal8Bit();
        switch (type)
        {
        case QtDebugMsg:
            AZ_TracePrintf("Qt-Debug", "Qt-Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtWarningMsg:
            // this is irritating. They will fix it in qt next.
            if (!msg.startsWith("Cannot create accessible"))
            {
                AZ_TracePrintf("Qt-Debug", "Qt-Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            }
            break;
        case QtCriticalMsg:
            AZ_Warning("Qt-Error", false, "Qt-Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            AZ_Error("Qt-Fatal", false, "Qt-Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            abort();
        }
    }

    class AZQtApplicationStyle : public QProxyStyle
    {
    public:
        int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
        {
            if (hint == QStyle::SH_TabBar_Alignment)
            {
                return Qt::AlignLeft;
            }
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    };

    class AZQtApplication
        : public QApplication
    {
    public:
        AZ_CLASS_ALLOCATOR(AZQtApplication, AZ::SystemAllocator);
        AZQtApplication(int& argc, char** argv)
            : QApplication(argc, argv)
        {
            setStyle(new AZQtApplicationStyle);
            qInstallMessageHandler(myMessageOutput);
        }

        ~AZQtApplication() override
        {
            qInstallMessageHandler(nullptr);
        }
    };

    void Framework::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Framework, AZ::Component>()
                ->Version(1)
            ;

            MainWindowSavedState::Reflect(serialize);
            QWidgetSavedState::Reflect(serialize);
        }
    }

    Framework::Framework()
    {
        pApplication = nullptr;
        m_bTicking = false;
        m_ActionPreferences = nullptr;
        m_ActionQuit = nullptr;
        m_ActionChangeProject = nullptr;
    }

    void Framework::RunAsAnotherInstance()
    {
    }


    // this is the entry point for the 'GUI' part of the application
    // this function blocks until the GUI is exit.
    // if you want to run headlessly, do not call this function.
    void Framework::Run()
    {
        pApplication->setOrganizationName("O3DE");
        pApplication->setApplicationName("O3DE Editor");

        bool GUIMode = true;
        LegacyFramework::FrameworkApplicationMessages::Bus::BroadcastResult(GUIMode,
            &LegacyFramework::FrameworkApplicationMessages::Bus::Events::IsRunningInGUIMode);

        // if we're not in GUI Mode why bother reigstering fonts and style sheets?
        if (GUIMode)
        {
            // add the style sheets folder to the search path, if it exists:
            QDir styleSheetPath(QApplication::applicationDirPath());
            if (styleSheetPath.cd("StyleSheets"))
            {
                QDir::addSearchPath("UI", styleSheetPath.absolutePath()); // add it as the "UI" prefix (just like the other stylesheet images.)
            }
            // enable the built-in stylesheet by default:
            bool enableStyleSheet = true;

            const AZ::CommandLine* comp = nullptr;
            AZ::ComponentApplicationBus::Broadcast([&comp](AZ::ComponentApplicationRequests* requests)
                {
                    comp = requests->GetAzCommandLine();
                });
            if (comp != nullptr)
            {
                if (comp->HasSwitch("nostyle"))
                {
                    enableStyleSheet = false;
                }

                // if you specify the "forcestyle" option, then it will use the stylesheet you choose, in the stylesheets folder.
                if (comp->HasSwitch("forcestyle"))
                {
                    AZStd::string switchValue = comp->GetSwitchValue("forcestyle", 0);
                    QFile cssFile(QString("UI:%1.css").arg(switchValue.c_str()));
                    if (cssFile.exists())
                    {
                        enableStyleSheet = false; // dont use the built-in style sheet!
                        cssFile.open(QFile::ReadOnly);
                        QString styleSheet = QLatin1String(cssFile.readAll());
                        pApplication->setStyleSheet(styleSheet);
                    }
                }
            }

            if ((pApplication->styleSheet().isEmpty()) && (enableStyleSheet))
            {
                QDir::addSearchPath("UI", ":/StyleSheetImages");
                QFile file(":/styles/style_dark.qss");
                bool success = file.open(QFile::ReadOnly);

                if (success)
                {
                    QString errorString = file.errorString();
                    AZ_TracePrintf("UIFramework", "\nError: %s\n", errorString.toStdString().c_str());

                    QString styleSheet = QLatin1String(file.readAll());
                    pApplication->setStyleSheet(styleSheet);
                }
                else
                {
                    QString errorString = file.errorString();
                    AZ_Error("UIFramework", true, "Error Loading StyleSheet: %s", errorString.toStdString().c_str());
                }
            }
        }

        // start ticking bus.
        m_ptrTicker = QTickBusTicker::SpinUp();

        connect(m_ptrTicker, SIGNAL(doTick()), this, SLOT(performBusTick()));

        QTimer::singleShot(0, this, SLOT(BootStrapRemainingSystems()));

        // register global hotkeys:
        FrameworkMessages::Bus::Broadcast(
            &FrameworkMessages::Bus::Events::RegisterHotkey,
            AzToolsFramework::HotkeyDescription(
                AZ_CRC_CE("GeneralOpenAssetBrowser"),
                "Alt+Shift+O",
                "Open Asset Browser",
                "General",
                1,
                HotkeyDescription::SCOPE_WINDOW));

        // run our message loop.  for now, we'll use a timer to do polling.
        // we can always change that to a zero-timed timer which always tick, and sleep ourselves if we want to manage the event loop ourself...


        // the following is a BLOCKING message which 'runs' the application's main event loop:
        // exec will automatically continue going until the quit() signal is received
        // closing the very last 'main' window will also issue the quit() signal.

        pApplication->installEventFilter(this);

        pApplication->exec();

        if (m_ptrTicker)
        {
            // if the ticker is alive it means we failed to properly perform the "UserWantsToQuit" sequence.
            // see still need to clean up:
            m_ptrTicker->cancel();
            QApplication::processEvents();
            AZ::ComponentApplication* pApp = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(pApp, &AZ::ComponentApplicationBus::Events::GetApplication);
            if (pApp)
            {
                pApp->Tick();
            }
            azdestroy(m_ptrTicker);
            m_ptrTicker = nullptr;
        }
    }

    Framework::~Framework()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        delete m_ActionPreferences;
        m_ActionPreferences = nullptr;

        delete m_ActionQuit;
        m_ActionQuit = nullptr;

        delete m_ActionChangeProject;
        m_ActionChangeProject = nullptr;

        pApplication = nullptr;
    }

    // once we set the project, we can then tell all our other windows to restore our state.
    void Framework::OnProjectSet(const char* /*projectPath*/)
    {
        QTimer::singleShot(0, this, SLOT(BootStrapRemainingSystems()));
    }

    void Framework::BootStrapRemainingSystems()
    {
        LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::OnRestoreState);
        LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::OnReady);
    }

    void Framework::OnSystemTick()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        CheckForReadyToQuit();
    }

    void Framework::Init()
    {
        pApplication = aznew AZQtApplication(__argc, __argv);
    }

    void Framework::Activate()
    {
        FrameworkMessages::Handler::BusConnect();
        LegacyFramework::CoreMessageBus::Handler::BusConnect();
    }

    void Framework::Deactivate()
    {
        LegacyFramework::CoreMessageBus::Handler::BusDisconnect();
        FrameworkMessages::Handler::BusDisconnect();
    }

    bool Framework::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::ApplicationDeactivate)
        {
            LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::ApplicationDeactivated);
        }
        else if (event->type() == QEvent::ApplicationActivate)
        {
            LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::ApplicationActivated);
        }
        return QObject::eventFilter(obj, event); // Unhandled events are passed to the base class
    }

    void Framework::performBusTick()
    {
        // TEMPORARY: Until we move all modal dialogs in a new message box queue, where we don't execute them in the middle of a tick!
        //AZ_Assert(!m_bTicking, "You may not insert events into the event queue which would cause a tick to occur while a tick is already occuring.  If you're in the GUI, you should be using QT to schedule QT events and Qt Timers, not the TICKBUS");
        if (m_bTicking)
        {
            return; // prevent re-entry, in case someone calls Qt Process Events while in a Qt function!
        }
        m_bTicking = true;
        // Tick the component app.
        AZ::ComponentApplication* pApp = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(pApp, &AZ::ComponentApplicationBus::Events::GetApplication);
        if (pApp && m_ptrTicker)
        {
            AZ::SystemTickBus::ExecuteQueuedEvents();
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickEvents::OnSystemTick);
            pApp->Tick();
        }

        m_bTicking = false;
    }

    // register a hotkey to make a known hotkey that can be modified by the user
    void Framework::RegisterHotkey(const HotkeyDescription& desc)
    {
        // it is acceptable to multi-register the same hotkey.
        HotkeyDescriptorContainerType::pair_iter_bool newInsertion = m_hotkeyDescriptors.insert(desc.m_HotKeyIDCRC);
        if (newInsertion.second)
        {
            newInsertion.first->second = HotkeyData(desc);
        }
    }

    // register an action to belong to a particular registerd hotkey.
    // when you do this, it will automatically change the action to use the new hotkey and also update it when it changes
    void Framework::RegisterActionToHotkey(const AZ::u32 hotkeyID, QAction* pAction)
    {
        HotkeyDescriptorContainerType::iterator finder = m_hotkeyDescriptors.find(hotkeyID);
        AZ_Assert(finder != m_hotkeyDescriptors.end(), "Hotkey not found in register - call RegisterHotkey first!");

        AZ_Assert(m_liveHotkeys.find(pAction) == m_liveHotkeys.end(), "You may not register the same action twice");

        HotkeyData& target = finder->second;
        target.m_actionsBound.insert(pAction);
        m_liveHotkeys.insert(AZStd::make_pair(pAction, hotkeyID));

        pAction->setShortcut(QKeySequence(target.m_desc.m_currentKey.c_str()));

        connect(pAction, SIGNAL(destroyed(QObject*)), this, SLOT(onActionDestroyed(QObject*)));
    }

    void Framework::onActionDestroyed(QObject* pObject)
    {
        LiveHotkeyContainer::iterator finder = m_liveHotkeys.find((QAction*)pObject);
        AZ_Assert(finder != m_liveHotkeys.end(), "Could not find registered action!");

        AZ::u32 hotkeyID = finder->second;
        m_liveHotkeys.erase(finder);

        HotkeyDescriptorContainerType::iterator descFinder = m_hotkeyDescriptors.find(hotkeyID);
        AZ_Assert(descFinder != m_hotkeyDescriptors.end(), "onActionDestroyed - hotkey not found in register - call RegisterHotkey first!");
        HotkeyData& target = descFinder->second;
        target.m_actionsBound.erase((QAction*)pObject);
    }

    // note that you don't HAVE to unregister it.  Qt sends us a message when an action is destroyed.  So just delete the action if you want.
    void Framework::UnregisterActionFromHotkey(QAction* pAction)
    {
        onActionDestroyed(pAction);
    }

    // ebus result aggrecator.  Returns true if nobody is listening
    // otherwise it will only return true if EVERY listener returns true.  It is logical-and
    class Ebus_Event_AllOkay
    {
    private:
        bool currentValue;
    public:
        Ebus_Event_AllOkay() { currentValue = true; }
        void operator=(bool other) { currentValue = currentValue & other; }
        bool Accepted() { return currentValue; }
        void Reset() { currentValue = false; }
    };

    // the user has asked to quit.
    void Framework::UserWantsToQuit()
    {
        QTimer::singleShot(0, this, SLOT(UserWantsToQuitProcess()));
    }

    void Framework::UserWantsToQuitProcess()
    {
        // start the shutdown sequence:
        Ebus_Event_AllOkay check;

        LegacyFramework::CoreMessageBus::BroadcastResult(check, &LegacyFramework::CoreMessageBus::Events::OnGetPermissionToShutDown);
        if (!check.Accepted())
        {
            return;
        }

        // save current project specific and global settings in case shutdown is a crash.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);


        CheckForReadyToQuit();
    }

    void Framework::CheckForReadyToQuit()
    {
        // poll components to determine if its okay for the application to shut down:

        Ebus_Event_AllOkay check;

        LegacyFramework::CoreMessageBus::BroadcastResult(check, &LegacyFramework::CoreMessageBus::Events::CheckOkayToShutDown);
        if (!check.Accepted())
        {
            // the above could cause contexts to generate threaded requests that are outstanding (like a long data save).
            // we keep the app running until those requests have been completed.
            AZ::SystemTickBus::Handler::BusConnect();
            return;
        }

        LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::OnSaveState);
        LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::OnDestroyState);

        // we successfully got permission to quit!
        // pump the tickbus one last time!
       // QApplication::processEvents();
        AZ::ComponentApplication* pApp = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(pApp, &AZ::ComponentApplicationBus::Events::GetApplication);
        if (pApp)
        {
            pApp->Tick();
        }

        m_ptrTicker->cancel();

        azdestroy(m_ptrTicker);
        m_ptrTicker = nullptr;

        QApplication::quit();
    }

    void Framework::AddComponentInfo(MainWindowDescription& desc)
    {
        m_MainWindowList.push_back(desc);
    }
    void Framework::GetComponentsInfo(AZStd::list<MainWindowDescription>& theComponents)
    {
        theComponents = m_MainWindowList;
    }
    void Framework::ApplicationCensusReply(bool isOpen)
    {
        if (isOpen)
        {
            ++m_ApplicationCensusResults;
        }
    }
    void Framework::RequestMainWindowClose(AZ::Uuid id)
    {
        // trigger a callback accumulator incremented by contexts via ApplicationCensusReply
        // this is not asynchronous
        m_ApplicationCensusResults = 0;
        LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::ApplicationCensus);

        if (m_ApplicationCensusResults > 1)
        {
            // if more than one window is open then simply tell it to close
            LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::ApplicationHide, id);
        }
        else
        {
            // if this is the last main window (context) open then shut down the app
            UserWantsToQuit();
        }

        // else send a reply message telling context ID to close itself
    }

    void Framework::PopulateApplicationMenu(QMenu* theMenu)
    {
        (void)theMenu;

        // Since we quickly pulled the applications into two separate apps to fix up some UX flow
        // this menu doesn't make any sense. Keeping the logic here in case we decide to revert the previous change when a proper solution is attempted
#if 0
        //"Application"
        //  + Open Lua Editor
        //  + Open World Editor
        //  + Open Model Viewer
        //  + Preferences...
        //  ------------------------
        //  + Close Current Window (Alt+F4) (from outside)
        //  -------------------------
        //  + Quit (ALT+Q)

        theMenu->setTitle(LegacyFramework::appName());
        QList<QAction*> aList = theMenu->actions();

        // Lazy instantiate all these actions, apply the copies on subsequent requests
        // It's the QT Way (tm)
        if (m_ComponentWindowsActions.size() == 0)
        {
            AZStd::list<MainWindowDescription>::iterator mwlIter = m_MainWindowList.begin();
            while (mwlIter != m_MainWindowList.end())
            {
                using namespace AzToolsFramework;
                typedef FrameworkMessages::Bus HotkeyBus;

                QAction* a = new QAction(QString("Show ") + (*mwlIter).name.c_str(), this);
                char output[64];
                (*mwlIter).ContextID.ToString(output, AZ_ARRAY_SIZE(output), true, true);
                a->setData(output);
                HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterHotkey, (*mwlIter).hotkeyDesc);
                HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, (*mwlIter).hotkeyDesc.m_HotKeyIDCRC, a);
                m_ComponentWindowsActions.push_back(a);
                ++mwlIter;
            }
        }
        AZStd::list<QAction*>::iterator cwaIter = m_ComponentWindowsActions.begin();
        AZStd::list<MainWindowDescription>::iterator mwlIter = m_MainWindowList.begin();
        while (mwlIter != m_MainWindowList.end() && cwaIter != m_ComponentWindowsActions.end())
        {
            theMenu->insertAction(*aList.begin(), *cwaIter);
            ++mwlIter;
            ++cwaIter;
        }
        connect(theMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnShowWindowTriggered(QAction*)));

        /*
        if (!m_ActionPreferences)
        {
            m_ActionPreferences = new QAction("Preferences", this);
            connect(m_ActionPreferences, SIGNAL(triggered()), this, SLOT(OnMenuPreferences()));
        }
        theMenu->insertAction(*aList.begin(), m_ActionPreferences);
        */

        theMenu->insertSeparator(*aList.begin());
        theMenu->addSeparator();

        if (!m_ActionQuit)
        {
            using namespace AzToolsFramework;
            typedef FrameworkMessages::Bus HotkeyBus;

            m_ActionQuit = new QAction("Quit", this);
            AzToolsFramework::HotkeyDescription hk(AZ_CRC_CE("GlobalQuitStandaloneTools"), "Alt+Q", "Quit", "General", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterHotkey, hk);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, hk.m_HotKeyIDCRC, m_ActionQuit);
            connect(m_ActionQuit, SIGNAL(triggered()), this, SLOT(OnMenuQuit()));
        }
        theMenu->addAction(m_ActionQuit);
#endif
    }

    void Framework::OnMenuPreferences()
    {
    }

    void Framework::OnMenuQuit()
    {
        UserWantsToQuit();
    }

    void Framework::OnShowWindowTriggered(QAction* action)
    {
        QVariant qv = action->data();
        if (qv.isValid())
        {
            AZ::Uuid id(qv.toString().toUtf8().constData());
            LegacyFramework::CoreMessageBus::Broadcast(&LegacyFramework::CoreMessageBus::Events::ApplicationShow, id);
        }
    }
}   // END namespace AzToolsFramework

#include "UI/LegacyFramework/moc_UIFramework.cpp"
