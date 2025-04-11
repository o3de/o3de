/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Component/Component.h>
#include <AzCore/EBus/Event.h>

#include <QComboBox>
#include <QMainWindow>
#include <QScopedPointer>
#include <QSettings>
#include <QList>
#include <QPointer>
#include <QToolButton>
#include <QTimer>

#include "Include/SandboxAPI.h"
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include "IEditor.h"

#include <Core/EditorActionsHandler.h>
#endif

class ActionManager;
class AssetImporterManager;
class CLayoutViewPane;
class CLayoutWnd;
class CMainFrame;
class EngineConnectionListener;
class LevelEditorMenuHandler;
class MainStatusBar;
class UndoStackStateAdapter;

class QComboBox;
class QToolButton;
class QtViewport;
class QtViewPaneManager;
class QWidgetAction;

struct QtViewPane;

namespace AzQtComponents
{
    class DockMainWindow;
}

namespace AzToolsFramework
{
    class Ticker;
    class QtSourceControlNotificationHandler;

    //! @name Reverse URLs.
    //! Used to identify common actions and override them when necessary.
    //@{
    constexpr inline AZ::Crc32 EditModeMove = AZ_CRC_CE("org.o3de.action.editor.editmode.move");
    constexpr inline AZ::Crc32 EditModeRotate = AZ_CRC_CE("org.o3de.action.editor.editmode.rotate");
    constexpr inline AZ::Crc32 EditModeScale = AZ_CRC_CE("org.o3de.action.editor.editmode.scale");
    constexpr inline AZ::Crc32 SnapToGrid = AZ_CRC_CE("org.o3de.action.editor.snaptogrid");
    constexpr inline AZ::Crc32 SnapAngle = AZ_CRC_CE("org.o3de.action.editor.snapangle");
    //@}
}

#define MAINFRM_LAYOUT_NORMAL "NormalLayout"
#define MAINFRM_LAYOUT_PREVIEW "PreviewLayout"

// Sub-classing so we can add slots to our toolbar widgets
// Using lambdas is prone to crashes since the lambda doesn't know when the widget is deleted.

class UndoRedoToolButton
    : public QToolButton
{
    Q_OBJECT
public:
    explicit UndoRedoToolButton(QWidget* parent);
public Q_SLOTS:
    void Update(int count);
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
class SANDBOX_API MainWindow
    : public QMainWindow
    , public IEditorNotifyListener
    , private AzToolsFramework::SourceControlNotificationBus::Handler
{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

#ifdef Q_OS_WIN
    HWND GetNativeHandle();
#endif // #ifdef Q_OS_WIN

    void Initialize();

    // Returns the old and original main frame which we're porting away from.
    // ActionManager still needs it, to SendMessage() to it.
    CMainFrame* GetOldMainFrame() const;

    bool IsPreview() const;

    // The singleton is just a hack for now, it should be removed once everything
    // is ported to Qt.
    static MainWindow* instance();

    MainStatusBar* StatusBar() const;
    CLayoutWnd* GetLayout() const;

    void OpenViewPane(int paneId);
    void OpenViewPane(QtViewPane* pane);

    void SetActiveView(CLayoutViewPane* vp);

    /**
     * Returns the active view layout (Perspective, Top, Bottom, or Left, etc).
     * This particularly useful when in multi-layout mode, it represents the default viewport to use
     * when needing to interact with one.
     * When the user gives mouse focus to a viewport it becomes the active one, when unfocusing it
     * however, it remains the active one, unless another viewport got focus.
     */
    CLayoutViewPane* GetActiveView() const;
    QtViewport* GetActiveViewport() const;

    void AdjustToolBarIconSize(AzQtComponents::ToolBar::ToolBarIconSize size);
    void InvalidateControls();
    void OnCustomizeToolbar();
    void SaveConfig();
    void RefreshStyle();

    //! Reset timers used for auto saving.
    void StopAutoSaveTimers();
    void StartAutoSaveTimers();
    void ResetAutoSaveTimers();
    void ResetBackgroundUpdateTimer();

    LevelEditorMenuHandler* GetLevelEditorMenuHandler() { return m_levelEditorMenuHandler; }

    bool event(QEvent* event) override;

Q_SIGNALS:
    void ToggleRefCoordSys();
    void UpdateRefCoordSys();
    void DeleteSelection();

protected:
    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent *event) override;

    bool focusNextPrevChild(bool next) override;

private:
    void OnGotoSelected();

    void ToggleConsole();
    void RegisterOpenWndCommands();
    void InitCentralWidget();
    void InitStatusBar();

    template <class TValue>
    void ReadConfigValue(const QString& key, TValue& value)
    {
        value = m_settings.value(key, value).template value<TValue>();
    }

    // AzToolsFramework::SourceControlNotificationBus::Handler:
    void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override;

    QWidget* CreateSpacerRightWidget();

private Q_SLOTS:
    void OnStopAllSounds();
    void OnRefreshAudioSystem();
    void SaveLayout();
    void ViewDeletePaneLayout(const QString& layoutName);
    void ViewRenamePaneLayout(const QString& layoutName);
    void ViewLoadPaneLayout(const QString& layoutName);
    void ViewSavePaneLayout(const QString& layoutName);
    void OnConnectionStatusClicked();
    void OnUpdateConnectionStatus();
    void ShowConnectionDisconnectedDialog();
    void OnEscapeAction();

    void OnOpenAssetImporterManager(const QStringList& list);
    void OnOpenAssetImporterManagerAtPath(const QStringList& list, const QString& path);

private:
    friend class EditorActionsHandler;

    bool IsGemEnabled(const QString& uuid, const QString& version) const;

    // Broadcast the SystemTick event
    void SystemTick();

    QStringList coordSysList() const;
    void RegisterStdViewClasses();
    CMainFrame* m_oldMainFrame;
    QtViewPaneManager* m_viewPaneManager;
    UndoStackStateAdapter* m_undoStateAdapter;

    CLayoutViewPane* m_activeView;
    QSettings m_settings;

    AssetImporterManager* m_assetImporterManager;
    LevelEditorMenuHandler* m_levelEditorMenuHandler = nullptr;

    CLayoutWnd* m_pLayoutWnd;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZStd::shared_ptr<EngineConnectionListener> m_connectionListener;
    QTimer* m_connectionLostTimer;

    QScopedPointer<AzToolsFramework::QtSourceControlNotificationHandler> m_sourceControlNotifHandler;

    EditorActionsHandler m_editorActionsHandler;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    static MainWindow* m_instance;

    AzQtComponents::DockMainWindow* m_viewPaneHost;

    QTimer* m_autoSaveTimer;
    QTimer* m_autoRemindTimer;
    QTimer* m_backgroundUpdateTimer;

    bool m_connectedToAssetProcessor = false;
    bool m_showAPDisconnectDialog = false;
    bool m_selectedEntityHasRoot = false;

    friend class WidgetAction;
    friend class LevelEditorMenuHandler;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for MainWindow
    class MainWindowEditorFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MainWindowEditorFuncsHandler, "{C879102B-C767-4349-8F06-B69119CAC462}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AZ
