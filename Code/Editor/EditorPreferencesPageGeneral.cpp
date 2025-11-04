/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesPageGeneral.h"

// Qt
#include <QMessageBox>

// AzToolsFramework
#include <AzToolsFramework/API/SettingsRegistryUtils.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Prefab/PrefabSettings.h>
#include <AzQtComponents/Components/StyleManager.h>

// Editor
#include "MainWindow.h"
#include "Core/QtEditorApplication.h"

#define EDITORPREFS_EVENTNAME "EPGEvent"
#define EDITORPREFS_EVENTVALTOGGLE "operation"
#define UNDOSLICESAVE_VALON "UndoSliceSaveValueOn"
#define UNDOSLICESAVE_VALOFF "UndoSliceSaveValueOff"


void CEditorPreferencesPage_General::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<GeneralSettings>()
        ->Version(3)
        ->Field("PreviewPanel", &GeneralSettings::m_previewPanel)
        ->Field("EnableSourceControl", &GeneralSettings::m_enableSourceControl)
        ->Field("ClearConsole", &GeneralSettings::m_clearConsoleOnGameModeStart)
        ->Field("ConsoleBackgroundColorTheme", &GeneralSettings::m_consoleBackgroundColorTheme)
        ->Field("AutoloadLastLevel", &GeneralSettings::m_autoLoadLastLevel)
        ->Field("ShowTimeInConsole", &GeneralSettings::m_bShowTimeInConsole)
        ->Field("ToolbarIconSize", &GeneralSettings::m_toolbarIconSize)
        ->Field("StylusMode", &GeneralSettings::m_stylusMode)
        ->Field("ShowNews", &GeneralSettings::m_bShowNews)
        ->Field("EnableSceneInspector", &GeneralSettings::m_enableSceneInspector)
        ->Field("RestoreViewportCamera", &GeneralSettings::m_restoreViewportCamera);

    // note, despite this class name being LevelSaveSettings, it is used for general prefab settings
    // and the name is retained to avoid breaking things
    serialize.Class<LevelSaveSettings>()
        ->Version(1)
        ->Field("SaveAllPrefabsPreference", &LevelSaveSettings::m_saveAllPrefabsPreference)
        ->Field("DetachPrefabRemovesContainer", &LevelSaveSettings::m_bDetachPrefabRemovesContainer);


    serialize.Class<Messaging>()
        ->Version(2)
        ->Field("ShowDashboard", &Messaging::m_showDashboard);

    serialize.Class<Undo>()
        ->Version(2)
        ->Field("UndoLevels", &Undo::m_undoLevels);

    serialize.Class<CEditorPreferencesPage_General>()
        ->Version(1)
        ->Field("General Settings", &CEditorPreferencesPage_General::m_generalSettings)
        ->Field("Prefab Save Settings", &CEditorPreferencesPage_General::m_levelSaveSettings)
        ->Field("Messaging", &CEditorPreferencesPage_General::m_messaging)
        ->Field("Undo", &CEditorPreferencesPage_General::m_undo);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<GeneralSettings>("General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_previewPanel, "Show Geometry Preview Panel", "Show Geometry Preview Panel")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSourceControl, "Enable Source Control", "Enable Source Control")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_clearConsoleOnGameModeStart, "Clear Console at game startup", "Clear Console when game mode starts")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_consoleBackgroundColorTheme, "Console Background", "Console Background")
                ->EnumAttribute(AzToolsFramework::ConsoleColorTheme::Light, "Light")
                ->EnumAttribute(AzToolsFramework::ConsoleColorTheme::Dark, "Dark")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_autoLoadLastLevel, "Auto-load last level at startup", "Auto-load last level at startup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_bShowTimeInConsole, "Show Time In Console", "Show Time In Console")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_toolbarIconSize, "Toolbar Icon Size", "Toolbar Icon Size")
                ->EnumAttribute(AzQtComponents::ToolBar::ToolBarIconSize::IconNormal, "Default")
                ->EnumAttribute(AzQtComponents::ToolBar::ToolBarIconSize::IconLarge, "Large")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_stylusMode, "Stylus Mode", "Stylus Mode for tablets and other pointing devices")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_restoreViewportCamera, EditorPreferencesGeneralRestoreViewportCameraSettingName, "Keep the original editor viewport transform when exiting game mode.")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSceneInspector, "Enable Scene Inspector (EXPERIMENTAL)", "Enable the option to inspect the internal data loaded from scene files like .fbx. This is an experimental feature. Restart the Scene Settings if the option is not visible under the Help menu.");

        editContext->Class<LevelSaveSettings>("Prefab Save Settings", "")
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox, &LevelSaveSettings::m_saveAllPrefabsPreference, "Save All Nested Prefabs",
                    "This option controls whether nested prefabs should be saved when a prefab is saved.")
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::AskEveryTime, "Ask every time")
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::SaveAll, "Save all")
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::SaveNone, "Save none")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &LevelSaveSettings::m_bDetachPrefabRemovesContainer, "Detach removes container entity", 
                    "When you choose the 'detach' option on a prefab container, should the container entity be removed also?");

        editContext->Class<Messaging>("Messaging", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Messaging::m_showDashboard, "Show Welcome to Open 3D Engine at startup", "Show Welcome to Open 3D Engine at startup");

        editContext->Class<Undo>("Undo", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Undo::m_undoLevels, "Undo Levels", "This field specifies the number of undo levels")
            ->Attribute(AZ::Edit::Attributes::Min, 0)
            ->Attribute(AZ::Edit::Attributes::Max, 10000);

        editContext->Class<CEditorPreferencesPage_General>("General Editor Preferences", "Class for handling General Editor Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_generalSettings, "General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_levelSaveSettings, "Prefab Settings", "General Prefab Settings")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_messaging, "Messaging", "Messaging")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_undo, "Undo", "Undo Preferences");
    }
}

CEditorPreferencesPage_General::CEditorPreferencesPage_General()
{
    InitializeSettings();

    m_icon = QIcon(":/res/Global.svg");
}

const char* CEditorPreferencesPage_General::GetTitle()
{
    return "General Settings";
}

QIcon& CEditorPreferencesPage_General::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_General::OnApply()
{
    using namespace AzToolsFramework::Prefab::Settings;

    //general settings
    gSettings.bPreviewGeometryWindow = m_generalSettings.m_previewPanel;
    gSettings.enableSourceControl = m_generalSettings.m_enableSourceControl;
    gSettings.clearConsoleOnGameModeStart = m_generalSettings.m_clearConsoleOnGameModeStart;
    gSettings.consoleBackgroundColorTheme = m_generalSettings.m_consoleBackgroundColorTheme;
    gSettings.bShowTimeInConsole = m_generalSettings.m_bShowTimeInConsole;
    gSettings.bShowDashboardAtStartup = m_messaging.m_showDashboard;
    gSettings.bAutoloadLastLevelAtStartup = m_generalSettings.m_autoLoadLastLevel;
    gSettings.stylusMode = m_generalSettings.m_stylusMode;
    gSettings.restoreViewportCamera = m_generalSettings.m_restoreViewportCamera;
    gSettings.enableSceneInspector = m_generalSettings.m_enableSceneInspector;
    AzToolsFramework::SetRegistry(AzToolsFramework::Prefab::Settings::DetachPrefabRemovesContainerName, m_levelSaveSettings.m_bDetachPrefabRemovesContainer);

    if (static_cast<int>(m_generalSettings.m_toolbarIconSize) != gSettings.gui.nToolbarIconSize)
    {
        gSettings.gui.nToolbarIconSize = static_cast<int>(m_generalSettings.m_toolbarIconSize);
        MainWindow::instance()->AdjustToolBarIconSize(m_generalSettings.m_toolbarIconSize);
    }

    //prefabs
    gSettings.levelSaveSettings.saveAllPrefabsPreference = m_levelSaveSettings.m_saveAllPrefabsPreference;

    //undo
    gSettings.undoLevels = m_undo.m_undoLevels;
}

void CEditorPreferencesPage_General::InitializeSettings()
{
    using namespace AzToolsFramework::Prefab::Settings;

    //general settings
    m_generalSettings.m_previewPanel = gSettings.bPreviewGeometryWindow;
    m_generalSettings.m_enableSourceControl = gSettings.enableSourceControl;
    m_generalSettings.m_clearConsoleOnGameModeStart = gSettings.clearConsoleOnGameModeStart;
    m_generalSettings.m_consoleBackgroundColorTheme = gSettings.consoleBackgroundColorTheme;
    m_generalSettings.m_bShowTimeInConsole = gSettings.bShowTimeInConsole;
    m_generalSettings.m_autoLoadLastLevel = gSettings.bAutoloadLastLevelAtStartup;
    m_generalSettings.m_stylusMode = gSettings.stylusMode;
    m_generalSettings.m_restoreViewportCamera = gSettings.restoreViewportCamera;
    m_generalSettings.m_enableSceneInspector = gSettings.enableSceneInspector;
    m_generalSettings.m_toolbarIconSize = static_cast<AzQtComponents::ToolBar::ToolBarIconSize>(gSettings.gui.nToolbarIconSize);

    //prefabs
    m_levelSaveSettings.m_saveAllPrefabsPreference = gSettings.levelSaveSettings.saveAllPrefabsPreference;
    m_levelSaveSettings.m_bDetachPrefabRemovesContainer = AzToolsFramework::GetRegistry(DetachPrefabRemovesContainerName, DetachPrefabRemovesContainerDefault);

    //Messaging
    m_messaging.m_showDashboard = gSettings.bShowDashboardAtStartup;

    //undo
    m_undo.m_undoLevels = gSettings.undoLevels;

}
