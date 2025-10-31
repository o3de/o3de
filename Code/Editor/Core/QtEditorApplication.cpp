
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "QtEditorApplication.h"

// Qt
#include <QAbstractEventDispatcher>
#include <QScopedValueRollback>
#include <QToolBar>
#include <QLoggingCategory>


#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

// AzQtComponents
#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

// Editor
#include "Settings.h"
#include "CryEdit.h"

Q_LOGGING_CATEGORY(InputDebugging, "o3de.editor.input")

// internal, private namespace:
namespace
{
    class EditorGlobalEventFilter
        : public AzQtComponents::GlobalEventFilter
    {
    public:
        explicit EditorGlobalEventFilter(QObject* watch)
            : AzQtComponents::GlobalEventFilter(watch) {}

        bool eventFilter(QObject* obj, QEvent* e) override
        {
            static bool isRecursing = false;

            if (isRecursing)
            {
                return false;
            }

            QScopedValueRollback<bool> guard(isRecursing, true);

            // Detect Widget move
            // We're doing this before the events are actually consumed to avoid confusion
            if (IsDragGuardedWidget(obj))
            {
                switch (e->type())
                {
                    case QEvent::MouseButtonPress:
                    {
                        m_widgetDraggedState = WidgetDraggedState::Clicked;
                        break;
                    }
                    case QEvent::Move:
                    case QEvent::MouseMove:
                    {
                        if (m_widgetDraggedState == WidgetDraggedState::Clicked)
                        {
                            m_widgetDraggedState = WidgetDraggedState::Dragged;
                        }
                        break;
                    }
                }
            }

            if (e->type() == QEvent::MouseButtonRelease)
            {
                m_widgetDraggedState = WidgetDraggedState::None;
            }

            switch (e->type())
            {
                case QEvent::KeyPress:
                case QEvent::KeyRelease:
                {
                    if (GetIEditor()->IsInGameMode())
                    {
                        // don't let certain keys fall through to the game when it's running
                        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
                        auto key = keyEvent->key();

                        if ((key == Qt::Key_Alt) || (key == Qt::Key_AltGr) || ((key >= Qt::Key_F1) && (key <= Qt::Key_F35)))
                        {
                            return true;
                        }
                    }
                }
                break;

                case QEvent::Shortcut:
                {
                    // Eat shortcuts in game mode or when a guarded widget is being dragged
                    if (GetIEditor()->IsInGameMode() || m_widgetDraggedState == WidgetDraggedState::Dragged)
                    {
                        return true;
                    }
                }
                break;

                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                case QEvent::MouseButtonDblClick:
                case QEvent::MouseMove:
                {
#if AZ_TRAIT_OS_PLATFORM_APPLE
                    auto widget = qobject_cast<QWidget*>(obj);
                    if (widget && widget->graphicsProxyWidget() != nullptr)
                    {
                        QMouseEvent* me = static_cast<QMouseEvent*>(e);
                        QWidget* target = qApp->widgetAt(QCursor::pos());
                        if (target)
                        {
                            QMouseEvent ev(me->type(), target->mapFromGlobal(QCursor::pos()), me->button(), me->buttons(), me->modifiers());
                            qApp->notify(target, &ev);
                            return true;
                        }
                    }
#endif
                    GuardMouseEventSelectionChangeMetrics(e);
                }
                break;
            }

            return GlobalEventFilter::eventFilter(obj, e);
        }

    private:
        bool m_mouseButtonWasDown = false;

        void GuardMouseEventSelectionChangeMetrics(QEvent* e)
        {
            // Force the metrics collector to queue up any selection changed metrics until mouse release, so that we don't
            // get flooded with multiple selection changed events when one, sent on mouse release, is enough.
            if (e->type() == QEvent::MouseButtonPress)
            {
                if (!m_mouseButtonWasDown)
                {
                    m_mouseButtonWasDown = true;
                }
            }
            else if (e->type() == QEvent::MouseButtonRelease)
            {
                // This is a tricky case. We don't want to send the end selection change event too early
                // because there might be other things responding to the mouse release after this, and we want to
                // block handling of the selection change events until we're entirely finished with the mouse press.
                // So, queue the handling with a single shot timer, but then check the state of the mouse buttons
                // to ensure that they haven't been pressed in between the release and the timer firing off.
                QTimer::singleShot(0, this, [this]() {
                    if (!QApplication::mouseButtons() && m_mouseButtonWasDown)
                    {
                        m_mouseButtonWasDown = false;
                    }
                });
            }
        }

        //! Detect if the event's target is a Widget we want to guard from shortcuts while it's being dragged.
        //! This function can be easily expanded to handle exceptions.
        bool IsDragGuardedWidget(const QObject* obj)
        {
            return qobject_cast<const QWidget*>(obj) != nullptr;
        }

        //! Enum to keep track of Widget dragged state
        enum class WidgetDraggedState
        {
            None,       //!< No widget is being clicked nor dragged
            Clicked,    //!< A widget has been clicked on but has not been dragged
            Dragged,    //!< A widget is being dragged
        };

        WidgetDraggedState m_widgetDraggedState = WidgetDraggedState::None;
    };

    static void LogToDebug([[maybe_unused]] QtMsgType Type, [[maybe_unused]] const QMessageLogContext& Context, const QString& message)
    {
        AZ::Debug::Platform::OutputToDebugger("Qt", message.toUtf8().data());
        AZ::Debug::Platform::OutputToDebugger("", "\n");
    }
}

namespace Editor
{
    void ScanDirectories(QFileInfoList& directoryList, const QStringList& filters, QFileInfoList& files, ScanDirectoriesUpdateCallBack updateCallback)
    {
        while (!directoryList.isEmpty())
        {
            QDir directory(directoryList.front().absoluteFilePath(), "*", QDir::Name | QDir::IgnoreCase, QDir::AllEntries);
            directoryList.pop_front();

            if (directory.exists())
            {
                // Append each file from this directory that matches one of the filters to files
                directory.setNameFilters(filters);
                directory.setFilter(QDir::Files);
                files.append(directory.entryInfoList());

                // Add all of the subdirectories from this directory to the queue to be searched
                directory.setNameFilters(QStringList("*"));
                directory.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
                directoryList.append(directory.entryInfoList());
                if (updateCallback)
                {
                    updateCallback();
                }
            }
        }
    }

    EditorQtApplication::EditorQtApplication(int& argc, char** argv)
        : AzQtApplication(argc, argv)
        , m_stylesheet(new AzQtComponents::O3DEStylesheet(this))
    {
        setWindowIcon(QIcon(":/Application/res/o3de_editor.ico"));

        // set the default key store for our preferences:
        setApplicationName("O3DE Editor");

        installEventFilter(this);

        // Disable our debugging input helpers by default
        QLoggingCategory::setFilterRules(QStringLiteral("o3de.editor.input.*=false"));

        // Initialize our stylesheet here to allow Gems to register stylesheets when their system components activate.
        AZ::IO::FixedMaxPath engineRootPath;
        {
            using namespace AZ::SettingsRegistryMergeUtils;
            AZ::SettingsRegistryImpl settingsRegistry;
            AZ::CommandLine commandLine;
            commandLine.Parse(argc, argv);

            ParseCommandLine(commandLine);
            StoreCommandLineToRegistry(settingsRegistry, commandLine);
            MergeSettingsToRegistry_CommandLine(settingsRegistry, commandLine, {});
            MergeSettingsToRegistry_AddRuntimeFilePaths(settingsRegistry);

            settingsRegistry.Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }
        m_stylesheet->initialize(this, engineRootPath);
    }

    void EditorQtApplication::Initialize()
    {
        GetIEditor()->RegisterNotifyListener(this);

        // install QTranslator
        InstallEditorTranslators();

        // install hooks and filters last and revoke them first
        InstallFilters();

        // install this filter. It will be a parent of the application and cleaned up when it is cleaned up automically
        auto globalEventFilter = new EditorGlobalEventFilter(this);
        installEventFilter(globalEventFilter);
    }

    void EditorQtApplication::LoadSettings()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(context, "No serialize context");
        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/EditorUserSettings.xml", resolvedPath, AZ_MAX_PATH_LEN);
        m_localUserSettings.Load(resolvedPath, context);
        m_localUserSettings.Activate(AZ::UserSettings::CT_LOCAL);
        AZ::UserSettingsOwnerRequestBus::Handler::BusConnect(AZ::UserSettings::CT_LOCAL);
        m_activatedLocalUserSettings = true;
    }

    void EditorQtApplication::UnloadSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            SaveSettings();
            m_localUserSettings.Deactivate();
            AZ::UserSettingsOwnerRequestBus::Handler::BusDisconnect();
            m_activatedLocalUserSettings = false;
        }
    }

    void EditorQtApplication::SaveSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            char resolvedPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/EditorUserSettings.xml", resolvedPath, AZ_ARRAY_SIZE(resolvedPath));
            m_localUserSettings.Save(resolvedPath, context);
        }
    }

    void EditorQtApplication::maybeProcessIdle()
    {
        if (!m_isMovingOrResizing)
        {
            if (auto winapp = CCryEditApp::instance())
            {
                winapp->OnIdle(0);
            }
        }
        if (m_applicationActive)
        {
            QTimer::singleShot(1, this, &EditorQtApplication::maybeProcessIdle);
        }
    }

    void EditorQtApplication::InstallQtLogHandler()
    {
        qInstallMessageHandler(LogToDebug);
    }

    void EditorQtApplication::InstallFilters()
    {
        if (auto dispatcher = QAbstractEventDispatcher::instance())
        {
            dispatcher->installNativeEventFilter(this);
        }
    }

    void EditorQtApplication::UninstallFilters()
    {
        if (auto dispatcher = QAbstractEventDispatcher::instance())
        {
            dispatcher->removeNativeEventFilter(this);
        }
    }

    EditorQtApplication::~EditorQtApplication()
    {
        if (GetIEditor())
        {
            GetIEditor()->UnregisterNotifyListener(this);
        }

        UninstallFilters();

        UninstallEditorTranslators();
    }

    EditorQtApplication* EditorQtApplication::instance()
    {
        return static_cast<EditorQtApplication*>(QApplication::instance());
    }

    void EditorQtApplication::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        switch (event)
        {
            case eNotify_OnStyleChanged:
                RefreshStyleSheet();
                emit skinChanged();
            break;

            case eNotify_OnQuit:
                GetIEditor()->UnregisterNotifyListener(this);
            break;
        }
    }

    QColor EditorQtApplication::InterpolateColors(QColor a, QColor b, float factor)
    {
        return QColor(int(a.red() * (1.0f - factor) + b.red() * factor),
            int(a.green() * (1.0f - factor) + b.green() * factor),
            int(a.blue() * (1.0f - factor) + b.blue() * factor),
            int(a.alpha() * (1.0f - factor) + b.alpha() * factor));
    }

    void EditorQtApplication::RefreshStyleSheet()
    {
        m_stylesheet->Refresh();
    }

    void EditorQtApplication::setIsMovingOrResizing(bool isMovingOrResizing)
    {
        if (m_isMovingOrResizing == isMovingOrResizing)
        {
            return;
        }

        m_isMovingOrResizing = isMovingOrResizing;
    }

    bool EditorQtApplication::isMovingOrResizing() const
    {
        return m_isMovingOrResizing;
    }

    const QColor& EditorQtApplication::GetColorByName(const QString& name)
    {
        return m_stylesheet->GetColorByName(name);
    }

    bool EditorQtApplication::IsActive()
    {
        return applicationState() == Qt::ApplicationActive;
    }

    QTranslator* EditorQtApplication::CreateAndInitializeTranslator(const QString& filename, const QString& directory)
    {
        Q_ASSERT(QFile::exists(directory + "/" + filename));

        QTranslator* translator = new QTranslator();
        translator->load(filename, directory);
        installTranslator(translator);
        return translator;
    }

    void EditorQtApplication::InstallEditorTranslators()
    {
        m_editorTranslator =        CreateAndInitializeTranslator("editor_en-us.qm", ":/Translations");
        m_assetBrowserTranslator =  CreateAndInitializeTranslator("assetbrowser_en-us.qm", ":/Translations");
    }

    void EditorQtApplication::DeleteTranslator(QTranslator*& translator)
    {
        removeTranslator(translator);
        delete translator;
        translator = nullptr;
    }

    void EditorQtApplication::UninstallEditorTranslators()
    {
        DeleteTranslator(m_editorTranslator);
        DeleteTranslator(m_assetBrowserTranslator);
    }

    void EditorQtApplication::EnableOnIdle(bool enable)
    {
        m_applicationActive = enable;
        if (enable)
        {
            QTimer::singleShot(0, this, &EditorQtApplication::maybeProcessIdle);
        }
    }

    bool EditorQtApplication::OnIdleEnabled() const
    {
        return m_applicationActive;
    }

    bool EditorQtApplication::eventFilter(QObject* object, QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
            m_pressedButtons |= reinterpret_cast<QMouseEvent*>(event)->button();
            break;
        case QEvent::MouseButtonRelease:
            m_pressedButtons &= ~(reinterpret_cast<QMouseEvent*>(event)->button());
            break;
        case QEvent::KeyPress:
            m_pressedKeys.insert(reinterpret_cast<QKeyEvent*>(event)->key());
            break;
        case QEvent::KeyRelease:
            m_pressedKeys.remove(reinterpret_cast<QKeyEvent*>(event)->key());
            break;
        default:
            break;
        }
        return QApplication::eventFilter(object, event);
    }
} // end namespace Editor

#include <Core/moc_QtEditorApplication.cpp>
