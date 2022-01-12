/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/TickBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioConfig.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/GUIOptions.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/PluginOptionsBus.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/CommandManagerCallback.h>
#include <MCore/Source/MCoreCommandManager.h>
#include <MCore/Source/CommandLine.h>
#include <MysticQt/Source/RecentFiles.h>
#include <Editor/ActorEditorBus.h>

#include <AzQtComponents/Components/DockMainWindow.h>
#include <QDialog>
#endif

// forward declarations
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QDropEvent)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QMessageBox)
QT_FORWARD_DECLARE_CLASS(QAbstractButton)
QT_FORWARD_DECLARE_CLASS(QTextEdit)

struct SelectionItem;

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace EMotionFX
{
    class AnimGraph;
    class MotionSet;
}

namespace MCore
{
    class CommandGroup;
}

namespace MysticQt
{
    class KeyboardShortcutManager;
}

namespace AzQtComponents
{
    class FancyDocking;
}

namespace EMStudio
{
    // forward declaration
    class DirtyFileManager;
    class EMStudioPlugin;
    class FileManager;
    class MainWindow;
    class NativeEventFilter;
    class NodeSelectionWindow;
    class PreferencesWindow;
    class UndoMenuCallback;


    class ErrorWindow
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(ErrorWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        explicit ErrorWindow(QWidget* parent = nullptr);
        void Init(const AZStd::vector<AZStd::string>& errors);
        
    private:
        QTextEdit* m_textEdit = nullptr;
    };

    // the main window
    class EMSTUDIO_API MainWindow
        : public AzQtComponents::DockMainWindow
        , private PluginOptionsNotificationsBus::Router
        , public EMotionFX::ActorEditorRequestBus::Handler
        , private AZ::TickBus::Handler
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MainWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
        ~MainWindow();

        void UpdateCreateWindowMenu();
        void UpdateLayoutsMenu();
        void UpdateUndoRedo();
        void DisableUndoRedo();
        static void Reflect(AZ::ReflectContext* context);
        void Init();

        MCORE_INLINE QMenu* GetLayoutsMenu()                                    { return m_layoutsMenu; }

        void LoadActor(const char* fileName, bool replaceCurrentScene);
        void LoadCharacter(const AZ::Data::AssetId& actorAssetId, const AZ::Data::AssetId& animgraphId, const AZ::Data::AssetId& motionSetId);
        void LoadFile(const AZStd::string& fileName, int32 contextMenuPosX = 0, int32 contextMenuPosY = 0, bool contextMenuEnabled = true, bool reload = false);
        void LoadFiles(const AZStd::vector<AZStd::string>& filenames, int32 contextMenuPosX = 0, int32 contextMenuPosY = 0, bool contextMenuEnabled = true, bool reload = false);

        void Activate(const AZ::Data::AssetId& actorAssetId, const EMotionFX::AnimGraph* animGraph, const EMotionFX::MotionSet* motionSet);

        MysticQt::RecentFiles* GetRecentWorkspaces()                            { return &m_recentWorkspaces; }

        GUIOptions& GetOptions()                                                { return m_options; }

        void Reset(bool clearActors = true, bool clearMotionSets = true, bool clearMotions = true, bool clearAnimGraphs = true, MCore::CommandGroup* commandGroup = nullptr);

        // settings
        void SavePreferences();
        void LoadPreferences();

        void UpdateResetAndSaveAllMenus();

        void UpdateSaveActorsMenu();
        void EnableMergeActorMenu();
        void DisableMergeActorMenu();
        void EnableSaveSelectedActorsMenu();
        void DisableSaveSelectedActorsMenu();

        void OnWorkspaceSaved(const char* filename);

        MCORE_INLINE QComboBox* GetApplicationModeComboBox()                    { return m_applicationMode; }
        DirtyFileManager*   GetDirtyFileManager() const                         { return m_dirtyFileManager; }
        FileManager*        GetFileManager() const                              { return m_fileManager; }
        PreferencesWindow*  GetPreferencesWindow() const                        { return m_preferencesWindow; }

        size_t GetNumLayouts() const                                            { return m_layoutNames.size(); }
        const char* GetLayoutName(uint32 index) const                           { return m_layoutNames[index].c_str(); }
        const char* GetCurrentLayoutName() const;

        static const char* GetEMotionFXPaneName();
        MysticQt::KeyboardShortcutManager* GetShortcutManager() const           { return m_shortcutManager; }

        AzQtComponents::FancyDocking* GetFancyDockingManager() const            { return m_fancyDockingManager; }

        QMessageBox* GetRemoveLayoutDialog();

        void AddRecentActorFile(const QString& fileName);

        void LoadKeyboardShortcuts();

    public slots:
        void OnAutosaveTimeOut();
        void LoadLayoutAfterShow();
        void RaiseFloatingWidgets();
        void LoadCharacterFiles();

        void OnSaveLayoutDialogAccept();
        void OnSaveLayoutDialogReject();

    protected:
        void moveEvent(QMoveEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void LoadDefaultLayout();

    private:
        // ActorEditorRequests
        EMotionFX::ActorInstance* GetSelectedActorInstance() override;
        EMotionFX::Actor* GetSelectedActor() override;

        void BroadcastSelectionNotifications();
        EMotionFX::Actor*           m_prevSelectedActor;
        EMotionFX::ActorInstance*   m_prevSelectedActorInstance;

        QMenu*                  m_createWindowMenu;
        QMenu*                  m_layoutsMenu;
        QAction*                m_undoAction;
        QAction*                m_redoAction;

        // keyboard shortcut manager
        MysticQt::KeyboardShortcutManager* m_shortcutManager;

        // layouts (application modes)
        AZStd::vector<AZStd::string> m_layoutNames;
        bool m_layoutLoaded;

        // menu actions
        QAction*                m_resetAction;
        QAction*                m_saveAllAction;
        QAction*                m_mergeActorAction;
        QAction*                m_saveSelectedActorsAction;

        // application mode
        QComboBox*              m_applicationMode;

        PreferencesWindow*      m_preferencesWindow;

        FileManager*  m_fileManager;

        MysticQt::RecentFiles   m_recentActors;
        MysticQt::RecentFiles   m_recentWorkspaces;

        // dirty files
        DirtyFileManager*       m_dirtyFileManager;

        void SetWindowTitleFromFileName(const AZStd::string& fileName);

        // drag & drop support
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        AZStd::string           m_droppedActorFileName;

        // General options
        GUIOptions                              m_options;
        bool                                    m_loadingOptions;
        
        QTimer*                                 m_autosaveTimer;
        
        AZStd::vector<AZStd::string>            m_characterFiles;

        NativeEventFilter*                      m_nativeEventFilter;

        void closeEvent(QCloseEvent* event) override;
        void showEvent(QShowEvent* event) override;

        void OnOptionChanged(const AZStd::string& optionChanged) override;

        UndoMenuCallback*                       m_undoMenuCallback = nullptr;

        AzQtComponents::FancyDocking*           m_fancyDockingManager = nullptr;

        QMessageBox*                            m_reallyRemoveLayoutDialog = nullptr;
        QString                                 m_removeLayoutNameText;
        QString                                 m_layoutFileBeingRemoved;

        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandImportActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandImportMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveWorkspaceCallback);
        CommandImportActorCallback*         m_importActorCallback;
        CommandRemoveActorCallback*         m_removeActorCallback;
        CommandRemoveActorInstanceCallback* m_removeActorInstanceCallback;
        CommandImportMotionCallback*        m_importMotionCallback;
        CommandRemoveMotionCallback*        m_removeMotionCallback;
        CommandCreateMotionSetCallback*     m_createMotionSetCallback;
        CommandRemoveMotionSetCallback*     m_removeMotionSetCallback;
        CommandLoadMotionSetCallback*       m_loadMotionSetCallback;
        CommandCreateAnimGraphCallback*     m_createAnimGraphCallback;
        CommandRemoveAnimGraphCallback*     m_removeAnimGraphCallback;
        CommandLoadAnimGraphCallback*       m_loadAnimGraphCallback;
        CommandSelectCallback*              m_selectCallback;
        CommandUnselectCallback*            m_unselectCallback;
        CommandClearSelectionCallback*      m_clearSelectionCallback;
        CommandSaveWorkspaceCallback*       m_saveWorkspaceCallback;

        class MainWindowCommandManagerCallback : public MCore::CommandManagerCallback
        {
        public:
            MainWindowCommandManagerCallback();

            bool NeedToClearRecorder(MCore::Command* command, const MCore::CommandLine& commandLine) const;

            //////////////////////////////////////////////////////////////////////////////////////
            /// CommandManagerCallback implementation
            void OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine) override;
            void OnPostExecuteCommand(MCore::CommandGroup* /*group*/, MCore::Command* /*command*/, const MCore::CommandLine& /*commandLine*/, bool /*wasSuccess*/, const AZStd::string& /*outResult*/) override { }
            void OnPreUndoCommand(MCore::Command* command, const MCore::CommandLine& commandLine) override;
            void OnPreExecuteCommandGroup(MCore::CommandGroup* /*group*/, bool /*undo*/) override { }
            void OnPostExecuteCommandGroup(MCore::CommandGroup* /*group*/, bool /*wasSuccess*/) override { }
            void OnAddCommandToHistory(size_t /*historyIndex*/, MCore::CommandGroup* /*group*/, MCore::Command* /*command*/, const MCore::CommandLine& /*commandLine*/) override { }
            void OnRemoveCommand(size_t /*historyIndex*/) override { }
            void OnSetCurrentCommand(size_t /*index*/) override { }
            void OnShowErrorReport(const AZStd::vector<AZStd::string>& errors) override;
        private:
            AZStd::vector<AZStd::string> m_skipClearRecorderCommands;
            AZStd::unique_ptr<ErrorWindow> m_errorWindow = nullptr;
        };

        MainWindowCommandManagerCallback m_mainWindowCommandManagerCallback;

    private:
        // AZ::TickBus::Handler overrides
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
        int GetTickOrder() override;

        void UpdatePlugins(float timeDelta);

        void EnableUpdatingPlugins();
        void DisableUpdatingPlugins();

    public slots:
        void OnFileOpenActor();
        void OnFileSaveSelectedActors();
        void OnReset();
        void OnFileMergeActor();
        void OnOpenDroppedActor();
        void OnRecentFile(QAction* action);
        void OnMergeDroppedActor();
        void OnFileNewWorkspace();
        void OnFileOpenWorkspace();
        void OnFileSaveWorkspace();
        void OnFileSaveWorkspaceAs();
        void OnWindowCreate(bool checked);
        void OnLayoutSaveAs();
        void OnRemoveLayout();
        void OnLoadLayout();
        void OnUndo();
        void OnRedo();
        void OnOpenAutosaveFolder();
        void OnOpenSettingsFolder();
        void OnPreferences();
        void OnSaveAll();
        void ApplicationModeChanged(int index);
        void ApplicationModeChanged(const QString& text);
        void OnUpdateRenderPlugins();
        void OnRemoveLayoutButtonClicked(QAbstractButton* button);

     signals:
        void HardwareChangeDetected();
    };

} // namespace EMStudio
