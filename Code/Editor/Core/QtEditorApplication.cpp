
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
#include <QDir>
#include <QScopedValueRollback>
#include <QToolBar>
#include <QLoggingCategory>
#include <QLocale>


#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>

// AzQtComponents
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

// Editor
#include "Settings.h"
#include "CryEdit.h"

#include <AzToolsFramework/Translation/Translation.h>
#include <AzToolsFramework/Translation/TranslationManager.h>

Q_LOGGING_CATEGORY(InputDebugging, "o3de.editor.input")

// internal, private namespace:
namespace
{
    class EditorGlobalEventFilter
        : public QObject
    {
    public:
        explicit EditorGlobalEventFilter(QObject* watch)
            : QObject(watch) {}

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
                            QMouseEvent ev(me->type(),
                                           target->mapFromGlobal(QCursor::pos()),
                                           QCursor::pos(),
                                           me->button(),
                                           me->buttons(),
                                           me->modifiers());
                            qApp->notify(target, &ev);
                            return true;
                        }
                    }
#endif
                    GuardMouseEventSelectionChangeMetrics(e);
                }
                break;
            }

            return false;
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

    void EditorQtApplication::InstallEditorTranslators()
    {
        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
        #endif

        // Load saved language or detect system language
        m_currentLanguage = GetSavedLanguage();
        AZ_Printf("EditorI18n", "Loading translations for language: %s\n",
            m_currentLanguage.c_str());

        // Load Qt's base translations FIRST (for standard button text: OK, Cancel, Save, Discard, Yes, No, etc.)
        // Qt standard buttons get their text from Qt's own translation files (qtbase_*.qm, qt_*.qm).
        // Without loading these, standard buttons like Save/Cancel/OK/Yes/No remain in English.
        QList<QTranslator*> qtBaseTranslators =
            AzToolsFramework::TranslationManager::LoadQtBaseTranslations(m_currentLanguage.c_str(), this);
        for (QTranslator* translator : qtBaseTranslators)
        {
            m_translators.append(translator);
        }

        // Get all registered translator modules
        AZStd::vector<AZStd::string> translatorModules = GetTranslatorModules();

        int loadedCount = 0;

        // Load translators with fallback support
        for (const AZStd::string& moduleName : translatorModules)
        {
            InstallTranslatorWithFallback(moduleName, m_currentLanguage);
            loadedCount++;
        }

        AZ_Printf("EditorI18n", "Translation system initialized: %d/%zu modules processed (+ %d Qt base translators)\n",
            loadedCount, translatorModules.size(), qtBaseTranslators.size());
    }

    void EditorQtApplication::UninstallEditorTranslators()
    {
        for (QTranslator* translator : m_translators)
        {
            if (translator)
            {
                AzToolsFramework::UnloadTranslator(translator);
            }
        }
        m_translators.clear();
    }

    // ========== i18n Helper Methods ==========

    AZStd::vector<AZStd::string> EditorQtApplication::GetTranslatorModules() const
    {
        AZStd::vector<AZStd::string> modules;

        // ========== Auto-discover translation modules from .qm files ==========
        // Instead of maintaining a hardcoded list, scan the translation directory
        // for all available .qm files and extract module names from filenames.
        // Naming convention: <ModuleName>_<language>.qm
        // Example: AtomToolsFramework_zh_CN.qm -> module "AtomToolsFramework"
        //
        // This ensures ALL Gems and modules with generated translations are
        // automatically loaded without manual list maintenance.

        if (!m_currentLanguage.empty() && m_currentLanguage != "en" && m_currentLanguage != "en_US")
        {
            // Build fallback chain to search multiple language directories
            // e.g., for "zh_CN" -> ["zh_CN", "zh", "en_US", "en"]
            AZStd::vector<AZStd::string> languagesToScan = BuildLanguageFallbackChain(m_currentLanguage);

            // Resolve the translations root directory
            AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath;
            const QString baseDirPath = "@engroot@/Assets/Editor/Translations";

            QByteArray baseDirPathUtf8 = baseDirPath.toUtf8();
            auto* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO &&
                fileIO->ResolvePath(baseDirPathUtf8.constData(), resolvedPath.data(), resolvedPath.size()))
            {
                QDir translationsRoot(resolvedPath.data());

                QSet<QString> discoveredModules;

                for (const AZStd::string& lang : languagesToScan)
                {
                    // Skip English directories (source language, no .qm files)
                    if (lang == "en" || lang == "en_US")
                    {
                        continue;
                    }

                    QDir langDir(translationsRoot.filePath(lang.c_str()));
                    if (!langDir.exists())
                    {
                        continue;
                    }

                    // List all .qm files matching the pattern *_<language>.qm
                    const QString qmPattern = QString("*_%1.qm").arg(lang.c_str());
                    const QStringList qmFiles = langDir.entryList(
                        QStringList() << qmPattern,
                        QDir::Files,
                        QDir::Name);

                    // Extract module names from filenames
                    const QString suffix = QString("_%1.qm").arg(lang.c_str());
                    for (const QString& qmFile : qmFiles)
                    {
                        QString moduleName = qmFile;
                        moduleName.chop(suffix.length());

                        if (!moduleName.isEmpty())
                        {
                            discoveredModules.insert(moduleName.toUtf8().constData());
                        }
                    }
                }

                // Convert to sorted vector for deterministic loading order
                modules.reserve(discoveredModules.size());
                for (const auto& module : discoveredModules)
                {
                    modules.push_back(module.toUtf8().constData());
                }
                std::sort(modules.begin(), modules.end());

                AZ_TracePrintf("EditorI18n",
                    "Auto-discovered %zu translation module(s) for language '%s'\n",
                    modules.size(), m_currentLanguage.c_str());
            }
            else
            {
                AZ_Warning("EditorI18n", false,
                    "Could not resolve translations directory: %s",
                    baseDirPathUtf8.constData());
            }
        }

        // ========== Scan Gem-local translation directories ==========
        // Gems can ship their own .qm files under {gem_root}/Editor/Translations/{language}/
        // These are not present in the centralized Assets/Editor/Translations/ directory,
        // so we must discover them separately via VisitActiveGems.
        if (!m_currentLanguage.empty() && m_currentLanguage != "en" && m_currentLanguage != "en_US")
        {
            if (auto* registry = AZ::SettingsRegistry::Get())
            {
                AZStd::vector<AZStd::string> languagesToScan = BuildLanguageFallbackChain(m_currentLanguage);
                QSet<QString> gemDiscoveredModules;

                AZ::SettingsRegistryMergeUtils::VisitActiveGems(*registry,
                    [&](AZStd::string_view /*gemName*/, AZStd::string_view gemPath)
                    {
                        QString qGemPath = QString::fromUtf8(gemPath.data(), static_cast<int>(gemPath.size()));

                        for (const AZStd::string& lang : languagesToScan)
                        {
                            if (lang == "en" || lang == "en_US")
                            {
                                continue;
                            }

                            QDir langDir(qGemPath + QString("/Editor/Translations/%1").arg(lang.c_str()));
                            if (!langDir.exists())
                            {
                                continue;
                            }

                            const QString qmPattern = QString("*_%1.qm").arg(lang.c_str());
                            const QStringList qmFiles = langDir.entryList(
                                QStringList() << qmPattern, QDir::Files);

                            const QString suffix = QString("_%1.qm").arg(lang.c_str());
                            for (const QString& qmFile : qmFiles)
                            {
                                QString moduleName = qmFile;
                                moduleName.chop(suffix.length());
                                if (!moduleName.isEmpty())
                                {
                                    gemDiscoveredModules.insert(moduleName);
                                }
                            }
                        }
                    });

                int gemModuleCount = 0;
                for (const QString& mod : gemDiscoveredModules)
                {
                    AZStd::string modStr(mod.toUtf8().constData());
                    if (AZStd::find(modules.begin(), modules.end(), modStr) == modules.end())
                    {
                        modules.push_back(modStr);
                        gemModuleCount++;
                    }
                }
                std::sort(modules.begin(), modules.end());

                if (gemModuleCount > 0)
                {
                    AZ_TracePrintf("EditorI18n",
                        "Discovered %d additional translation module(s) from active Gems\n",
                        gemModuleCount);
                }
            }
        }

        // ========== Dynamically Registered Modules ==========
        // Add any modules registered at runtime from Gems
        for (const AZStd::string& registeredModule : m_registeredModules)
        {
            if (AZStd::find(modules.begin(), modules.end(), registeredModule) == modules.end())
            {
                modules.push_back(registeredModule);
            }
        }

        return modules;
    }

    AZStd::string EditorQtApplication::GetSavedLanguage() const
    {
        // Use TranslationManager for cross-process language synchronization
        AZStd::string language = AzToolsFramework::TranslationManager::GetCurrentLanguage();

        AZ_TracePrintf("EditorI18n", "Loaded language from TranslationManager: %s\n",
            language.c_str());

        return language;
    }

    bool EditorQtApplication::SaveLanguage(const AZStd::string& language)
    {
        // Use TranslationManager for cross-process language synchronization
        bool success = AzToolsFramework::TranslationManager::SetLanguage(language.c_str());

        if (success)
        {
            AZ_TracePrintf("EditorI18n", "Saved language preference via TranslationManager: %s\n",
                language.c_str());
        }
        else
        {
            AZ_Warning("EditorI18n", false, "Failed to save language preference via TranslationManager: %s",
                language.c_str());
        }

        return success;
    }

    AZStd::vector<AZStd::string> EditorQtApplication::BuildLanguageFallbackChain(const AZStd::string& language) const
    {
        AZStd::vector<AZStd::string> fallbackChain;

        // First: exact match (e.g., "zh_CN")
        fallbackChain.push_back(language);

        // Second: language without region (e.g., "zh")
        size_t underscorePos = language.find('_');
        if (underscorePos != AZStd::string::npos)
        {
            AZStd::string baseLanguage = language.substr(0, underscorePos);
            if (AZStd::find(fallbackChain.begin(), fallbackChain.end(), baseLanguage) == fallbackChain.end())
            {
                fallbackChain.push_back(baseLanguage);
            }
        }

        // Third: English as universal fallback
        if (language != "en" && language != "en_US")
        {
            if (AZStd::find(fallbackChain.begin(), fallbackChain.end(), AZStd::string("en_US")) == fallbackChain.end())
            {
                fallbackChain.push_back("en_US");
            }
            if (AZStd::find(fallbackChain.begin(), fallbackChain.end(), AZStd::string("en")) == fallbackChain.end())
            {
                fallbackChain.push_back("en");
            }
        }

        return fallbackChain;
    }

    void EditorQtApplication::InstallTranslatorWithFallback(const AZStd::string& moduleName, const AZStd::string& language)
    {
        AZStd::vector<AZStd::string> fallbackChain = BuildLanguageFallbackChain(language);

        // Build fallback chain string for logging
        AZStd::string fallbackChainStr;
        for (size_t i = 0; i < fallbackChain.size(); ++i)
        {
            if (i > 0)
            {
                fallbackChainStr += " -> ";
            }
            fallbackChainStr += fallbackChain[i];
        }

        AZ_TracePrintf("EditorI18n", "Loading translator '%s' with fallback chain: %s\n",
            moduleName.c_str(),
            fallbackChainStr.c_str());

        // Try each language in the fallback chain
        // Note: We use AzToolsFramework::CreateAndLoadTranslatorForLanguage here for Editor-specific needs
        // Standalone tools can use TranslationManager::LoadModuleTranslatorForLanguage directly
        for (const AZStd::string& fallbackLang : fallbackChain)
        {
            QTranslator* translator = AzToolsFramework::CreateAndLoadTranslatorForLanguage(
                QString::fromUtf8(moduleName.c_str()),
                QString::fromUtf8(fallbackLang.c_str()));

            if (translator)
            {
                m_translators.append(translator);
                AZ_TracePrintf("EditorI18n", "  [OK] Loaded '%s' for language '%s'\n",
                    moduleName.c_str(),
                    fallbackLang.c_str());
                return; // Success, stop trying
            }
        }

        // If all fallbacks failed, it's OK for English (source language)
        if (language == "en" || language == "en_US")
        {
            AZ_TracePrintf("EditorI18n", "  [INFO] '%s' using source language (no translation needed)\n",
                moduleName.c_str());
        }
        else
        {
            // Build comma-separated list of tried languages
            AZStd::string triedLanguages;
            for (size_t i = 0; i < fallbackChain.size(); ++i)
            {
                if (i > 0)
                {
                    triedLanguages += ", ";
                }
                triedLanguages += fallbackChain[i];
            }

            AZ_Warning("EditorI18n", false,
                "  [MISS] No translation found for '%s' (tried: %s)",
                moduleName.c_str(),
                triedLanguages.c_str());
        }
    }

    // ========== Public i18n API ==========

    AZStd::string EditorQtApplication::GetCurrentLanguage() const
    {
        return m_currentLanguage;
    }

    bool EditorQtApplication::SetLanguage(const AZStd::string& languageCode)
    {
        if (languageCode == m_currentLanguage)
        {
            AZ_TracePrintf("EditorI18n", "Language already set to: %s\n",
                languageCode.c_str());
            return true;
        }

        AZ_Printf("EditorI18n", "Changing language from '%s' to '%s'...\n",
            m_currentLanguage.c_str(),
            languageCode.c_str());

        // Unload current translators
        UninstallEditorTranslators();

        // Update current language
        m_currentLanguage = languageCode;

        // Save to settings
        SaveLanguage(languageCode);

        // Reload Qt base translations first (for standard button text)
        QList<QTranslator*> qtBaseTranslators =
            AzToolsFramework::TranslationManager::LoadQtBaseTranslations(m_currentLanguage.c_str(), this);
        for (QTranslator* translator : qtBaseTranslators)
        {
            m_translators.append(translator);
        }

        // Reload translators for new language
        AZStd::vector<AZStd::string> translatorModules = GetTranslatorModules();
        for (const AZStd::string& moduleName : translatorModules)
        {
            InstallTranslatorWithFallback(moduleName, m_currentLanguage);
        }

        // Emit signal to notify UI
        emit languageChanged(languageCode);

        AZ_Printf("EditorI18n", "Language changed successfully to: %s\n",
            languageCode.c_str());

        return true;
    }

    AZStd::vector<AZStd::string> EditorQtApplication::GetAvailableLanguages() const
    {
        // Use TranslationManager's centralized language list
        QStringList languages = AzToolsFramework::TranslationManager::GetAvailableLanguages();
        
        AZStd::vector<AZStd::string> result;
        result.reserve(languages.size());
        for (const QString& lang : languages)
        {
            result.push_back(lang.toUtf8().constData());
        }
        
        return result;
    }

    bool EditorQtApplication::RegisterTranslatorModule(const AZStd::string& moduleName)
    {
        if (AZStd::find(m_registeredModules.begin(), m_registeredModules.end(), moduleName) != m_registeredModules.end())
        {
            AZ_TracePrintf("EditorI18n", "Module '%s' already registered\n",
                moduleName.c_str());
            return true;
        }

        m_registeredModules.push_back(moduleName);

        AZ_Printf("EditorI18n", "Registered translator module: %s\n",
            moduleName.c_str());

        // Load translator for current language
        InstallTranslatorWithFallback(moduleName, m_currentLanguage);

        return true;
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

