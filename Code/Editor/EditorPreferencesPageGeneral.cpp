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
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
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
        ->Field("ApplyConfigSpec", &GeneralSettings::m_applyConfigSpec)
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

    serialize.Class<LevelSaveSettings>()
        ->Version(1)
        ->Field("SaveAllPrefabsPreference", &LevelSaveSettings::m_saveAllPrefabsPreference);

    serialize.Class<Messaging>()
        ->Version(2)
        ->Field("ShowDashboard", &Messaging::m_showDashboard)
        ->Field("ShowCircularDependencyError", &Messaging::m_showCircularDependencyError);

    serialize.Class<Undo>()
        ->Version(2)
        ->Field("UndoLevels", &Undo::m_undoLevels)
        ->Field("UndoSliceOverrideSaves", &Undo::m_undoSliceOverrideSaveValue);;

    serialize.Class<DeepSelection>()
        ->Version(2)
        ->Field("DeepSelectionRange", &DeepSelection::m_deepSelectionRange)
        ->Field("StickDuplicate", &DeepSelection::m_stickDuplicate);

    serialize.Class<SliceSettings>()
        ->Version(1)
        ->Field("DynamicByDefault", &SliceSettings::m_slicesDynamicByDefault);

    serialize.Class<CEditorPreferencesPage_General>()
        ->Version(1)
        ->Field("General Settings", &CEditorPreferencesPage_General::m_generalSettings)
        ->Field("Level Save Settings", &CEditorPreferencesPage_General::m_levelSaveSettings)
        ->Field("Messaging", &CEditorPreferencesPage_General::m_messaging)
        ->Field("Undo", &CEditorPreferencesPage_General::m_undo)
        ->Field("Deep Selection", &CEditorPreferencesPage_General::m_deepSelection)
        ->Field("Slice Settings", &CEditorPreferencesPage_General::m_sliceSettings);



    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<GeneralSettings>("General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_previewPanel, "Show Geometry Preview Panel", "Show Geometry Preview Panel")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_applyConfigSpec, "Hide objects by config spec", "Hide objects by config spec")
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

        editContext->Class<LevelSaveSettings>("Level Save Settings", "")
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox, &LevelSaveSettings::m_saveAllPrefabsPreference, "Save All Prefabs Preference",
                    "This option controls whether prefabs should be saved along with the level")
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::AskEveryTime, "Ask every time")
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::SaveAll, "Save all")
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::SaveNone, "Save none");

        editContext->Class<Messaging>("Messaging", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Messaging::m_showDashboard, "Show Welcome to Open 3D Engine at startup", "Show Welcome to Open 3D Engine at startup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Messaging::m_showCircularDependencyError, "Show Error: Circular dependency", "Show an error message when adding a slice instance to the target slice would create a cyclic asset dependency. All other valid overrides will be saved even if this is turned off.");

        editContext->Class<Undo>("Undo", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Undo::m_undoLevels, "Undo Levels", "This field specifies the number of undo levels")
            ->Attribute(AZ::Edit::Attributes::Min, 0)
            ->Attribute(AZ::Edit::Attributes::Max, 10000)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Undo::m_undoSliceOverrideSaveValue, "Undo Slice Override Saves", "Allow slice saves to be undone");

        editContext->Class<DeepSelection>("Selection", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &DeepSelection::m_stickDuplicate, "Stick duplicate to cursor", "Stick duplicate to cursor")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &DeepSelection::m_deepSelectionRange, "Deep selection range", "Deep Selection Range")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f);

        editContext->Class<SliceSettings>("Slices", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SliceSettings::m_slicesDynamicByDefault, "New Slices Dynamic By Default", "When creating slices, they will be set to dynamic by default");

        editContext->Class<CEditorPreferencesPage_General>("General Editor Preferences", "Class for handling General Editor Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_generalSettings, "General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_levelSaveSettings, "Level Save Settings", "File>Save")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_messaging, "Messaging", "Messaging")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_undo, "Undo", "Undo Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_deepSelection, "Selection", "Selection")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_sliceSettings, "Slices", "Slice Settings");
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
    //general settings
    gSettings.bPreviewGeometryWindow = m_generalSettings.m_previewPanel;
    gSettings.bApplyConfigSpecInEditor = m_generalSettings.m_applyConfigSpec;
    gSettings.enableSourceControl = m_generalSettings.m_enableSourceControl;
    gSettings.clearConsoleOnGameModeStart = m_generalSettings.m_clearConsoleOnGameModeStart;
    gSettings.consoleBackgroundColorTheme = m_generalSettings.m_consoleBackgroundColorTheme;
    gSettings.bShowTimeInConsole = m_generalSettings.m_bShowTimeInConsole;
    gSettings.bShowDashboardAtStartup = m_messaging.m_showDashboard;
    gSettings.m_showCircularDependencyError = m_messaging.m_showCircularDependencyError;
    gSettings.bAutoloadLastLevelAtStartup = m_generalSettings.m_autoLoadLastLevel;
    gSettings.stylusMode = m_generalSettings.m_stylusMode;
    gSettings.restoreViewportCamera = m_generalSettings.m_restoreViewportCamera;
    gSettings.enableSceneInspector = m_generalSettings.m_enableSceneInspector;

    if (static_cast<int>(m_generalSettings.m_toolbarIconSize) != gSettings.gui.nToolbarIconSize)
    {
        gSettings.gui.nToolbarIconSize = static_cast<int>(m_generalSettings.m_toolbarIconSize);
        MainWindow::instance()->AdjustToolBarIconSize(m_generalSettings.m_toolbarIconSize);
    }

    //prefabs
    gSettings.levelSaveSettings.saveAllPrefabsPreference = m_levelSaveSettings.m_saveAllPrefabsPreference;

    //undo
    gSettings.undoLevels = m_undo.m_undoLevels;

    gSettings.m_undoSliceOverrideSaveValue = m_undo.m_undoSliceOverrideSaveValue;

    //deep selection
    gSettings.deepSelectionSettings.fRange = m_deepSelection.m_deepSelectionRange;
    gSettings.deepSelectionSettings.bStickDuplicate = m_deepSelection.m_stickDuplicate;

    //slices
    gSettings.sliceSettings.dynamicByDefault = m_sliceSettings.m_slicesDynamicByDefault;
}

void CEditorPreferencesPage_General::InitializeSettings()
{
    //general settings
    m_generalSettings.m_previewPanel = gSettings.bPreviewGeometryWindow;
    m_generalSettings.m_applyConfigSpec = gSettings.bApplyConfigSpecInEditor;
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

    //Messaging
    m_messaging.m_showDashboard = gSettings.bShowDashboardAtStartup;
    m_messaging.m_showCircularDependencyError = gSettings.m_showCircularDependencyError;

    //undo
    m_undo.m_undoLevels = gSettings.undoLevels;
    m_undo.m_undoSliceOverrideSaveValue = gSettings.m_undoSliceOverrideSaveValue;

    //deep selection
    m_deepSelection.m_deepSelectionRange = gSettings.deepSelectionSettings.fRange;
    m_deepSelection.m_stickDuplicate = gSettings.deepSelectionSettings.bStickDuplicate;

    //slices
    m_sliceSettings.m_slicesDynamicByDefault = gSettings.sliceSettings.dynamicByDefault;
}
