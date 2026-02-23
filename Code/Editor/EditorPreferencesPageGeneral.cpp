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
#include <QCoreApplication>

// AzToolsFramework
#include <AzToolsFramework/API/SettingsRegistryUtils.h>
#include <AzToolsFramework/Translation/TranslationManager.h>
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

namespace EditorPreferencesStrings
{
    static const char* GeneralSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "General Settings");
    static const char* GeneralSettingsClassDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "General Editor Preferences");

    static const char* PreviewPanelName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Show Geometry Preview Panel");
    static const char* PreviewPanelDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Show Geometry Preview Panel");
    static const char* EnableSourceControlName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Enable Source Control");
    static const char* EnableSourceControlDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Enable Source Control");
    static const char* ClearConsoleName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Clear Console at game startup");
    static const char* ClearConsoleDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Clear Console when game mode starts");
    static const char* ConsoleBackgroundName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Console Background");
    static const char* ConsoleBackgroundDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Console Background");
    static const char* ConsoleThemeLight = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Light");
    static const char* ConsoleThemeDark = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Dark");
    static const char* AutoLoadLastLevelName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Auto-load last level at startup");
    static const char* AutoLoadLastLevelDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Auto-load last level at startup");
    static const char* ShowTimeInConsoleName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Show Time In Console");
    static const char* ShowTimeInConsoleDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Show Time In Console");
    static const char* ToolbarIconSizeName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Toolbar Icon Size");
    static const char* ToolbarIconSizeDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Toolbar Icon Size");
    static const char* ToolbarIconDefault = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Default");
    static const char* ToolbarIconLarge = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Large");
    static const char* StylusModeName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Stylus Mode");
    static const char* StylusModeDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Stylus Mode for tablets and other pointing devices");
    static const char* EditorLanguageName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Editor Language");
    static const char* EditorLanguageDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Select the language for the editor interface (requires restart)");
    static const char* RestoreViewportCameraDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Keep the original editor viewport transform when exiting game mode.");
    static const char* EnableSceneInspectorName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Enable Scene Inspector (EXPERIMENTAL)");
    static const char* EnableSceneInspectorDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Enable the option to inspect the internal data loaded from scene files like .fbx. This is an experimental feature. Restart the Scene Settings if the option is not visible under the Help menu.");

    static const char* PrefabSaveSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Prefab Save Settings");
    static const char* SaveAllNestedPrefabsName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Save All Nested Prefabs");
    static const char* SaveAllNestedPrefabsDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "This option controls whether nested prefabs should be saved when a prefab is saved.");
    static const char* SaveAllAskEveryTime = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Ask every time");
    static const char* SaveAllSaveAll = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Save all");
    static const char* SaveAllSaveNone = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Save none");
    static const char* DetachRemovesContainerName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Detach removes container entity");
    static const char* DetachRemovesContainerDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "When you choose the 'detach' option on a prefab container, should the container entity be removed also?");

    static const char* MessagingClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Messaging");
    static const char* ShowDashboardName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Show Welcome to Open 3D Engine at startup");
    static const char* ShowDashboardDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Show Welcome to Open 3D Engine at startup");

    static const char* UndoClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Undo");
    static const char* UndoLevelsName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Undo Levels");
    static const char* UndoLevelsDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "This field specifies the number of undo levels");

    static const char* EditorPreferencesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "General Editor Preferences");
    static const char* EditorPreferencesClassDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Class for handling General Editor Preferences");
    static const char* PrefabSettingsName = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Prefab Settings");
    static const char* PrefabSettingsDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "General Prefab Settings");
    static const char* UndoPreferencesDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Undo Preferences");

    static const char* LanguageChangedTitle = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "Language Changed");
    static const char* LanguageChangedMessage = QT_TRANSLATE_NOOP("EditorPreferencesPageGeneral", "The editor language has been changed. Some UI elements will update immediately, but you may need to restart the editor for all changes to take effect.");
}

void CEditorPreferencesPage_General::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<GeneralSettings>()
        ->Version(4)
        ->Field("PreviewPanel", &GeneralSettings::m_previewPanel)
        ->Field("EnableSourceControl", &GeneralSettings::m_enableSourceControl)
        ->Field("ClearConsole", &GeneralSettings::m_clearConsoleOnGameModeStart)
        ->Field("ConsoleBackgroundColorTheme", &GeneralSettings::m_consoleBackgroundColorTheme)
        ->Field("AutoloadLastLevel", &GeneralSettings::m_autoLoadLastLevel)
        ->Field("ShowTimeInConsole", &GeneralSettings::m_bShowTimeInConsole)
        ->Field("ToolbarIconSize", &GeneralSettings::m_toolbarIconSize)
        ->Field("StylusMode", &GeneralSettings::m_stylusMode)
        ->Field("ShowNews", &GeneralSettings::m_bShowNews)
        ->Field("EditorLanguage", &GeneralSettings::m_editorLanguage)
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
        using namespace EditorPreferencesStrings;

        editContext->Class<GeneralSettings>(GeneralSettingsClassName, GeneralSettingsClassDesc)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_previewPanel, PreviewPanelName, PreviewPanelDesc)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSourceControl, EnableSourceControlName, EnableSourceControlDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_clearConsoleOnGameModeStart, ClearConsoleName, ClearConsoleDesc)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_consoleBackgroundColorTheme, ConsoleBackgroundName, ConsoleBackgroundDesc)
                ->EnumAttribute(AzToolsFramework::ConsoleColorTheme::Light, ConsoleThemeLight)
                ->EnumAttribute(AzToolsFramework::ConsoleColorTheme::Dark, ConsoleThemeDark)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_autoLoadLastLevel, AutoLoadLastLevelName, AutoLoadLastLevelDesc)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_bShowTimeInConsole, ShowTimeInConsoleName, ShowTimeInConsoleDesc)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_toolbarIconSize, ToolbarIconSizeName, ToolbarIconSizeDesc)
                ->EnumAttribute(AzQtComponents::ToolBar::ToolBarIconSize::IconNormal, ToolbarIconDefault)
                ->EnumAttribute(AzQtComponents::ToolBar::ToolBarIconSize::IconLarge, ToolbarIconLarge)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_stylusMode, StylusModeName, StylusModeDesc)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_editorLanguage, EditorLanguageName, EditorLanguageDesc)
                ->Attribute(AZ::Edit::Attributes::StringList, &CEditorPreferencesPage_General::GetAvailableLanguages)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_restoreViewportCamera, EditorPreferencesGeneralRestoreViewportCameraSettingName, RestoreViewportCameraDesc)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSceneInspector, EnableSceneInspectorName, EnableSceneInspectorDesc);

        editContext->Class<LevelSaveSettings>(PrefabSaveSettingsClassName, "")
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox, &LevelSaveSettings::m_saveAllPrefabsPreference, SaveAllNestedPrefabsName, SaveAllNestedPrefabsDesc)
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::AskEveryTime, SaveAllAskEveryTime)
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::SaveAll, SaveAllSaveAll)
                ->EnumAttribute(AzToolsFramework::Prefab::SaveAllPrefabsPreference::SaveNone, SaveAllSaveNone)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &LevelSaveSettings::m_bDetachPrefabRemovesContainer, DetachRemovesContainerName, DetachRemovesContainerDesc);

        editContext->Class<Messaging>(MessagingClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Messaging::m_showDashboard, ShowDashboardName, ShowDashboardDesc);

        editContext->Class<Undo>(UndoClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Undo::m_undoLevels, UndoLevelsName, UndoLevelsDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0)
            ->Attribute(AZ::Edit::Attributes::Max, 10000);

        editContext->Class<CEditorPreferencesPage_General>(EditorPreferencesClassName, EditorPreferencesClassDesc)
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_generalSettings, GeneralSettingsClassName, GeneralSettingsClassDesc)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_levelSaveSettings, PrefabSettingsName, PrefabSettingsDesc)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_messaging, MessagingClassName, MessagingClassName)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_undo, UndoClassName, UndoPreferencesDesc);
    }
}

CEditorPreferencesPage_General::CEditorPreferencesPage_General()
{
    InitializeSettings();

    m_icon = QIcon(":/res/Global.svg");
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

    // Apply language change.
    // m_editorLanguage stores a pure language code (e.g., "zh_CN"), so no parsing is needed.
    auto* app = Editor::EditorQtApplication::instance();
    AZ_Assert(app, "EditorQtApplication instance is null. Cannot apply language change.");
    if (app != nullptr)
    {
        AZStd::string currentLang = app->GetCurrentLanguage();
        AZStd::string newLang = m_generalSettings.m_editorLanguage.c_str();

        if (currentLang != newLang && newLang.length() != 0)
        {
            if (app->SetLanguage(newLang))
            {
                QMessageBox::information(
                    nullptr,
                    QCoreApplication::translate("EditorPreferencesPageGeneral", EditorPreferencesStrings::LanguageChangedTitle),
                    QCoreApplication::translate("EditorPreferencesPageGeneral", EditorPreferencesStrings::LanguageChangedMessage));
            }
        }
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

    // Store the pure language code (e.g., "zh_CN") directly.
    // The ComboBox StringList also uses pure codes, so they will match correctly.
    if (auto* app = Editor::EditorQtApplication::instance())
    {
        AZStd::string currentLang = app->GetCurrentLanguage();
        m_generalSettings.m_editorLanguage = currentLang;
    }

    //prefabs
    m_levelSaveSettings.m_saveAllPrefabsPreference = gSettings.levelSaveSettings.saveAllPrefabsPreference;
    m_levelSaveSettings.m_bDetachPrefabRemovesContainer = AzToolsFramework::GetRegistry(DetachPrefabRemovesContainerName, DetachPrefabRemovesContainerDefault);

    //Messaging
    m_messaging.m_showDashboard = gSettings.bShowDashboardAtStartup;

    //undo
    m_undo.m_undoLevels = gSettings.undoLevels;

}

AZStd::vector<AZStd::string> CEditorPreferencesPage_General::GetAvailableLanguages()
{
    AZStd::vector<AZStd::string> languages;

    // Return pure language codes. The ComboBox stores these codes directly,
    // avoiding fragile "Display Name (code)" format parsing.
    // Users see codes like "en_US", "zh_CN" which are self-explanatory.
    QStringList codes = AzToolsFramework::TranslationManager::GetAvailableLanguages();
    for (const QString& code : codes)
    {
        languages.push_back(code.toUtf8().constData());
    }

    return languages;
}
